## Phase 8: Assembler — Port from Original (EASy68K/Edit68K/)

### Task 8.1 — Transfer Lexer

Primary source: `EASy68K/Edit68K/asm.h` — original token definitions and lexer structures.
Secondary reference: `EASy68K-qt/src/core/assembler/lexer.h` + `lexer.cc` — good tokenizer foundation, but missing EASy68K-specific tokens.

Steps:
1. Copy files
2. Change namespace to `easym68k::asm`
3. Change include guard
4. Add EASy68K-specific tokens: `ORG`, `EQU`, `SET`, `DC`, `DS`, `SECTION`, `INCLUDE`, `MACRO`, `MEND`, `IF`, `THEN`, `ELSE`, `ENDI`, `WHILE`, `ENDW`, `FOR`, `ENDF`, `REPEAT`, `UNTIL`, `SWITCH`, `ENDSW`, `COMMENT`, `LOAD`, `END`, `OPT`, `XREF`
5. Run `clang-format`

**Quality Gate 8.1:** `lexer_test.cc` passes with tokens for every EASy68K directive and instruction.

---

### Task 8.2 — Transfer Parser and Symbol Table

Primary source: `EASy68K/Edit68K/asm.h` + `ASSEMBLE.CPP` + `OPPARSE.CPP` — original parser and instruction operand parsing.
Secondary reference: `EASy68K-qt/src/core/assembler/parser.h` + `parser.cc` — instruction/directive parsing framework, needs EASy68K compatibility.

Steps:
1. Copy files, change namespace/guards
2. Add directive parsing for all EASy68K directives (currently stubs)
3. Add EASy68K instruction mnemonic recognition
4. Run `clang-format`

**Quality Gate 8.2:** `parser_test.cc` and `symbol_table_test.cc` pass.

---

### Task 8.3 — Transfer Expression Evaluator

Primary source: `EASy68K/Edit68K/EVAL.CPP` — the original expression evaluator with operator precedence.
Secondary reference: `EASy68K-qt/src/core/assembler/expression.h` + `expression_evaluator.cc` — reasonable base, but verify operator precedence matches original.

Steps:
1. Copy files, change namespace/guards
2. Verify operator precedence matches EASy68K original (`EVAL.CPP`)
3. Add EASy68K-specific operators if missing
4. Run `clang-format`

**Quality Gate 8.3:** `expression_test.cc` passes with comprehensive arithmetic, logical, and relational expressions.

---

### Task 8.4 — Transfer Assembler Core

Primary source: `EASy68K/Edit68K/asm.h` + `ASSEMBLE.CPP` — the original assembler engine with jump-table dispatch.
Secondary reference: `EASy68K-qt/src/core/assembler/assembler.h` + `assembler.cc` — framework is reasonable but many methods are stubs.

Steps:
1. Copy files, change namespace/guards
2. Wire up lexer, parser, symbol table, expression evaluator
3. Verify two-pass assembly structure
4. Run `clang-format`

**Quality Gate 8.4:** `assembler_test.cc` passes for NOP, ORG, labels, MOVEQ, DC.W (existing EASy68K-qt test cases).

---

### Task 8.5 — Implement Missing Assembler Components

The assembler infrastructure (lexer, parser, expression evaluator, symbol table, instruction table metadata) is complete. This task fills in the ~4,100 remaining lines across 9 original source files that make the assembler functionally complete. The original assembler is approximately 5,900 lines; approximately 1,800 lines (31%) have been ported. This task covers the remaining 69%.

**Current capability gap:** Only NOP and MOVEQ produce machine code. Every other instruction produces "Unknown instruction" errors. No macros, no conditional assembly, no INCLUDE, no S-record output, no listing generation. A program as simple as `MOVE.L #$1234,D0 / ADD.L D0,D1 / RTS` cannot be assembled.

**Source transition rule:** All code is ported from the **original Borland sources** (`EASy68K/Edit68K/`), not from EASy68K-qt stubs.

**Porting checklist for each subtask:**
1. Read the original `.CPP` file
2. Replace `AnsiString` with `std::string`
3. Replace `TColor`/`FontStyle` with `uint32_t` RGB
4. Remove `#include <vcl.h>`
5. Implement in the target file following the class structure
6. Write tests

---

#### Task 8.5.1 — Instruction Table Tests and Assembler Wiring

The instruction table (`instruction_table.cc`, 254 lines) already exists with 124 entries covering the full 68000 ISA. It is **not wired into the assembler** — `HandleInstruction()` only handles NOP/MOVEQ; `InstructionSize()` returns 2 for everything.

Source: `EASy68K/Edit68K/INSTTABL.CPP` (907 lines) + `INSTLOOK.CPP` (155 lines) — already ported as metadata only.

