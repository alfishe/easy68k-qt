# EASy68K Code Transfer Plan

## 1. Purpose

This document specifies exactly what code from the existing `EASy68K-qt` repository will be transferred into the new port, how it will be adapted, and what will be discarded. It is a companion to the [Porting Proposal](porting_proposal.md) and should be read alongside it.

**Scope:** This plan covers only the **initial transfer phase** (Phases 0–2 of the execution plan). After the salvageable EASy68K-qt components (types, memory, S-Record loader) are transferred, all subsequent work ports directly from the original EASy68K Borland sources. See the execution plan's "Source Transition Point" for details.

**Source repository:** `EASy68K-qt` (prior porting attempt)
**Target:** New project following the architecture defined in the porting proposal
**After Phase 2, primary source switches to:** `EASy68K/` (original Borland codebase)

---

## 2. Transfer Summary

| Category | Source Lines | Action | Estimated Target Lines | Effort |
|----------|-------------|--------|----------------------|--------|
| Simulator types/enums | 168 | **Adopt with modifications** | ~180 | Low |
| Memory subsystem | 237 + 124 | **Adopt with cleanup** | ~300 | Low |
| Addressing modes | 234 | **Discard** (sentinel EA pattern) | ~250 | High (rewrite) |
| Instruction decoder | 194 | **Discard** (linear search) | ~200 | High (rewrite) |
| Opcode implementations | ~3,900 | **Transfer logic, rework patterns** | ~4,200 | Medium |
| CPU class | 375 + 368 | **Discard** (architecture mismatch) | ~500 | High (rewrite) |
| S-Record loader | 199 | **Extract and adopt** | ~220 | Low |
| Assembler core | ~2,900 | **Adopt with significant gaps filled** | ~4,500 | Medium-High |
| GUI (simulator) | ~1,350 | **Discard** (stubs) | ~3,000 | High (new) |
| GUI (editor) | ~1,860 | **Discard** (stubs) | ~2,500 | High (new) |
| Tests | 1,067 | **Adopt pattern, expand heavily** | ~5,000 | Medium |
| Build system | 46 | **Discard** (C++11, wrong structure) | ~120 | Low |

**Bottom line:** ~5,500 lines of logic are salvageable (types, memory, opcodes, S-Record, assembler). ~8,000 lines of architecture and stubs will be discarded. The net transfer saves an estimated 3–4 weeks of typing but requires careful adaptation of every transferred line.

---

## 3. File-by-File Transfer Specification

### 3.1 Simulator Types — `src/core/simulator/types.h`

**Verdict:** ADOPT WITH MODIFICATIONS

**What to keep:**
- `DataSize` enum class
- `Condition` enum class (all 16 condition codes)
- `ExceptionVector` enum class (vectors 0–47)
- `MemoryFlags` enum
- All `constexpr` constants: `kMemorySize`, `kAddressMask`, `kByteMask`/`kWordMask`/`kLongMask`
- SR bit definitions: `kSrCarry` through `kSrTrace`
- Addressing mode masks: `kAddrModeDn` through `kAddrModeAll`

**What to change:**

| Item | Change | Reason |
|------|--------|--------|
| `ExecResult` enum | Rename to `SimResult`, add `kTrapException`, remove `kBreakpoint` (breakpoints are GUI concern) | Align with proposal Section 6.5 |
| `kAddrRegCount = 9` | Change to 8 + separate `kSspIndex` constant | SSP at index 8 is an implementation detail, not a register count |
| Addressing mode constants | Move to `addressing_mode.h` in new structure | Separation of concerns |
| Add `EffectiveAddr` struct | New tagged union type (see proposal Section 6.2) | Replaces sentinel addresses |
| Add `EaTarget` enum | `kDataReg`, `kAddrReg`, `kMemory`, `kImmediate` | Discriminator for EffectiveAddr |

**Transfer steps:**
1. Copy `types.h` to `include/easym68k/sim/types.h`
2. Rename `ExecResult` → `SimResult`, adjust values
3. Add `EffectiveAddr` and `EaTarget` definitions
4. Remove addressing mode masks (move to dedicated header)
5. Change `kAddrRegCount` semantics
6. Run `clang-format` with Google Style

