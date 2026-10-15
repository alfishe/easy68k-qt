# O-10 — Register Allocation: Keeping Hot Variables in D0–D7/A0–A6 and Minimising Spills

**Track:** `optimization` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `O-03`, `L2-05`

## Goal

Assign the 68000's 8 data registers (`D0–D7`) and 7 address registers (`A0–A6`) so inner loops never spill temporaries to memory or the stack, and save only the callee-saved registers actually touched.

## New Concepts

- A register-to-register `ADD.L Dn,Dm` takes 8 cycles; spilling to `-4(A6)` takes 16–20 cycles per access
- `MOVEM.L D2-D7/A2-A6,-(A7)` + restore costs `188` cycles for 11 registers — never blindly save all registers in a small subroutine that only needs `D0–D1`!

## Predict (Before You Run)

In a leaf subroutine that only uses scratch registers `D0, D1, A0`, how many cycles are wasted by an unnecessary `LINK A6,#0` + `MOVEM.L D2-D7/A2-A5,-(A7)` prologue/epilogue?

## Run & Observe

1. Compare `naive.x68` and `main.x68` in **Edit68K**, assemble (`F9`), and run in **Sim68K**.
2. Headless check: `easy68k-run examples/optimization/O-10-register-allocation`.
- Inspect `D0 = 55` (Fibonacci(10)), `D4` (stack-spilling + full `MOVEM` cycles = 656), and `D5` (register-allocated leaf cycles = 172).

## Explain

Keeping `prev` (`D0`), `curr` (`D1`), and the loop counter (`D2`) in registers and saving only `D2` cuts total cycle count by nearly 4×.

## Common Mistakes

- Using a `SAVE_ALL` (`MOVEM.L D0-D7/A0-A6,-(A7)`) macro inside a tight inner loop.

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Compute iterative `Fib(10) = 55` in `D0` keeping all variables in registers (`D0, D1, D2`) in at most `180` cycles.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `O-11-compiler-vs-hand` to compare compiler register allocation against hand assembly.
