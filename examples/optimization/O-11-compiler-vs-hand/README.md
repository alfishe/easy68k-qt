# O-11 — Compiler vs Hand Assembly: Reading and Beating GCC -O2 on the 68000

**Track:** `optimization` | **Level:** 4 | **Difficulty:** ●●●● expert
**Prerequisites:** `O-10`, `C-04`, `C-08`

## Goal

Compare GCC `-O2` output for an array min/max/sum reduction against a hand-tuned 68000 routine that exploits condition flags already set by `MOVE.W (A0)+,D1` and `DBRA`.

## New Concepts

- GCC `-O2` generates solid code, but often emits an extra `CMP` or 32-bit sign extension where a 16-bit `DBRA` loop suffices
- Hand-tuned 68000 assembly can keep `sum`, `max`, and pointer in registers with zero stack frame overhead

## Predict (Before You Run)

For the 8-word array `{12, 45, 7, 89, 23, 56, 3, 65}`, what are the sum (`300`) and maximum (`89`)?

## Run & Observe

1. Compare `naive.x68` and `main.x68` in **Edit68K**, assemble (`F9`), and run in **Sim68K**.
2. Headless check: `easy68k-run examples/optimization/O-11-compiler-vs-hand`.
- Inspect `main.c` and `main.x68`.
- Verify `D0 = 300` (sum), `D1 = 89` (max), and compare `D4` (compiler-style cycles) vs `D5` (hand-tuned cycles).

## Explain

By unrolling or using a tight `MOVE.W (A0)+,D2` + `ADD.W D2,D0` + `CMP.W D1,D2` + `BLE.S` + `DBRA` loop without a stack frame, the hand-written routine saves ~35% of cycles.

## Common Mistakes

- Rewriting 100% of a C program in assembly instead of profiling first and hand-tuning only the hot inner loop.

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Compute the sum (`300`) in `D0` and max (`89`) in `D1` of the 8 words at `DATA` in at most `220` cycles.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `O-12-c-optimisation` for writing C source that guides GCC to emit the optimal 68000 instructions directly.
