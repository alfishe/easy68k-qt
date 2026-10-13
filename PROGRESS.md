# EASy68K Port — Task Progress Tracker

This file is the single source of truth for resumption. Any agent picking up work
must read this file first to determine what is done and what to do next.

## Rules

1. **Update after every task.** When a task from `doc/execution_plan.md` is completed,
   append a `DONE` entry to the appropriate phase section below. Include the git
   commit hash and a one-line summary of what was delivered.

2. **Update on task start.** When beginning a task, change its entry from `TODO` to
   `ACTIVE` with the current date. Only one task may be `ACTIVE` at a time.

3. **Commit this file with every phase commit.** `PROGRESS.md` must be included in
   the commit that delivers the phase's work. This guarantees the hash in each
   `DONE` entry points to a commit containing the corresponding progress update.

4. **Never delete entries.** Only append or change status (`TODO` → `ACTIVE` → `DONE`).
   If a task needs rework, add a `REWORK` entry below the original `DONE`.

5. **Decisions log.** If a task required a non-obvious design decision (e.g., choosing
   one pattern over another, deviating from the plan), add a `Decision:` line to the
   `DONE` entry. This prevents future agents from revisiting settled questions.

6. **Next task.** The `NEXT` marker at the bottom always points to the task an agent
   should start on. Update it when completing a task.

## Phase 0: Project Infrastructure

| Task | Status | Commit | Summary |
|------|--------|--------|---------|
| 0.1 Create Repository and Directory Tree | DONE | `7c5fe58` | Directory tree, .gitignore, .clang-format, CMakeLists.txt skeleton |
| 0.2 Top-Level CMakeLists.txt | DONE | `7c5fe58` | Top-level CMake with C++17, subdirectories, vcpkg toolchain |
| 0.3 Library CMakeLists.txt Files | DONE | `7c5fe58` | sim/ and asm/ library targets, test executable, CompilerWarnings.cmake |
| 0.4 .clang-format Configuration | DONE | `7c5fe58` | Google style with 2-column indent |
| 0.5 CI Pipeline Configuration | TODO | | |
| 0.6 Golden Test Data Infrastructure | TODO | | |

## Phase 1: Types and Memory

| Task | Status | Commit | Summary |
|------|--------|--------|---------|
| 1.1 Transfer and Adapt types.h | DONE | `7c5fe58` | SimResult enum, DataSize, Condition, ExceptionVector, MemoryFlags, SR constants |
| 1.2 Create effective_addr.h | DONE | `7c5fe58` | EaTarget enum, EffectiveAddr struct with MakeDataRegEa/MakeAddrRegEa/MakeMemoryEa/MakeImmediateEa |
| 1.3 Transfer and Adapt Memory Class | DONE | `7c5fe58` | vector storage, big-endian Read/Write, MemoryAccessResult, alignment checks |
| 1.4 Port Memory Tests | DONE | `7c5fe58` | 17 MemoryTest + 5 EffectiveAddrTest, all passing |

## Phase 2: S-Record Loader

| Task | Status | Commit | Summary |
|------|--------|--------|---------|
| 2.1 Transfer and Extract SRecordLoader | DONE | `7c5fe58` | Standalone SRecordLoader with checksum validation, three load methods |
| 2.2 S-Record Loader Tests | DONE | `7c5fe58` | 16 SRecordLoaderTest, all passing |

## Phase 3: Addressing Modes

| Task | Status | Commit | Summary |
|------|--------|--------|---------|
| 3.1 Create addressing_mode.h | DONE | `e4ea892` | Mode masks, combined masks, CalculateEA/ReadEA/WriteEA declarations |
| 3.2 Implement addressing_mode.cc | DONE | `e4ea892` | All 12 EA modes from RUN.CPP eff_addr(), FetchWord/FetchLong, ByteInc for SP |
| 3.3 Addressing Mode Tests | DONE | `e4ea892` | 37 EATest covering all modes + ReadEA/WriteEA |

