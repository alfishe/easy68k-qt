// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// M68kSimulator — 68000 simulator core.
// Ported from EASy68K/Sim68K/RUN.CPP: runprog(), exec_inst(), exceptionHandler().

#ifndef EASY68K_SIM_SIMULATOR_H_
#define EASY68K_SIM_SIMULATOR_H_

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "easym68k/sim/break_condition.h"
#include "easym68k/sim/cpu_state.h"
#include "easym68k/sim/effective_addr.h"
#include "easym68k/sim/isim_observer.h"
#include "easym68k/sim/memory.h"
#include "easym68k/sim/types.h"

namespace easym68k::sim {

// Forward declarations for Trap #15 I/O interfaces (injected via Config)
class ITextIO;
class IFileIO;
class ISerialIO;
class INetworkIO;
class IGraphicsIO;
class ISoundIO;
class IPeripheralIO;
class ISimulatorEnv;
class IPrintIO;
class ILogger;
class IExecutionLogger;

struct Breakpoint {
  uint32_t address;
  bool enabled;
};

class M68kSimulator {
 public:
  struct Config {
    ITextIO* text_io = nullptr;
    IFileIO* file_io = nullptr;
    ISerialIO* serial_io = nullptr;
    INetworkIO* network_io = nullptr;
    IGraphicsIO* graphics_io = nullptr;
    ISoundIO* sound_io = nullptr;
    IPeripheralIO* peripheral_io = nullptr;
    ISimulatorEnv* sim_env = nullptr;
    IPrintIO* print_io = nullptr;
    ILogger* logger = nullptr;

    // Optional execution/output logging (Options > Log Output). Null = off,
    // with no per-instruction cost beyond a pointer check.
    IExecutionLogger* execution_logger = nullptr;
    // SIMHALT ($FFFF $FFFF) halts the simulator when true; when false it is a
    // Line 1111 exception (`*[sim68k]simhalt_off`, SIM68Ku.cpp).
    bool simhalt_enabled = true;

    // Exception processing (Options > Enable Exceptions). When false, any
    // exception condition halts the simulator instead of vectoring, trace is
    // ignored and interrupts are not serviced (RUN.CPP runprog()).
    bool exceptions_enabled = true;

    // 68020 bit field instructions (Options > Enable Bit Field Instructions,
    // `*[sim68k]bitfield`); off by default as in Sim68K. When off they are
    // illegal instructions.
    bool bitfield_enabled = false;
  };

  explicit M68kSimulator(Config config);
  ~M68kSimulator() = default;

  M68kSimulator(const M68kSimulator&) = delete;
  M68kSimulator& operator=(const M68kSimulator&) = delete;

  // Loads reset vectors from memory and enters supervisor mode (power-on
  // style reset; registers and cycles cleared, SR = S only).
  void Reset();

  // Program start / Rewind, as the original's initSim() after loading a
  // program: D0-D7/A0-A6 = 0, cycles = 0, SR = $2000, USP = $00FF0000,
  // SSP = $01000000, PC = start address. Vectors are NOT read. Clears halt,
  // STOP, pending interrupts and stop requests.
  void InitProgram();

  // Hardware window Reset button (hardwareu.cpp ResetBtnClick()): enter
  // supervisor, clear trace, interrupt mask = 7, SSP/PC from vectors $0/$4,
  // +40 cycles, wake from STOP. Other registers are kept.
  void HardwareReset();

  // Program start address: set by LoadSRecord*() from the S7/S8/S9 record;
  // a GUI may override it (the original uses the .L68 listing's first line).
  uint32_t GetStartAddress() const { return start_address_; }
  void SetStartAddress(uint32_t address) { start_address_ = address & kAddressMask; }

  // Hardware window memory map (one optional range per type, as in the
  // original). Replaces all map flags in memory.
  struct MemoryMapConfig {
    struct Range {
      bool enabled = false;
      uint32_t start = 0;
      uint32_t end = 0;
    };
    Range rom;         // writes ignored
    Range read;        // bus error on write
    Range protected_;  // bus error unless supervisor
    Range invalid;     // bus error on access
  };
  void ConfigureMemoryMap(const MemoryMapConfig& map);

  // Hardware I/O addresses are always accessible regardless of the memory map
  // (UTILS.CPP memoryMapCheck()): 7-segment display (16 bytes), LEDs,
  // toggle switches and push buttons.
  void SetHardwareAddresses(uint32_t seg7, uint32_t leds, uint32_t switches, uint32_t buttons);

