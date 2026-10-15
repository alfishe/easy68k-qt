# L1-05 — Multiply and Divide: MULU, MULS, DIVU, DIVS

**Track:** `core` · **Level:** 1 · **Difficulty:** ●● · **Requires:** L1-03

## Goal

Multiply 16-bit operands into 32-bit products with `MULU`/`MULS`, divide 32-bit dividends by 16-bit divisors with `DIVU`/`DIVS`, and unpack quotient and remainder using `SWAP`.

## New concepts

- `MULU.W <ea>,Dn` multiplies unsigned 16-bit `Dn.W * <ea>.W` -> full 32-bit product in `Dn.L`.
- `MULS.W <ea>,Dn` multiplies signed 16-bit `Dn.W * <ea>.W` -> signed 32-bit product in `Dn.L`.
- `DIVU.W <ea>,Dn` divides 32-bit `Dn.L` by 16-bit `<ea>.W` -> **quotient in low word** (`Dn[15:0]`) and **remainder in high word** (`Dn[31:16]`).
- `SWAP Dn` exchanges the high and low 16-bit words of `Dn`.

## Predict

Before running, write down your answers:

1. After `MOVEQ #100,D2` and `DIVU.W #7,D2`, what is the quotient (`100 / 7`) and what is the remainder (`100 % 7`)?
2. Which half of `D2` holds the remainder?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Step through `DIVU.W #7,D2` in Sim68K-Qt: `D2` becomes `$0002000E` (`$0002` remainder in high word, `$000E` = 14 quotient in low word).
2. Watch `SWAP D3` move the remainder `$0002` into the low word of `D3`.

## Explain

100 = 7 * 14 + 2, so `DIVU.W #7,D2` writes quotient 14 (`$000E`) into `D2[15:0]` and remainder 2 (`$0002`) into `D2[31:16]`. Notice also the cycle counter: `MULU`/`MULS` take ~40–70 cycles and `DIVU`/`DIVS` take ~80–158 cycles!

## Common mistakes

- Forgetting to clear or sign-extend the upper 16 bits of `Dn` before `DIVU`/`DIVS` divides a 32-bit dividend with leftover upper bits.
- Dividing by zero triggers vector 5 (Divide-by-Zero exception).

## Exercises

1. ●● Divide `50` by `6` using `DIVU.W #6,D0`; put the quotient (`8`) in `D0.L` and the remainder (`2`) in `D1.L`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L1-07 (`shifts-rotates`), S-08 (`exceptions`), O-06 (`multiply-divide-free`).
