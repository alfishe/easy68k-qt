# L2-06 — Stack Frames with LINK and UNLK

**Track:** `core` · **Level:** 2 · **Difficulty:** ●● · **Requires:** L2-04, L2-05

## Goal

Allocate stack-frame local variables and access stack-passed arguments at fixed offsets from frame pointer `A6` using `LINK` and `UNLK`.

## New concepts

- `LINK A6,#-N` pushes old `A6`, copies `SP` to `A6`, and subtracts `N` from `SP` to allocate `N` bytes of local variables.
- Inside the function: `0(A6)` is saved `A6`, `4(A6)` is the return address, `8(A6)` is the first stack argument, and `-4(A6)` is the first 4-byte local variable.
- `UNLK A6` sets `SP = A6` (freeing locals) and pops saved `A6`.

## Predict

Before running, write down your answers:

1. When two longword arguments (`10` and `20`) are pushed before `BSR RECT_PERIM`, at what positive displacements from `A6` do they reside after `LINK A6,#-4`?
2. Why is `UNLK A6` safe even if `SP` was moved inside the function body?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Open `View > Stack` in Sim68K-Qt, select `A6` in the register dropdown, and step through `LINK A6,#-4`.
2. Observe the negative-offset local at `-4(A6)` and positive-offset arguments at `8(A6)` and `12(A6)`.

## Explain

Using a dedicated frame pointer (`A6`) keeps parameter offsets (`8(A6)`, `12(A6)`) and local variable offsets (`-4(A6)`) constant throughout the function, even when temporary values are pushed onto `SP`.

## Common mistakes

- Using positive displacement in `LINK A6,#-4` — local stack space grows downward toward lower addresses, so the displacement must be negative!

## Exercises

1. ●● Write a function using `LINK A6,#-4` and `UNLK A6` that takes one longword stack argument `x` at `8(A6)` and returns `x + 100` in `D0` for `x = 23` (`123`). — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L2-07 (`recursion`), C-03 (`calling-convention`).