  // Executes a single instruction, handles exceptions, then evaluates the
  // advanced breakpoints (as RUN.CPP runprog() does after exec_inst()).
  SimResult Step();

  // Runs until halt, stop_requested_, breakpoint, or max_instructions.
  // Like runprog(), each iteration executes an instruction first and then
  // checks the PC breakpoints and the advanced breakpoints, so a run started
  // on a breakpoint address executes that instruction.
  // max_instructions < 0 means unlimited.
  SimResult Run(int max_instructions = -1);

  // Step Over (SIM68Ku.cpp StepExecute): executes one instruction; if it is a
  // BSR/JSR, keeps running until execution returns to the instruction after
  // it (CODE8.CPP stepToAddr), a breakpoint is hit, the program halts or a
  // stop is requested.
  SimResult StepOver();

  // Run To Cursor (SIM68Ku.cpp RunToCursorExecute): runs until `address` is
  // reached unless it is already a PC breakpoint. The run-to address stays
  // armed until it is reached or the program is initialized (runToAddr), so
  // a run stopped earlier by a breakpoint still stops there later.
  SimResult RunTo(uint32_t address, int max_instructions = -1);
  uint32_t GetRunToAddress() const { return run_to_addr_; }

  // Resets the pass counters of the break expressions; Sim68K does this when
  // Run is selected (BreaksFrm->resetDebug()).
  void ResetBreakPassCounts();

  // Thread-safe stop request; checked at top of each Run iteration.
  void RequestStop();

  // Inject a hardware interrupt at the given level (1-7).
  // Thread-safe; may be called from any thread (e.g., timer, hardware sim).
  // The interrupt is processed on the next Step() or Run() iteration.
  void Interrupt(int level);

  // Returns the current pending IRQ bitmask (for testing).
  uint8_t GetPendingIrqs() const;

  // Clear all pending interrupts.
  void ClearPendingIrqs();

  // Runtime toggles, initialised from Config. Thread-safe; the simulator owns
  // these flags (Trap #15 task 32 subtask 5 re-enables exceptions directly).
  void SetSimhaltEnabled(bool enabled);
  bool IsSimhaltEnabled() const;
  void SetExceptionsEnabled(bool enabled);
  bool IsExceptionsEnabled() const;
  void SetBitFieldEnabled(bool enabled);
  bool IsBitFieldEnabled() const;

  // Event observers (see isim_observer.h for threading rules). Null and
  // duplicate registrations are ignored. Safe to call from inside OnEvent().
  void AddObserver(ISimObserver* observer);
  void RemoveObserver(ISimObserver* observer);

  // State and memory accessors
  CpuState& State() { return state_; }
  const CpuState& State() const { return state_; }
  Memory& GetMemory() { return memory_; }
  const Memory& GetMemory() const { return memory_; }

  // Advanced breakpoints (break_condition.h)
  void SetBreakCondition(int index, const BreakCondition& condition);
  const BreakCondition& GetBreakCondition(int index) const;
  // Replaces the expressions (at most kMaxBreakExpressions) and resets their
  // pass counters.
  void SetBreakExpressions(std::vector<BreakExpression> expressions);
  const std::vector<BreakExpression>& GetBreakExpressions() const { return break_expressions_; }
  void ClearAdvancedBreakpoints();
  // True if an advanced breakpoint expression broke after the last Step().
  bool AdvancedBreakpointHit() const { return advanced_break_hit_; }

  // Breakpoints
  void AddBreakpoint(uint32_t address);
  void RemoveBreakpoint(uint32_t address);
  void ClearBreakpoints();
  bool HasBreakpoint(uint32_t address) const;

  // Program loading
  bool LoadSRecord(const std::string& filename);
  bool LoadSRecordLines(const std::vector<std::string>& lines);

  // Condition check — public for direct testability.
  bool CheckCondition(Condition cond) const;

  // Fetch next instruction word/long from PC (advances PC).
  uint16_t FetchWord();
  uint32_t FetchLong();

 private:
  SimResult ExecuteInstruction(uint16_t opcode);

  // Adds count to the machine cycle counter (port of UTILS.CPP inc_cyc()).
  void IncrementCycles(uint64_t count) { state_.cycles += count; }

  // EA wrappers that use state_.pc and state_ implicitly.
  EffectiveAddr CalcEA(int mode, int reg, DataSize size);
  uint32_t ReadEA(const EffectiveAddr& ea, DataSize size);
  SimResult WriteEA(const EffectiveAddr& ea, uint32_t value, DataSize size);