---

### 3.2 Memory Subsystem — `src/core/simulator/memory.h` + `memory.cc`

**Verdict:** ADOPT WITH CLEANUP

**What to keep:**
- Big-endian storage policy (ReadWord/ReadLong byte assembly, WriteWord/WriteLong byte decomposition)
- Alignment checking (`IsAligned`)
- 24-bit address masking (`MaskAddress`)
- Memory flags system (`kMemNormal`, `kMemInvalid`, `kMemProtected`, `kMemRom`)
- ROM/protected write guards
- `MemoryAccessResult` struct
- `LoadData` bulk operations
- `RawData()` for debug display
- `SetFlags`/`GetFlags`/`IsValidRead`/`IsValidWrite`/`IsRom`

**What to change:**

| Item | Change | Reason |
|------|--------|--------|
| `uint8_t* data_` / `uint8_t* flags_` | Replace with `std::vector<uint8_t>` | RAII, no manual delete, noexcept move for free |
| Manual copy-ctor/delete/move | Remove entirely | `std::vector` handles this |
| `ReadByte()` returns `uint8_t` | Change to return `MemoryAccessResult` | Consistency; allows bus-error reporting on invalid addresses |
| No bounds checking on `ReadByte` | Add `kMemInvalid` flag check | Original simulator can detect unmapped memory |
| Missing checksum in S-Record | Add to loader, not memory class | N/A |
| `MemoryAccessResult` | Add `std::string error_message` field | Better diagnostics |

**Transfer steps:**
1. Copy `memory.h` → `include/easym68k/sim/memory.h`, `memory.cc` → `src/sim/memory.cc`
2. Replace raw pointers with `std::vector<uint8_t>`
3. Remove manual dtor/copy/move — use `= default`
4. Unify `ReadByte` return type with `MemoryAccessResult`
5. Add `kMemInvalid` check to `ReadByte`
6. Add `error_message` to `MemoryAccessResult`
7. Run golden memory tests

---

### 3.3 Addressing Modes — `src/core/simulator/addressing_modes.cc`

**Verdict:** DISCARD AND REWRITE

**Fatal flaw:** Uses sentinel address values (`0xFF000000+`) to distinguish register targets from memory, duplicating the original's pointer-range discrimination pattern in address space. This is the exact problem the `EffectiveAddr` tagged union is designed to solve.

**What can be referenced during rewrite:**
- The mode/register decoding switch structure (modes 0–7, sub-modes 7.0–7.4)
- Post-increment/decrement logic (SP word-alignment for byte ops)
- Displacement and indexed addressing PC-relative calculations
- The general flow of: decode → fetch extension words → compute address → read value

**What must be different in the rewrite:**
- Return `EffectiveAddr` instead of `uint32_t ea`
- No sentinel addresses — `EaTarget::kDataReg`/`kAddrReg`/`kMemory`/`kImmediate` discriminator
- Index register decoding uses `EffectiveAddr` for the index value too
- Do NOT read the value in CalculateEA — separate address calculation from value fetch
- All addressing mode logic becomes a free function or static method, not a Cpu member

**Transfer steps:**
1. Do NOT copy the file
2. Write new `src/sim/addressing_mode.cc` from scratch
3. Use the original as a reference for mode/register decoding logic
4. Return `EffectiveAddr`, never a raw address that needs range checking
5. Split "compute address" from "read value at address" — two separate operations

---

### 3.4 Instruction Decoder — `src/core/simulator/instruction_decoder.cc`

**Verdict:** DISCARD AND REWRITE

**Fatal flaw:** Linear search through a 40-entry table. The original uses bit-field dispatch. A proper decoder should use the top 4 bits as a primary index, then refine.

**What can be referenced:**
- The mask/value/handler pattern is a valid concept
- The instruction table entries themselves (which opcodes map to which handlers)
- Priority ordering (more specific masks first)

**What must be different:**
- Use a two-level dispatch: primary index on bits 15–12, then refinement
- Or use a compiled decode tree (switch on top 4 bits, nested switches)
- The handler signatures change (they receive `EffectiveAddr` not sentinel `uint32_t`)
- Must handle SIMHALT (`0xFFFFFFFF`) as a first-class instruction

