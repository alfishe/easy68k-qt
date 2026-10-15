# L1-06 — Bitwise Logic and Bit Manipulation (BTST, BSET, BCLR, BCHG)

**Track:** `core` · **Level:** 1 · **Difficulty:** ● · **Requires:** L1-01

## Goal

Manipulate individual bits and bitmasks using `AND`, `OR`, `EOR`, `NOT`, `BTST`, `BSET`, `BCLR`, and `BCHG`.

## New concepts

- `AND` clears bits where the mask is 0; `OR` sets bits where the mask is 1; `EOR` toggles bits where the mask is 1.
- `BTST #n,Dn` copies the complement of bit `n` into the `Z` flag (`Z=1` if bit `n` is 0).
- `BSET`, `BCLR`, and `BCHG` test bit `n` (setting `Z` from the old bit) and then set, clear, or toggle bit `n`.

## Predict

Before running, write down your answers:

1. What is `$F0 & $3C`, `$F0 | $0F`, and `$FF ^ $AA`?
2. Starting from `D3 = 0`, what is `D3` after `BSET #3,D3`, `BSET #7,D3`, and `BCHG #3,D3`?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Step through `main.x68` and watch `Z` change as `BSET`/`BCLR`/`BCHG` test each bit before modifying it.

## Explain

On a data register destination, `BTST`/`BSET`/`BCLR`/`BCHG` operate on all 32 bits (bit number modulo 32). On a memory byte destination, they operate on 8 bits (bit number modulo 8).

## Common mistakes

- Expecting `BTST #n,Dn` to set `Z=1` when the bit is 1: `Z=1` means the tested bit was **zero** (`BEQ` branches when the bit is 0; `BNE` branches when the bit is 1)!

## Exercises

1. ● Convert the uppercase ASCII letter `'A'` (`$41`) in `D0.B` to lowercase `'a'` (`$61`) by setting bit 5 with `BSET #5,D0`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L1-07 (`shifts-rotates`).
