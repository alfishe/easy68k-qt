# C-08 — Mixing C and Assembly: Calling Hand-Written 68000 Routines from C and Back

**Track:** `c` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `C-03`, `L1-06`

## Goal

Write external 68000 assembly routines callable from C (`extern uint32_t asm_popcount32(uint32_t x);`) and call a C function back from assembly.

## New Concepts

- Leaf assembly functions can read their arguments directly off `4(A7), 8(A7), ...` without needing `LINK`/`UNLK`
- Only `D0, D1, A0, A1` may be modified freely; if `D2–D7` or `A2–A6` are used, save/restore them with `MOVEM.L`
- Assembly code calls a C function by pushing arguments right-to-left, executing `JSR`/`BSR`, and cleaning the stack

## Predict (Before You Run)

How many bits are set in `0xF0F055AA`, and what does `asm_popcount32(0xF0F055AA) + asm_call_c_double(21)` return in `D0`?

## Run & Observe

1. Compare `main.c` and the annotated compiler output `main.x68` in **Edit68K**, or load `main.S68` in **Sim68K**.
2. Headless check: `easy68k-run examples/c/C-08-c-calls-asm`.
- Step through `asm_popcount32` (`16` set bits) and `asm_call_c_double` (calls C `c_double(21)` -> `42`).
- Verify `D0 = 58` (`16 + 42`).

## Explain

Because C and assembly share the same stack and register ABI, calling between them is seamless as long as stack offsets and callee-saved register rules are respected.

## Common Mistakes

- Modifying `D2` in an assembly routine called from C without saving it, silently corrupting a variable in the C caller.

## Exercises

1. **Exercise 1** (`exercises/ex1.c` / `exercises/ex1.x68` / `exercises/ex1.toml`): Implement `int32_t asm_abs32(int32_t x)` in assembly (`4(A7)` -> `D0`) and return `asm_abs32(-1234)` (`1234`) in `D0`.
   - Reference solution: `solutions/ex1.c` (`solutions/ex1.x68`).

## Going Further

See `P-06-cpp-game-framework` for C++ classes with hand-tuned 68000 assembly hot paths.
