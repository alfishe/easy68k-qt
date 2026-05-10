## Phase 9: Simulator Core Library — Full Parity & Headless Readiness

This phase closes all remaining gaps between the ported simulator core and the
original EASy68K simulator (`Sim68K/RUN.CPP`, `hardwareu.cpp`, `SIM68Ku.cpp`).
After this phase, the simulator can execute any valid M68K program headlessly with
identical behavior to the original — including interrupts, cycle counting, SIMHALT
configuration, execution logging, and all exception processing.

The core must remain **GUI-independent**: all features are accessible via the
`M68kSimulator` public API and `Config` struct interfaces alone. No Qt or platform-
specific code is introduced.

**Prerequisite:** Phase 8 (Assembler) complete. Golden assembly tests (8.6) passing.
The assembler is needed to generate `.S68` test programs for Phase 9.

---

### Task 9.1 — Interrupt Injection and Processing

Port the original's interrupt system from `RUN.CPP:1083-1117` (`irqHandler()`)
and `hardwareu.cpp:757-775` (`IRQprocess()`).

**New public API on `M68kSimulator`:**

```cpp
// Inject a hardware interrupt at the given level (1-7).
// Thread-safe; may be called from any thread (e.g., timer, hardware sim).
// The interrupt is processed on the next Step() or Run() iteration.
void Interrupt(int level);

// Returns the current pending IRQ bitmask (for testing).
uint8_t GetPendingIrqs() const;

// Clear all pending interrupts.
void ClearPendingIrqs();
```

**Implementation details:**

1. Add `std::atomic<uint8_t> pending_irqs_{0}` to `M68kSimulator`. Bit 0 = IRQ1,
   bit 6 = IRQ7. `Interrupt(level)` sets the appropriate bit atomically.

2. **IRQ check in `Step()`** — after the existing exception processing `switch`,
   if `pending_irqs_ & int_mask` is non-zero, call `HandleIrq()`:
   - Push exception stack frame (class 2 format: info word + PC + SR)
   - Clear the highest-priority pending IRQ bit
   - Set the SR interrupt mask to the serviced level
   - Load PC from autovector address (`$64 + level * 4`)
   - Add 34 cycle penalty (per original `inc_cyc(34)`)

3. **STOP wake-on-interrupt** — When `state_.stopped == true` and a non-masked
   IRQ is pending, `Interrupt()` must clear `stopped` and `halted` so that the
   next `Step()` or `Run()` call resumes execution. Port from `hardwareu.cpp:763-774`.

4. **Interrupt priority** — IRQ7 (NMI) can never be masked. Levels 1-6 are masked
   when `SR interrupt mask >= IRQ level`. Port from `RUN.CPP:1086-1114`.

**Source references:**
- `RUN.CPP:878-880` — IRQ check after each instruction
- `RUN.CPP:1083-1117` — `irqHandler()` with autovector dispatch
- `hardwareu.cpp:757-775` — `IRQprocess()` with STOP wake
- `hardwareu.cpp:796-903` — Auto-IRQ timer management (GUI side, not ported here)

**New test file:** `tests/sim/interrupt_test.cc`

Minimum test cases (15):
1. `InterruptSetsPendingBit` — Interrupt(5) sets bit 4 in pending bitmask
2. `InterruptLevel1Through7` — all 7 levels set correct bits
3. `IrqBelowMaskNoException` — mask=7, IRQ level 3 → no exception
4. `IrqAboveMaskGeneratesException` — mask=3, IRQ level 5 → exception
5. `Irq7NmiCannotBeMasked` — mask=7, IRQ level 7 → exception (NMI)
6. `AutovectorDispatchCorrectAddress` — IRQ level 3 loads PC from vector $6C
7. `IrqSetsNewInterruptMask` — after servicing IRQ5, SR mask = 5
8. `IrqClearsPendingBit` — after servicing, the serviced IRQ bit is cleared
9. `IrqStackFrameCorrect` — PC and SR pushed in correct order, supervisor set
10. `IrqClearsTraceFlag` — trace mode active, IRQ clears T bit
11. `MultiplePendingIrqsHighestFirst` — IRQ3+IRQ5 pending, services IRQ5 first
12. `StopWakesOnInterrupt` — STOP instruction active, IRQ arrives, execution resumes
13. `StopDoesNotWakeOnMaskedIrq` — STOP active, masked IRQ, stays stopped
14. `ThreadSafeInterrupt` — Interrupt() called from another thread during Run()
15. `ClearPendingIrqs` — clears all pending without processing