**Transfer steps:**
1. Do NOT copy the file
2. Write new `src/sim/decode.cc` with hierarchical dispatch
3. Reference the existing table for which opcodes exist and their mask/value pairs
4. Add SIMHALT as a recognized instruction (Line-F subtype)

---

### 3.5 Opcode Implementations — `opcodes_move.cc`, `opcodes_arithmetic.cc`, `opcodes_logic.cc`, `opcodes_branch.cc`, `opcodes_misc.cc`

**Verdict:** TRANSFER LOGIC, REWORK PATTERNS

These ~3,900 lines contain the bulk of the salvageable work. The instruction logic (how ADD computes its result, how MOVEM builds its register mask, how DBcc decrements and branches) is largely correct. The surrounding patterns (sentinel EA, `WriteToEA` free function, incomplete flag computation) must be replaced.

**What to keep (per-instruction logic):**
- `OpMove`: Size decoding from MOVE-specific bit pattern (01=byte, 11=word, 10=long), source/dest EA extraction, register write masking
- `OpMovea`: Word sign-extension for `.W` variant
- `OpMoveq`: 8-bit immediate sign extension to 32-bit
- `OpAdd`/`OpSub`: Opmode decoding (register direction), size selection, flag computation structure
- `OpAnd`/`OpOr`/`OpEor`: Same pattern as ADD/SUB
- `OpMulu`/`OpMuls`/`OpDivu`/`OpDivs`: 32×32→32 multiply, 32÷16 divide with overflow
- `OpBcc`/`OpBra`/`OpBsr`: Displacement decoding (byte/word), condition checking
- `OpDbcc`: Counter decrement, branch/fallthrough logic
- `OpScc`: Condition evaluation, byte write
- `OpJsr`/`OpJmp`/`OpRts`/`OpRte`/`OpRtr`: Stack manipulation, PC load
- `OpMovem`: Register list bitmask, pre-decrement/post-increment ordering
- `OpLea`/`OpPea`: EA calculation without value fetch
- `OpLink`/`OpUnlk`: Frame pointer setup/teardown
- `OpSwap`/`OpExt`: Register manipulation
- `OpExg`: Three exchange variants (Dn-Dn, An-An, Dn-An)
- `OpChk`/`OpTrap`/`OpTrapv`: Exception generation
- `OpTas`: Atomic test-and-set
- `OpStop`/`OpReset`/`OpNop`/`OpIllegal`: Miscellaneous
- `OpShiftRotate`: Count decode (immediate vs register), direction, type (arithmetic/logical/rotate)
- `OpBitOp`: Static vs dynamic bit number, BTST/BCHG/BCLR/BSET variants
- `OpAbcd`/`OpSbcd`/`OpNbcd`: BCD arithmetic with extend flag

**What must change in every handler:**

| Pattern in EASy68K-qt | Replacement in new code |
|------------------------|------------------------|
| `IsDataRegEA(ea)` / `IsAddrRegEA(ea)` | Switch on `ea.target == EaTarget::kDataReg` etc. |
| `DataRegFromEA(ea)` / `AddrRegFromEA(ea)` | `ea.index` |
| Memory target `ea` address | `ea.address` |
| `WriteToEA(cpu, ea, value, size)` free function | Method on `M68kSimulator` that switches on `EffectiveAddr.target` |
| `cpu->GetState().d[reg]` direct access | `sim.WriteDataReg(reg, value, size)` / `sim.ReadDataReg(reg, size)` |
| Incomplete `UpdateFlags()` | Per-operation flag computation (V/C/X differ for add vs subtract vs logic) |
| `constexpr kEaDataRegBase` / `kEaAddrRegBase` in each .cc | Deleted — replaced by `EffectiveAddr` |
| `Cpu*` parameter to helper functions | `M68kSimulator&` reference |
| `AddCycles(N)` | Keep, but cycle counts must be verified against Motorola documentation |

**Critical: Flag computation must be completed.** The existing `UpdateFlags()` only sets N and Z. Every arithmetic instruction needs proper V/C/X computation:

