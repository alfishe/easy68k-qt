## Phase 6: Opcode Implementation — Port from Original (EASy68K/Sim68K/CODE1-CODE9.CPP)

All opcode implementations in this phase are ported from the **original EASy68K Borland sources**. The instruction logic in `CODE1.CPP`–`CODE9.CPP` is the authoritative reference for behavioral parity. EASy68K-qt's opcode files may be consulted as a secondary reference for C++ code structure, but every handler must be verified against the original and must not inherit EASy68K-qt's sentinel EA patterns or incomplete flag computation.

### Task 6.1 — Move Instructions

Primary source: `EASy68K/Sim68K/CODE1.CPP` — original MOVE/MOVEA/MOVEQ/MOVEM/MOVEP/LEA/PEA/EXG/LINK/UNLK/SWAP implementations.
Secondary reference: `EASy68K-qt/src/core/simulator/opcodes_move.cc` — for C++ code structure only, not for logic correctness.

Port procedure (apply to ALL opcode files — porting from original, not copying from EASy68K-qt):

1. Read the original `CODE*.CPP` function for the instruction
2. Translate Borland C++ to modern C++17: replace `AnsiString` with `std::string`, remove `#include <vcl.h>`, replace global arrays (`D[]`, `A[]`, `memory[]`) with `CpuState` and `Memory` class methods
3. Replace `put()` / `value_of()` calls with `ReadEA()` / `WriteEA()` on `EffectiveAddr`
4. Replace `eff_addr()` calls with `CalculateEA()` returning `EffectiveAddr`
5. Replace `mem_put()` / `mem_req()` with `Memory::Write()` / `Memory::Read()`
6. Complete flag computation per M68000 Programmers Reference Manual (the original delegates to `UpdateFlags*` helpers)
7. Cross-reference EASy68K-qt's implementation if needed for C++ idioms, but do NOT trust its flag computation
8. Write tests
9. Run `clang-format`

**Instructions in this file:**
- `OpMove` (byte/word/long)
- `OpMovea` (word/long with sign extension)
- `OpMoveq` (8-bit immediate with sign extension)
- `OpMovem` (register list to/from memory)
- `OpMovep` (register ↔ memory byte-swapped)
- `OpMoveFromSr`, `OpMoveToCcr`, `OpMoveToSr`
- `OpMoveUsp`
- `OpLea`
- `OpPea`
- `OpExg`
- `OpLink`
- `OpUnlk`
- `OpSwap`

**Quality Gate 6.1:**
- `opcodes_move_test.cc` passes with at least 30 test cases
- MOVE.B/W/L to/from all addressing modes
- MOVEA.W sign-extends, MOVEA.L does not
- MOVEQ positive, negative, zero values with correct flags
- MOVEM with all register list patterns
- EXG all three variants
- LINK/UNLK frame pointer behavior
- All flag values verified per M68000 Programmers Reference Manual

---

### Task 6.2 — Arithmetic Instructions

Primary source: `EASy68K/Sim68K/CODE2.CPP` + `CODE3.CPP` — original ADD/ADDI/ADDQ/ADDX/SUB/SUBI/SUBQ/SUBX/MULU/MULS/DIVU/DIVS/NEG/NEGX/CLR/CMP/TST/EXT implementations.
Secondary reference: `EASy68K-qt/src/core/simulator/opcodes_arithmetic.cc` — for C++ code structure only.

Same port procedure as Task 6.1, plus:

**Critical:** Complete V/C/X flag computation. Every arithmetic handler must call the appropriate flag method:

- ADD/ADDI/ADDQ/ADDX → `UpdateFlagsAdd(src, dst, result, size)`
- SUB/SUBI/SUBQ/SUBX → `UpdateFlagsSub(src, dst, result, size)`
- MULU/MULS → V = overflow, C = 0, X unchanged
- DIVU/DIVS → V = overflow on divide-by-zero or quotient overflow, C = 0

**Instructions in this file:**
- `OpAdd`, `OpAdda`, `OpAddi`, `OpAddq`, `OpAddx`
- `OpSub`, `OpSuba`, `OpSubi`, `OpSubq`, `OpSubx`
- `OpMuls`, `OpMulu`, `OpDivs`, `OpDivu`
- `OpNeg`, `OpNegx`
- `OpClr`
- `OpCmp`, `OpCmpa`, `OpCmpi`, `OpCmpm`
- `OpTst`
- `OpExt`

**Quality Gate 6.2:**
- `opcodes_arithmetic_test.cc` passes with at least 50 test cases
- Every ADD/SUB variant tested with values that trigger: carry, overflow, zero, negative
- MULU overflow (result > 0xFFFF) sets V correctly
- DIVU divide-by-zero generates exception
- NEG of zero sets C but not X (this is a known subtle case)
- CLR always clears N/Z/V/C (X unchanged)

---

### Task 6.3 — Logic Instructions

Primary source: `EASy68K/Sim68K/CODE4.CPP` — original AND/ANDI/OR/ORI/EOR/EORI/NOT/TAS/BTST/BCHG/BCLR/BSET implementations.
Secondary reference: `EASy68K-qt/src/core/simulator/opcodes_logic.cc` — for C++ code structure only.

Same port procedure. All logic ops: V = 0, C = 0, X unchanged, N/Z from result.

