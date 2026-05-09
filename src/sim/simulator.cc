// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from EASy68K/Sim68K/RUN.CPP: runprog(), exec_inst(), exceptionHandler()

#include "easym68k/sim/simulator.h"

#include <algorithm>

#include "easym68k/sim/addressing_mode.h"
#include "easym68k/sim/srecord_loader.h"

namespace easym68k::sim {

M68kSimulator::M68kSimulator(Config config) : config_(config) {}

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

void M68kSimulator::HandleException(ExceptionVector vector) {
  state_.ssp -= 4;
  memory_.WriteLong(state_.ssp & kAddressMask, inst_pc_);
  state_.ssp -= 2;
  memory_.WriteWord(state_.ssp & kAddressMask, state_.sr);
  state_.sr |= kSrSupervisor;
  state_.sr &= static_cast<uint16_t>(~kSrTrace);
  uint32_t vec_addr = static_cast<uint32_t>(vector) * 4;
  state_.pc = memory_.ReadLong(vec_addr).value & kAddressMask;
}

// =============================================================================
// Step — fetch, execute, handle exception
// =============================================================================

SimResult M68kSimulator::Step() {
  if (state_.halted)
    return SimResult::kHalted;

  inst_pc_ = state_.pc;
  bool trace_before = state_.GetFlag(kSrTrace);

  uint16_t opcode = FetchWord();
  SimResult result = ExecuteInstruction(opcode);

  switch (result) {
    case SimResult::kOk:
      if (trace_before) {
        HandleException(ExceptionVector::kTrace);
        return SimResult::kTraceException;
      }
      break;
    case SimResult::kBadInstruction:
      HandleException(ExceptionVector::kIllegalInst);
      break;
    case SimResult::kPrivilegeViolation:
      HandleException(ExceptionVector::kPrivilege);
      break;
    case SimResult::kDivideByZero:
      HandleException(ExceptionVector::kDivideByZero);
      break;
    case SimResult::kChkException:
      HandleException(ExceptionVector::kChk);
      break;
    case SimResult::kTrapVException:
      HandleException(ExceptionVector::kTrapV);
      break;
    case SimResult::kTrapException:
      HandleException(static_cast<ExceptionVector>(static_cast<int>(ExceptionVector::kTrap0) +
                                                   (opcode & 0x0F)));
      break;
    case SimResult::kTraceException:
      HandleException(ExceptionVector::kTrace);
      break;
    case SimResult::kLine1010:
      HandleException(ExceptionVector::kLine1010);
      break;
    case SimResult::kLine1111:
      HandleException(ExceptionVector::kLine1111);
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

  return result;
}

// =============================================================================
// Run
// =============================================================================

SimResult M68kSimulator::Run(int max_instructions) {
  int count = 0;
  SimResult result = SimResult::kOk;

  while (!stop_requested_.load(std::memory_order_relaxed) && !state_.halted) {
    if (HasBreakpoint(state_.pc))
      break;
    result = Step();
    if (result != SimResult::kOk)
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

}  // namespace easym68k::sim
