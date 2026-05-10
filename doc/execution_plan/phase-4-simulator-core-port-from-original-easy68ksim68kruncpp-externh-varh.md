## Phase 4: Simulator Core — Port from Original (EASy68K/Sim68K/RUN.CPP, extern.h, var.h)

### Task 4.1 — Create Simulator Header

New file: `include/easym68k/sim/simulator.h`

```cpp
#ifndef EASY68K_SIM_SIMULATOR_H_
#define EASY68K_SIM_SIMULATOR_H_

#include <atomic>
#include <cstdint>
#include <vector>

#include "easym68k/sim/types.h"
#include "easym68k/sim/effective_addr.h"
#include "easym68k/sim/memory.h"

namespace easym68k::sim {

// Forward declarations for Trap #15 interfaces
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

struct CpuState {
  uint32_t d[8];     // Data registers D0-D7
  uint32_t a[8];     // Address registers A0-A7
  uint32_t usp;      // User Stack Pointer (separate from A7)
  uint32_t ssp;      // Supervisor Stack Pointer
  uint32_t pc;       // Program Counter
  uint16_t sr;       // Status Register
  uint64_t cycles;   // Cycle counter
  bool halted;
  bool stopped;

  void Reset(Memory& memory);

  bool IsSupervisor() const { return (sr & kSrSupervisor) != 0; }
  void SetSupervisor(bool value);
  int GetInterruptMask() const { return (sr & kSrIntMask) >> 8; }
  void SetInterruptMask(int level);
  bool GetFlag(uint16_t flag) const { return (sr & flag) != 0; }
  void SetFlag(uint16_t flag, bool value);
  uint32_t GetSP() const { return IsSupervisor() ? ssp : a[7]; }
  void SetSP(uint32_t value);
};

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
  };

  explicit M68kSimulator(Config config);
  ~M68kSimulator() = default;

  // Non-copyable, movable
  M68kSimulator(const M68kSimulator&) = delete;
  M68kSimulator& operator=(const M68kSimulator&) = delete;

  // Execution
  void Reset();
  SimResult Step();
  SimResult Run(int max_instructions = -1);
  void RequestStop();

  // State access
  CpuState& State() { return state_; }
  const CpuState& State() const { return state_; }
  Memory& GetMemory() { return memory_; }
  const Memory& GetMemory() const { return memory_; }

  // Breakpoints
  int AddBreakpoint(uint32_t address);
  void RemoveBreakpoint(int id);
  void ClearBreakpoints();
  bool HasBreakpoint(uint32_t address) const;

  // Program loading
  bool LoadSRecord(const std::string& filename);
  bool LoadSRecordLines(const std::vector<std::string>& lines);

 private:
  // Fetch
  uint16_t FetchWord();
  uint32_t FetchLong();

  // Instruction dispatch
  SimResult ExecuteInstruction(uint16_t opcode);

  // Addressing mode
  EffectiveAddr CalculateEA(int mode, int reg, DataSize size);
  uint32_t ReadEA(const EffectiveAddr& ea, DataSize size);
  SimResult WriteEA(const EffectiveAddr& ea, uint32_t value, DataSize size);

  // Condition codes
  bool CheckCondition(Condition cond) const;

  // Exceptions
  void HandleException(ExceptionVector vector);

  // Flag computation (per-operation)
  void UpdateFlagsAdd(uint32_t src, uint32_t dst, uint32_t result, DataSize size);
  void UpdateFlagsSub(uint32_t src, uint32_t dst, uint32_t result, DataSize size);
  void UpdateFlagsLogic(uint32_t result, DataSize size);
  void UpdateFlagsMove(uint32_t result, DataSize size);
  void UpdateFlagsShift(uint32_t result, DataSize size, bool carry, int count);

  // Opcode handlers (declared in opcodes_*.cc)
  // ...

  CpuState state_;
  Memory memory_;
  Config config_;
  std::vector<Breakpoint> breakpoints_;
  std::atomic<bool> stop_requested_;
  uint32_t start_address_;
};

}  // namespace easym68k::sim

#endif
```

**Quality Gate 4.1:** Header compiles. No raw I/O callbacks. All Trap #15 interfaces are injected. `CpuState` has separate `usp`/`ssp` instead of `a[8]` hack.

---

### Task 4.2 — Implement Simulator Core

New file: `src/sim/simulator.cc`

Primary source: `EASy68K/Sim68K/RUN.CPP` — the original `runprog()`, `one_inst()`, and execution loop.
Primary source: `EASy68K/Sim68K/extern.h` + `var.h` — global state declarations that must be encapsulated into `M68kSimulator`.
Primary source: `EASy68K/Sim68K/UTILS.CPP` — `put()`, `value_of()`, `mem_put()`, `mem_req()`, exception handling.
Secondary reference: `EASy68K-qt/src/core/simulator/cpu.cc` for `CpuState::Reset()`, `CheckCondition()`, `HandleException()` — these are correct in EASy68K-qt and can be used as a starting point, but must be verified against the original.

Write from scratch:
- `M68kSimulator::Reset()` — uses injected Memory, reads vectors
- `M68kSimulator::Step()` — fetch opcode, dispatch, return `SimResult`
- `M68kSimulator::Run()` — loop with `stop_requested_` check and breakpoint check
- `M68kSimulator::RequestStop()` — sets atomic flag
- All flag computation methods (`UpdateFlagsAdd`, `UpdateFlagsSub`, etc.) — **complete** V/C/X/N/Z
- `FetchWord()` / `FetchLong()` — advance PC, return values from Memory

**Quality Gate 4.2:**
- `simulator_test.cc` passes: Reset, register access, Step (NOP), Run, RequestStop, breakpoints
- `CheckCondition` test: all 16 conditions verified
- `HandleException` test: stack frame format correct (PC then SR pushed)
- `UpdateFlagsAdd` test: V/C/X computed correctly for multiple operand combinations
- `UpdateFlagsSub` test: borrow/V computed correctly
- ASan build passes

---

### Task 4.3 — Simulator Unit Tests

New file: `tests/sim/simulator_test.cc`

Minimum test cases:

1. `ResetInitializesFromVectors` — SSP from $0, PC from $4
2. `SetGetRegisters` — D0, A1, PC, SR
3. `SupervisorMode` — IsSupervisor true after reset, SetSupervisor toggles
4. `InterruptMask` — Get/SetInterruptMask
5. `SPSwitching` — GetSP returns SSP in supervisor, USP in user mode
6. `StepNop` — Step executes NOP, PC advances by 2
7. `StepIllegal` — Step on illegal opcode returns `kBadInstruction`
8. `RunStopRequested` — Run until RequestStop from another thread
9. `RunBreakpoint` — Run stops at breakpoint
10. `RunMaxInstructions` — Run respects max_instructions limit
11. `CheckConditionAll16` — verify every condition code with appropriate flag setup
12. `HandleExceptionStackFrame` — PC and SR pushed in correct order, PC loaded from vector
13. `HandleExceptionSupervisorSwitch` — switches to supervisor mode on exception
14. `FlagAddOverflow` — ADD sets V correctly
15. `FlagAddCarry` — ADD sets C and X correctly
16. `FlagSubBorrow` — SUB sets C (borrow) correctly
17. `FlagLogicClearsVC` — AND/OR/EOR clear V and C, leave X unchanged
18. `FlagMoveClearsVC` — MOVE clears V and C, leaves X unchanged

**Quality Gate 4.3:** All 18 simulator tests pass.

---