- **ADD:** V = (src_sign == dst_sign) && (result_sign != src_sign); C = carry out of MSB; X = C
- **SUB:** V = (src_sign != dst_sign) && (result_sign != dst_sign); C = borrow; X = C
- **Logic (AND/OR/EOR):** V = 0; C = 0; X unchanged
- **MOVE:** V = 0; C = 0; N/Z from result; X unchanged
- **Shift/Rotate:** V and C depend on last bit shifted out

**Transfer steps (per file):**
1. Copy the .cc file to `src/sim/opcodes_*.cc`
2. Remove all sentinel EA constants and helper functions
3. Replace `IsDataRegEA`/`IsAddrRegEA` with `EffectiveAddr` target checks
4. Replace `WriteToEA` with proper `M68kSimulator` methods
5. Complete all flag computations per the M68000 Programmers Reference Manual
6. Change handler signatures to match new Cpu class methods
7. Add `SIMHALT` handler (opcode `0xFFFFFFFF`, Line-F subtype)
8. Verify each handler against the original `CODE1.CPP`–`CODE9.CPP` logic
9. Run existing opcode tests, then expand with golden tests

---

### 3.6 CPU Class — `src/core/simulator/cpu.h` + `cpu.cc`

**Verdict:** DISCARD AND REWRITE

**Fatal flaws:**
- Cpu owns Memory by value — no dependency injection
- I/O callbacks are toy-level (`std::function<void(char)>`)
- Trap callback is just `std::function<void(int)>` — cannot express Trap #15 protocol
- `ea1_`, `ea2_`, `ea1_value_`, `ea2_value_` are class members acting as implicit state — this is the original's global variable pattern repeated
- `CpuState` uses index 8 for SSP — the proposal requires proper USP/SSP management

**What can be referenced:**
- `CpuState::Reset()` — SSP/PC initialization from vectors 0/4
- `CpuState::GetSP()`/`SetSP()` — supervisor-aware SP access
- `CheckCondition()` — all 16 condition codes are correct
- `HandleException()` — stack frame push (PC+SR), vector fetch, supervisor switch
- Breakpoint data structure

**What must be different in the rewrite:**
- Class named `M68kSimulator` (per proposal)
- Memory injected via reference, not owned
- All 9 Trap #15 interface pointers held by the simulator
- `EffectiveAddr` returned by addressing mode calculation
- No implicit EA state stored as members — pass as parameters
- Complete flag computation inline in each handler
- `SimResult` return type instead of `ExecResult`
- `ILogger*` for logging
- Thread-safe `std::atomic<bool> stop_requested_`
- `Step()` returns `SimResult`, `Run()` loops with stop check

**Transfer steps:**
1. Do NOT copy the files
2. Write new `include/easym68k/sim/simulator.h` and `src/sim/simulator.cc`
3. Reference existing `CpuState` for register layout and `Reset()` logic
4. Reference existing `CheckCondition()` — it's correct
5. Reference existing `HandleException()` for stack frame format
6. Inject Memory, all 9 Trap #15 interfaces, and ILogger via constructor
7. Implement `Step()`, `Run()`, `Reset()` per proposal
8. Add `SIMHALT` as a first-class instruction returning `SimResult::kHalted`

---

### 3.7 S-Record Loader — `src/core/simulator/srecord_loader.cc`

**Verdict:** EXTRACT AND ADOPT

**What to keep:**
- S0/S1/S2/S3/S5/S6/S7/S8/S9 record type handling
- Hex parsing helpers (`HexCharToInt`, `ParseHexByte`, `ParseHexValue`)
- 16/24/32-bit address decoding per record type
- Start address extraction from S7/S8/S9

**What to change:**

| Item | Change | Reason |
|------|--------|--------|
| Attached to `Cpu` class | Extract as standalone `SRecordLoader` class | Proposal requires EASyBIN to reuse this; cannot be coupled to CPU |
| No checksum validation | Add checksum verification (accumulate all bytes, compare against final byte, complement check) | Data integrity |
| Writes directly to `memory_` | Accept a `Memory&` parameter or return `std::vector<std::pair<uint32_t, std::vector<uint8_t>>>` | Decoupling |
| Returns `bool` | Return a structured result with error details | Better error reporting |
| Only file + lines input | Also support `std::istream` | Flexibility |

