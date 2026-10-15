# L2-08 — Jump Tables and Switch Dispatch

**Track:** `core` · **Level:** 2 · **Difficulty:** ●● · **Requires:** L2-04

## Goal

Dispatch a multi-way `switch` in $O(1)$ time using a table of handler addresses and indexed indirect addressing (`MOVEA.L 0(A0,D0.W),A0` + `JMP (A0)`).

## New concepts

- Each entry in a 32-bit jump table (`DC.L CASE0, CASE1, ...`) is 4 bytes.
- Multiply the 0-based case index `D0` by 4 (`LSL.W #2,D0`) to convert it into a byte offset.
- Always bounds-check the index before indexing into a jump table!

## Predict

Before running, write down your answers:

1. Why must the case index in `D0` be shifted left by 2 (`LSL.W #2,D0`) before indexing a table of `DC.L` addresses?
2. What value is placed in `D1` when `D0 = 2` (selecting `CASE_MUL`) for `D1 = 7, D2 = 6`?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Step through `LSL.W #2,D0`, `MOVEA.L 0(A0,D0.W),A0`, and `JMP (A0)` in Sim68K-Qt.

## Explain

Instead of a chain of `CMP` + `BEQ` branches whose cost grows with the number of cases, a jump table dispatches any valid case in constant time.

## Common mistakes

- Forgetting `LSL.W #2,D0` reads an unaligned or mid-entry address and triggers an Address Error or wild jump!

## Exercises

1. ●● Change the selector `D0` to `1` (`CASE_SUB`) so `D1 = 20 - 8 = 12`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L3-01 (`addressing-modes`), L3-11 (`position-independence`).
