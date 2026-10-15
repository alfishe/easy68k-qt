# O-05 — Block Memory Copy and Fill: MOVEM.L vs Byte/Long Loops

**Track:** `optimization` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `O-04`, `L2-05`

## Goal

Use `MOVEM.L (A0)+,D0-D7` and `MOVEM.L D0-D7,(A1)` to copy or clear 32 bytes (8 longwords) in a single instruction pair, achieving maximum 68000 bus bandwidth.

## New Concepts

- Byte-by-byte copy (`MOVE.B (A0)+,(A1)+` + `DBRA`) costs `(12 + 10) * 32 = 704` cycles for 32 bytes
- `MOVEM.L (A0)+,D0-D7` (12 + 8*8 = 76 cycles) + `MOVEM.L D0-D7,(A1)` (8 + 8*8 = 72 cycles) copies 32 bytes in `148` cycles — nearly 5× faster!

## Predict (Before You Run)

How many cycles does copying 32 bytes take with a byte loop (`MOVE.B (A0)+,(A1)+`) versus a single `MOVEM.L (A0)+,D0-D7` + `MOVEM.L D0-D7,(A1)`?

## Run & Observe

1. Compare `naive.x68` and `main.x68` in **Edit68K**, assemble (`F9`), and run in **Sim68K**.
2. Headless check: `easy68k-run examples/optimization/O-05-block-memory`.
- Inspect `$2000..$201F` (8 longwords copied).
- Compare `D6` (byte-loop cycles = 740) vs `D7` (`MOVEM.L` cycles = 180).

## Explain

`MOVEM.L` fetches only two instruction words total (`MOVEM` load + `MOVEM` store) to transfer 32 bytes (16 bus words), eliminating 31 loop branches and 62 instruction fetches.

## Common Mistakes

- Remember that the 68000 does not allow `(An)+` as the destination of `MOVEM.L regs,(An)+`; use `MOVEM.L regs,(A1)` followed by `LEA 32(A1),A1` (or pre-decrement `-(A1)` for backward fills).

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Copy the 32 bytes (8 longwords) from `BLK` to `$2000` using `MOVEM.L (A0)+,D0-D7` and `MOVEM.L D0-D7,(A1)` in at most `190` cycles.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `O-13-graphics-fast-paths` for fast screen clears.
