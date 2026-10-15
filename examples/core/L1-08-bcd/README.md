# L1-08 — Binary-Coded Decimal (BCD): ABCD, SBCD, NBCD

**Track:** `core` · **Level:** 1 · **Difficulty:** ●● · **Requires:** L1-04

## Goal

Perform packed two-digit decimal arithmetic per byte using `ABCD`, `SBCD`, and `NBCD`.

## New concepts

- In packed **BCD**, each nibble holds one decimal digit `0`–`9` (so byte `$47` represents decimal `47`).
- `ABCD Dy,Dx` adds two packed BCD bytes **plus the `X` flag** (`Dx.B = Dx.B + Dy.B + X` in base 10).
- Always clear `X` (`MOVE #0,CCR` or `ANDI #$EE,CCR`) before the first `ABCD`/`SBCD`/`NBCD`!

## Predict

Before running, write down your answers:

1. In binary hex, `$28 + $35 = $5D`. What does `ABCD D1,D0` produce when `D0.B = $28`, `D1.B = $35`, and `X = 0`?
2. What happens when `$58 + $47` exceeds `99` in a single BCD byte?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Step through `ABCD D1,D0` in Sim68K-Qt: `D0.B` becomes `$63` (decimal 28 + 35 = 63).
2. Step through the second `ABCD`: `$58 + $47 = $05` with `X=1` and `C=1` (representing 105)!

## Explain

`ABCD` adjusts any nibble sum above 9 by adding 6 and propagating a decimal carry into the upper nibble (and into `X`/`C` when the upper nibble exceeds 9). Multi-byte BCD numbers chain `ABCD -(A0),-(A1)` across bytes just like `ADDX`.

## Common mistakes

- Forgetting to clear `X` before starting a BCD addition or subtraction adds an unwanted `+1` from whatever instruction last set `X`!

## Exercises

1. ●● Compute the ten's-complement negation of BCD `$37` in `D0.B` (`100 - 37 = 63` -> `$63`) using `MOVE #0,CCR` and `NBCD D0`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- P-01 (`calculator`).
