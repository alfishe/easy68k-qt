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
Decision: CHK leaves N/Z/V/C undefined (spec allows this). Original sets Z as a side-effect; not replicated since spec explicitly marks all flags undefined for CHK. (Superseded in 10.2: N is defined when the exception is taken — set if Dn < 0, cleared if Dn > bound — and is now set from the sign of Dn as the original does; Z/V/C stay unchanged.)
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
| 8.7 Bit-Field Instruction Encoding (parity A4) | DONE | `e0826c2` | Done as part of 8.12: BFCHG/BFCLR/BFEXTS/BFEXTU/BFFFO/BFINS/BFSET/BFTST flavors + bitField() builder; {offset:width} parsed like fieldParse() (Dn or 0-31 offset, Dn or 1-32 width, BAD_BITFIELD/INV_FORWARD_REF); covered by the instruction matrix. The OPT BITFIELD switch (instLookup: INV_OPCODE when off) comes with 8.9 options |
| 8.8 OPT CRE Symbol Table + MEX/SEX Listing Expansion (parity A5, A6) | DONE | `29fad1c` | Done by 8.13 (LISTING.CPP/SYMBOL.CPP optCRE() ported; OPT CRE/MEX/SEX/CEX/WAR/BIT and option defaults); corpus verified in default, expanded (MEX/SEX/CEX) and no-CRE variants |
| 8.9 AssemblerOptions Struct (parity A7) | DONE | `29fad1c` | Done by 8.13: `AssemblerOptions` (generate listing/S-record, bit field, CRE, CEX, MEX, SEX, warnings, tab type/size) = the original Options dialog + editor settings; `easy68k-asm` has the reference driver's flags. Auto-save before assembling and the source template are GUI items (13.19) |
| 8.10 Reference Oracle + Oracle-Generated Golden Data | DONE | `76f5fb0`, `7017448` | `tools/reference/edit68k` builds the original Edit68K assembler core on Linux (`easy68k-ref-asm`, opt-in `BUILD_REFERENCE_TOOLS`); `tools/easy68k-asm` port CLI; `compare_asm.sh` differential check; `data/golden/asm` regenerated by the oracle; `known_failures.txt` tracks remaining gaps |
| 8.11 Assembler Object-Code Parity (S-records match the oracle) | DONE | `ae21b7e` | Pass-2 BACKREF semantics (labels defined before operand parsing, backref only once reached in pass 2); unsized backward branches in range → short form; word alignment adds no pad byte; DS/EVEN write no data; S-records split per OBJECT.CPP; END operand = start address (S8/listing; $1000 + warning if missing; forward ref error; odd → +1 warning); AND/OR #imm,Dn use the EA-immediate form, EOR #imm → EORI; all 21 golden S68 files match (asm 685, sim 688, integration 10; ASan+UBSan clean) |