  // Pushes stack frame for vector, sets supervisor mode, clears trace, loads new PC.
  // Pushes stacked_pc and SR on the SSP, sets supervisor, clears trace, loads
  // the vector. RUN.CPP stacks OLD_PC = PC after the instruction for every
  // class 1/2 exception except illegal instruction (OLD_PC -= 2).
  void HandleException(ExceptionVector vector, uint32_t stacked_pc);

  // Process the highest priority pending interrupt.
  void HandleIrq(int level);

  // Class 0 (address/bus error) exception processing for a faulted step.
  SimResult HandleMemoryFault(const CpuState& before, uint16_t opcode);
  // False if SSP is odd or beyond the 16 MB space (no frame can be built).
  bool SupervisorStackUsable() const;

  // Observer notification helpers.
  void Notify(const SimEvent& event);
  void Notify(SimEventType type, uint32_t pc, SimResult result = SimResult::kOk);
  void DeliverAsyncEvents();  // events recorded by Interrupt() on other threads
  void NotifyHaltOrStop(SimResult result);

  // Hierarchical instruction decoder (defined in decode.cc).
  SimResult DispatchGroup0(uint16_t opcode);
  SimResult OpMove(uint16_t opcode);
  SimResult DispatchMoveOrMovea(uint16_t opcode);
  SimResult DispatchGroup4(uint16_t opcode);
  SimResult DispatchGroup5(uint16_t opcode);
  SimResult DispatchGroup6(uint16_t opcode);
  SimResult OpMoveq(uint16_t opcode);
  SimResult DispatchGroup8(uint16_t opcode);
  SimResult DispatchGroup9(uint16_t opcode);
  SimResult OpLine1010(uint16_t opcode);
  SimResult DispatchGroupB(uint16_t opcode);
  SimResult DispatchGroupC(uint16_t opcode);
  SimResult DispatchGroupD(uint16_t opcode);
  SimResult DispatchGroupE(uint16_t opcode);
  SimResult DispatchSimhalt(uint16_t opcode);

  // Leaf instruction implementations (opcodes_move.cc)
  SimResult OpMovea(uint16_t opcode);
  SimResult OpMovem(uint16_t opcode);
  SimResult OpMovep(uint16_t opcode);
  SimResult OpMoveFrSr(uint16_t opcode);
  SimResult OpMoveToCcr(uint16_t opcode);
  SimResult OpMoveToSr(uint16_t opcode);
  SimResult OpMoveUsp(uint16_t opcode);
  SimResult OpExg(uint16_t opcode);
  SimResult OpLea(uint16_t opcode);
  SimResult OpPea(uint16_t opcode);
  SimResult OpLink(uint16_t opcode);
  SimResult OpUnlk(uint16_t opcode);
  SimResult OpSwap(uint16_t opcode);

  // Leaf instruction implementations (opcodes_arithmetic.cc)
  SimResult OpAdd(uint16_t opcode);
  SimResult OpAdda(uint16_t opcode);
  SimResult OpAddi(uint16_t opcode);
  SimResult OpAddq(uint16_t opcode);
  SimResult OpAddx(uint16_t opcode);
  SimResult OpSub(uint16_t opcode);
  SimResult OpSuba(uint16_t opcode);
  SimResult OpSubi(uint16_t opcode);
  SimResult OpSubq(uint16_t opcode);
  SimResult OpSubx(uint16_t opcode);
  SimResult OpMulu(uint16_t opcode);
  SimResult OpMuls(uint16_t opcode);
  SimResult OpDivu(uint16_t opcode);
  SimResult OpDivs(uint16_t opcode);
  SimResult OpNeg(uint16_t opcode);
  SimResult OpNegx(uint16_t opcode);
  SimResult OpClr(uint16_t opcode);
  SimResult OpCmp(uint16_t opcode);
  SimResult OpCmpa(uint16_t opcode);
  SimResult OpCmpi(uint16_t opcode);
  SimResult OpCmpm(uint16_t opcode);
  SimResult OpTst(uint16_t opcode);
  SimResult OpExt(uint16_t opcode);
  SimResult OpAbcd(uint16_t opcode);
  SimResult OpSbcd(uint16_t opcode);
  SimResult OpNbcd(uint16_t opcode);
  SimResult OpChk(uint16_t opcode);