**Quality Gate 9.1:** All 15 interrupt tests pass. `Interrupt()` is thread-safe.
STOP instruction wakes on non-masked interrupt.

---

### Task 9.2 — Cycle Counting

Port the original's `inc_cyc()` from `RUN.CPP`. The original increments a cycle
counter in every instruction handler. The ported `CpuState::cycles` field exists
but nothing increments it.

**Implementation details:**

1. Add `void IncrementCycles(uint64_t count)` private helper to `M68kSimulator`
   that adds to `state_.cycles`.

2. **Cycle data table** — Create `src/sim/cycle_data.cc` containing a lookup table
   of base cycle counts per instruction/encoding type. Source: M68000 Programmer's
   Reference Manual, Section 8 (Instruction Execution Times).

   Key cycle counts from the PRM:
   | Instruction | Byte | Word | Long | EA Penalty |
   |-------------|------|------|------|------------|
   | MOVE reg→reg | 4 | 4 | 4 | +EA |
   | MOVE reg→mem | 8 | 8 | 12 | +EA |
   | MOVE mem→reg | 4 | 4 | 4 | +EA |
   | ADD/SUB reg,reg | 4 | 4 | 8 | — |
   | ADD/SUB #imm,reg | 8 | 8 | 16 | — |
   | MULU | 70 | | | +EA |
   | MULS | 70 | | | +EA |
   | DIVU | 140 | | | +EA |
   | DIVS | 158 | | | +EA |
   | Bcc taken | 10 | | | — |
   | Bcc not-taken | 8 (byte), 12 (word) | | | — |
   | BRA/BSR | 10 | | | — |
   | NOP | 4 | | | — |
   | LEA | 4 | | | +EA |
   | PEA | 12 | | | +EA |
   | JSR | 8 | | | +EA |
   | JMP | 4 | | | +EA |
   | RTS | 16 | | | — |
   | RTE | 20 | | | — |
   | TRAP | 34 | | | — |
   | STOP | 4 | | | — |
   | RESET | 132 | | | — |
   | Exception processing | 34-44 | | | — |

3. **EA penalty** — Each addressing mode adds cycle penalties. Add `EaCyclePenalty()`
   function to `cycle_data.cc`:
   | Mode | Penalty |
   |------|---------|
   | Dn | 0 |
   | An | 0 |
   | (An) | 4 |
   | (An)+ | 4 |
   | -(An) | 6 |
   | d(An) | 8 |
   | d(An,Xi) | 10 |
   | (d8,An,Xn) | — |
   | #imm | 4 (word), 8 (long) |
   | d(PC) | 8 |
   | d(PC,Xi) | 10 |

4. **Instrument every opcode handler** — Each `Op*()` method calls
   `IncrementCycles(base + ea_penalty)` at the appropriate point. This is
   mechanical but touches every handler in all 5 opcode files.

**Source references:**
- `RUN.CPP` — `inc_cyc()` calls scattered throughout all instruction handlers
- `extern.h` — `unsigned long cycles` global declaration
- M68000 PRM Section 8 — authoritative cycle count tables

**New test file:** `tests/sim/cycle_test.cc`

Minimum test cases (20):
1. `NopIs4Cycles` — NOP increments cycles by 4
2. `MoveRegRegWord` — MOVE.W D0,D1 = 4 cycles
3. `MoveRegMemWord` — MOVE.W D0,(A0) = 8 cycles + 4 EA = 12
4. `MoveMemRegLong` — MOVE.L (A0),D0 = 4 cycles + 4 EA = 8
5. `AddRegRegByte` — ADD.B D0,D1 = 4 cycles
6. `AddRegRegLong` — ADD.L D0,D1 = 8 cycles
7. `AddImmReg` — ADDI.W #1,D0 = 8 cycles
8. `SubImmReg` — SUBI.W #1,D0 = 8 cycles
9. `MuluIs70Cycles` — MULU D0,D1 = 70 + EA
10. `MulsIs70Cycles` — MULS D0,D1 = 70 + EA
11. `DivuIs140Cycles` — DIVU D0,D1 = 140 + EA
12. `DivsIs158Cycles` — DIVS D0,D1 = 158 + EA
13. `BccTakenIs10` — BRA taken = 10 cycles
14. `BccNotTakenByte` — BRA not-taken byte = 8 cycles
15. `BccNotTakenWord` — BRA not-taken word = 12 cycles
16. `LeaIs4PlusEA` — LEA (A0),A1 = 4 + 4 = 8
17. `RtsIs16Cycles` — RTS = 16
18. `RteIs20Cycles` — RTE = 20
19. `TrapIs34Cycles` — TRAP #15 = 34
20. `ResetIs132Cycles` — RESET = 132
21. `ResetClearsCycles` — Trap #15 task 30 resets counter to 0
22. `GetCyclesReturnsCurrent` — Trap #15 task 31 returns current count
23. `EaPenaltyAnIndirect` — (An) adds 4 cycles
24. `EaPenaltyAnPostInc` — (An)+ adds 4 cycles
25. `EaPenaltyAnPreDec` — -(An) adds 6 cycles
26. `EaPenaltyAnDisplacement` — d(An) adds 8 cycles
27. `EaPenaltyAnIndex` — d(An,Xi) adds 10 cycles
28. `EaPenaltyImmediate` — #imm adds 4 (word) or 8 (long)
29. `MultipleInstructionsAccumulate` — 10 NOPs = 40 cycles

