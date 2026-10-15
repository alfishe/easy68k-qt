# O-02 — Instruction Selection: MOVEQ, ADDQ/SUBQ, SUB.L Dn,Dn, and LEA Arithmetic

**Track:** `optimization` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `O-01`, `L3-01`

## Goal

Replace slow 6-byte immediate instructions (`MOVE.L #imm,Dn`, `ADDI.L #imm,Dn`, `CLR.L Dn`, `ADDA.L #imm,An`) with single-word 68000 quick instructions (`MOVEQ`, `ADDQ`, `SUBQ`, `LEA d(An),An`).

## New Concepts

- `MOVEQ #imm8,Dn` (4 cycles, 2 bytes) vs `MOVE.L #imm32,Dn` (12 cycles, 6 bytes)
- `ADDQ.L #1..8,Dn` (8 cycles, 2 bytes) vs `ADDI.L #imm,Dn` (16 cycles, 6 bytes)
- `MOVEQ #0,Dn` or `SUB.L Dn,Dn` (4/6 cycles) vs `CLR.L Dn` (6 cycles)
- `LEA d16(An),An` (8 cycles) for adding a 16-bit signed offset to an address register

## Predict (Before You Run)

How many cycles does `MOVEQ #0,D0` + `MOVEQ #7,D1` + `ADDQ.L #5,D1` + `SUBQ.L #2,D1` take compared to using `CLR.L`, `MOVE.L #7`, `ADDI.L #5`, and `SUBI.L #2`?

## Run & Observe

1. Compare `naive.x68` and `main.x68` in **Edit68K**, assemble (`F9`), and run in **Sim68K**.
2. Headless check: `easy68k-run examples/optimization/O-02-instruction-selection`.
- Inspect `D0 = 10`, `D2 = 10` (both compute the same value), `D4 = 58` (naive cycles), and `D5 = 24` (optimised cycles — more than 2× faster!).

## Explain

On the 68000, every 16-bit extension word fetch costs 4 bus cycles. Single-word quick instructions (`MOVEQ`, `ADDQ`, `SUBQ`) encode small constants inside the 16-bit opcode word, saving 8 cycles and 4 bytes per instruction.

## Common Mistakes

- Forgetting that `MOVEQ` sign-extends its 8-bit immediate (`-128..+127`) across all 32 bits of `Dn`.

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Optimise the sequence in `exercises/ex1.x68` (`D0 = 100`, `D1 = 7`, `D0 += 8`, `D1 -= 3`, `D0 += D1` -> `112`) using `MOVEQ`, `ADDQ.L`, and `SUBQ.L` to run in at most `36` cycles.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `O-03-addressing-costs` for effective-address cycle costs.
