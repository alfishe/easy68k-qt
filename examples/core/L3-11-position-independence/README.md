# L3-11 — Position-Independent Code (PIC) with PC-Relative Addressing

**Track:** `core` · **Level:** 3 · **Difficulty:** ●●● · **Requires:** L3-01

## Goal

Write relocatable machine code using `LEA label(PC),An`, `d8(PC,Xn)`, and relative branches (`BSR`/`BRA`/`Bcc`), then copy the routine to another RAM address and execute the copy!

## New concepts

- Code that uses only PC-relative branches (`BRA`, `BSR`, `Bcc`) and PC-relative data reads (`label(PC)`, `label(PC,Xn)`) contains **zero absolute addresses**.
- Such position-independent code (PIC) can be copied anywhere in memory (or loaded into ROM at any address) and executed directly.

## Predict

Before running, write down your answers:

1. If `PIC_FUNC` is assembled at `$1020`, copied byte-for-byte to `$3000`, and called via `JSR $3000`, will `LEA CONSTS(PC),A0` inside the copy read `$3000`'s copy of `CONSTS`?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Step into `JSR $3000` (`F7`) in Sim68K-Qt and watch the relocated instructions at `$3000` execute and return `D0 = 100 + 200 + 300 = 600`!

## Explain

Because `LEA CONSTS(PC),A0` encodes the signed byte distance from the instruction's `PC` to `CONSTS`, copying both the code and its trailing constant pool preserves that relative distance at `$3000`.

## Common mistakes

- Using `LEA CONSTS,A0` (absolute address) instead of `LEA CONSTS(PC),A0` hardcodes the original address instead of the relocated address.
- Remember the 68000 does **not** allow PC-relative addressing as a `MOVE` destination (`MOVE.L D0,VAR(PC)` is illegal on 68000; use `LEA VAR(PC),A0` + `MOVE.L D0,(A0)` if writing).

## Exercises

1. ●●● Load the 2nd word (`250`) from `TBL(PC)` using PC-relative indexed addressing `TBL(PC,D1.W)` with `D1 = 2` into `D0.W`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- Track S (`systems`), Track O (`optimization`).