**Quality Gate 9.2:** All 29 cycle tests pass. Trap #15 tasks 30-31 return correct
cycle counts. Cycle counts match original EASy68K within ±1 cycle for all tested
instructions.

---

### Task 9.3 — SIMHALT Configuration and Exception Toggle

Port two runtime configuration options from the original that affect core behavior.

**9.3a — SIMHALT enable/disable**

The original supports `*[sim68k]simhalt_off` in .L68 listing comments to disable
the SIMHALT instruction. When disabled, opcode `0xFFFF` is treated as a Line-F
exception instead of halting the simulator.

Source: `SIM68Ku.cpp:320-328` — `*[sim68k]` command parsing.

**Changes:**

1. Add `bool simhalt_enabled = true` to `M68kSimulator::Config`.

2. Modify `DispatchSimhalt()` in `decode.cc`:
   ```cpp
   // Before (always halts):
   if (opcode == 0xFFFF) return SimResult::kHalted;

   // After (configurable):
   if (opcode == 0xFFFF) {
     if (!config_.simhalt_enabled) return SimResult::kLine1111;
     return SimResult::kHalted;
   }
   ```

**9.3b — Exception processing enable/disable**

The original has `exceptions` flag controlled by `Hardware->setExceptionsEnabled()`.
When disabled, the simulator skips exception processing for illegal instructions,
privilege violations, etc.

Source: `hardwareu.cpp:831-855` — Trap #15 task 32 subtask 5.

**Changes:**

1. Add `bool exceptions_enabled = true` to `M68kSimulator::Config`.

2. In `Step()`, when `exceptions_enabled == false`, skip the exception-handling
   `switch` after `ExecuteInstruction()`. Return `result` directly without calling
   `HandleException()`.

3. Wire to existing `ISimulatorEnv::SetExceptionsEnabled()` — the `SimulatorEnv`
   backend can flip this flag at runtime via Trap #15 task 32 subtask 5.

**New test cases** added to `tests/sim/simulator_test.cc`:

1. `SimhaltEnabledHalts` — default config, SIMHALT returns kHalted
2. `SimhaltDisabledReturnsLineF` — simhalt_enabled=false, SIMHALT returns kLine1111
3. `SimhaltDisabledTriggersException` — SIMHALT when disabled → HandleException(kLine1111)
4. `ExceptionsEnabledProcessesIllegal` — illegal opcode → kIllegalInst exception
5. `ExceptionsDisabledSkipsIllegal` — exceptions_enabled=false, illegal opcode → returns kBadInstruction without exception processing
6. `ExceptionsDisabledSkipPrivilege` — MOVE to SR in user mode → no exception
7. `ConfigDefaultsBothEnabled` — default Config has both flags true
8. `SimhaltToggleAtRuntime` — change simhalt_enabled between Steps

**Quality Gate 9.3:** All 8 config tests pass. SIMHALT can be toggled. Exceptions
can be disabled. Default behavior is unchanged (both enabled).

---

### Task 9.4 — Execution Logging Interface

Port the core execution logging capability from the original's `RUN.CPP:834-845`
and `logU.cpp`. The original writes PC, opcode, register state, and optional memory
dump to a file on each instruction step.

This task adds a **core logging interface** so that any client (GUI, CLI, test
harness) can receive execution trace data without modifying the simulator core.

**New interface file:** `include/easym68k/sim/iexecution_logger.h`