  // Leaf instruction implementations (opcodes_logic.cc)
  SimResult OpOr(uint16_t opcode);
  SimResult OpOri(uint16_t opcode);
  SimResult OpOriToCcr(uint16_t opcode);
  SimResult OpOriToSr(uint16_t opcode);
  SimResult OpAnd(uint16_t opcode);
  SimResult OpAndi(uint16_t opcode);
  SimResult OpAndiToCcr(uint16_t opcode);
  SimResult OpAndiToSr(uint16_t opcode);
  SimResult OpEor(uint16_t opcode);
  SimResult OpEori(uint16_t opcode);
  SimResult OpEoriToCcr(uint16_t opcode);
  SimResult OpEoriToSr(uint16_t opcode);
  SimResult OpNot(uint16_t opcode);
  SimResult OpTas(uint16_t opcode);

  // Leaf instruction implementations (opcodes_branch.cc)
  SimResult OpBra(uint16_t opcode);
  SimResult OpBsr(uint16_t opcode);
  SimResult OpBcc(uint16_t opcode);
  SimResult OpDbcc(uint16_t opcode);
  SimResult OpScc(uint16_t opcode);
  SimResult OpJmp(uint16_t opcode);
  SimResult OpJsr(uint16_t opcode);
  SimResult OpRts(uint16_t opcode);
  SimResult OpRte(uint16_t opcode);
  SimResult OpRtr(uint16_t opcode);

  // Leaf instruction implementations (opcodes_misc.cc)
  SimResult OpTrap(uint16_t opcode);
  SimResult OpTrapV(uint16_t opcode);
  SimResult OpIllegal(uint16_t opcode);
  SimResult OpReset(uint16_t opcode);
  SimResult OpStop(uint16_t opcode);

  // Trap #15 I/O dispatch (trap15_dispatch.cc)
  SimResult DispatchTrap15();

  // Trap #15 text output helpers: display via ITextIO and, when output logging
  // is enabled, copy to the execution logger (logU.cpp OlogFile).
  void EmitText(const char* text, bool crlf);
  void EmitChar(char ch);
  void LogOutputText(const char* text, int length);

  // Leaf instruction implementations (opcodes_shift.cc)
  SimResult OpShiftRotate(uint16_t opcode);
  SimResult OpBitField(uint16_t opcode);
  SimResult OpBtst(uint16_t opcode);
  SimResult OpBchg(uint16_t opcode);
  SimResult OpBclr(uint16_t opcode);
  SimResult OpBset(uint16_t opcode);

  CpuState state_{};
  Memory memory_;
  Config config_;
  std::vector<Breakpoint> breakpoints_;
  BreakCondition break_conditions_[kMaxBreakConditions];
  std::vector<BreakExpression> break_expressions_;
  std::vector<int> break_pass_counts_;  // bpCountCond[]
  bool advanced_break_hit_ = false;
  bool step_over_ = false;     // sstep
  uint32_t step_to_addr_ = 0;  // stepToAddr (0 = none)
  uint32_t run_to_addr_ = 0;   // runToAddr (0 = none)
  // CODE8.CPP BSR/JSR: in Step Over mode the first call sets the return
  // address to stop at.
  void NoteSubroutineCall(uint32_t return_address) {
    if (step_over_ && step_to_addr_ == 0)
      step_to_addr_ = return_address;
  }
  SimResult StepInstruction();
  bool IsConditionMet(const BreakCondition& condition);
  bool IsExpressionBreak(int index);
  bool EvaluateBreakExpressions();
  std::atomic<bool> stop_requested_{false};
  std::atomic<uint8_t> pending_irqs_{0};
  std::atomic<bool> simhalt_enabled_{true};
  std::atomic<bool> exceptions_enabled_{true};
  std::atomic<bool> bitfield_enabled_{false};
  std::vector<ISimObserver*> observers_;
  int notify_depth_{0};                       // >0 while delivering an event
  bool observers_dirty_{false};               // null slots to compact after delivery
  std::atomic<uint8_t> async_irq_events_{0};  // levels passed to Interrupt()
  std::atomic<bool> async_resumed_event_{false};
  uint32_t inst_pc_{0};            // PC of the instruction currently executing
  uint16_t last_opcode_{0};        // IR stacked when the opcode fetch faults
  bool last_step_faulted_{false};  // RUN.CPP errflg (double fault => halt)
  uint32_t start_address_{0};
};

}  // namespace easym68k::sim

#endif  // EASY68K_SIM_SIMULATOR_H_
