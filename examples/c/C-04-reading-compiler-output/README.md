# C-04 — Reading Compiler Output: -O0 vs -O2 Code Generation on the 68000

**Track:** `c` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `C-03`, `S-16`

## Goal

Compare how GCC compiles the same C loop at `-O0` (stack locals and redundant loads/stores) versus `-O2` (register allocation, `-fomit-frame-pointer`, and `DBRA`).

## New Concepts

- `-O0` spills every local variable (`i`, `sum`) to the stack frame (`-4(A6)`, `-8(A6)`) on every iteration
- `-O2` keeps `i` and `sum` in registers (`D0`, `D1`), uses `MULS.W`, and lowers countdown loops to `DBRA`
- Measuring the cycle difference with Trap #15 tasks 30/31

## Predict (Before You Run)

For `sum_of_squares(6)` (`1^2 + 2^2 + ... + 6^2 = 91`), which version (`sum_sq_O0` vs `sum_sq_O2`) uses fewer cycles, and do both return `91`?

## Run & Observe

1. Compare `main.c` and the annotated compiler output `main.x68` in **Edit68K**, or load `main.S68` in **Sim68K**.
2. Headless check: `easy68k-run examples/c/C-04-reading-compiler-output`.
- Inspect `D0 = 91` (result), `D2` (`-O0` cycles), and `D3` (`-O2` cycles, less than half of `D2`).

## Explain

At `-O0`, every read or write of a C local variable touches memory relative to `A6`. At `-O2`, variables live in data registers (`4` cycles per arithmetic op instead of `12–20` cycles).

## Common Mistakes

- Benchmarking C code compiled at `-O0` and concluding that C is inherently slow on the 68000.

## Exercises

1. **Exercise 1** (`exercises/ex1.c` / `exercises/ex1.x68` / `exercises/ex1.toml`): Compute the dot product of `{1, 2, 3, 4}` and `{5, 6, 7, 8}` (`5 + 12 + 21 + 32 = 70`) using register-allocated `-O2`-style assembly (`(A0)+`, `(A1)+`, `MULS`, `DBRA`) and return `70` in `D0`.
   - Reference solution: `solutions/ex1.c` (`solutions/ex1.x68`).

## Going Further

See `O-11-compiler-vs-hand` in Track O.