Steps:
1. Write `tests/asm/instruction_table_test.cc`:
   - **Lookup tests:** Verify `InstructionTable::Lookup()` returns the correct `InstrEntry` for all 124 instruction mnemonics (name, encoding type, condition code, size flags, operand count, mode masks, base opcode)
   - **Lookup negative tests:** Verify unknown mnemonics return `std::nullopt`
   - **ExtWordCount tests:** Verify extension word count for every addressing mode + size combination (13 modes × 3 sizes + immediate/long = ~42 cases)
   - **SizeField tests:** Verify B→0, W→1, L→2 mapping
   - **Mode mask tests:** Verify `kModeD`, `kModeA`, `kModeEA`, `kModeCtrl`, `kModeAlt`, `kModeData`, `kModeUSP` bit masks match original `asm.h` definitions
   - **Table completeness:** Verify every 68000 instruction from INSTTABL.CPP is present (no gaps)
2. Wire `InstructionTable` into `Assembler` class:
   - Add `InstructionTable instruction_table_;` member to `assembler.h`
   - Replace `InstructionSize()` hardcoded logic with table lookup: look up the mnemonic, compute size from base opcode words + ExtWordCount for each operand
   - Replace `HandleInstruction()` NOP/MOVEQ special-casing with table-driven dispatch: look up mnemonic → get `InstrEncoding` → dispatch to the appropriate code generator handler
3. Remove all `// TODO(Task 8.5)` comments in `assembler.cc`

**Quality Gate 8.5.1:** `instruction_table_test.cc` passes with 50+ test cases. `InstructionSize()` returns correct values for all instruction types (verified by existing assembler tests + new size-specific tests). `HandleInstruction()` no longer falls through to "Unknown instruction" for any valid 68000 opcode — it dispatches to the code generator (which may still be stub-level at this point, but the dispatch path must be correct).

---

#### Task 8.5.2 — Code Generator: EA Encoding and Extension Words

Port the foundational encoding functions from CODEGEN.CPP. These are the building blocks used by every instruction encoding handler.

Source: `EASy68K/Edit68K/CODEGEN.CPP` (212 lines) — `effAddr()`, `extWords()`, `output()`.

Steps:
1. Port `effAddr()` (CODEGEN.CPP:44-95) → `CodeGenerator::EffAddr()`:
   - Maps each `AddressMode` to a 6-bit effective address code: Dn=0x00|reg, An=0x08|reg, (An)=0x10|reg, (An)+=0x18|reg, -(An)=0x20|reg, d(An)=0x28|reg, d(An,Xi)=0x30|reg, Abs.W=0x38, Abs.L=0x39, d(PC)=0x3A, d(PC,Xi)=0x3B, #imm=0x3C
   - Returns `uint16_t` with mode and register fields packed into the low 6 bits
   - Error on invalid register numbers (An register > 7, etc.)
2. Port `extWords()` (CODEGEN.CPP:97-196) → `CodeGenerator::ExtWords()`:
   - Emit extension words for displacement, address, and immediate operands
   - Range checking with specific error codes: `INV_16_BIT_DATA`, `INV_8_BIT_DATA`, `INV_OFFSET`
   - Symbol reference handling: `backRef` flag determines whether value is resolved (pass 2) or placeholder (pass 1)
   - PC-relative displacement calculation: `disp = value - (loc + 2)`
   - Indexed addressing: extension word with scale/disp/reg fields
   - Immediate values: byte/word/long size-appropriate encoding
3. Port `output()` (CODEGEN.CPP:28-42) → `CodeGenerator::Output()`:
   - Writes a 16-bit instruction word to the output buffer
   - Advances location counter by 2
   - Triggers listing generator callback (if active)
4. Write `tests/asm/code_generator_test.cc`:
   - **EffAddr tests:** All 13 addressing modes produce correct 6-bit codes (13 test cases minimum)
   - **ExtWords displacement tests:** d(An) with positive/negative/out-of-range displacements
   - **ExtWords indexed tests:** d(An,Xi) with all register combinations, scale factors
   - **ExtWords absolute tests:** Abs.W vs Abs.L selection based on value range
   - **ExtWords immediate tests:** #imm for B/W/L sizes, range checking errors
   - **ExtWords PC-relative tests:** forward and backward references, displacement calculation
   - **Output tests:** Instruction word written correctly, location counter advanced

**Quality Gate 8.5.2:** `code_generator_test.cc` passes with 40+ test cases. EA encoding produces correct bit patterns for all 13 addressing modes. Extension words are emitted correctly for all mode+size combinations. Range checking produces appropriate error codes. PC-relative displacement calculation is correct.

---

#### Task 8.5.3 — Code Generator: Instruction Encoding Handlers

Port all instruction encoding handler functions from BUILD.CPP. This is the core of the assembler — the code that produces actual machine code.

Source: `EASy68K/Edit68K/BUILD.CPP` (889 lines) — 20+ handler functions.

