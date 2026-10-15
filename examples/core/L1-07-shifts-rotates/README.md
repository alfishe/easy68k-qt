# L1-07 — Shifts and Rotates: ASL, ASR, LSL, LSR, ROL, ROR

**Track:** `core` · **Level:** 1 · **Difficulty:** ●● · **Requires:** L1-06

## Goal

Use logical shifts (`LSL`/`LSR`), arithmetic shifts (`ASL`/`ASR`), and rotates (`ROL`/`ROR`) to scale integers by powers of 2 and pack/unpack fields.

## New concepts

- `LSR` shifts 0s into the MSB (unsigned divide by $2^n$); `ASR` replicates the sign bit into the MSB (signed divide by $2^n$, rounding toward $-\infty$).
- Immediate shift counts encode 1..8 in the opcode (`#1`..`#8`); larger counts use a data register (`LSL.L D1,D0`).
- `ROL`/`ROR` rotate bits around without losing them and update `C` (leaving `X` unchanged).

## Predict

Before running, write down your answers:

1. What is `D1.B` after `ASR.B #2,D1` vs `LSR.B #2,D1` when `D1.B = $F0` (-16 signed, 240 unsigned)?
2. How can you multiply `D0` by 10 using only shifts and additions (`D0*8 + D0*2`)?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Compare `D1` (`ASR.B #2` on `$F0` -> `$FC` = -4) with `D2` (`LSR.B #2` on `$F0` -> `$3C` = 60).
2. Compare the cycle count of `LSL.L #3,D0` + `ADD.L` vs `MULU.W #10,D0`!

## Explain

`ASR.B #2` shifts `$F0` (`1111_0000`) right by 2 while replicating bit 7 (`1`), yielding `$FC` (`1111_1100` = -4). `LSR.B #2` shifts in zeros, yielding `$3C` (`0011_1100` = 60).

## Common mistakes

- Writing `LSL.L #12,D0`: immediate shift counts only allow `#1`..`#8`! Load `12` into a register or use two shifts (`LSL.L #8,D0` + `LSL.L #4,D0`).

## Exercises

1. ●● Swap the nibbles (4-bit halves) of byte `$3C` in `D0.B` using `ROL.B #4,D0` so `D0.B` becomes `$C3`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- O-06 (`multiply-divide-free`).