**Transfer steps:**
1. Copy to `src/sim/srecord_loader.cc`, header to `include/easym68k/sim/srecord_loader.h`
2. Remove `Cpu::` prefix — make it a standalone class
3. Add checksum validation
4. Change to accept `Memory&` or return structured data
5. Add `std::istream` overload
6. Improve error reporting (line number, record type, specific error)
7. Write golden tests with known-good and deliberately-malformed S-Records

---

### 3.8 Assembler Core — `src/core/assembler/`

**Verdict:** ADOPT WITH SIGNIFICANT GAPS FILLED

**Files to transfer:**

| File | Lines | Assessment |
|------|-------|------------|
| `assembler.h` + `assembler.cc` | 162 + 905 | Framework is reasonable; many methods are stubs or incomplete |
| `lexer.h` + `lexer.cc` | ~100 + 688 | Good tokenizer foundation |
| `parser.h` + `parser.cc` | 166 + 936 | Instruction/directive parsing; needs EASy68K compatibility |
| `symbol_table.h` + `symbol_table.cc` | ~60 + 240 | Functional symbol table |
| `expression.h` + `expression_evaluator.cc` | ~40 + 343 | Expression evaluation |
| `error_reporter.cc` | ~50 | Basic error reporting |
| `code_generator.cc` | ~30 | **Stub** — needs full implementation |
| `directives.cc` | ~30 | **Stub** — needs EASy68K directive set |
| `listing_generator.cc` | ~30 | **Stub** — needs listing format matching original |
| `macro_processor.cc` | ~30 | **Stub** — needs full macro implementation |
| `instruction_table.cc` | ~50 | Partial instruction table |

**What to keep:**
- The `asm68k` namespace and `Assembler` class public API
- The lexer's token types and scanning logic
- The parser's line-by-line processing structure
- The symbol table's label/value storage
- The expression evaluator's operator precedence and arithmetic

**What must be added or reworked:**

| Missing Feature | Original Source | Priority |
|----------------|----------------|----------|
| Complete instruction encoding table | `INSTTABL.CPP` | Critical |
| `DC` directive with all data types | `DIRECTIV.CPP` | Critical |
| `DS` directive (reserve space) | `DIRECTIV.CPP` | Critical |
| `EQU`/`SET` symbol definition | `DIRECTIV.CPP` | Critical |
| `ORG`/`SECTION` address control | `DIRECTIV.CPP` | Critical |
| `INCLUDE` file inclusion | `DIRECTIV.CPP` | High |
| Macro definition/expansion (`MACRO`/`MEND`) | `MACRO.CPP` | High |
| Structured control flow (`IF`/`THEN`/`ELSE`/`ENDI`, `WHILE`/`ENDW`, `FOR`/`ENDF`, `REPEAT`/`UNTIL`, `SWITCH`/`ENDSW`) | `STRUCTURED.CPP` | High |
| Listing file generation matching original format | `LISTING.CPP` | High |
| `MOVEM` register list encoding | `MOVEM.CPP` | Medium |
| Binary output file generation | `BINFILE.CPP` | Medium |
| `OPT` assembler options | `DIRECTIV.CPP` | Medium |
| `COMMENT` block delimiters | `DIRECTIV.CPP` | Low |
| `LOAD`/`END` directives | `DIRECTIV.CPP` | Low |

**Transfer steps:**
1. Copy all assembler files to `src/asm/` and `include/easym68k/asm/`
2. Update namespace to `easym68k::asm` (or keep `asm68k` — decide during implementation)
3. Rename `.cpp` extensions to `.cc`
4. Fill in all stub implementations by porting from the original Borland source
5. Ensure byte-for-byte output parity with original for all golden test files
6. Add macro processor and structured control flow
7. Add listing generator matching original `.L68` format
8. Expand test suite to cover every directive and instruction

---

### 3.9 GUI Applications — `src/gui/`

**Verdict:** DISCARD

**Simulator GUI (1,350 lines):**
- `mainwindow.cc` uses `stub_pc_` / `stub_cycles_` — not connected to `sim68k::Cpu`
- `consolewidget.cc` — basic text display, no Trap #15 integration
- `disassemblyview.cc` — displays hex words, no real disassembly
- `registerwidget.cc` — basic register display
- `memorywindow.cc` — basic hex view
- No hardware window, no sound, no graphics, no network, no serial, no printing