Steps:
1. Port each encoding handler as a `CodeGenerator` method:

   | Handler | Instructions | Lines | Key Logic |
   |---------|-------------|-------|----------|
   | `zeroOp` | ILLEGAL, NOP, RESET, RTE, RTR, RTS, TRAPV | 103-111 | Emit mask word only |
   | `oneOp` | ASd/LSd/ROd/ROXd \u003cea\u003e, CLR, JMP, JSR, NBCD, NEG, NEGX, NOT, PEA, TAS, TST | 136-146 | Emit mask | EA(source) + ext words |
   | `arithReg` | ADD/ADDA/AND/CHK/CMP/CMPA/DIVS/DIVU/LEA/MULS/MULU/OR/SUB/SUBA \u003cea\u003e,Dn | 169-205 | Emit mask | EA(src) | (Dn\u003c\u003c9) + ext words; **peephole: ADDA/SUBA #1-8 → ADDQ/SUBQ** |
   | `arithAddr` | ADD/AND/BCHG/BCLR/BSET/BTST/EOR/OR/SUB Dn,\u003cea\u003e | 224-234 | Emit mask | EA(dest) | (Sn\u003c\u003c9) + ext words |
   | `immedInst` | ADDI/ANDI/CMPI/EORI/ORI/SUBI | 299-329 | Emit mask | EA(dest) + ext words for both; **peephole: ADDI/SUBI #1-8 → ADDQ/SUBQ** |
   | `quickMath` | ADDQ/SUBQ | 341-354 | Emit mask | (data&7)\u003c\u003c9 | EA(dest) + ext words; validate data 1-8 |
   | `shiftReg` | ASd/LSd/ROd/ROXd register | 746-763 | Emit mask | (count\u003c\u003c9) | Dn; validate shift count 1-8 |
   | `branch` | Bcc/BRA/BSR | 562-593 | Short: mask | (disp&0xFF); Word: mask + disp word; size hint and range logic |
   | `dbcc` | DBcc | 685-706 | Emit mask | Dn + disp word |
   | `scc` | Scc | 720-729 | Emit mask | EA(source) + ext words |
   | `move` | MOVE/MOVEA | 62-87 | Dest EA re-mapped to bits 11-9/5-3; **peephole: MOVE #imm,Dn → MOVEQ** |
   | `moveq` | MOVEQ | 603-614 | Emit mask | (Dn\u003c\u003c9) | (data&0xFF); validate -128..255 |
   | `immedToCCR` | ANDI/EORI/ORI to CCR | 627-642 | Emit mask + immediate byte word |
   | `immedWord` | ANDI/EORI/ORI to SR, RTD, STOP | 657-672 | Emit mask + immediate word |
   | `staticBit` | BCHG/BCLR/BSET/BTST #n,\u003cea\u003e | 469-483 | Emit mask | EA(dest) + data byte word + ext words |
   | `exg` | EXG | 773-792 | DnxDn/DnxAx/AxxAx mode bits; An register goes in bottom 3 bits for mixed-mode |
   | `twoReg` | ABCD/ADDX/CMPM/SBCD/SUBX | 807-816 | Emit mask | (Dn\u003c\u003c9) | Sn |
   | `oneReg` | EXT/SWAP/UNLK | 829-838 | Emit mask | Dn/An |
   | `moveUSP` | MOVE USP,An / MOVE An,USP | 850-862 | Emit mask | An register number |
   | `link` | LINK | 871-886 | Emit mask | An + disp word; validate -32768..32767 |
   | `trap` | TRAP | 537-548 | Emit mask | (vector&0xF); validate 0-15 |
   | `movep` | MOVEP | 364-395 | Dn↔d(An); 4 sub-forms (word/long × reg→mem/mem→reg); direction bit at bit 7, size bit at bit 6 |
   | `bitField` | BFCHG/BFCLR/BFEXTS/BFEXTU/BFFFO/BFINS/BFSET/BFTST | 253-284 | 68020 bit-field instructions (optional, guarded by `#if 0` or config flag) |

2. Wire all handlers into `HandleInstruction()` dispatch via the `InstrEncoding` enum from `instruction_table.h`:
   ```cpp
   switch (entry.encoding) {
     case InstrEncoding::kFixed:     return code_gen_.zeroOp(entry.base_opcode, ...);
     case InstrEncoding::kEaBidirect: return code_gen_.arithReg(entry.base_opcode, ...);
     case InstrEncoding::kMove:      return code_gen_.move(entry.base_opcode, ...);
     case InstrEncoding::kBranch:    return code_gen_.branch(entry.base_opcode, ...);
     // ... all 26 encoding types
   }
   ```
3. Implement peephole optimizations from BUILD.CPP:
   - **ADDI #1-8 → ADDQ** (BUILD.CPP:309-320, `immedInst()`): immediate value 1-8 and not forced long
   - **ADDA/SUBA #1-8 → ADDQ/SUBQ** (BUILD.CPP:178-197, `arithReg()`): same for address register arithmetic
   - **MOVE #imm,Dn → MOVEQ** (BUILD.CPP:68-74, `move()`): immediate fits 8 bits, dest is Dn, size is .L
