// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from EASy68K/Sim68K/RUN.CPP: runprog(), exec_inst(), exceptionHandler()

#include "easym68k/sim/simulator.h"

#include <algorithm>

#include "easym68k/sim/addressing_mode.h"
#include "easym68k/sim/cycle_data.h"
#include "easym68k/sim/iexecution_logger.h"
#include "easym68k/sim/srecord_loader.h"

namespace easym68k::sim {

M68kSimulator::M68kSimulator(Config config)
    : config_(config),
      pending_irqs_(0),
      simhalt_enabled_(config.simhalt_enabled),
      exceptions_enabled_(config.exceptions_enabled) {
  memory_.SetFaultPcSource(&state_.pc);
}

// =============================================================================
// Reset
// =============================================================================

void M68kSimulator::Reset() {
  state_ = CpuState{};
  state_.sr = kSrSupervisor;  // start in supervisor mode
  state_.ssp = memory_.ReadLong(0x0).value;
  state_.pc = memory_.ReadLong(0x4).value & kAddressMask;
  state_.halted = false;
  state_.stopped = false;
  stop_requested_.store(false, std::memory_order_relaxed);
  pending_irqs_.store(0, std::memory_order_relaxed);
  async_irq_events_.store(0, std::memory_order_relaxed);
  async_resumed_event_.store(false, std::memory_order_relaxed);
  last_step_faulted_ = false;
  memory_.ClearFault();
}

void M68kSimulator::InitProgram() {
  // STARTSIM.CPP initSim()
  state_ = CpuState{};
  state_.sr = 0x2000;
  state_.a[7] = 0x00FF0000;  // user stack
  state_.usp = state_.a[7];
  state_.ssp = 0x01000000;  // supervisor stack
  state_.pc = start_address_ & kAddressMask;
  state_.halted = false;
  state_.stopped = false;
  stop_requested_.store(false, std::memory_order_relaxed);
  pending_irqs_.store(0, std::memory_order_relaxed);
  async_irq_events_.store(0, std::memory_order_relaxed);
  async_resumed_event_.store(false, std::memory_order_relaxed);
  last_step_faulted_ = false;
  memory_.ClearFault();
}

void M68kSimulator::HardwareReset() {
  // hardwareu.cpp ResetBtnClick()
  state_.sr |= kSrSupervisor;
  state_.sr &= static_cast<uint16_t>(~kSrTrace);
  state_.SetInterruptMask(7);
  state_.ssp = memory_.ReadLong(0x0).value;
  state_.pc = memory_.ReadLong(0x4).value & kAddressMask;
  IncrementCycles(40);  // reset execution time
  state_.stopped = false;
  state_.halted = false;
}

void M68kSimulator::ConfigureMemoryMap(const MemoryMapConfig& map) {
  memory_.SetFlags(0, kAddressMask, kMemNormal);
  auto apply = [this](const MemoryMapConfig::Range& r, uint8_t flag) {
    if (r.enabled)
      memory_.AddFlags(r.start, r.end, flag);
  };
  apply(map.invalid, kMemInvalid);
  apply(map.protected_, kMemProtected);
  apply(map.read, kMemReadOnly);
  apply(map.rom, kMemRom);
}

void M68kSimulator::SetHardwareAddresses(uint32_t seg7, uint32_t leds, uint32_t switches,
                                         uint32_t buttons) {
  memory_.SetMapExemptions({{seg7 & kAddressMask, (seg7 + 15) & kAddressMask},
                            {leds & kAddressMask, leds & kAddressMask},
                            {switches & kAddressMask, switches & kAddressMask},
                            {buttons & kAddressMask, buttons & kAddressMask}});
}

// =============================================================================
// Fetch helpers
// =============================================================================

uint16_t M68kSimulator::FetchWord() {
  uint16_t val = static_cast<uint16_t>(memory_.ReadWord(state_.pc).value);
  state_.pc = (state_.pc + 2) & kAddressMask;
  return val;
}

uint32_t M68kSimulator::FetchLong() {
  uint32_t val = memory_.ReadLong(state_.pc).value;
  state_.pc = (state_.pc + 4) & kAddressMask;
  return val;
}

// =============================================================================
// EA wrappers (forward state_/memory_ to free functions from addressing_mode.h)
// =============================================================================

EffectiveAddr M68kSimulator::CalcEA(int mode, int reg, DataSize size) {
  return ::easym68k::sim::CalculateEA(memory_, state_, state_.pc, mode, reg, size);
}

uint32_t M68kSimulator::ReadEA(const EffectiveAddr& ea, DataSize size) {
  return ::easym68k::sim::ReadEA(memory_, state_, ea, size);
}

SimResult M68kSimulator::WriteEA(const EffectiveAddr& ea, uint32_t value, DataSize size) {
  return ::easym68k::sim::WriteEA(memory_, state_, ea, value, size);
}

// =============================================================================
// Condition check — all 16 68000 conditions
// Ported from EASy68K/Sim68K/UTILS.CPP: check_condition()
// =============================================================================

bool M68kSimulator::CheckCondition(Condition cond) const {
  bool c = state_.GetFlag(kSrCarry);
  bool v = state_.GetFlag(kSrOverflow);
  bool z = state_.GetFlag(kSrZero);
  bool n = state_.GetFlag(kSrNegative);

  switch (cond) {
    case Condition::kTrue:
      return true;
    case Condition::kFalse:
      return false;
    case Condition::kHigh:
      return !c && !z;
    case Condition::kLowOrSame:
      return c || z;
    case Condition::kCarryClear:
      return !c;
    case Condition::kCarrySet:
      return c;
    case Condition::kNotEqual:
      return !z;
    case Condition::kEqual:
      return z;
    case Condition::kOverflowClear:
      return !v;
    case Condition::kOverflowSet:
      return v;
    case Condition::kPlus:
      return !n;
    case Condition::kMinus:
      return n;
    case Condition::kGreaterOrEqual:
      return (n && v) || (!n && !v);
    case Condition::kLessThan:
      return (n && !v) || (!n && v);
    case Condition::kGreaterThan:
      return (n && v && !z) || (!n && !v && !z);
    case Condition::kLessOrEqual:
      return z || (n && !v) || (!n && v);
  }
  return false;
}

// =============================================================================
// Exception handler
// Ported from EASy68K/Sim68K/RUN.CPP: exceptionHandler() class 1/2 path.
// Pushes PC (longword) then SR (word) onto SSP, enters supervisor, clears trace.
// =============================================================================

void M68kSimulator::HandleException(ExceptionVector vector, uint32_t stacked_pc) {
  uint32_t vec_addr = static_cast<uint32_t>(vector) * 4;
  if (!SupervisorStackUsable()) {
    // RUN.CPP exceptionHandler(): "SSP odd or outside memory space" — the
    // vector has been fetched but no frame is built and execution stops.
    state_.pc = memory_.ReadLong(vec_addr).value & kAddressMask;
    state_.halted = true;
    return;
  }
  memory_.SetSupervisor(true);  // exception processing runs in supervisor state
  state_.ssp -= 4;
  memory_.WriteLong(state_.ssp & kAddressMask, stacked_pc);
  state_.ssp -= 2;
  memory_.WriteWord(state_.ssp & kAddressMask, state_.sr);
  state_.sr |= kSrSupervisor;
  state_.sr &= static_cast<uint16_t>(~kSrTrace);
  state_.pc = memory_.ReadLong(vec_addr).value & kAddressMask;
  if (!observers_.empty()) {
    SimEvent ev{SimEventType::kExceptionRaised, inst_pc_, SimResult::kOk};
    ev.vector = static_cast<int>(vector);
    Notify(ev);
  }
}

bool M68kSimulator::SupervisorStackUsable() const {
  // RUN.CPP exceptionHandler(): A[8] odd, negative or > MEMSIZE is fatal.
  return (state_.ssp & 1) == 0 && state_.ssp <= 0x01000000u;
}

// =============================================================================
// Class 0 exception (address / bus error)
// Ported from RUN.CPP exceptionHandler() class 0 path and UTILS.CPP
// mem_put()/mem_req(). Called when an instruction (or its opcode fetch)
// faulted; `before` is the CPU state at the start of the instruction.
// =============================================================================

SimResult M68kSimulator::HandleMemoryFault(const CpuState& before, uint16_t opcode) {
  MemoryFault fault = memory_.GetFault();
  memory_.ClearFault();

  // The original returns from the instruction as soon as the access fails,
  // before results are stored; restore the registers (cycles included) to
  // their state at the start of the instruction.
  state_ = before;

  bool double_fault = last_step_faulted_;
  last_step_faulted_ = true;

  if (!exceptions_enabled_.load(std::memory_order_relaxed)) {
    // "Address Error/Bus Error: Instruction at ... accessing address ..."
    state_.halted = true;
    NotifyHaltOrStop(fault.kind);
    return fault.kind;
  }

  if (double_fault) {
    // RUN.CPP runprog(): "Address or Bus error during exception processing."
    state_.halted = true;
    NotifyHaltOrStop(fault.kind);
    return fault.kind;
  }

  ExceptionVector vector = (fault.kind == SimResult::kAddressError) ? ExceptionVector::kAddressError
                                                                    : ExceptionVector::kBusError;
  uint32_t vec_addr = static_cast<uint32_t>(vector) * 4;

  if (!SupervisorStackUsable()) {
    state_.pc = memory_.ReadLong(vec_addr).value & kAddressMask;
    state_.halted = true;
    NotifyHaltOrStop(fault.kind);
    return fault.kind;
  }

  // Stacked PC: reads use OLD_PC = PC-2 at the time of the access
  // (mem_req()), writes use the instruction address; the frame holds OLD_PC+2.
  uint32_t frame_pc = fault.is_read ? fault.pc : inst_pc_ + 2;
  uint16_t info = (fault.address == frame_pc) ? 0x6 : 0x5;  // FC: program / data
  if (fault.is_read)
    info |= 0x10;

  IncrementCycles(50);  // address or bus exception time
  memory_.SetSupervisor(true);
  state_.ssp -= 4;
  memory_.WriteLong(state_.ssp & kAddressMask, frame_pc);
  state_.ssp -= 2;
  memory_.WriteWord(state_.ssp & kAddressMask, state_.sr);
  state_.ssp -= 2;
  memory_.WriteWord(state_.ssp & kAddressMask, opcode);
  state_.ssp -= 4;
  memory_.WriteLong(state_.ssp & kAddressMask, fault.address);
  state_.ssp -= 2;
  memory_.WriteWord(state_.ssp & kAddressMask, info);
  memory_.ClearFault();

  state_.sr |= kSrSupervisor;
  state_.sr &= static_cast<uint16_t>(~kSrTrace);
  state_.pc = memory_.ReadLong(vec_addr).value & kAddressMask;

  if (!observers_.empty()) {
    SimEvent ev{SimEventType::kExceptionRaised, inst_pc_, fault.kind};
    ev.vector = static_cast<int>(vector);
    Notify(ev);
  }
  return fault.kind;
}

// =============================================================================
// Step — fetch, execute, handle exception
// =============================================================================

SimResult M68kSimulator::Step() {
  if (async_irq_events_.load(std::memory_order_relaxed) != 0 ||
      async_resumed_event_.load(std::memory_order_relaxed))
    DeliverAsyncEvents();  // drains even with no observers registered

  if (state_.halted)
    return SimResult::kHalted;

  // Check for pending non-masked interrupts before fetching. Interrupts are
  // only serviced while exception processing is enabled (RUN.CPP).
  uint8_t mask = pending_irqs_.load(std::memory_order_relaxed);
  if (mask != 0 && exceptions_enabled_.load(std::memory_order_relaxed)) {
    int current_mask = state_.GetInterruptMask();
    for (int lvl = 7; lvl >= 1; --lvl) {
      if ((mask & (1 << (lvl - 1))) != 0) {
        if (lvl == 7 || lvl > current_mask) {
          HandleIrq(lvl);
          return SimResult::kOk;
        }
      }
    }
  }

  inst_pc_ = state_.pc;
  bool trace_before = state_.GetFlag(kSrTrace);

  // Snapshot for class 0 faults; memory map checks use the current mode.
  const CpuState before = state_;
  memory_.ClearFault();
  memory_.SetSupervisor(state_.IsSupervisor());

  uint16_t opcode = FetchWord();
  if (memory_.HasFault())  // odd PC or bus error on opcode fetch
    return HandleMemoryFault(before, last_opcode_);
  last_opcode_ = opcode;

  // Execution logging happens before the instruction executes, as in
  // RUN.CPP exec_inst(): memory rows, then registers + instruction line.
  if (config_.execution_logger != nullptr) {
    IExecutionLogger* log = config_.execution_logger;
    IExecutionLogger::Mode mode = log->GetMode();
    if (mode != IExecutionLogger::Mode::kOff) {
      if (mode == IExecutionLogger::Mode::kInstructionsRegistersAndMemory)
        log->LogMemoryDump(log->GetDumpStart(), log->GetDumpBytes(), memory_);
      log->LogInstruction(inst_pc_, opcode, state_);
    }
  }

  SimResult result = ExecuteInstruction(opcode);
  if (!observers_.empty())
    Notify(SimEventType::kInstructionExecuted, inst_pc_, result);

  if (memory_.HasFault())
    return HandleMemoryFault(before, opcode);
  last_step_faulted_ = false;

  if (!exceptions_enabled_.load(std::memory_order_relaxed)) {
    // RUN.CPP runprog(), "exception processing not enabled": every exception
    // condition halts the program; no stack frame, no exception cycles, and
    // the trace bit is ignored.
    if (result == SimResult::kStopInstruction)
      state_.stopped = true;
    if (result != SimResult::kOk)
      state_.halted = true;
    NotifyHaltOrStop(result);
    return result;
  }

  // Exception cycle counts follow RUN.CPP runprog() exception processing.
  switch (result) {
    case SimResult::kOk:
      if (trace_before) {
        IncrementCycles(kCyclesTrace);
        HandleException(ExceptionVector::kTrace, state_.pc);
        return SimResult::kTraceException;
      }
      break;
    case SimResult::kBadInstruction:
      IncrementCycles(kCyclesIllegalInst);
      HandleException(ExceptionVector::kIllegalInst, inst_pc_);
      break;
    case SimResult::kPrivilegeViolation:
      IncrementCycles(kCyclesPrivilege);
      HandleException(ExceptionVector::kPrivilege, state_.pc);
      break;
    case SimResult::kDivideByZero:
      IncrementCycles(kCyclesDivideByZero);
      HandleException(ExceptionVector::kDivideByZero, state_.pc);
      break;
    case SimResult::kChkException:
      IncrementCycles(kCyclesChk);
      HandleException(ExceptionVector::kChk, state_.pc);
      break;
    case SimResult::kTrapVException:
      IncrementCycles(kCyclesTrapV);
      HandleException(ExceptionVector::kTrapV, state_.pc);
      break;
    case SimResult::kTrapException:
      IncrementCycles(kCyclesTrap);
      HandleException(
          static_cast<ExceptionVector>(static_cast<int>(ExceptionVector::kTrap0) + (opcode & 0x0F)),
          state_.pc);
      break;
    case SimResult::kTraceException:
      IncrementCycles(kCyclesTrace);
      HandleException(ExceptionVector::kTrace, state_.pc);
      break;
    case SimResult::kLine1010:
      IncrementCycles(kCyclesLineA);
      HandleException(ExceptionVector::kLine1010, state_.pc);
      break;
    case SimResult::kLine1111:
      IncrementCycles(kCyclesLineF);
      HandleException(ExceptionVector::kLine1111, state_.pc);
      break;
    case SimResult::kStopInstruction:
      state_.halted = true;
      state_.stopped = true;
      break;
    case SimResult::kAddressError:
    case SimResult::kBusError:
    case SimResult::kHalted:
      state_.halted = true;
      break;
  }

  NotifyHaltOrStop(result);
  return result;
}

void M68kSimulator::NotifyHaltOrStop(SimResult result) {
  if (observers_.empty() || !state_.halted)
    return;
  Notify(state_.stopped ? SimEventType::kStopped : SimEventType::kHalted, inst_pc_, result);
}

// =============================================================================
// Run
// =============================================================================

SimResult M68kSimulator::Run(int max_instructions) {
  int count = 0;
  SimResult result = SimResult::kOk;

  while (!stop_requested_.load(std::memory_order_relaxed) && !state_.halted) {
    if (HasBreakpoint(state_.pc)) {
      if (!observers_.empty())
        Notify(SimEventType::kBreakpointHit, state_.pc);
      break;
    }
    // Like the original run loop, keep running through exceptions (which
    // vector to their handlers); stop only on halt, breakpoint or request.
    result = Step();
    if (state_.halted)
      break;
    ++count;
    if (max_instructions >= 0 && count >= max_instructions)
      break;
  }

  return result;
}

void M68kSimulator::RequestStop() {
  stop_requested_.store(true, std::memory_order_relaxed);
}

// =============================================================================
// Breakpoints
// =============================================================================

void M68kSimulator::AddBreakpoint(uint32_t address) {
  if (!HasBreakpoint(address)) {
    breakpoints_.push_back({address & kAddressMask, true});
  }
}

void M68kSimulator::RemoveBreakpoint(uint32_t address) {
  address &= kAddressMask;
  breakpoints_.erase(
      std::remove_if(breakpoints_.begin(), breakpoints_.end(),
                     [address](const Breakpoint& bp) { return bp.address == address; }),
      breakpoints_.end());
}

void M68kSimulator::ClearBreakpoints() {
  breakpoints_.clear();
}

bool M68kSimulator::HasBreakpoint(uint32_t address) const {
  address &= kAddressMask;
  for (const auto& bp : breakpoints_) {
    if (bp.enabled && bp.address == address)
      return true;
  }
  return false;
}

// =============================================================================
// Program loading
// =============================================================================

bool M68kSimulator::LoadSRecord(const std::string& filename) {
  SRecordLoader loader;
  auto r = loader.LoadFromFile(filename, memory_);
  if (r.success && r.has_start_address)
    start_address_ = r.start_address;
  return r.success;
}

bool M68kSimulator::LoadSRecordLines(const std::vector<std::string>& lines) {
  SRecordLoader loader;
  auto r = loader.LoadFromLines(lines, memory_);
  if (r.success && r.has_start_address)
    start_address_ = r.start_address;
  return r.success;
}

// =============================================================================
// Interrupts
// =============================================================================

void M68kSimulator::Interrupt(int level) {
  if (level < 1 || level > 7) {
    return;
  }

  // Set bit atomically
  pending_irqs_.fetch_or(static_cast<uint8_t>(1 << (level - 1)));
  // Observer events are delivered on the simulator thread (next Step()).
  async_irq_events_.fetch_or(static_cast<uint8_t>(1 << (level - 1)));

  // Wake from STOP if not masked (only when interrupts can be serviced)
  if (state_.stopped && exceptions_enabled_.load(std::memory_order_relaxed)) {
    int current_mask = state_.GetInterruptMask();
    if (level == 7 || level > current_mask) {
      state_.stopped = false;
      state_.halted = false;
      async_resumed_event_.store(true);
    }
  }
}

uint8_t M68kSimulator::GetPendingIrqs() const {
  return pending_irqs_.load(std::memory_order_relaxed);
}

void M68kSimulator::ClearPendingIrqs() {
  pending_irqs_.store(0, std::memory_order_relaxed);
}

// =============================================================================
// Runtime toggles
// =============================================================================

void M68kSimulator::SetSimhaltEnabled(bool enabled) {
  simhalt_enabled_.store(enabled, std::memory_order_relaxed);
}

bool M68kSimulator::IsSimhaltEnabled() const {
  return simhalt_enabled_.load(std::memory_order_relaxed);
}

void M68kSimulator::SetExceptionsEnabled(bool enabled) {
  exceptions_enabled_.store(enabled, std::memory_order_relaxed);
}

bool M68kSimulator::IsExceptionsEnabled() const {
  return exceptions_enabled_.load(std::memory_order_relaxed);
}

void M68kSimulator::HandleIrq(int level) {
  if (!SupervisorStackUsable()) {
    state_.halted = true;  // RUN.CPP exceptionHandler(): SSP odd/out of range
    return;
  }
  memory_.SetSupervisor(true);
  // Push exception stack frame
  state_.ssp -= 4;
  memory_.WriteLong(state_.ssp & kAddressMask, state_.pc);
  state_.ssp -= 2;
  memory_.WriteWord(state_.ssp & kAddressMask, state_.sr);

  // Set SR supervisor bit
  state_.sr |= kSrSupervisor;
  // Clear trace flag
  state_.sr &= static_cast<uint16_t>(~kSrTrace);

  // Set SR interrupt mask to serviced level
  state_.SetInterruptMask(level);

  // Clear serviced interrupt bit
  pending_irqs_.fetch_and(static_cast<uint8_t>(~(1 << (level - 1))));

  // Load PC from autovector address
  uint32_t vec_addr = (24 + level) * 4;
  state_.pc = memory_.ReadLong(vec_addr).value & kAddressMask;

  // Add 34 cycles penalty (RUN.CPP irqHandler() inc_cyc(34))
  IncrementCycles(kCyclesIrq);

  if (!observers_.empty()) {
    SimEvent ev{SimEventType::kExceptionRaised, state_.pc, SimResult::kOk};
    ev.interrupt_level = level;
    ev.vector = 24 + level;  // autovector
    Notify(ev);
  }
}

// =============================================================================
// Observers
// =============================================================================

void M68kSimulator::AddObserver(ISimObserver* observer) {
  if (observer == nullptr)
    return;
  for (ISimObserver* o : observers_) {
    if (o == observer)
      return;
  }
  observers_.push_back(observer);
}

void M68kSimulator::RemoveObserver(ISimObserver* observer) {
  for (auto& o : observers_) {
    if (o == observer) {
      o = nullptr;  // never erase during delivery; compact afterwards
      observers_dirty_ = true;
    }
  }
  if (notify_depth_ == 0 && observers_dirty_) {
    observers_.erase(std::remove(observers_.begin(), observers_.end(), nullptr), observers_.end());
    observers_dirty_ = false;
  }
}

void M68kSimulator::Notify(const SimEvent& event) {
  ++notify_depth_;
  // Fixed bound: observers added during delivery get the next event.
  const size_t n = observers_.size();
  for (size_t i = 0; i < n; ++i) {
    if (observers_[i] != nullptr)
      observers_[i]->OnEvent(event);
  }
  if (--notify_depth_ == 0 && observers_dirty_) {
    observers_.erase(std::remove(observers_.begin(), observers_.end(), nullptr), observers_.end());
    observers_dirty_ = false;
  }
}

void M68kSimulator::Notify(SimEventType type, uint32_t pc, SimResult result) {
  Notify(SimEvent{type, pc, result});
}

void M68kSimulator::DeliverAsyncEvents() {
  uint8_t levels = async_irq_events_.exchange(0);
  bool resumed = async_resumed_event_.exchange(false);
  if (observers_.empty())
    return;
  for (int lvl = 1; lvl <= 7; ++lvl) {
    if (levels & (1 << (lvl - 1))) {
      SimEvent ev{SimEventType::kInterruptRequested, state_.pc, SimResult::kOk};
      ev.interrupt_level = lvl;
      Notify(ev);
    }
  }
  if (resumed)
    Notify(SimEventType::kResumed, state_.pc);
}

}  // namespace easym68k::sim
