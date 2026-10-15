# L2-04 — Subroutines and the Hardware Stack: BSR, JSR, RTS

**Track:** `core` · **Level:** 2 · **Difficulty:** ●● · **Requires:** L0-06, L2-02

## Goal

Write nested subroutines using `BSR`, `JSR`, and `RTS`, and watch return addresses get pushed and popped on the hardware stack (`A7`).

## New concepts

- `BSR label` / `JSR <ea>` decrements `SP` (`A7`) by 4, stores the 32-bit address of the next instruction at `(SP)`, and jumps to the target.
- `RTS` reads the 32-bit address at `(SP)` into `PC` and increments `SP` by 4.
- Nested calls stack their return addresses LIFO (Last-In, First-Out).

## Predict

Before running, write down your answers:

1. By how many bytes does `A7` decrease when `START` calls `SUM_OF_SQUARES` and `SUM_OF_SQUARES` calls `SQUARE`?
2. What value is in `A7` after `SUM_OF_SQUARES` returns to `START`?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Open `View > Stack` in Sim68K-Qt and press Trace Into (`F7`) through the nested `BSR` calls.
2. Watch two 4-byte return addresses get pushed onto the stack and popped off by `RTS`.

## Explain

Because return addresses live on the stack pointed to by `A7`, subroutines can call other subroutines (or themselves recursively) to arbitrary depth as long as stack space remains.

## Common mistakes

- Pushing temporary values onto `(A7)` inside a subroutine and forgetting to pop them before `RTS` causes `RTS` to jump to the temporary data instead of the return address!

## Exercises

1. ●● Write a `CUBE` subroutine that computes `D0 = D0^3` for `D0 = 4` (`64`). — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L2-05 (`parameters`), L2-06 (`stack-frames`), L2-07 (`recursion`).
