# O-13 — Graphics Fast Paths: Span Drawing vs Pixel Loops and Frame Budgets

**Track:** `optimization` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `O-04`, `S-03`, `S-04`

## Goal

Replace per-pixel `DrawPixel` (task 82) nested loops with span/primitive calls (`DrawLine` task 84 / `DrawRectangle` task 87) and double buffering (tasks 92/94) to stay within a 60 Hz frame cycle budget.

## New Concepts

- Drawing a `16×4` bar pixel-by-pixel (`64` `TRAP #15` task 82 calls) executes 64 trap dispatches (~3,500 cycles)
- Drawing the same `16×4` bar with a single `DrawRectangle` (task 87) executes **one** `TRAP #15` call (~60 cycles!)
- Keeping total per-frame cycles well below the 60 Hz budget

## Predict (Before You Run)

How many `TRAP #15` calls does filling a `16×4` rectangle take with `DrawPixel` (task 82) versus `DrawRectangle` (task 87)?

## Run & Observe

1. Compare `naive.x68` and `main.x68` in **Edit68K**, assemble (`F9`), and run in **Sim68K**.
2. Headless check: `easy68k-run examples/optimization/O-13-graphics-fast-paths`.
- Compare `D6` (64-pixel loop cycles = 3,360) vs `D7` (single `DrawRectangle` task 87 cycles = 48 — 70× fewer 68000 cycles!).

## Explain

Each `TRAP #15` instruction plus register setup costs ~50–60 CPU cycles. Batching pixel spans into `DrawLine` (task 84) or `DrawRectangle` (task 87) moves the inner rasterisation loop into native host code.

## Common Mistakes

- Clearing the screen before a frame by plotting `640×480` black pixels in a 68000 loop instead of a single filled rectangle or task 11 `$FF00`.

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Draw a filled rectangle from `(10, 10)` to `(25, 13)` using task 87 (instead of a pixel loop) and present the frame with task 94 in at most `120` cycles, leaving `D0 = 94`.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `P-02-snake-pong` and `P-07-demo-effect`.
