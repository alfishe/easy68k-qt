# O-12 — Writing C That Compiles Well on the 68000: 16-Bit Counters, Pointers, and restrict

**Track:** `optimization` | **Level:** 4 | **Difficulty:** ●●●● expert
**Prerequisites:** `O-11`, `C-05`

## Goal

Structure C source code (`int16_t` countdown loops, `*dst++ = *src++`, `restrict` pointers, and `uint16_t` arithmetic) so `m68k-gcc -O2` emits `DBRA`, `(An)+`, and `MULU.W` instead of `__mulsi3` library calls.

## New Concepts

- On the 68000, `int32_t * int32_t` requires a ~150-cycle software `__mulsi3` call, whereas `(int32_t)a_i16 * (int32_t)b_i16` compiles to a single `MULS.W` instruction!
- Counting down a 16-bit loop counter (`for (int16_t n = count; --n >= 0; )`) lets GCC emit `DBRA`
- `restrict` tells the compiler that output and input pointers do not alias

## Predict (Before You Run)

Why does multiplying two `int32_t` variables in C on the 68000 cost ~3× more cycles than multiplying two `int16_t` variables widened to `int32_t`?

## Run & Observe

1. Compare `naive.x68` and `main.x68` in **Edit68K**, assemble (`F9`), and run in **Sim68K**.
2. Headless check: `easy68k-run examples/optimization/O-12-c-optimisation`.
- Inspect `main.c` and `main.x68`.
- Compare `D4` (32-bit `__mulsi3` loop cycles) vs `D5` (16-bit `MULS.W` + `DBRA` cycles) for scaling 4 array elements (`{10, 20, 30, 40} * 3` -> sum `300`).

## Explain

The Motorola 68000 only has hardware `16×16 -> 32` `MULS.W`/`MULU.W` (unlike the 68020's `MULS.L`). Keeping inputs as `int16_t` lets the compiler emit hardware `MULS.W` directly.

## Common Mistakes

- Using `int` (32-bit) for small array indices and loop counters when `int16_t` fits and unlocks `DBRA` and `MULS.W`.

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Scale the 4 words `{10, 20, 30, 40}` by `3` and sum them (`300`) in `D0` using `(A0)+`, `MULS.W`, and `DBRA` in at most `220` cycles.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `P-06-cpp-game-framework` for combining C-level tuning with assembly hot paths.