**Instructions:**
- `OpAnd`, `OpAndi`, `OpAndiToCcr`, `OpAndiToSr`
- `OpOr`, `OpOri`, `OpOriToCcr`, `OpOriToSr`
- `OpEor`, `OpEori`, `OpEoriToCcr`, `OpEoriToSr`
- `OpNot`
- `OpTas` (move to misc or keep here)
- `OpBitOp` (BTST/BCHG/BCLR/BSET — static and dynamic)

**Quality Gate 6.3:**
- `opcodes_logic_test.cc` passes with at least 30 test cases
- ANDI/ORI/EORI to CCR: verify individual flag bit manipulation
- ANDI/ORI/EORI to SR: privileged instruction check
- BTST/BCHG/BCLR/BSET: both static (immediate) and dynamic (register) bit numbers
- NOT: verify flags (N/Z from result, V=0, C=0)

---

### Task 6.4 — Branch Instructions

Primary source: `EASy68K/Sim68K/CODE5.CPP` + `CODE6.CPP` — original BRA/BSR/Bcc/DBcc/Scc/JMP/JSR/RTS/RTE/RTR implementations.
Secondary reference: `EASy68K-qt/src/core/simulator/opcodes_branch.cc` — for C++ code structure only.

**Instructions:**
- `OpBra`, `OpBsr`
- `OpBcc` (all 14 conditional branches)
- `OpDbcc`
- `OpScc`
- `OpJmp`, `OpJsr`
- `OpRts`, `OpRte`, `OpRtr`

**Quality Gate 6.4:**
- `opcodes_branch_test.cc` passes with at least 25 test cases
- BRA short (byte displacement) and word displacement
- BSR pushes return address correctly
- Bcc taken/not-taken with all 14 conditions
- DBcc loop count decrement and branch/fallthrough
- Scc sets byte to $00 or $FF
- RTE restores SR and PC from stack

---

### Task 6.5 — Miscellaneous Instructions

Primary source: `EASy68K/Sim68K/CODE7.CPP` — original CHK/TRAP/TRAPV/ILLEGAL/NOP/RESET/STOP/LINE1010/LINE1111 implementations.
Secondary reference: `EASy68K-qt/src/core/simulator/opcodes_misc.cc` — for C++ code structure only.

**Instructions:**
- `OpChk`
- `OpTrap`
- `OpTrapv`
- `OpIllegal`
- `OpNop`
- `OpReset`
- `OpStop`
- `OpLine1010`
- `OpLine1111`
- `OpSimhalt` (new — opcode `0xFFFF`)

**Quality Gate 6.5:**
- `opcodes_misc_test.cc` passes with at least 15 test cases
- CHK exception when Dn < 0 or Dn > bound
- TRAP generates exception with correct vector
- TRAPV traps only when V flag is set
- STOP loads SR and enters stopped state
- SIMHALT returns `SimResult::kHalted`

---

### Task 6.6 — Shift/Rotate Instructions

Primary source: `EASy68K/Sim68K/CODE8.CPP` — original ASL/ASR/LSL/LSR/ROL/ROR/ROXL/ROXR implementations.
Secondary reference: `EASy68K-qt/src/core/simulator/opcodes_logic.cc` (shift section) — for C++ code structure only.

**Instructions:**
- `OpShiftRotate` — register and memory variants
- ASL/ASR (arithmetic shift left/right)
- LSL/LSR (logical shift left/right)
- ROL/ROR (rotate left/right)
- ROXL/ROXR (rotate through extend left/right)

**Quality Gate 6.6:**
- `opcodes_shift_test.cc` passes with at least 20 test cases
- Shift count from immediate (1–8) and register (0–63, modulo 64 for long, modulo 8 for byte/word)
- C flag = last bit shifted out
- V flag set for ASL if sign changes during shift
- Memory shifts always shift by 1
- ROXL/ROXR include X flag in rotation

---

### Task 6.7 — Flag Computation Verification Suite

New file: `tests/sim/flag_computation_test.cc`

Systematic verification of N/Z/V/C/X for every instruction category with specific operand values:

| Instruction | Size | Operands | Expected N | Z | V | C | X |
|-------------|------|----------|-----------|---|---|---|---|
| ADD.L | Long | $7FFFFFFF + $00000001 | 1 | 0 | 1 | 0 | 0 |
| ADD.L | Long | $80000000 + $80000000 | 1 | 0 | 0 | 1 | 1 |
| SUB.W | Word | $0000 - $0001 | 1 | 0 | 0 | 1 | 1 |
| SUB.W | Word | $8000 - $0001 | 0 | 0 | 1 | 0 | 0 |
| AND.B | Byte | $FF & $0F | 0 | 0 | 0 | 0 | unchanged |
| MOVE.W | Word | $FFFF | 1 | 0 | 0 | 0 | unchanged |
| ASL.W | Word | $4000, count=1 | 0 | 0 | 1 | 0 | unchanged |
| ASL.W | Word | $C000, count=1 | 1 | 0 | 1 | 0 | unchanged |
| ... (60+ total rows) | | | | | | | |

**Quality Gate 6.7:** All 60+ flag computation test cases pass. This is the single most important quality gate — incorrect flags will silently produce wrong program behavior.

---
