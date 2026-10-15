# C-03 — m68k C Calling Convention: Stack Arguments, D0 Return, and Callee-Saved Registers

**Track:** `c` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `C-01`, `L2-05`, `L2-06`

## Goal

Master the standard m68k C ABI: stack arguments at `8(A6), 12(A6), ...`, integer return in `D0`, caller-saved `D0–D1/A0–A1`, and callee-saved `D2–D7/A2–A6`.

## New Concepts

- Arguments are pushed right-to-left as 32-bit longwords; the caller cleans up with `LEA k(A7),A7`
- With a frame pointer (`LINK A6,#-locals`), `0(A6)` is saved `A6`, `4(A6)` is return `PC`, `8(A6)` is arg 1, `12(A6)` is arg 2, `16(A6)` is arg 3
- Scratch (caller-saved): `D0, D1, A0, A1`
- Preserved (callee-saved): `D2–D7, A2–A6`

## Predict (Before You Run)

What does `weighted_sum(10, 20, 30)` (`a*2 + b*3 + c*4`) return in `D0`, and why must `weighted_sum` save `D2` if it uses `D2` as a temporary?

## Run & Observe

1. Compare `main.c` and the annotated compiler output `main.x68` in **Edit68K**, or load `main.S68` in **Sim68K**.
2. Headless check: `easy68k-run examples/c/C-03-calling-convention`.
- Step into `weighted_sum` and inspect `8(A6) = 10`, `12(A6) = 20`, `16(A6) = 30` in the Stack window.
- Confirm `D2` (`$11223344`) is preserved across the call while `D0 = 200`.

## Explain

Because `D2–D7` and `A2–A6` belong to the caller under the m68k ABI, any function that modifies `D2` must save it on entry and restore it before `RTS`.

## Common Mistakes

- Reading the first C parameter at `4(A6)` instead of `8(A6)` when `LINK A6,#...` is used.

## Exercises

1. **Exercise 1** (`exercises/ex1.c` / `exercises/ex1.x68` / `exercises/ex1.toml`): Implement `int32_t clamp_val(int32_t x, int32_t lo, int32_t hi)` following the m68k C ABI (`8(A6)`, `12(A6)`, `16(A6)`) and evaluate `clamp_val(150, 0, 100)` (`100`) in `D0`.
   - Reference solution: `solutions/ex1.c` (`solutions/ex1.x68`).

## Going Further

See `C-08-c-calls-asm` for calling hand-written assembly from C.
