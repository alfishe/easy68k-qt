# O-06 — Multiply- and Divide-Free Arithmetic: Shifts, Adds, and Strength Reduction

**Track:** `optimization` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `O-02`, `L1-05`, `L1-07`

## Goal

Replace expensive `MULU` (~40–70 cycles) and `DIVU` (~140 cycles) instructions with constant shift-and-add chains (`x * 10 = (x << 3) + (x << 1)`, `x * 320 = (x << 8) + (x << 6)`).

## New Concepts

- `MULU #10,D0` takes ~42 cycles; `ADD.L D0,D0` (x2) + `MOVE.L D0,D1` + `LSL.L #2,D0` (x8) + `ADD.L D1,D0` takes 26 cycles and works on full 32-bit values!
- Screen row offset `y * 320` = `(y << 8) + (y << 6)`
- Unsigned division/modulo by powers of two (`x / 16` = `LSR.L #4`, `x % 16` = `ANDI.L #15`)

## Predict (Before You Run)

For `y = 100`, what is `y * 320 + (y / 8)` (`32000 + 12 = 32012`), and how many cycles does replacing `MULU #320` + `DIVU #8` with shifts and adds save?

## Run & Observe

1. Compare `naive.x68` and `main.x68` in **Edit68K**, assemble (`F9`), and run in **Sim68K**.
2. Headless check: `easy68k-run examples/optimization/O-06-multiply-divide-free`.
- Inspect `D0 = 32012`, `D4` (`MULU`+`DIVU` cycles = 200), and `D5` (shift-and-add cycles = 52 — nearly 4× faster!).

## Explain

The 68000 has no barrel shifter (`LSL #n` takes `6 + 2n` cycles), yet decomposing constants with 2 or 3 set bits (`10 = 8 + 2`, `320 = 256 + 64`) is still far faster than microcoded `MULU`/`DIVU`.

## Common Mistakes

- Using `LSR` instead of `ASR` when dividing signed negative numbers.

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Compute `x * 10` for `x = 123` (`1230`) in `D0` without `MULU`/`MULS` in at most `48` cycles.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `O-08-fixed-point` for fractional scaling without floating point or division.