4. Write `tests/asm/code_generator_test.cc` (extended):
   - **Per-handler tests:** At least 2 test cases per handler (positive + boundary)
   - **Peephole optimization tests:** Verify ADDI #3,D0 encodes as ADDQ; verify MOVE.L #5,D0 encodes as MOVEQ
   - **Branch displacement tests:** Short (±127), word (±32767), size hint (.B/.W/.L), BSR
   - **MOVEM placeholder tests:** Skip until Task 8.5.4
   - **End-to-end instruction tests:** Assemble `ADD.L D0,D1` and verify opcode bytes `D081`; assemble `MOVE.W (A0)+,D2` and verify `3018`; assemble `BRA label` and verify displacement

**Quality Gate 8.5.3:** `code_generator_test.cc` passes with 60+ test cases. Every 68000 instruction (except MOVEM and bit-field) produces correct machine code, including MOVEP with all 4 sub-forms. Peephole optimizations fire correctly. Branch displacement calculation handles short and word cases. `HandleInstruction()` dispatches all encoding types without "Unknown instruction" errors.

---

#### Task 8.5.4 — MOVEM Register List Parsing and Encoding

Port the MOVEM register list parser and encoder. This is a separate concern because the register list syntax is unique to MOVEM and requires its own parser.

Source: `EASy68K/Edit68K/MOVEM.CPP` (373 lines).

Steps:
1. Port register list parsing:
   - Syntax: `D0-D7/A0-A7` with ranges (`D0-D3`), individual registers (`D5`), and combinations (`D0-D3/D5/A0-A2`)
   - Register list symbol expansion: labels defined with REG directive
   - Register mask computation: 16-bit mask where bit N = register N (D0=bit0, D7=bit7, A0=bit8, A7=bit15)
   - Pre-decrement mode: register bit order is reversed (A7=bit0, D0=bit15) per M68000 manual
2. Port MOVEM instruction encoding:
   - Two forms: `MOVEM register_list,\u003cea\u003e` (register → memory) and `MOVEM \u003cea\u003e,register_list` (memory → register)
   - Size: .W (word) or .L (long)
   - Base opcodes: 0x4880 (register→memory .W), 0x48C0 (register→memory .L), 0x4C80 (memory→register .W), 0x4CC0 (memory→register .L)
3. Write `tests/asm/movem_test.cc`:
   - **Parsing tests:** Single register, range, combined, all-registers
   - **Mask computation tests:** D0-D7 = 0x00FF, A0-A7 = 0xFF00, D0/A0 = 0x0101, etc.
   - **Pre-decrement reversal tests:** D0-D7/A0 in -(An) mode = reversed mask
   - **Encoding tests:** MOVEM.W D0-D3,(A0) produces correct opcode + mask; MOVEM.L (A1)+,D0-D7/A0-A7 produces correct opcode + mask
   - **REG directive integration:** `REG D0-D3/A0-A2` defines a named register list symbol
4. Wire MOVEM handler into `HandleInstruction()` dispatch

**Quality Gate 8.5.4:** `movem_test.cc` passes with 25+ test cases. Register list parsing handles all valid syntax including ranges and register list symbols. Pre-decrement mode reverses register bit order. MOVEM.W and MOVEM.L produce correct opcodes and register masks.

---

#### Task 8.5.5 — Remaining Directives

Port the directives not yet implemented. Currently working: ORG, EQU, SET, DC.B/W/L, DS.B/W/L, EVEN, ODD, END. Missing from the original DIRECTIV.CPP:

Source: `EASy68K/Edit68K/DIRECTIV.CPP` (1043 lines).

Steps:
1. **DCB — Define Constant Block** (DIRECTIV.CPP:363-432):
   - Syntax: `DCB.size count,value`
   - Fill `count` copies of `value` (B/W/L size)
   - Word-align before DCB.W/L (same as DC alignment)
2. **DC string literals** (DIRECTIV.CPP:355-361):
   - Syntax: `DC.B 'Hello',0` or `DC.B "World"`
   - Single-quoted and double-quoted strings in DC.B
   - Each character becomes one byte; closing quote is not stored
3. **INCLUDE** (DIRECTIV.CPP:638-843):
   - Syntax: `INCLUDE 'filename'`
   - Recursive file inclusion: the included file is assembled as if its lines appeared inline
   - Nesting depth limit (original: no explicit limit, but practical limit via file handles)
   - Search path: current directory first, then include paths
4. **INCBIN** (DIRECTIV.CPP:845-870):
   - Syntax: `INCBIN 'filename'`
   - Raw binary file inclusion: file bytes are emitted directly into the output