```cpp
class IExecutionLogger {
 public:
  virtual ~IExecutionLogger() = default;

  // Called after each instruction execution in Step()/Trace mode.
  // pc: address of the instruction that just executed
  // opcode: the fetched opcode word
  // state: full CPU state after execution
  virtual void LogInstruction(uint32_t pc, uint16_t opcode,
                               const CpuState& state) = 0;

  // Called to dump a memory range (when "Instructions + Registers + Memory"
  // logging mode is active).
  virtual void LogMemoryDump(uint32_t start, uint32_t length,
                              const Memory& memory) = 0;

  // Called for each Trap #15 text output when output logging is active.
  virtual void LogOutput(const char* text, int length) = 0;

  // Configuration: what to log
  enum class Mode { kOff, kInstructions, kInstructionsAndMemory };
  virtual void SetMode(Mode mode) = 0;
  virtual Mode GetMode() const = 0;

  // Memory dump range configuration
  virtual void SetDumpRange(uint32_t start, uint32_t bytes) = 0;
};
```

**Changes to `M68kSimulator`:**

1. Add `IExecutionLogger* execution_logger = nullptr` to `Config`.

2. In `Step()`, after instruction execution, if `config_.execution_logger` is
   non-null and mode != kOff:
   ```cpp
   if (config_.execution_logger &&
       config_.execution_logger->GetMode() != IExecutionLogger::Mode::kOff) {
     config_.execution_logger->LogInstruction(inst_pc_, opcode, state_);
     if (config_.execution_logger->GetMode() == Mode::kInstructionsAndMemory) {
       config_.execution_logger->LogMemoryDump(dump_start, dump_bytes, memory_);
     }
   }
   ```

3. In `DispatchTrap15()`, for all text output tasks (0-1, 6, 13-14, 17-18, 20),
   call `LogOutput()` if logger is active.

**Source references:**
- `RUN.CPP:834-845` — execution log writing in the instruction loop
- `SIM68Ku.cpp` — log configuration dialog
- `logU.cpp` — log file management (full 296-line source)

**New test file:** `tests/sim/execution_logger_test.cc`

Minimum test cases (12):
1. `NullLoggerIsNoOp` — nullptr execution_logger, Step() works normally
2. `LogInstructionCalledPerStep` — mock logger receives LogInstruction for each Step()
3. `LogInstructionReceivesCorrectPc` — logged PC matches instruction address
4. `LogInstructionReceivesCorrectOpcode` — logged opcode matches fetched word
5. `LogInstructionReceivesPostExecutionState` — logged state reflects instruction result
6. `ModeOffNoLogging` — Mode::kOff → no LogInstruction calls
7. `ModeInstructionsOnly` — Mode::kInstructions → LogInstruction but no LogMemoryDump
8. `ModeInstructionsAndMemory` — Mode::kInstructionsAndMemory → both called
9. `DumpRangeConfigurable` — SetDumpRange sets the range used in LogMemoryDump
10. `LogOutputCapturesText` — Trap #15 task 0 triggers LogOutput
11. `LogOutputCapturesChar` — Trap #15 task 6 triggers LogOutput
12. `MultipleStepsLogMultiple` — 10 Steps → 12 LogInstruction calls

**Quality Gate 9.4:** All 12 execution logger tests pass. Logger is fully optional.
No performance impact when logger is null. Mode switching works at runtime.

---

### Task 9.5 — Event/Observer System

Add an observer pattern to `M68kSimulator` that decouples monitoring and debugging
from the core execution loop. This enables multiple simultaneous consumers of
simulator state changes without modifying core code.

**New interface file:** `include/easym68k/sim/isim_observer.h`

```cpp
enum class SimEventType {
  kInstructionExecuted,
  kBreakpointHit,
  kExceptionRaised,
  kInterruptRequested,
  kHalted,
  kStopped,       // STOP instruction
  kResumed,       // Woke from STOP
};

struct SimEvent {
  SimEventType type;
  uint32_t pc;
  SimResult result;
  int interrupt_level;  // valid for kInterruptRequested
};

class ISimObserver {
 public:
  virtual ~ISimObserver() = default;
  virtual void OnEvent(const SimEvent& event) = 0;
};
```

**Changes to `M68kSimulator`:**

1. Add observer management:
   ```cpp
   void AddObserver(ISimObserver* observer);
   void RemoveObserver(ISimObserver* observer);
   ```

2. Add `std::vector<ISimObserver*> observers_` member.