Decision: The oracle (original assembler built from `../Easy68k`) is authoritative for the golden data; unit tests that encoded the port's earlier behaviour (word-form back branches in WHILE/REPEAT, zero-filled DS) were updated to the oracle's output, which is quoted in the test comments. `AssemblyResult::org`/`code` now start at the first emitted byte, so leading DS space is not part of the image, as in the S-records.
| 8.12 Faithful Instruction Table + Builders, Instruction Matrix vs Oracle | DONE | `e0826c2` | `instruction_set.{h,cc}`: INSTTABL.CPP flavor lists (generated verbatim), BUILD.CPP/CODEGEN.CPP builders and createCode()/pickMask() flavor selection, run in both passes (pass 1 only advances loc) — replaces the collapsed InstructionTable, CodeGenerator and pass-1 InstructionSize(); MOVEM per MOVEM.CPP; original status codes + ERROR.CPP messages (`original_status.{h,cc}`), one most-severe status per line; parser: FORCING_SHORT (.W absolute truncated), invalid size suffix → INV_SIZE_CODE, lenient undefined/register-list symbols in instruction operands (UNDEFINED/REG_LIST_SPEC, instruction still built), SP is not a register-list register; `tools/reference/gen_instruction_matrix.py` + 98,583-case `data/golden/asm/matrix` (132 mnemonics x modes x sizes, incl. errors) all match the oracle (bytes + diagnostic class); replaced the 214 encoder-internal unit tests (code_generator_test, instruction_table_test) |
| 8.13 Assembly Loop Port: Listing (.L68) Parity, Directive/Macro/Structured Corpus vs Oracle | DONE | `29fad1c` | `src/asm/core/`: ASSEMBLE/DIRECTIV/MACRO/STRUCTURED/LISTING/SYMBOL/EVAL/OPPARSE/OBJECT/ERROR/MOVEM/INSTLOOK/CODEGEN.CPP ported function by function (globals → `AsmCore` members, FILE → in-memory `MemFile`, VCL → small replacements), sharing the 8.12 flavor table/builders; `Assembler` is now a thin API (editor tab-settings trailer, include lookup incl. case-insensitive names, results, original status per message, symbols); old front end (assembler internals, macro/structured processors, listing/S-record generators) removed; all 21 golden programs match the oracle byte for byte (.L68 and .S68, `known_failures.txt` empty); new 7-program corpus × 3 option variants (21 cases) matches; instruction matrix still 98,583/98,583; tests updated to oracle-verified behaviour (no EVEN, no conditional ELSE, IFC ignores case, END in INCLUDE ends assembly, SIMHALT object word, original message texts, no error limit) |
| 8.14 Remove Unused Assembler Front End (lexer/parser/expression evaluator/symbol table) | DONE | `3d15f4b` | Removed lexer, parser, expression evaluator and symbol table (and their 4 unit-test files) now that the core port does all parsing; instruction_set no longer depends on them; README status/testing sections updated (asm 394 tests incl. matrix, golden, corpus) |

Decision: The assembler core is a function-by-function port of the original sources (8.13) rather than a reimplementation: the listing format, macro/structured expansion and error behaviour come from the original's text pipeline, and only a faithful port reproduces them exactly. Differences from the original are limited to (a) undefined behaviour made defined — popping an empty structured-statement stack reports "No matching IF/WHILE/REPEAT/FOR/DBLOOP statement was found", 32-bit arithmetic wraps via unsigned math, larger internal buffers (expression stack 64, macro line copies) where the original overflows; (b) environment — in-memory files, "Created On" text supplied by the caller, include files read through a reader that also tries the name as written (the original runs on a case-insensitive file system); (c) `printf`-style output of user text uses "%s".
Decision: Port-only extensions were dropped for parity: EVEN/ODD/XDEF/XREF directives, ELSE inside IFxx/ENDC, double-quoted strings, error limit (SetMaxErrors is accepted but ignored), case-sensitive IFC. The corresponding unit tests now assert the original's behaviour (verified with the reference oracle).
Decision: Sim68K erases memory to $FF on start-up and before loading a program (SIM68Ku.cpp); `Memory::Erase()` provides it. This matters for parity: the original assembler writes SIMHALT's two $FFFF words at the same address (one S-record word), so the second word comes from the erased memory.

Decision: The instruction matrix compares object bytes and the diagnostic class (none/warning/error) per case; for cases the original reports as "Undefined symbol" only the length is compared, because the original builds them from an uninitialized value (EVAL.CPP evalNumber() leaves the number unset). Message texts are now the original's for instruction diagnostics; the listing task will make all messages and their listing format match.
Decision: The old encoder unit tests (CodeGenerator::Encode/InstructionTable internals) were removed with those classes; the oracle-generated matrix supersedes them (every mnemonic, flavor, addressing mode and size, including the error paths) and the remaining assembler tests exercise the public Assembler API.
Decision: Remaining assembler work found by the oracle: listing (.L68) format parity, and replacing the collapsed one-row-per-mnemonic instruction table and the separate pass-1 `InstructionSize()` with a faithful port of INSTTABL.CPP flavors + BUILD.CPP builders run in both passes (fixes EA-mode validation such as `TST An`, and keeps pass-1 sizes identical to pass-2 code by construction).

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
| 9.6 Core Integration Test | DONE | `426acd0` | tests/integration/core_integration_test.cc (`easym68k-integration-tests`): assemble → S-record load → InitProgram → headless run; full program, cycle accumulation, interrupt during Run, STOP/wake, logger trace, observer events, SIMHALT off, exceptions off, memory protection + interrupt, Trap #15 text I/O; 10 tests (stable over 100 repeats; ASan+UBSan clean) |
| 9.7 Memory Map Semantics + Address/Bus Error Class 0 Exception Frame (parity S3, S10) | DONE | `9a7a002` | Memory map per memoryMapCheck() (Invalid / Protected-unless-supervisor / new kMemReadOnly / ROM writes ignored; hardware I/O exempt); Memory fault latch; class 0 frame (PC, SR, IR, address, info word FC5/6+R/W), +50 cycles, double fault and bad SSP halt; ConfigureMemoryMap/SetHardwareAddresses; stacked-PC rule fixed; Run() continues through exceptions; 27 new tests (688 sim tests; ASan+UBSan clean) |