5. **SECTION** (DIRECTIV.CPP:45-87):
   - Syntax: `SECTION name,flags`
   - Section support (CODE, DATA, BSS)
   - Affects listing and S-record output
6. **OFFSET** (DIRECTIV.CPP:89-107):
   - Syntax: `OFFSET expression`
   - Sets assembly mode where labels are offsets rather than absolute addresses
7. **ORG odd-address auto-alignment** (DIRECTIV.CPP:112-115):
   - If ORG target is odd, auto-increment to next even address with ODD_ADDRESS warning
   - Currently a TODO in `assembler.cc:109`
8. **OPT** (DIRECTIV.CPP:564-630):
   - Syntax: `OPT option[,option...]`
   - Options from original: `CRE` (cross-reference enable), `MEX`/`NOMEX` (macro expansion listing), `SEX`/`NOSEX` (structured expansion listing), `WAR`/`NOWAR` (warning messages), `CEX`/`NOCEX` (constant expansion in listing), `BIT` (bit-field instruction support)
9. **FAIL** (DIRECTIV.CPP:902-915):
   - Syntax: `FAIL message`
   - Forces an assembly error with a custom message
10. **LIST/NOLIST** (DIRECTIV.CPP:917-960):
    - Control listing output on/off
11. **PAGE** (DIRECTIV.CPP:962-975):
    - Page eject in listing output
12. **MEMORY** (DIRECTIV.CPP:899-991):
    - Define memory range attributes (ROM, READ, PROTECTED, INVALID) with start/end addresses
    - Memory map data is written as additional S0 records in the S-record output file (OBJECT.CPP:326-341)
13. **SIMHALT** (DIRECTIV.CPP:1002-1020):
    - Emit SIMHALT opcode (0xFFFF) as a simulator halt marker
14. **REG** (DIRECTIV.CPP:1022-1043):
    - Syntax: `label REG register_list`
    - Define a named register list symbol for use with MOVEM
15. **XDEF/XREF** — full implementation:
    - Currently no-ops; emit proper linker metadata in S-record output
16. Write `tests/asm/directives_test.cc`:
    - **DCB tests:** DCB.B 10,$FF; DCB.W 5,$1234; DCB.L 3,$DEADBEEF; zero count; negative count error
    - **DC string tests:** `DC.B 'Hello'`, `DC.B "World",0`, empty string, string with escape chars
    - **INCLUDE tests:** Simple include, nested include, include of non-existent file (error), include cycle detection
    - **INCBIN tests:** Binary file inclusion, non-existent file error
    - **SECTION tests:** CODE/DATA/BSS sections, section switching
    - **OFFSET tests:** Offset mode labels, return to absolute mode
    - **ORG odd-address tests:** `ORG $1001` produces ODD_ADDRESS warning and sets LC to $1002
    - **OPT tests:** Each option toggles correctly
    - **FAIL tests:** FAIL produces error with correct message
    - **LIST/NOLIST tests:** Listing output toggled on/off
    - **PAGE tests:** Page break emitted
    - **SIMHALT tests:** Emits 0xFFFF
    - **REG tests:** Named register list defined and usable with MOVEM
    - **XDEF/XREF tests:** Metadata recorded for S-record output

**Quality Gate 8.5.5:** `directives_test.cc` passes with 50+ test cases. All 16 directive types work correctly. INCLUDE handles nested includes and reports missing files. DCB fills correctly for all sizes. DC.B string handling matches original. ORG odd-address produces ODD_ADDRESS warning and auto-aligns.

---

#### Task 8.5.6 — Conditional Assembly

Port conditional assembly directives from the original ASSEMBLE.CPP. These allow portions of source code to be included or excluded based on conditions evaluated at assembly time.

Source: `EASy68K/Edit68K/ASSEMBLE.CPP` (lines 386-541).

Steps:
1. Port conditional assembly directives:
   - **IFC/IFNC** — string comparison: `IFC 'string1','string2'` / `IFNC 'string1','string2'`
   - **IFEQ/IFNE** — expression comparison: `IFEQ expression` (assemble if == 0) / `IFNE expression` (assemble if != 0)
   - **IFLT/IFLE/IFGT/IFGE** — relational: `IFLT expression` (assemble if < 0), etc.
   - **ELSE** — toggle assembly state within conditional block
   - **ENDC** — end conditional block
2. Implement nesting support:
   - IF within IF (nested conditionals)
   - Each nesting level has its own active/inactive state
   - When an outer level is inactive, inner levels are skipped entirely