3. Emit events at key points in `Step()` and `Run()`:
   - `kInstructionExecuted` — after each successful Step()
   - `kBreakpointHit` — when Run() stops at a breakpoint
   - `kExceptionRaised` — when HandleException() is called
   - `kInterruptRequested` — when Interrupt() is called
   - `kHalted` — when state becomes halted
   - `kStopped` — when STOP instruction executes
   - `kResumed` — when STOP wakes from interrupt

4. **Thread safety** — Observer notifications happen on the simulator's execution
   thread (the thread calling Step()/Run()). Observers must be fast or defer work.
   No mutex needed since the call is synchronous.

**New test file:** `tests/sim/observer_test.cc`

Minimum test cases (12):
1. `AddRemoveObserver` — add and remove, no crash
2. `InstructionExecutedEvent` — Step() emits kInstructionExecuted
3. `EventHasCorrectPc` — event.pc matches instruction address
4. `BreakpointHitEvent` — Run() stops at BP, emits kBreakpointHit
5. `ExceptionRaisedEvent` — illegal opcode → kExceptionRaised
6. `HaltedEvent` — SIMHALT → kHalted
7. `StoppedEvent` — STOP instruction → kStopped
8. `ResumedEvent` — STOP + Interrupt() → kResumed
9. `InterruptRequestedEvent` — Interrupt(5) → kInterruptRequested
10. `MultipleObservers` — 3 observers all receive the same event
11. `RemoveObserverDuringRun` — removing observer between Steps is safe
12. `NullObserverIgnored` — nullptr observer is silently ignored

**Quality Gate 9.5:** All 12 observer tests pass. No performance impact when
observer list is empty (check vector empty before iterating). Observer removal
is safe during iteration.

---

### Task 9.6 — Simulator Core Integration Test

A comprehensive integration test that exercises all Phase 9 features together
to prove the simulator can run a non-trivial M68K program headlessly with full
parity.

**New test file:** `tests/sim/core_integration_test.cc`

Test program (assembled by the Phase 8 assembler or hand-coded):

```asm
    ORG     $1000
START:
    MOVE.W  #$1234,D0     ; load test value
    MOVE.W  #$5678,D1     ; load test value
    ADD.W   D0,D1         ; D1 = $68AC
    MOVE.W  D1,$2000      ; store result
    TRAP    #15            ; task 9 = terminate
    DC.W    9
```

Integration test cases (10):
1. `FullProgramExecution` — assemble, load, run, verify D1=$68AC at $2000
2. `CycleCountAccumulated` — above program, verify total cycles > 0
3. `InterruptDuringRun` — run a long loop, inject IRQ3, verify handler called
4. `StopWakeCycle` — STOP instruction, IRQ wakes it, continues execution
5. `LoggerCapturesFullTrace` — run with logger, verify all instructions logged
6. `ObserverReceivesAllEvents` — run with observer, verify event sequence
7. `SimhaltDisabledStillRuns` — program with 0xFFFF data, simhalt_off, no halt
8. `ExceptionsDisabledNoTrap` — illegal opcode, exceptions off, no exception
9. `MemoryProtectionWithInterrupt` — protected memory, IRQ handler writes to it
10. `HeadlessTrap15IO` — program does text output via mock ITextIO, verify output

**Quality Gate 9.6:** All 10 integration tests pass. The simulator runs a complete
M68K program from S-Record load through termination with correct register state,
memory state, cycle count, interrupt handling, and event trace.

---

### Phase 9 Summary

| Task | Description | Tests | Source |
|------|-------------|-------|--------|
| 9.1 | Interrupt Injection & Processing | 15 | RUN.CPP irqHandler(), hardwareu.cpp IRQprocess() |
| 9.2 | Cycle Counting | 29 | RUN.CPP inc_cyc(), M68000 PRM Section 8 |
| 9.3 | SIMHALT Config & Exception Toggle | 8 | SIM68Ku.cpp *[sim68k], hardwareu.cpp setExceptionsEnabled() |
| 9.4 | Execution Logging Interface | 12 | RUN.CPP logging, logU.cpp |
| 9.5 | Event/Observer System | 12 | New (not in original, enables extensibility) |
| 9.6 | Core Integration Test | 10 | Cross-cutting |

**Total new tests:** ~86

**Phase exit criteria:**
- All 86 Phase 9 tests pass
- Simulator can run any valid M68K program headlessly
- Interrupt injection is thread-safe
- Cycle counting matches original ±1 cycle
- SIMHALT and exception toggles work
- Execution logging works without GUI
- Observer system delivers events to all registered listeners
- Zero Qt dependencies in core library

---
