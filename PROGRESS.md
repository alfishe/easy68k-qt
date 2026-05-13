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
| 8.5.5 Remaining Directives (DCB/DC strings/INCLUDE/INCBIN/SECTION/OFFSET/OPT/FAIL/LIST/PAGE/etc.) | DONE | `pending` | DCB/DC strings/SIMHALT/INCLUDE(mock reader)/INCBIN/SECTION/OFFSET/OPT/FAIL/LIST/NOLIST/PAGE/ORG odd-align; kStringLiteral mode; double-quoted strings in lexer; 49 DirectivesTest (550 total) |

Decision: Odd ORG address alignment rounds DOWN (`new_loc &= ~1u`) rather than UP (`newLoc++`) as the original does. For ORG $1001: original → $1002, port → $1000. Round-down is intentional — rounding up skips a byte and is arguably the worse choice. Both are valid error-recovery behaviors; the difference is documented, not a bug.
| 8.5.6 Conditional Assembly (IFC/IFNC/IFEQ/IFNE/IFLT/IFLE/IFGT/IFGE/ELSE/ENDC) | TODO | | |
| 8.5.7 Structured Control Flow (WHILE/FOR/REPEAT/IF/DBLOOP/UNLESS) | TODO | | |
| 8.5.8 Macro Processor (MACRO/ENDM/params/\@/NARG/IFARG/MEXIT) | TODO | | |
| 8.5.9 Error Reporter (43+ error codes) | TODO | | |
| 8.5.10 Listing Generator (.L68 output) | TODO | | |
| 8.5.11 Object/S-Record Output (S0/S1/S2/S3/S8) | TODO | | |
| 8.6 Golden Assembly Tests | TODO | | |

## Phase 9: Simulator Core Library — Full Parity & Headless Readiness

| Task | Status | Commit | Summary |
|------|--------|--------|---------|
| 9.1 Interrupt Injection & Processing | TODO | | |
| 9.2 Cycle Counting | TODO | | |
| 9.3 SIMHALT Config & Exception Toggle | TODO | | |
| 9.4 Execution Logging Interface | TODO | | |
| 9.5 Event/Observer System | TODO | | |
| 9.6 Core Integration Test | TODO | | |

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

---

**NEXT:** Task 8.5.6 — Conditional Assembly (IFC/IFNC/IFEQ/IFNE/IFLT/IFLE/IFGT/IFGE/ELSE/ENDC)