3. Implement in `src/asm/directives.cc` with a conditional stack in the assembler state
4. Write `tests/asm/conditional_assembly_test.cc`:
   - **IFC tests:** Equal strings assemble, unequal strings skip; case-sensitive comparison
   - **IFNC tests:** Opposite of IFC
   - **IFEQ/IFNE tests:** Zero/non-zero expression values
   - **IFLT/IFLE/IFGT/IFGE tests:** Negative, zero, positive expression values
   - **ELSE tests:** ELSE toggles assembly state
   - **ENDC tests:** ENDC restores previous state
   - **Nesting tests:** IF within IF, ELSE within ELSE, 3+ levels deep
   - **Error tests:** ENDC without IF, IF without ENDC, ELSE without IF
   - **Integration tests:** Conditional assembly with EQU/SET symbols, forward references

**Quality Gate 8.5.6:** `conditional_assembly_test.cc` passes with 25+ test cases. All 8 conditional types work. Nesting works to arbitrary depth. ELSE toggles correctly. Mismatched delimiters produce appropriate errors.

---

#### Task 8.5.7 — Structured Control Flow

Port EASy68K-specific structured assembly constructs. These are educational extensions that generate conditional branch instructions at assembly time, allowing students to write high-level control flow without manually computing branch displacements.

Source: `EASy68K/Edit68K/STRUCTURED.CPP` (748 lines).

Steps:
1. Port each structured construct:
   - **WHILE expression / ENDW** — while loop: generate `IFxx` + `BRA` at top, `Bcc` at bottom
   - **FOR expr TO expr [STEP expr] / ENDF** — counted for loop: generate `CMPI` + `Bcc` + `BRA` at top, `ADDQ` + `BRA` at bottom
   - **REPEAT / UNTIL expression** — post-test loop: generate `Bcc` at bottom (branch back while condition false)
   - **IF expression / ELSE / ENDI** — structured if: generate `IFxx` + `BRA` skip, ELSE generates `BRA` around else block
   - **DBLOOP Dn,label / UNTIL expression** — DBcc loop: generate `DBcc` + `Bcc` for post-test
   - **UNLESS expression** — inverted if: assemble when expression is false
2. Each construct generates real 68000 instructions; the structured directives are syntactic sugar that the assembler expands
3. Implement in `src/asm/structured.cc` (new file) with a structured-control stack in the assembler state
4. Write `tests/asm/structured_test.cc`:
   - **WHILE tests:** Basic while loop, nested while, condition always false (zero iterations)
   - **FOR tests:** Basic for loop, step value, reverse step, nested for
   - **REPEAT tests:** Basic repeat-until, always-true condition (one iteration)
   - **IF/ELSE/ENDI tests:** If-only, if-else, nested if-else
   - **DBLOOP tests:** Basic DBLOOP, with UNTIL post-condition
   - **UNLESS tests:** Basic unless, unless-else
   - **Error tests:** ENDW without WHILE, ENDF without FOR, ENDI without IF, mismatched nesting
   - **Generated code tests:** Verify the generated branch instructions have correct displacements and opcodes

**Quality Gate 8.5.7:** `structured_test.cc` passes with 30+ test cases. All 6 structured constructs generate correct 68000 branch instructions. Nested structures work. Mismatched delimiters produce errors. Generated branch displacements are correct.

---

#### Task 8.5.8 — Macro Processor

Port the macro system that allows users to define and invoke named code templates with parameter substitution.

Source: `EASy68K/Edit68K/MACRO.CPP` (407 lines).

Steps:
1. Port macro definition and expansion:
   - **MACRO/ENDM** — define a macro: `name MACRO` ... `ENDM`
   - **Parameter substitution:** ``–`\9` positional parameters, `\A`–`\Z` named parameters
   - **Parameter substitution:** `\0` is reserved for the size extension letter (B/W/L) when called with a size suffix; `\1`–`\9` positional parameters; `\A`–`\Z` named parameters
   - **`\@` label numbering** — each macro expansion generates unique local labels by incrementing a counter
   - **NARG** — expands to the number of actual arguments passed to the current macro (MACRO.CPP:329-334)
   - **IFARG** — conditional expansion: `IFARG n` assembles following code only if parameter n was supplied
   - **MEXIT** — early exit from macro expansion (like `return` from a macro)
   - **Nested macro support** — macros can call other macros, depth limit MACRO_NEST_LIMIT=256 (matching original)
   - **Continuation lines** — lines starting with `&` are appended to the previous line
2. Store macro definitions in a macro table (separate from symbol table)
3. During assembly, when a line's opcode matches a macro name, expand the macro with the provided arguments
4. Up to 37 parameters per macro (matching original)
5. Implement in `src/asm/macro_processor.cc`
6. Write `tests/asm/macro_test.cc`:
   - **Definition tests:** Simple macro, macro with parameters, nested macro definition
   - **Expansion tests:** No-arg macro, 1-arg, multi-arg, default parameter handling
   - **Parameter substitution tests:** ``–`\9` correctly replaced, missing parameters produce empty string
   - **`\@` label tests:** Unique labels per expansion, multiple `\@` in same macro
   - **NARG tests:** NARG expands to correct argument count; NARG outside macro is error
   - **`\0` size tests:** Macro called with `.B`/`.W`/`.L` suffix passes correct size letter via `\0`
   - **IFARG tests:** IFARG with supplied/unsupplied parameters
   - **MEXIT tests:** Early exit skips remaining macro body
   - **Nested macro tests:** Macro calling another macro, recursive macro (with depth limit)
   - **Error tests:** Undefined macro call, ENDM without MACRO, MACRO without ENDM, parameter count mismatch

