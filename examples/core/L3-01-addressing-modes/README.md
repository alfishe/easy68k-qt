# L3-01 — All 68000 Addressing Modes Side by Side

**Track:** `core` · **Level:** 3 · **Difficulty:** ●● · **Requires:** L2-08

## Goal

Use every Motorola 68000 addressing mode (`Dn`, `An`, `(An)`, `(An)+`, `-(An)`, `d16(An)`, `d8(An,Xn)`, `(xxx).W`, `(xxx).L`, `d16(PC)`, `d8(PC,Xn)`, `#imm`) and compare their encodings and cycle costs.

## New concepts

- Register direct (`Dn`, `An`) and register indirect (`(An)`, `(An)+`, `-(An)`): no extension words.
- Displacement (`d16(An)`, `d16(PC)`) and indexed (`d8(An,Xn)`, `d8(PC,Xn)`): one 16-bit extension word.
- Absolute short (`$2000.W`, 1 extension word) vs absolute long (`$00012000.L`, 2 extension words).

## Predict

Before running, write down your answers:

1. What happens to `A0` after `MOVE.W (A0)+,D1` (post-increment on a `.W` word)?
2. Why does `MOVE.B (A7)+,D0` increment `A7` by **2** instead of 1?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Step through `main.x68` in Sim68K-Qt watching `A0` advance by 2 on `.W` post-increment and return by 2 on `.W` pre-decrement.

## Explain

Post-increment `(An)+` and pre-decrement `-(An)` adjust `An` by 1 for `.B`, 2 for `.W`, and 4 for `.L` — except `A7` (`SP`), which always adjusts by at least 2 on `.B` to keep the hardware stack word-aligned!

## Common mistakes

- Forgetting that `-(An)` decrements **before** the access while `(An)+` increments **after** the access.

## Exercises

1. ●● Use indexed addressing `0(A0,D1.W)` to load the 3rd longword (index 2 -> byte offset `8`) of `LONGS DC.L 100,200,300,400` into `D0` (`300`). — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L3-02 (`arrays`), O-03 (`addressing-costs`).
