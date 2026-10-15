# C-12 — Templates and constexpr: Zero-Cost Compile-Time Abstractions Verified by Cycle Count

**Track:** `c` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `C-10`, `S-16`

## Goal

Use C++17 `constexpr` functions and templates to shift expensive 68000 computations (`factorial`, lookup tables, bit shifts) to compile time, and verify with the cycle counter that the runtime cost is a single `MOVE`.

## New Concepts

- `constexpr` functions evaluated on compile-time constants fold into immediate operands (`MOVE.L #720,D0`, 12 cycles) with zero runtime loop or `MULU` instructions
- `template <int Shift> inline int32_t scale_pow2(int32_t x)` emits a single `ASL.L #Shift,D0` instead of a library multiply call
- Verifying zero-cost abstraction claims using Trap #15 tasks 30/31

## Predict (Before You Run)

How many 68000 cycles does `constexpr_factorial(6)` (`720`) take at runtime compared to a loop calling `MULU` 6 times?

## Run & Observe

1. Compare `main.cpp` and the annotated compiler output `main.x68` in **Edit68K**, or load `main.S68` in **Sim68K**.
2. Headless check: `easy68k-run examples/c/C-12-cpp-templates-constexpr`.
- Inspect `main.x68`: `constexpr_factorial(6) + scale_pow2<3>(5)` folds to `720 + 40 = 760` (`D0 = 760`) in under 60 total cycles!

## Explain

On a 68000 where `MULU` costs up to 70 cycles and `DIVU` costs up to 140 cycles, `constexpr` table generation and template shift parameters eliminate hundreds of cycles per call.

## Common Mistakes

- Calling a `constexpr` function with a non-`constexpr` variable when you intended compile-time evaluation (use `constexpr auto kVal = ...;` to force compile-time evaluation).

## Exercises

1. **Exercise 1** (`exercises/ex1.cpp` / `exercises/ex1.x68` / `exercises/ex1.toml`): Evaluate `constexpr fib(10)` (`55`) + `scale_pow2<3>(5)` (`40`) = `95` in `D0` within `max_cycles = 60`.
   - Reference solution: `solutions/ex1.cpp` (`solutions/ex1.x68`).

## Going Further

See `O-07-lookup-tables` for trading ROM space for cycle speed.