**Quality Gate 8.5.8:** `macro_test.cc` passes with 25+ test cases. Macro definition and expansion work. Parameter substitution is correct including `\0` size parameter. `\@` labels are unique per expansion. NARG returns correct argument count. IFARG and MEXIT work. Nested macros work to depth 256. Error cases are caught.

---

#### Task 8.5.9 — Error Reporter

Port the error reporting system with specific error codes and messages.

Source: `EASy68K/Edit68K/ERROR.CPP` (293 lines) + `asm.h` (error code constants).

Steps:
1. Port all error codes from the original `asm.h`:
   - 43+ specific error codes: `INV_16_BIT_DATA`, `INV_8_BIT_DATA`, `INV_MOVE_QUICK_CONST`, `INV_QUICK_CONST`, `INV_SHIFT_COUNT`, `INV_VECTOR_NUM`, `INV_BRANCH_DISP`, `INV_OFFSET`, `INV_EA`, `INV_SIZE_CODE`, `MISSING_SIZE_CODE`, `ILLEGAL_SIZE_CODE`, `INV_OP`, `INV_REGISTER`, `INV_LABEL`, `INV_SYMBOL`, `MULT_DEFD`, `UNDEF_SYMBOL`, `DIV_BY_ZERO`, etc.
   - Warning codes: `ODD_ADDRESS`, `INV_MEMORY`, etc.