Decision: CpuState (GetAReg/SetAReg for SSP/USP) added as part of Phase 3 rather than
Phase 4 because CalculateEA needs CpuState&.

## Phase 4: Simulator Core

| Task | Status | Commit | Summary |
|------|--------|--------|---------|
| 4.1 Create Simulator Header | DONE | `92c8edd` | M68kSimulator class, Config struct with 9 Trap #15 interfaces, Breakpoint, deleted copy |
| 4.2 Implement Simulator Core | DONE | `92c8edd` | Reset, FetchWord/Long, HandleException (class 1/2 stack frame), Step, Run, CheckCondition |
| 4.3 Simulator Unit Tests | DONE | `92c8edd` | 13 SimTest + 13 FlagTest, all passing |

Decision: flag_computation decomposed into 5 typed functions (UpdateFlagsAdd/Sub/Logic/Move/Shift)
instead of the original's monolithic cc_update() with CASE_* enums.

## Phase 5: Instruction Decoder

| Task | Status | Commit | Summary |
|------|--------|--------|---------|
| 5.1 Implement Hierarchical Decoder | DONE | `d87e240` | Two-level switch on opcode bits 15-12, 15 DispatchGroup methods in decode.cc |
| 5.2 Decoder Tests | DONE | `d87e240` | 13 DecodeTest (12 original + SingleFfffIsLineF added during review) |

Decision: SIMHALT requires double 0xFFFF word (matching original RUN.CPP line 773).
Decision: NOP/STOP moved from simulator.cc to DispatchGroup4 in decode.cc.

## Phase 6: Opcode Implementation

| Task | Status | Commit | Summary |
|------|--------|--------|---------|
| 6.1 Move Instructions | DONE | `87ca91d` | Full dispatch for all 16 opcode groups; 13 move-class ops + 5 flag helpers; 37 tests |
| 6.2 Arithmetic Instructions | DONE | `46432cf` | All 27 arithmetic ops (ADD/SUB/MUL/DIV/NEG/CMP/TST/EXT/BCD/CHK); 69 tests |
| 6.3 Logic Instructions | DONE | `ecd8bdb` | OR/AND/EOR + immediate + CCR/SR variants, NOT, TAS; 40 tests |
| 6.4 Branch Instructions | DONE | `aebef79` | BRA/BSR/Bcc/DBcc/Scc/JMP/JSR/RTS/RTE/RTR; FetchWord made public; 32 tests |

Decision: ABCD/SBCD/NBCD set X=C (per M68000 manual). Original EASy68K omits X update — a bug.
Decision: CHK negative-check uses int16_t cast. Original masks to WORD_MASK then tests < 0, which is always false — a bug. Port is correct.
Decision: CHK leaves N/Z/V/C undefined (spec allows this). Original sets Z as a side-effect; not replicated since spec explicitly marks all flags undefined for CHK.
Decision: FetchWord/FetchLong moved to public section so the FetchBranchDisp static helper in opcodes_branch.cc can call them without code duplication.
| 6.5 Miscellaneous Instructions | DONE | `9ee95e2` | TRAP/TRAPV/ILLEGAL/RESET/STOP; decode.cc STOP delegated to OpStop (SR load fix); 19 tests |

Decision: STOP fully ported from CODE9.CPP: immediate word fetched before privilege check (PC always advances), trace flag preserved across SR load, post-load supervisor check added. Original decode.cc stub skipped SR load entirely — now fixed by delegating to OpStop.
| 6.6 Shift/Rotate Instructions | DONE | `fbe37ed` | OpShiftRotate (register+memory), OpBtst/Bchg/Bclr/Bset; 25 tests + DecodeTest stub updated |

