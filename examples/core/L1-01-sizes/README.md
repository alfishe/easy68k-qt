# L1-01 — Operation Sizes (.B, .W, .L) and Sign Extension (EXT)

**Track:** `core` · **Level:** 1 · **Difficulty:** ● · **Requires:** L0-03

## Goal

Understand how `.B`, `.W`, and `.L` operations affect only the lower 8, 16, or 32 bits of a data register, and sign-extend values with `EXT.W` and `EXT.L`.

## New concepts

- Byte (`.B`) operations on `Dn` modify only bits 7..0; bits 31..8 are untouched!
- Word (`.W`) operations on `Dn` modify only bits 15..0; bits 31..16 are untouched!
- `EXT.W Dn` sign-extends bit 7 into bits 15..8; `EXT.L Dn` sign-extends bit 15 into bits 31..16.

## Predict

Before running, write down your answers:

1. If `D0 = $12345678`, what is `D0` after `MOVE.B #$AA,D0`?
2. If `D1 = $00000080` (-128 as a signed byte), what is `D1` after `EXT.W D1` and then `EXT.L D1`?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Step through `main.x68` in Sim68K-Qt watching the upper bytes of `D0` remain intact during `.B` and `.W` moves.
2. Watch `D1` sign-extend from `$00000080` to `$0000FF80` (`EXT.W`) and `$FFFFFF80` (`EXT.L`).

## Explain

Unlike many RISC architectures that zero-extend sub-register writes, the 68000 preserves upper bytes on `.B` and `.W` data register destinations. To widen a signed byte to 32 bits, execute `EXT.W Dn` followed by `EXT.L Dn` (or `EXTB.L` on 68020+, not on 68000).

## Common mistakes

- Assuming `MOVE.W val,D0` clears the upper 16 bits of `D0` before a `DIVU`/`DIVS` or `.L` addition — always `CLR.L D0` or `MOVEQ #0,D0` first when zero-extending!

## Exercises

1. ● Sign-extend the signed byte `-10` (`$F6`) in `D0.B` to a full 32-bit signed longword in `D0` (`$FFFFFFF6`). — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L1-02 (`flags`), L1-03 (`signed-unsigned`).