**Editor GUI (1,860 lines):**
- `mainwindow.cc` — tab interface with menus and toolbars (shell only)
- `codeeditor.cc` — line numbers and basic editing
- `highlighter.cc` — basic 68K syntax highlighting (can be referenced)
- `finddialog.cc` — find/replace UI (can be referenced)
- `outputpanel.cc` — basic text panel

**What can be referenced** (not copied, but studied while writing new GUI):
- The `highlighter.cc` keyword list and comment/string rules
- The `finddialog.cc` UI layout for find/replace
- The `mainwindow.cc` menu structure (File/Run/View/Options/Help is correct)
- The `codeeditor.cc` line-number margin implementation

**Transfer steps:**
1. Do NOT copy any GUI files
2. Build new GUI from scratch per proposal Section 7
3. Reference the above files for specific widget patterns
4. The syntax highlighter keyword list can be copied directly
5. Everything else is rewritten to connect to `M68kSimulator` properly

---

### 3.10 Tests — `tests/`

**Verdict:** ADOPT PATTERN, EXPAND HEAVILY

**Existing tests (1,067 lines):**

| File | Lines | Coverage | Action |
|------|-------|----------|--------|
| `cpu_test.cc` | 213 | Reset, register access, MOVEQ, NOP, RTS, breakpoints, SWAP, EXG, EXT, LINK | Adopt test structure; expand with ~50 more tests |
| `opcodes_test.cc` | 454 | BRA, Bcc, BSR, DBcc, some arithmetic | Adopt test structure; expand to cover all instruction categories |
| `memory_test.cc` | 111 | Read/write, alignment, endianness, ROM protection | Nearly complete; add invalid-address tests, boundary tests |
| `srecord_test.cc` | 76 | Basic S-Record loading | Expand with checksum validation, all record types, error cases |
| `assembler_test.cc` | 173 | Empty source, NOP, ORG, labels, MOVEQ, DC.W | Expand dramatically for all directives and instructions |
| `addressing_modes_test.cc` | 31 | Minimal | Rewrite entirely for `EffectiveAddr` pattern |
| `symbol_table_test.cc` | 3 | Stub | Rewrite with comprehensive symbol table tests |
| `parser_test.cc` | 3 | Stub | Rewrite with comprehensive parser tests |
| `expression_test.cc` | 3 | Stub | Rewrite with comprehensive expression tests |

**New test categories required (not in EASy68K-qt at all):**

| Test Suite | Estimated Tests | Description |
|------------|----------------|-------------|
| Golden Assembly Tests | ~100 | Byte-for-byte output comparison with original Edit68K |
| Golden Simulation Traces | ~50 | PC/register/memory trace comparison with original Sim68K |
| Trap #15 Mock Tests | ~80 | All 50+ task numbers with mock interface implementations |
| EffectiveAddr Tests | ~30 | Every addressing mode produces correct `EffectiveAddr` |
| Flag Computation Tests | ~60 | Every arithmetic/logic instruction, every size, N/Z/V/C/X |
| Exception Handler Tests | ~20 | Bus error, address error, privilege violation, trace, trap |
| Interrupt Tests | ~10 | IRQ levels, autovector, mask behavior |
| Assembler Directive Tests | ~50 | Every EASy68K directive |
| Macro Tests | ~20 | Nested macros, arguments, local labels |
| Structured Control Flow Tests | ~20 | IF/THEN/ELSE, WHILE, FOR, REPEAT, SWITCH |
| S-Record Edge Cases | ~20 | Malformed records, checksum errors, empty files |

**Transfer steps:**
1. Copy the CMake test infrastructure (`FetchContent` for GTest)
2. Adopt the `CpuTest` fixture pattern (create CPU, set up vectors, reset)
3. Adopt the `OpcodesTest` fixture pattern
4. Port `memory_test.cc` nearly verbatim (after Memory class changes)
5. Port `assembler_test.cc` basic tests as starting point
6. Write all new test categories from scratch
7. Set up `data/golden/` directory with reference files from original binaries