2. Implement error formatting:
   - Line number, column, error code, descriptive message
   - Warning vs. error distinction (warnings don't halt assembly)
   - Error limit: stop assembly after N errors (original: configurable, default 10)
3. Replace generic error strings in `assembler.cc` with specific error codes
4. Implement in `src/asm/error_reporter.cc`
5. Write `tests/asm/error_reporter_test.cc`:
   - **Error code tests:** Each error code produces its specific message
   - **Formatting tests:** Error output contains line number, column, and context
   - **Warning tests:** Warnings are counted but don't halt assembly
   - **Error limit tests:** Assembly stops after N errors
   - **Integration tests:** Assemble invalid source and verify specific error codes (not generic "Unknown instruction")

**Quality Gate 8.5.9:** `error_reporter_test.cc` passes with 20+ test cases. All 43+ error codes have specific messages. Errors include line/column numbers. Warnings don't halt assembly. Error limit works.

---

#### Task 8.5.10 — Listing Generator

Port the listing output generator that produces `.L68` files.

Source: `EASy68K/Edit68K/LISTING.CPP` (297 lines).

Steps:
1. Port listing format generation:
   - `.L68` line format: `HHHHHH MMMM LL SSSSSSSSSSSSSS ;comment`
   - H = hex address (6 chars), M = machine code hex (4 chars per word, up to 5 words), L = line number (2 chars), S = source text
   - Continuation lines for multi-word instructions
   - Lines with no code (directives, comments) show address only
2. Port page formatting:
   - Page headers with filename, page number, date
   - Page length configurable (default 60 lines)
   - PAGE directive forces page break
3. Port LIST/NOLIST integration:
   - Lines between NOLIST and LIST are suppressed from listing output
   - Macro expansion lines can be listed or suppressed
4. Implement in `src/asm/listing_generator.cc`
5. Write `tests/asm/listing_test.cc`:
   - **Line format tests:** Address, machine code, line number, source text in correct columns
   - **Multi-word instruction tests:** Continuation lines for instructions with extension words
   - **Directive listing tests:** ORG, EQU, DS show address only; DC shows data bytes
   - **Page formatting tests:** Page break at correct line, header content
   - **LIST/NOLIST tests:** Lines suppressed between NOLIST/LIST
   - **Macro expansion tests:** Expanded lines shown with `+` prefix

**Quality Gate 8.5.10:** `listing_test.cc` passes with 20+ test cases. Listing output format matches original `.L68` format. Pagination works. LIST/NOLIST controls output. Macro expansion lines are marked.

---

#### Task 8.5.11 — Object/S-Record Output

Port the S-record output generator that produces `.S68`/`.S19` files.

Source: `EASy68K/Edit68K/OBJECT.CPP` (368 lines).

Steps:
1. Port S-record generation:
   - **S0 header record:** Module name, optional metadata
   - **S1 data record:** 16-bit address, up to 32 bytes payload, checksum
   - **S2 data record:** 24-bit address (for addresses > 64K)
   - **S3 data record:** 32-bit address (for addresses > 16M, rare)
   - **S8/S9 termination record:** Start address (entry point from END directive)
2. Port record construction:
   - Byte count field includes address + data + checksum
   - Checksum = one's complement of sum of all bytes in the record
   - Address type selection: S1 for addresses < $10000, S2 for < $1000000, S3 otherwise
   - Maximum 32 data bytes per record (configurable, original default)
3. Port XDEF/XREF metadata:
   - XDEF symbols emitted in S0 header or as special records
   - XREF symbols recorded for linker (informational in S-record context)
4. Port memory map S0 records (OBJECT.CPP:279-341):
   - MEMORY directive data (ROM, READ, PROTECTED, INVALID ranges) written as additional S0 records at end of file
   - `writeMap()` helper formats memory range attributes into S0 record payload
5. Implement in `src/asm/object_output.cc` (new file) or as part of `assembler.cc`
6. Write `tests/asm/object_output_test.cc`:
   - **S0 header tests:** Correct format, module name encoding
   - **S1 data tests:** 16-bit address, payload, checksum calculation
   - **S2 data tests:** 24-bit address for addresses > 64K
   - **S3 data tests:** 32-bit address for addresses > 16M
   - **S8/S9 termination tests:** Start address from END directive
   - **Checksum tests:** Verify one's complement sum for various payloads
   - **Record splitting tests:** Data > 32 bytes splits into multiple records
   - **Address type selection tests:** Correct S1/S2/S3 selection based on address range
   - **XDEF/XREF tests:** Symbol metadata included in output
   - **Memory map S0 tests:** MEMORY directive ranges produce correct S0 records with ROM/READ/PROTECTED/INVALID attributes
   - **Empty program tests:** S0 + S8/S9 with no data records
   - **Integration tests:** Assemble a program and verify complete S-record output matches expected

**Quality Gate 8.5.11:** `object_output_test.cc` passes with 25+ test cases. S-record output format is correct for all record types. Checksums are valid. Address type selection is correct. Record splitting handles large data blocks. XDEF/XREF metadata is included. Memory map S0 records are correct.

---

**Task 8.5 overall quality gate:** All 11 subtask quality gates pass. The assembler can assemble any valid 68000 program with any combination of instructions, directives, macros, and structured control flow. Output matches original EASy68K for golden test inputs.

**Subtask dependency order:**
1. **8.5.1** (instruction table wiring) — no dependencies
2. **8.5.2** (EA encoding + ext words) — depends on 8.5.1
3. **8.5.3** (instruction encoding handlers) — depends on 8.5.2
4. **8.5.4** (MOVEM) — depends on 8.5.3
5. **8.5.5** (remaining directives) — depends on 8.5.1 (can parallel with 8.5.2-8.5.4 for non-codegen directives like INCLUDE)
6. **8.5.6** (conditional assembly) — depends on 8.5.1
7. **8.5.7** (structured control flow) — depends on 8.5.3 (generates branch instructions)
8. **8.5.8** (macro processor) — depends on 8.5.5 (MACRO/ENDM are directive-like)
9. **8.5.9** (error reporter) — can parallel with 8.5.2-8.5.8
10. **8.5.10** (listing generator) — can parallel with 8.5.2-8.5.8
11. **8.5.11** (object output) — depends on 8.5.3 (needs assembled data)

**Estimated test count across all subtasks:** 370+ new tests

**Explicitly excluded from this port (not in original or commented out):**
- MOVES (68010) — commented out in original INSTTABL.CPP and BUILD.CPP
- MOVEC (68010) — commented out in original INSTTABL.CPP and BUILD.CPP
- BINFILE.CPP binary output — Teeside format; dead code in original (not in EDIT68K.bpr build, never called)
- Negated register lists (`/(-D0-D3)`) — not implemented in original MOVEM.CPP

---

### Task 8.6 — Golden Assembly Tests

New file: `tests/asm/golden_assembly_test.cc`

For each golden triplet in `data/golden/asm/`:
1. Read the `.X68` source file
2. Assemble it with `libeasym68k-asm`
3. Compare the output S-Record byte-for-byte against the golden `.S68`
4. Compare the listing output against the golden `.L68`

```cpp
TEST_P(GoldenAssemblyTest, ByteIdenticalOutput) {
  auto [x68_path, s68_path, l68_path] = GetParam();
  std::string source = ReadFile(x68_path);
  auto result = assembler.Assemble(source);
  ASSERT_TRUE(result.success);

  std::string golden_s68 = ReadFile(s68_path);
  EXPECT_EQ(assembler.GenerateSRecord(), golden_s68);

  std::string golden_l68 = ReadFile(l68_path);
  EXPECT_EQ(assembler.GetListing(), golden_l68);
}
```

**Quality Gate 8.6:** All 20+ golden assembly tests pass with byte-for-byte identical output. Any failure blocks all subsequent work.

---
