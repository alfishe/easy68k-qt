# L2-05 — Passing Parameters and Saving Registers with MOVEM

**Track:** `core` · **Level:** 2 · **Difficulty:** ●● · **Requires:** L2-04

## Goal

Pass arguments in registers or on the stack, and preserve callee-saved registers across subroutine calls using `MOVEM.L`.

## New concepts

- `MOVEM.L D2-D4/A2,-(SP)` pushes multiple registers onto the stack in a single instruction.
- `MOVEM.L (SP)+,D2-D4/A2` restores those exact registers in reverse order.
- C/68000 calling convention: `D0–D1`/`A0–A1` are caller-saved scratch registers; `D2–D7`/`A2–A6` are callee-saved.

## Predict

Before running, write down your answers:

1. If `D2 = $11111111` and `D3 = $22222222` before calling `DOT_PRODUCT`, what are `D2` and `D3` after `DOT_PRODUCT` returns?
2. Why does `MOVEM.L` use `-(SP)` when saving and `(SP)+` when restoring?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Open `View > Stack` and step across `MOVEM.L D2-D3,-(SP)` and `MOVEM.L (SP)+,D2-D3`.
2. Confirm that `D2` and `D3` inside `DOT_PRODUCT` are modified, then restored to `$11111111` and `$22222222` before `RTS`.

## Explain

Pre-decrement `MOVEM.L reglist,-(SP)` pushes higher-numbered registers first so the lowest-numbered register sits at the lowest stack address; post-increment `MOVEM.L (SP)+,reglist` reads them back in ascending address order.

## Common mistakes

- Using different register lists on push and pop corrupts the registers and leaves `SP` misaligned for `RTS`!

## Exercises

1. ●● Save `D2` on the stack, use `D2` as a scratch register to compute `(D0 + D1) * 2` for `D0 = 6, D1 = 9` (`30`), and restore `D2 = $DEADBEEF` before `RTS`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L2-06 (`stack-frames`), C-03 (`calling-convention`).
