# L2-07 — Recursion: Factorial and Stack Growth

**Track:** `core` · **Level:** 2 · **Difficulty:** ●● · **Requires:** L2-06

## Goal

Implement a recursive `FACTORIAL` subroutine (`n! = n * (n-1)!`) and observe how each recursive call allocates its own stack frame.

## New concepts

- Every recursive call must have a **base case** (`n <= 1 -> return 1`) that returns without recursing.
- The caller's `n` must be saved on the stack (or in a callee-saved register pushed on the stack) across the recursive call.

## Predict

Before running, write down your answers:

1. How many stack frames are active simultaneously when `FACT(5)` reaches its base case `FACT(1)`?
2. What is `5!` (`5 * 4 * 3 * 2 * 1`) in decimal and hex?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Set a breakpoint at the base case `.BASE` in Sim68K-Qt, press `Run` (`F9`), and inspect `View > Stack` to see all 5 stacked frames.

## Explain

`FACT(5)` saves `5` on the stack and calls `FACT(4)`, which saves `4` and calls `FACT(3)`, down to `FACT(1) = 1`. As each call returns, it pops its saved `n` and multiplies `D0 = D0 * n`, producing `120` (`$78`).

## Common mistakes

- Omitting the base case causes infinite recursion until the stack collides with program code or unmapped memory.

## Exercises

1. ●● Compute the triangular sum `TRI(6) = 6 + 5 + 4 + 3 + 2 + 1 = 21` recursively. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L2-08 (`jump-tables`), O-04 (`loops`).