Decision: Memory records the first failing access in a fault latch (with the PC at that moment); Step() checks it after the opcode fetch and after execution, restores the registers to their pre-instruction state (the original returns before storing results) and builds the class 0 frame. This catches faults from every access path (ReadEA discards errors; handlers access memory_ directly) without threading error returns through all handlers. Cycles on a fault are 50, without the EA time the original adds before detecting the error (≤ EA penalty difference).
Decision: Stacked PC for class 1/2 exceptions follows RUN.CPP: OLD_PC = PC after the instruction for TRAP/TRAPV/CHK/DIV0/trace/privilege/Line A/Line F, and the instruction address only for illegal instruction. Previously every exception stacked the instruction address, so a TRAP handler's RTE re-executed the TRAP. Class 0 stacks OLD_PC+2: the PC at the time of the access for reads, instruction address + 2 for writes; info word FC = 6 (program) when the faulting address equals that PC, else 5 (data), plus $10 for reads.
Decision: Run() now continues through exceptions (they vector to their handlers) and stops only on halt, breakpoint, stop request or instruction limit, as the original run loop does; previously any non-OK result stopped it.
Decision: Memory map reads check only the first byte (mem_req()), writes check the whole access range (memoryMapCheck()); ROM writes return kOk without writing. Trap #15 string-buffer map checks (memoryMapCheck(Invalid, ...) in CODE9.CPP) remain a follow-up for the Trap #15 back ends.
| 9.8 Bit-Field Instructions in Simulator (parity S8) | DONE | `dfd8262` | `opcodes_bitfield.cc`: CODE7.CPP BIT_FIELD() ported (BFTST/BFEXTU/BFCHG/BFEXTS/BFCLR/BFFFO/BFSET/BFINS, Dn and control modes, Dn/immediate offset and width, signed Dn offset, 5-byte fields, cycles incl. EA time); decoded before the memory shift form (OPCODES.H order); `Config::bitfield_enabled` (default off, as Sim68K) + Set/IsBitFieldEnabled; illegal when off or for non-control modes; 14 BitFieldTest (702 sim tests; ASan+UBSan clean) |

Decision: The bit field port keeps two original quirks for parity: a field in a data register does not wrap around (the register is shifted left by the offset, so bits beyond bit 0 read as 0), and when a memory field spans five bytes the fifth byte is written with the old second byte (CODE7.CPP writes `bf1` instead of the computed `bf4`). Both are covered by tests that cite the original.
| 9.9 Advanced Breakpoint Engine (parity S14) | DONE | `1b68a39` | `break_condition.h/.cc`: BPoint (register D0-D7/A0-A7/PC and memory conditions; ==, !=, >, >=, <, <=, any; read/write/either/any access; byte/word/long) and BPointExpr (postfix AND/OR over up to 100 conditions, 50 expressions, pass count) ported; Memory tracks the last read/write address per instruction (mem_req/mem_put readEA/writeEA); evaluated after every Step() as runprog() does (all expressions, counters advance), `AdvancedBreakpointHit()` + kBreakpointHit event; Run() now executes an instruction before checking breakpoints (a run started on a PC breakpoint proceeds, as in runprog()); 13 BreakConditionTest (715 sim tests; ASan+UBSan clean) |