Decision: `DecodeTest.BtstDynamic` was a Phase 5 stub expecting `kBadInstruction`; updated to expect `kOk` now that BTST is implemented.
Decision: ROL/ROR preserve X by saving `orig_x` before `UpdateFlagsShift` and restoring after (type==3). ASL V flag tracked per-iteration via `prev` variable.
Decision: Register shift count uses `% 64` (not `& sizeMask`) — the original's mask was a bug (count 1–7 become 0 for byte/word shifts). The port uses the correct M68000 PRM modulo-64 behaviour.
| 6.6 Shift/Rotate Instructions | REWORK | `f0e627c` | Register-form i/r bit (bit 5) was decoded inverted: 1 is register count, 0 is immediate (M68000 PRM, CODE7.CPP `if (inst & 0x20)`). Assembled programs (e.g. ASL.B #2,D0 = $E500) shifted by the wrong count. Fixed OpShiftRotate; corrected 30 hand-encoded opcodes in ShiftTest/InstrFlagTest; added 2 regression tests |
| 6.7 Flag Computation Verification Suite | DONE | `5645127` | 64 end-to-end InstrFlagTest cases: ADD/SUB/ADDX/SUBX/NEG/NEGX/CMP/TST/CLR/AND/OR/EOR/NOT/MOVE/shifts/MUL/EXT |

## Phase 7: Trap #15 Dispatch

| Task | Status | Commit | Summary |
|------|--------|--------|---------|
| 7.1 Define Trap #15 Interface Headers | DONE | `5cee051` | 10 interface headers: ITextIO/IFileIO/ISerialIO/INetworkIO/IGraphicsIO/ISoundIO/IPeripheralIO/ISimulatorEnv/IPrintIO/ILogger; all self-contained |
| 7.2 Implement Trap #15 Dispatch | DONE | `bfe85ce` | DispatchTrap15() 50-case switch; SetupWindow(bool fullscreen) on ISimulatorEnv; 2 new MiscTest cases; 402 tests |
| 7.3 Trap #15 Mock Tests | DONE | `fcfdee0` | 103 gmock tests; all 50+ task numbers; NiceMock fixture; null-interface and unknown-task edge cases; 505 total |

## Phase 8: Assembler

| Task | Status | Commit | Summary |
|------|--------|--------|---------|
| 8.1 Transfer Lexer | DONE | `7c5b148` | TokenType/SizeSpec/RegisterType enums, Token struct, Lexer class; all 68000 opcodes + EASy68K directives; 100 LexerTest |
| 8.2 Transfer Parser and Symbol Table | DONE | `dd18436` | Parser (AddressMode/Operand/ParsedLine/Parser), SymbolTable (SymbolFlags/ForwardRef/SymbolInfo); Pratt-precedence expression evaluator; 173 tests |
| 8.3 Transfer Expression Evaluator | DONE | `fa27014` | ExpressionEvaluator standalone class; Pratt parser matching EVAL.CPP precedence; parser delegates to evaluator; lexer .digit local-label support; 46 ExpressionTest + 6 new LexerTest; 228 total |
| 8.4 Transfer Assembler Core | DONE | `91cfef0` | Two-pass assembler (ORG/EQU/SET/DC/DS/EVEN/END directives; NOP/MOVEQ instructions; DC/DS word-alignment); forward-label resolution; 32 AssemblerTest; 260 total |
| 8.5.1 Instruction Table Tests & Assembler Wiring | DONE | `5da6f69` | 77 InstrTableTest cases; InstructionSize() table-driven; HandleInstruction() dispatches via CodeGenerator (kFixed+kMoveq fully wired; stubs for all other encodings) |
| 8.5.2 Code Generator: EA Encoding + Ext Words | DONE | `c264fc7` | EffAddr() all 13 EA modes to 6-bit codes; ExtWords() all mode+size combos with range checking; PC-relative disp = value-(loc+2) fixing original off-by-2; 62 CodeGeneratorTest |
| 8.5.3 Code Generator: Instruction Encoding Handlers | DONE | `pending` | All 36 InstrEncoding cases in Encode(); ADDQ/SUBQ/MOVEQ peepholes; branch short/word forms; MOVEP all 4 sub-forms; InstructionSize() peephole consistency; 213 CodeGeneratorTest (473 total) |
| 8.5.4 MOVEM Register List Parsing & Encoding | DONE | `pending` | kRegisterList mode; ParseRegisterList/ParseMovemOperands; REG directive; kMoveM encoder with mask reversal; InstructionSize fix; 28 MovemTest (501 asm / 1006 total) |

Decision: EA mode validation (reject Dn/An/Imm/PreDec-as-source/etc.) is NOT implemented in the encoder — the original's DestModes/SourceModes checks are error-reporter logic, deferred to Task 8.5.9. The encoding itself is correct for all valid inputs.
| 8.5.5 Remaining Directives (DCB/DC strings/INCLUDE/INCBIN/SECTION/OFFSET/OPT/FAIL/LIST/PAGE/etc.) | DONE | `3490a7d` | DCB/DC strings/SIMHALT/INCLUDE(mock reader)/INCBIN/SECTION/OFFSET/OPT/FAIL/LIST/NOLIST/PAGE/ORG odd-align; kStringLiteral mode; double-quoted strings in lexer; 49 DirectivesTest (550 total) |

Decision: Odd ORG address alignment rounds DOWN (`new_loc &= ~1u`) rather than UP (`newLoc++`) as the original does. For ORG $1001: original → $1002, port → $1000. Round-down is intentional — rounding up skips a byte and is arguably the worse choice. Both are valid error-recovery behaviors; the difference is documented, not a bug.
| 8.5.6 Conditional Assembly (IFC/IFNC/IFEQ/IFNE/IFLT/IFLE/IFGT/IFGE/ELSE/ENDC) | DONE | `f0f5c3c` | CondFrame stack in assembler; IsAssembling/OuterIsAssembling; HandleConditional; RunPass routes conditionals before skip check; 35 ConditionalTest (585 total) |

Decision: IFC/IFNC use case-sensitive comparison (original uses stricmp/case-insensitive). Case-sensitive is more predictable for macro argument matching and is what the execution plan specifies.
Decision: ELSE is supported (original has no ELSE for IFxx/ENDC — only for structured IF/ENDI). Extension is backwards-compatible since ELSE was unused in that context.
| 8.5.7 Structured Control Flow (WHILE/FOR/REPEAT/IF/DBLOOP/UNLESS) | DONE | `d81b63e` | structured.cc: IF/ELSE/ENDI, WHILE/ENDW, REPEAT/UNTIL, FOR/ENDF, DBLOOP/UNLESS; label injection via InjectLines; ELSE disambiguation vs conditional ELSE; 33 StructuredTest (619 total) |

Decision: Structured directives are checked in RunPass BEFORE the parser-error check because operands like `<EQ>` cause parse errors but still set line.opcode. ELSE disambiguation: when cond_stack_ is non-empty, ELSE routes to the conditional handler, not structured.
Decision: CMP with immediate source is encoded as CMP (0xB0xx) not CMPI (0x0Cxx) — this is what the assembler's instruction table emits for `CMP.W #n,Dn`.
| 8.5.8 Macro Processor (MACRO/ENDM/params/\@/NARG/IFARG/MEXIT) | DONE | `fb4df94` | Pre-scan ScanMacros; InvokeMacro with unified LocalFrame conditional stack; \0–\9/\@/NARG substitution; label-without-colon on macro calls (column-1 check); continuation lines (&); 29 MacroTest (648 total) |
| 8.5.9 Error Reporter (43+ error codes) | DONE | `782ec7d` | ErrorCode enum (45 codes + kGeneric); ErrorSeverity kError/kWarning; AddError/AddWarning overloads; error limit (default 10); odd-ORG demoted to warning; AssemblyError.code/.severity/.column; AssemblyResult.warning_count; 23 ErrorReporterTest (671 total) |
| 8.5.10 Listing Generator (.L68 output) | DONE | `2c27536` | EmittedItem capture; smart tab stops (12/20/44); constant truncation (...) and expansion (CEXflag); automatic pagination with PAGE breaks; EQU/SET resolved values (=XXXXXXXX); errors/warnings logging inside listing; 6 ListingGeneratorTest (677 total) |
| 8.5.11 Object/S-Record Output (S0/S1/S2/S3/S8) | DONE | `0867023` | SRecordGenerator with robust checksum verification, contiguous byte grouping, 16/24/32-bit addresses, S0 memory maps output, S8 execution start record, and 5 unit tests (682 total) |
| 8.6 Golden Assembly Tests | DONE | `3264a94` | 21 golden test cases covering full M68k instruction set, macros, directives, conditional assembly, structured control flow, and memory maps; byte-identical verify protection |
| 8.7 Bit-Field Instruction Encoding (parity A4) | TODO | | |
| 8.8 OPT CRE Symbol Table + MEX/SEX Listing Expansion (parity A5, A6) | TODO | | |
| 8.9 AssemblerOptions Struct (parity A7) | TODO | | |

## Phase 9: Simulator Core Library — Full Parity & Headless Readiness

| Task | Status | Commit | Summary |
|------|--------|--------|---------|
| 9.1 Interrupt Injection & Processing | DONE | `b4e7989` | Thread-safe autovectored interrupts (levels 1-7); wake-on-interrupt; priority masking; 15 unit tests in tests/sim/interrupt_test.cc |
| 9.2 Cycle Counting | DONE | `f0e627c` | cycle_data.{h,cc} (EA penalty, MOVE/MOVEM tables, MULU/MULS 38+2n, exact DIVU/DIVS timing); IncrementCycles() in every opcode handler + exception processing; Trap #15 tasks 30/31 use core counter; 88 CycleTest/CycleDataTest (608 sim tests) |

Decision: Cycle values are ported from the original EASy68K (RUN.CPP eff_addr() bwinc/linc, CODE1-CODE9.CPP inc_cyc()) rather than the simplified PRM table in the execution plan, since the quality gate is "match original ±1". Where they differ, the original wins: MOVE.W D0,(A0)=8 (plan: 12), MOVE.L (A0),D0=12 (plan: 8), LEA (A0),A1=4 (plan: 8), MULU/MULS=38+2n (plan: fixed 70), DIVU/DIVS use the exact Cwik timing routines (plan: fixed 140/158), TRAP #0-14=38 (plan: 34), TRAP #15 itself adds 0 cycles.
Decision: MOVE/MOVEA/MOVEM/LEA/PEA/JMP/JSR call eff_addr() with add_times=false in the original; their tables already include EA time, so no EaCyclePenalty() is added for them.
Decision: Trap #15 tasks 30/31 now read/clear `state_.cycles` directly (as CODE9.CPP does with the global `cycles`). `ISimulatorEnv::ClearCycles()/GetCycles()` were removed — the counter is owned by the core, not the environment.
Decision: Shift cycle count uses the port's (PRM-correct, modulo-64) shift count, not the original's buggy `& size` count, so timing matches the shift actually performed.
Decision: On instructions that end with `return WriteEA(...)`, cycles are added before the write. Write failures halt the simulator, so ordering on the error path is not observable.
Decision: `asm-tests` is registered with `WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}` so the golden test finds `data/golden/asm` under ctest.
| 9.3 SIMHALT Config & Exception Toggle | DONE | `511502c` | Config.simhalt_enabled/exceptions_enabled + thread-safe runtime setters owned by the core; SIMHALT off → Line F; exceptions off → halt (no frame/cycles), trace and IRQs ignored; Trap #15 32/5 re-enables headless; 11 SimConfigTest (621 sim tests) |

Decision: With exceptions disabled the port halts on any exception condition, matching RUN.CPP's "exception processing not enabled" branch ("... Execution halted"). The plan's wording ("return result without exception processing") is satisfied, and the simulator additionally stops, as the original does. Trace and interrupts are only processed when exceptions are enabled, as in the original.
Decision: Trap #15 task 32 subtask 5 sets the core flag directly (works without sim_env) and then notifies `ISimulatorEnv::SetExceptionsEnabled(true)` so a GUI can update its menu and settings.
| 9.4 Execution Logging Interface | DONE | `6d9fc39` | iexecution_logger.h (IExecutionLogger + thread-safe ExecutionLoggerBase); Config.execution_logger; Step() logs memory dump then instruction before execution; all Trap #15 text output routed via EmitText/EmitChar → LogOutput; 18 ExecutionLoggerTest (639 sim tests) |

Decision: Four execution log modes (Off / Instructions / Instructions+Registers / Instructions+Registers+Memory) instead of the plan's three, matching the original's Execution Log Type options; output logging is a separate switch (original Output Log Type).
Decision: The logger is called before the instruction executes, with pre-execution state, and the memory dump comes before the instruction line — the original's log layout (RUN.CPP exec_inst()). Plan test 5 was adapted accordingly (pre- instead of post-execution state); plan test 12's "12 calls for 10 steps" is a typo and tests 10.
Decision: The core logs every executed instruction while a mode is set. The original only logs while tracing (Step/Trace/Auto Trace, not Run); the Sim68K GUI must reproduce that by enabling the mode only in trace mode (parity audit G10). Echoed keyboard input is logged by the ITextIO back end (13.9), which sees the echo, as simIOu.cpp charOut() did.
| 9.5 Event/Observer System | DONE | `8e1dd35` | isim_observer.h (SimEventType/SimEvent/ISimObserver); Add/RemoveObserver; events for instruction executed, breakpoint, exception (with vector + IRQ level), interrupt requested, halted, stopped, resumed; removal-safe delivery; 16 ObserverTest (655 sim tests; ASan+UBSan clean) |

Decision: All observer callbacks run on the simulator thread. Interrupt() may be called from any thread, so kInterruptRequested and kResumed are recorded atomically and delivered at the start of the next Step(); events recorded while no observer is registered are dropped, and Reset() clears them.
Decision: kInstructionExecuted is emitted right after execution (before any kExceptionRaised the instruction causes); kExceptionRaised carries the vector number (and IRQ level for interrupts) so the GUI can reproduce the original's per-exception messages. STOP emits kStopped instead of kHalted.
| 9.6 Core Integration Test | TODO | | |
| 9.7 Memory Map Semantics + Address/Bus Error Class 0 Exception Frame (parity S3, S10) | DONE | `9a7a002` | Memory map per memoryMapCheck() (Invalid / Protected-unless-supervisor / new kMemReadOnly / ROM writes ignored; hardware I/O exempt); Memory fault latch; class 0 frame (PC, SR, IR, address, info word FC5/6+R/W), +50 cycles, double fault and bad SSP halt; ConfigureMemoryMap/SetHardwareAddresses; stacked-PC rule fixed; Run() continues through exceptions; 27 new tests (688 sim tests; ASan+UBSan clean) |

Decision: Memory records the first failing access in a fault latch (with the PC at that moment); Step() checks it after the opcode fetch and after execution, restores the registers to their pre-instruction state (the original returns before storing results) and builds the class 0 frame. This catches faults from every access path (ReadEA discards errors; handlers access memory_ directly) without threading error returns through all handlers. Cycles on a fault are 50, without the EA time the original adds before detecting the error (≤ EA penalty difference).
Decision: Stacked PC for class 1/2 exceptions follows RUN.CPP: OLD_PC = PC after the instruction for TRAP/TRAPV/CHK/DIV0/trace/privilege/Line A/Line F, and the instruction address only for illegal instruction. Previously every exception stacked the instruction address, so a TRAP handler's RTE re-executed the TRAP. Class 0 stacks OLD_PC+2: the PC at the time of the access for reads, instruction address + 2 for writes; info word FC = 6 (program) when the faulting address equals that PC, else 5 (data), plus $10 for reads.
Decision: Run() now continues through exceptions (they vector to their handlers) and stops only on halt, breakpoint, stop request or instruction limit, as the original run loop does; previously any non-OK result stopped it.
Decision: Memory map reads check only the first byte (mem_req()), writes check the whole access range (memoryMapCheck()); ROM writes return kOk without writing. Trap #15 string-buffer map checks (memoryMapCheck(Invalid, ...) in CODE9.CPP) remain a follow-up for the Trap #15 back ends.
| 9.8 Bit-Field Instructions in Simulator (parity S8) | TODO | | |
| 9.9 Advanced Breakpoint Engine (parity S14) | TODO | | |
| 9.10 Step Over / Run To Core API (parity S15) | TODO | | |
| 9.11 Program Init/Rewind + Hardware Reset Semantics (parity S17) | DONE | `bc53056` | InitProgram() = initSim() (PC=start, SR=$2000, USP=$00FF0000, SSP=$01000000, regs/cycles cleared, no vector fetch); HardwareReset() = Reset button (S=1, T=0, mask 7, SSP/PC from vectors, +40 cycles, wakes STOP); Get/SetStartAddress (S-record start used); 7 ProgramInitTest (662 sim tests; ASan+UBSan clean) |

Decision: The existing vector-based Reset() is kept as a power-on reset (used by unit tests); program open/rewind uses InitProgram() and the hardware window uses HardwareReset(), matching the original's two distinct paths.

Decision: Order revised during 9.6 preparation: 9.11 → 9.7 → 9.6 → 9.8 → 9.9 → 9.10. The integration test needs S-record start-address init (original initSim(): PC=start, SR=$2000, USP=$00FF0000, SSP=$01000000, no vector fetch) and the original memory map semantics (UTILS.CPP memoryMapCheck(): ROM writes ignored, Read = bus error on write, Protected = bus error unless supervisor, Invalid = bus error on access, class 0 frame), neither of which the port had.

Decision: Parity audit (`doc/execution_plan/parity-audit.md`) inventoried every menu, dialog, option and Trap #15 task of Sim68K, Edit68K and EASyBIN from the original `.dfm` forms and sources. It added tasks 8.7-8.9, 9.7-9.10 and 13.22-13.25. Order: finish 9.3-9.6 as planned, then 9.7-9.10, then 8.7-8.9, before Phase 10.
Decision: Each task is committed once it is tested and certified (full test suites green, clang-format clean, PROGRESS.md updated); the commit hash is recorded in a follow-up PROGRESS commit (user instruction, 2026-10-13).

## Phase 10: Golden Simulation Traces

| Task | Status | Commit | Summary |
|------|--------|--------|---------|
| 10.1 Implement Golden Trace Comparison | TODO | | |

## Phase 11: Exception and Interrupt Tests

| Task | Status | Commit | Summary |
|------|--------|--------|---------|
| 11.1 Exception Tests | TODO | | |
| 11.2 Interrupt Tests | TODO | | |

## Phase 12: CI and Code Quality

| Task | Status | Commit | Summary |
|------|--------|--------|---------|
| 12.1 Enable CI | TODO | | |
| 12.2 Code Coverage | TODO | | |
| 12.3 clang-tidy | TODO | | |

## Phase 13: GUI Implementation

| Task | Status | Commit | Summary |
|------|--------|--------|---------|
| 13.1 Implement Trap #15 Qt Backends | TODO | | |
| 13.2 Sim68K-Qt Main Window | TODO | | |
| 13.3 Register Display Widget | TODO | | |
| 13.4 Source Code View | TODO | | |
| 13.5 Stack Viewer Window | TODO | | |
| 13.6 Memory Viewer Window | TODO | | |
| 13.7 Memory Editing | TODO | | |
| 13.8 Hardware Window | TODO | | |
| 13.9 I/O Console Widget | TODO | | |
| 13.10 Graphics Output Window | TODO | | |
| 13.11 Sound System | TODO | | |
| 13.12 Log Window | TODO | | |
| 13.13 Serial I/O | TODO | | |
| 13.14 Network I/O | TODO | | |
| 13.15 Memory Range Definition Dialog | TODO | | |
| 13.16 Printing | TODO | | |
| 13.17 File I/O | TODO | | |
| 13.18 Settings Persistence | TODO | | |
| 13.19 Edit68K-Qt Editor Application | TODO | | |
| 13.20 EASyBIN-Qt Binary Utility | TODO | | |
| 13.21 Feature Parity Validation | TODO | | |
| 13.22 Advanced Break Points Dialog (parity G19) | TODO | | |
| 13.23 Sim68K Find / Find Next / Go to PC (parity G7) | TODO | | |
| 13.24 Auto Trace + Options Dialog (parity G12) | TODO | | |
| 13.25 Help System (parity H1) | TODO | | |

## Examples & Education (doc/examples/EDUCATION_STRATEGY.md)

| Task | Status | Commit | Summary |
|------|--------|--------|---------|
| E0 Strategy, Skeleton, Templates, Spec Format | DONE | `15d04b4` | Approved strategy (4 tracks + core path, 83 planned examples); `examples/` skeleton with track indexes generated from the curriculum; `_template/` (verified: assembles, runs, meets its spec); `example.toml` spec format; CC0-1.0 for examples |
| E1 easy68k-run CLI + Example Runner Test | TODO | | |
| E2 Levels 0–1 (15 examples) | TODO | | |
| E3 Levels 2–3 (19 examples) | TODO | | |
| E4 Track S (16 examples) | TODO | | |
| E5 Track C: toolchain, libeasy68k, C-01..C-09 (in parallel with E2–E3) | TODO | | |
| E6 Track C++: C-10..C-13 | TODO | | |
| E7 Track O (13 pairs) | TODO | | |
| E8 Capstones (7 projects) | TODO | | |
| E9 In-app Examples Browser + Solution Checker (post-parity) | TODO | | |

Decision: Examples are CC0-1.0 (user chose "CC"; CC0 is the Creative Commons option suited to code). GCC cross-compiler (C11, freestanding C++17) is the reference C/C++ toolchain. Core path and Track C proceed in parallel.
Decision: E2 depends on the assembler forward-reference fix (pass-2 backward-reference semantics found by the 9.6 integration test); examples must assemble identically to the original EASy68K.

---

Decision: IFARG and all body conditionals (IFC/etc.) are handled via a unified LocalFrame stack inside InvokeMacro — never injected to RunPass raw. IFC inside true IFARG is tracked as an "injected" frame (its ENDC is also injected); IFC inside false IFARG is tracked as a "local" frame (nothing injected). This ensures ENDC is always correctly paired.
Decision: Label-without-colon on macro calls requires the symbol to be at column 1 (ParseLabel column-1 guard). Without this guard, indented macro invocations like `  MYMACRO A,B` would misclassify `MYMACRO` as a label and `A` as the opcode.

Decision: ORG to an odd address was changed from kError to kWarning (kOddAddress) to match original EASy68K behaviour (ODD_ADDRESS was a warning in the original asm.h). Existing tests that check for "odd" in the message still pass because the warning message contains "odd".
Decision: The 4-argument AddError(int, string, ErrorCode, ErrorSeverity) is the internal implementation; the 2- and 3-argument public overloads format and delegate to it.

Decision: Refactored `ListLine` to use type-safe metadata structure (`left_column`, `right_column`, `is_error`, `is_warning`, `is_continuation`) instead of fragile string placeholders (`RIGHT_PLACEHOLDER`, `LINE_PLACEHOLDER`), eliminating any risk of parsing bugs during formatting.

Decision: Replicated exact S0/S1/S2/S3/S8 Motorola format with memory map S0 records. S8 termination address formats to 24-bit to match Borland. S0 checksum calculated programmatically to avoid hardcoded mismatch. SRecordGenerator buffers bytes to output SRECSIZE=36 records.

**NEXT:** Task 9.6 — Core Integration Test