---

### 3.11 Build System — `CMakeLists.txt`

**Verdict:** DISCARD

**Reasons:**
- C++11 standard (we need C++17)
- Missing sanitizer flags
- Missing CI configuration
- Wrong directory structure (flat `src/core/` vs. proposal's `src/asm/`, `src/sim/`, `gui/`)
- No separate library targets (`libeasym68k-asm`, `libeasym68k-sim`)
- Missing Qt module dependencies (Multimedia, SerialPort, Network, PrintSupport)

**What can be referenced:**
- The `FetchContent` pattern for GoogleTest
- The `BUILD_TESTING` / `BUILD_GUI` option pattern

**Transfer steps:**
1. Write new top-level `CMakeLists.txt` per proposal Section 6.7
2. Set `CMAKE_CXX_STANDARD 17`
3. Define `libeasym68k-asm` and `libeasym68k-sim` as static library targets
4. Define GUI application targets with proper Qt 6 module dependencies
5. Add ASan/UBSan flags for debug builds
6. Add CI configuration files

---

## 4. Transfer Execution Order

The transfer must follow a specific order to avoid integration holes. Each step produces a compilable, testable state.

### Step 1: Project Skeleton (Day 1)

Create the new project structure from scratch:

```
easym68k/
├── CMakeLists.txt
├── include/easym68k/
│   ├── asm/
│   └── sim/
├── src/
│   ├── asm/
│   └── sim/
├── gui/
├── tests/
├── data/golden/
└── ci/
```

- Write `CMakeLists.txt` with C++17, library targets, GTest
- Create empty placeholder files for all headers and sources
- Verify the build system compiles (empty targets)

### Step 2: Types and Memory (Days 2–3)

Transfer: `types.h` → `include/easym68k/sim/types.h` (with modifications from Section 3.1)
Transfer: `memory.h`/`memory.cc` → `include/easym68k/sim/memory.h` + `src/sim/memory.cc` (with cleanup from Section 3.2)

- Add `EffectiveAddr` and `EaTarget`
- Rename `ExecResult` → `SimResult`
- Replace raw pointers with `std::vector`
- Port `memory_test.cc`
- **Verify:** All memory tests pass

### Step 3: S-Record Loader (Day 4)

Transfer: `srecord_loader.cc` → standalone `SRecordLoader` class (Section 3.7)

- Extract from Cpu, add checksum validation, improve error reporting
- Port `srecord_test.cc` and expand
- **Verify:** S-Record loader tests pass, including checksum validation

### Step 4: Addressing Modes (Days 5–6)

Write new `src/sim/addressing_mode.cc` from scratch (Section 3.3)

- Return `EffectiveAddr` from all mode calculations
- Reference existing `addressing_modes.cc` for mode/register switch structure
- Write comprehensive `addressing_mode_test.cc`
- **Verify:** All addressing mode tests pass with `EffectiveAddr` results

### Step 5: Simulator Core (Days 7–9)

Write new `M68kSimulator` class (Section 3.6)

- Reference existing `cpu.cc` for `Reset()`, `HandleException()`, `CheckCondition()`
- Inject Memory, Trap #15 interfaces, ILogger
- Implement `Step()`, `Run()`, `Reset()`
- Add `SIMHALT` instruction
- **Verify:** Basic simulator tests pass (reset, register access)

### Step 6: Instruction Decoder (Days 10–11)

Write new `src/sim/decode.cc` with hierarchical dispatch (Section 3.4)

- Reference existing instruction table for opcode→handler mappings
- Add SIMHALT dispatch
- **Verify:** NOP, illegal instruction, SIMHALT tests pass

### Step 7: Opcode Transfer — Move Instructions (Days 12–14)

Transfer `opcodes_move.cc` logic (Section 3.5)

- Replace sentinel EA with `EffectiveAddr` pattern
- Replace `WriteToEA` with `M68kSimulator` methods
- Complete flag computation for MOVE/MOVEA/MOVEQ
- Port and expand `opcodes_test.cc` move tests
- **Verify:** All MOVE/MOVEA/MOVEQ/MOVEM tests pass against golden data

### Step 8: Opcode Transfer — Arithmetic Instructions (Days 15–18)

Transfer `opcodes_arithmetic.cc` logic

- Same EA/WriteToEA replacement
- **Critical:** Complete V/C/X flag computation for ADD/SUB/MUL/DIV/NEG/CLR/CMP
- Port and expand arithmetic tests
- **Verify:** All arithmetic tests pass with correct flags

### Step 9: Opcode Transfer — Logic Instructions (Days 19–21)

Transfer `opcodes_logic.cc` logic

- V=0, C=0 for all logic ops; X unchanged
- ANDI/ORI/EORI to CCR/SR must update individual flag bits
- Port and expand logic tests
- **Verify:** All logic tests pass with correct flags

### Step 10: Opcode Transfer — Branch and Miscellaneous (Days 22–25)

Transfer `opcodes_branch.cc` and `opcodes_misc.cc` logic

- BRA/Bcc/BSR/DBcc/Scc displacement handling
- TRAP/CHK/TRAPV exception generation
- STOP/RESET/RTE/RTR/RTS
- LINK/UNLK/PEA/LEA
- Shift/Rotate/Bit operations
- **Verify:** All branch and misc tests pass

### Step 11: Assembler Transfer (Days 26–35)

Transfer assembler files (Section 3.8)

- Copy lexer, parser, symbol_table, expression_evaluator
- Fill in stubs: code_generator, directives, listing_generator, macro_processor
- Port from original `ASSEMBLE.CPP`, `DIRECTIV.CPP`, `MACRO.CPP`, `STRUCTURED.CPP`, `LISTING.CPP`, `MOVEM.CPP`, `BINFILE.CPP`
- Build golden assembly test suite
- **Verify:** Byte-for-byte output parity with original Edit68K on all golden files

### Step 12: Trap #15 Interfaces (Days 36–40)

Implement the 9 decomposed interfaces (proposal Section 6.4)

- Define all interface headers in `include/easym68k/sim/`
- Implement mock versions for headless testing
- Write Trap #15 dispatch in `M68kSimulator` (task number → interface method)
- Write mock-based tests for all 50+ task numbers
- **Verify:** All Trap #15 mock tests pass

### Step 13: GUI Implementation (Days 41–60)

Build GUI from scratch (Section 3.9)

- Reference existing GUI for menu structures and highlighter keywords only
- Implement all Qt Trap #15 backend classes
- Build Sim68K-Qt, Edit68K-Qt, EASyBIN-Qt
- **Verify:** Feature parity validation against original

---

## 5. Risk Mitigation During Transfer

| Risk | Mitigation |
|------|------------|
| Opcode logic appears correct but has subtle bugs vs. original | Golden simulation trace comparison (Step 11 of proposal) |
| Flag computation is wrong in transferred opcodes | Dedicated flag test suite with every instruction/size/operand combination |
| `EffectiveAddr` refactoring breaks opcode logic | Each opcode file transferred and tested independently |
| Assembler stubs are deeper than expected | Original Borland source is available for direct porting |
| Memory `std::vector` refactoring breaks S-Record loader | S-Record tests run immediately after memory transfer |
| Build system doesn't link properly | Verify empty skeleton compiles on Day 1 before any code transfer |

---

## 6. What Is NOT Transferred

The following items from `EASy68K-qt` are explicitly excluded from transfer:

1. **Sentinel EA address constants** (`kEaDataRegBase`, `kEaAddrRegBase`) — replaced by `EffectiveAddr`
2. **`WriteToEA()` free function** — replaced by `M68kSimulator` methods
3. **Incomplete `UpdateFlags()`** — replaced by per-operation flag computation
4. **`Cpu::trap_callback_`** (single `std::function<void(int)>`) — replaced by 9 decomposed interfaces
5. **`Cpu::output_callback_`/`input_callback_`** — replaced by `ITextIO`
6. **All GUI stub implementations** — replaced by new Qt GUI
7. **`doc/PORTING_PLAN.md`** — superseded by our porting proposal
8. **C++11 standard setting** — upgraded to C++17
9. **Linear instruction decoder** — replaced by hierarchical dispatch
10. **`Cpu` owning `Memory` by value** — replaced by dependency injection