Decision: Advanced breakpoints keep the original's observable behaviour: only the last read and the last write of an instruction count (readEA/writeEA), a memory condition reads its location through mem_req and so becomes the "last read" for conditions evaluated after it, comparisons are signed 32-bit, and the pass counter of an expression with count 0 breaks while the condition is false. Undefined cases are defined instead: an invalid size uses a long, a malformed postfix expression does not break, and the breakpoint read of an unmapped or odd address yields 0 without raising a bus/address error (the original would take the exception during evaluation).
| 9.10 Step Over / Run To Core API (parity S15) | DONE | `c2ec310` | `StepOver()` (StepExecute + CODE8.CPP BSR/JSR stepToAddr: a call runs until the return address, nested calls included; stops on PC/advanced breakpoints, halt, stop request), `RunTo(addr)` (runToAddr, armed unless the address is a PC breakpoint, checked after each instruction, stays armed until reached or InitProgram), `ResetBreakPassCounts()` (Run's resetDebug()); InitProgram clears step/run-to state; 9 StepOverTest (724 sim tests; ASan+UBSan clean). Auto Trace (timer-driven stepping) is GUI: 13.24 |
| 9.11 Program Init/Rewind + Hardware Reset Semantics (parity S17) | DONE | `bc53056` | InitProgram() = initSim() (PC=start, SR=$2000, USP=$00FF0000, SSP=$01000000, regs/cycles cleared, no vector fetch); HardwareReset() = Reset button (S=1, T=0, mask 7, SSP/PC from vectors, +40 cycles, wakes STOP); Get/SetStartAddress (S-record start used); 7 ProgramInitTest (662 sim tests; ASan+UBSan clean) |

Decision: The existing vector-based Reset() is kept as a power-on reset (used by unit tests); program open/rewind uses InitProgram() and the hardware window uses HardwareReset(), matching the original's two distinct paths.

Decision: Order revised during 9.6 preparation: 9.11 → 9.7 → 9.6 → 9.8 → 9.9 → 9.10. The integration test needs S-record start-address init (original initSim(): PC=start, SR=$2000, USP=$00FF0000, SSP=$01000000, no vector fetch) and the original memory map semantics (UTILS.CPP memoryMapCheck(): ROM writes ignored, Read = bus error on write, Protected = bus error unless supervisor, Invalid = bus error on access, class 0 frame), neither of which the port had.

Decision: Parity audit (`doc/execution_plan/parity-audit.md`) inventoried every menu, dialog, option and Trap #15 task of Sim68K, Edit68K and EASyBIN from the original `.dfm` forms and sources. It added tasks 8.7-8.9, 9.7-9.10 and 13.22-13.25. Order: finish 9.3-9.6 as planned, then 9.7-9.10, then 8.7-8.9, before Phase 10.
Decision: Each task is committed once it is tested and certified (full test suites green, clang-format clean, PROGRESS.md and doc/execution_plan/parity-audit.md (snapshot + iteration log) updated); the commit hash is recorded in a follow-up PROGRESS commit (user instruction, 2026-10-13).

## Phase 10: Golden Simulation Traces

| Task | Status | Commit | Summary |
|------|--------|--------|---------|
| 10.0 Reference Simulator Oracle + Trace Comparison | DONE | `c467956` | `tools/reference/sim68k` builds the ORIGINAL Sim68K core (RUN, CODE1-9, UTILS, STARTSIM, SIMOPS1/2, STRUTILS, SCAN, BPoint*) as a 32-bit headless `easy68k-ref-sim` (VCL/GUI stand-ins in `compat/sim_prelude.h`, Trap #15 text on stdout/stdin); `tools/easy68k-sim` runs the port with the same trace format; `tools/reference/compare_sim.sh` diffs per-instruction register traces, memory hash and program output; `tools/reference/gen_sim_corpus.py` turns the assembler matrix into 131 programs (4,453 instruction cases); first finding fixed: LEA to A4-A7 and CHK on D4-D7 decoded as illegal (2 unit tests, 726 sim tests) |
| 10.1 Implement Golden Trace Comparison | DONE | `080908a` | `data/golden/sim/programs`: 23 programs assembled and run by the ORIGINAL Sim68K core (`tools/reference/regenerate_sim_golden.sh`): `src/smoke.x68`, `src/trap15_text.x68` (Trap #15 text/number tasks 0-6, 9, 12-15, 17, 18, 20, 30, 31 with keyboard input) and 21 instruction corpus programs whose oracle trace the port reproduces exactly (`corpus.txt`); per program the S-record, a digest of every instruction's registers, END/FINAL/HASH lines and the Trap #15 output. `tests/sim/golden_trace_test.cc` replays them through `easy68k-sim-trace` (runner shared with the `easy68k-sim` CLI); 24 tests (764 sim tests) |

Decision: The simulator oracle is the original core compiled with `-m32` (the sources assume 32-bit `long` and pointers) against headless stand-ins for the VCL forms. Only two Borland-only constructs are rewritten when the sources are staged (CODE7.CPP cast-as-lvalue shift and address-of-cast), with identical meaning. The driver mirrors TForm1::OpenFile (initSim, erase to $FF, toggle-switch byte 0, loadSrec) and runLoop (runprog until halted); the original's default is exceptions off (`-e` enables, like Sim68K `/e`). Traces exclude message-pane text (MSG lines).
Decision: First oracle run of the instruction corpus (exceptions on): 96 of 131 programs identical. The remaining differences are systematic, not per-instruction typos: (1) an address/bus error in the original builds the exception frame inside mem_req()/mem_put() at the moment of the access, with (An)+/-(An) updates and EA cycles already applied, and the instruction then carries on, whereas the port rolls the registers back to the start of the instruction; (2) flags the original computes for "undefined" cases through cc_update() (ABCD/SBCD/NBCD N and V, CHK, DIVS/DIVU/MULS, NEGX); (3) the original keeps a 32-bit PC (RTS/RTR/RTE to $FFFFFFFF), the port masks it to 24 bits; (4) EA cycles added before a faulting access.
Decision: Parity policy (user instruction, 2026-10-14): do NOT reproduce bugs of the original — fix them when found, and fix algorithmic inefficiencies too, but add no functional improvements (no new features or behaviour beyond the original's). This supersedes "bug-for-bug by default". The oracle stays the reference for everything the original does correctly; every intentional difference is a documented deviation (original bug or inefficiency) with a reason. The Phase 6 decisions (ABCD/SBCD/NBCD X, CHK sign test, shift count modulo 64) stand under this policy.
| 10.2 Oracle Difference Triage | DONE | `9f42410` | `compare_sim_blocks.py` compares the corpus case by case (state after each case's register load, cycle deltas) and `compare_sim_corpus.sh` runs all 131 programs plus the memory hash; `data/golden/sim/deviations.tsv` lists accepted differences with reasons. Port bugs fixed: NEGX C/X (0 - $FF..FF - 1), CHK N, CLR/Scc/MOVE from SR read the destination first (fault is a read), a faulting access no longer lets the instruction store its result (`Memory::SetFaultBlocksWrites`), NBCD digit algorithm (non-BCD inputs). Result: 4,288 executed cases, 0 unexplained differences (182 accepted: undefined flags, original bugs, fault side effects, 24-bit PC); 8 regression tests in `oracle_regression_test.cc` (734 sim tests) |

| 10.3 Opcode Sweep vs Oracle | DONE | `16ac30d` | `gen_sim_corpus.py --sweep` runs every first word ($0000-$FFFF except branches/JMP/JSR/DBcc/RTS/RTE/RTR/STOP/TRAP #15/UNLK and encodings that make the original overwrite its own code) with four $3000 extension words, 16 programs, 61,034 opcodes; `compare_sim_corpus.sh --sweep`. Found the port executing ~1,600 illegal encodings (no addressing-mode validation): new opcode legality table `src/sim/legality.cc` (`IsLegalOpcode`, built once, one lookup per instruction, rejects before any operand access); ABCD/SBCD now combine digits as the original (non-BCD inputs). Corpus generator: per-vector exception stubs, 128-byte slots, memory windows restored per case. Result: sweep and instruction corpus 0 unexplained differences; 6 legality tests + 1 (740 sim tests) |
| 10.4 Trap #15 vs Oracle | TODO | | Text/number tasks verified (golden `trap15_text`); remaining: file tasks 50-59 need a headless IFileIO back end (CODE9.CPP file code), plus options/time tasks |

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

Decision: The applications are GPL-2.0-or-later (user choice, 2026-10-14; commit `0c0433f`): root `LICENSE` is the GPLv2 text, matching the original EASy68K (GPL, unversioned, ships GPLv2) and the SPDX headers; "or later" keeps a GPLv3 move open.
Decision: Examples are CC0-1.0 (user chose "CC"; CC0 is the Creative Commons option suited to code). GCC cross-compiler (C11, freestanding C++17) is the reference C/C++ toolchain. Core path and Track C proceed in parallel.
Decision: E2 depends on the assembler forward-reference fix (pass-2 backward-reference semantics found by the 9.6 integration test); examples must assemble identically to the original EASy68K.

---

Decision: IFARG and all body conditionals (IFC/etc.) are handled via a unified LocalFrame stack inside InvokeMacro — never injected to RunPass raw. IFC inside true IFARG is tracked as an "injected" frame (its ENDC is also injected); IFC inside false IFARG is tracked as a "local" frame (nothing injected). This ensures ENDC is always correctly paired.
Decision: Label-without-colon on macro calls requires the symbol to be at column 1 (ParseLabel column-1 guard). Without this guard, indented macro invocations like `  MYMACRO A,B` would misclassify `MYMACRO` as a label and `A` as the opcode.

Decision: ORG to an odd address was changed from kError to kWarning (kOddAddress) to match original EASy68K behaviour (ODD_ADDRESS was a warning in the original asm.h). Existing tests that check for "odd" in the message still pass because the warning message contains "odd".
Decision: The 4-argument AddError(int, string, ErrorCode, ErrorSeverity) is the internal implementation; the 2- and 3-argument public overloads format and delegate to it.

Decision: Refactored `ListLine` to use type-safe metadata structure (`left_column`, `right_column`, `is_error`, `is_warning`, `is_continuation`) instead of fragile string placeholders (`RIGHT_PLACEHOLDER`, `LINE_PLACEHOLDER`), eliminating any risk of parsing bugs during formatting.

Decision: Replicated exact S0/S1/S2/S3/S8 Motorola format with memory map S0 records. S8 termination address formats to 24-bit to match Borland. S0 checksum calculated programmatically to avoid hardcoded mismatch. SRecordGenerator buffers bytes to output SRECSIZE=36 records.

Decision: Accepted deviations from the oracle (data/golden/sim/deviations.tsv), each checked by masking only the named fields of the matching cases: undefined flags (ABCD/SBCD/NBCD N,V; CHK Z,V,C; DIVS/DIVU N,Z on overflow), original bugs the port fixes (DIVS/DIVU overflow leaves C set; MULS sets V; MOVE reads its destination so a faulting destination stacks a read frame), fault side effects (the original raises address/bus errors inside the memory access after (An)+/-(An) updates and EA cycles; the port restores the instruction-start registers and cycles, as the 68000 aborts the instruction) and the 24-bit PC (the original keeps the upper byte popped by RTS/RTR/RTE; fetches are masked in both).
Decision: Invalid BCD digits are undefined in the PRM; the port's NBCD now uses the original's digit algorithm (same results for valid BCD), since that is not a bug and keeps parity.
Decision: Opcode legality is a table built once from the 68000 rules (`IsLegalOpcode`), replacing per-instruction checks: an algorithmic improvement over the original (decode table scan plus eff_addr() mask tests on every instruction) and the 68000's order (an illegal encoding is rejected before any operand access; the original may first advance (An)+/-(An), fetch extension words and add EA cycles, or take an address error from the source). Where the original accepts encodings the 68000 rejects (ADDQ.B/SUBQ.B #n,An; BCHG/BCLR/BSET with PC-relative or immediate destinations; BFCHG/BFCLR/BFSET/BFINS with PC-relative EAs) the port follows the 68000; BTST #n,#imm stays legal as in the original assembler and simulator. All listed in data/golden/sim/deviations.tsv.
Decision: Further original bugs found by the sweep and not reproduced: byte ABCD/SBCD/ADDX/SUBX -(A7) and CMPM (A7)+ step A7 by 1 (odd stack pointer); ORI/ANDI/EORI to CCR apply the whole immediate word to SR; the ASL/ASR V test for large register counts (operator precedence).
Decision: Golden simulation traces store a digest of the trace (FNV-1a 64 of every INIT/STEP line) plus the final lines instead of full traces, keeping the data small (848 KB, mostly S-records); on a mismatch `tools/reference/compare_sim.sh` shows the first differing instruction. Only programs without accepted deviations are golden, so every expected value is the original's.
**NEXT:** 10.4 — remaining Trap #15 tasks (headless file I/O back end) against the simulator oracle; then Phase 11
