# L0-06 — Stepping, Tracing, Breakpoints, and Rewind

**Track:** `core` · **Level:** 0 · **Difficulty:** ● · **Requires:** L0-05

## Goal

Use Trace Into (`F7`), Step Over (`F8`), Run (`F9`), PC breakpoints, and Rewind (`Ctrl+F2`) in Sim68K-Qt.

## New concepts

- **Trace Into (`F7`)** executes a single instruction, entering `BSR`/`JSR` subroutines.
- **Step Over (`F8`)** executes a `BSR`/`JSR` subroutine call as a single step.
- **PC Breakpoints** (click the green dot in the source gutter) pause `Run` (`F9`) when `PC` reaches that line.

## Predict

Before running, write down your answers:

1. If `PC` is on `BSR ADD_TEN`, what is the difference between pressing `F7` (Trace Into) vs `F8` (Step Over)?
2. What value is in `D0` after `ADD_TEN` is called twice starting from `D0 = 5`?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Open `main.x68` in Sim68K-Qt, press `F7` on the first `BSR ADD_TEN` to step inside the subroutine, and press `F8` on the second `BSR ADD_TEN` to step over it.
2. Click a green gutter dot to set a red PC breakpoint, press `Rewind`, and press `Run` (`F9`).

## Explain

`BSR ADD_TEN` pushes the address of the next instruction onto the stack (`A7`) and jumps to `ADD_TEN`. `RTS` pops that return address back into `PC`. Step Over (`F8`) automatically sets an internal target at the instruction after `BSR` so the subroutine runs at full speed and pauses on return.

## Common mistakes

- Jumping to a subroutine with `BRA` instead of `BSR` leaves no return address on the stack for `RTS`.

## Exercises

1. ● Call `ADD_TEN` a third time so `D0` and `D1` end up equal to `35`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L2-04 (`subroutines`), S-16 (`logging-debugging`).
