# L2-01 — Compare and Branch: CMP, TST, and Bcc Conditions

**Track:** `core` · **Level:** 2 · **Difficulty:** ●● · **Requires:** L1-03

## Goal

Write conditional branches using `CMP`, `TST`, and both signed (`BGT`/`BGE`/`BLT`/`BLE`) and unsigned (`BHI`/`BHS`/`BLO`/`BLS`) `Bcc` instructions.

## New concepts

- `CMP <src>,<dst>` computes `<dst> - <src>`; `BGT` branches when `<dst> > <src>` (signed) and `BHI` branches when `<dst> > <src>` (unsigned).
- `TST <ea>` compares `<ea>` against 0, setting `N` and `Z` and clearing `V` and `C`.
- `BRA` branches unconditionally; `.S` forces an 8-bit displacement when the target is within -128..+127 bytes.

## Predict

Before running, write down your answers:

1. After `CMP.L D0,D1`, does `BGT` branch when `D0 > D1` or when `D1 > D0`?
2. What value is placed in `D2` (`max(-10, 25)` signed) and `D3` (`max($FFFF0000, $00000010)` unsigned)?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Step through `main.x68` in Sim68K-Qt and watch `PC` jump or fall through each `BGE.S` and `BHS.S` branch.

## Explain

In Motorola syntax `CMP src,dst` subtracts the first operand (`src`) from the second (`dst`), so `CMP.L D0,D1` followed by `BGE` branches when `D1 >= D0` (second operand >= first operand).

## Common mistakes

- Reversing operand order on `CMP A,B`: always read `CMP A,B` + `Bcc` as 'branch if `B cc A`'.

## Exercises

1. ●● Compute the absolute value of signed `D0.L = -42` (`$FFFFFFD6`) into `D0.L` (`42`) using `TST.L D0`, `BPL.S`, and `NEG.L D0`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L2-02 (`loops-dbra`), L2-03 (`structured`).
