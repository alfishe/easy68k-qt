# L3-03 — Strings: Length, Copy, and In-Place Uppercase

**Track:** `core` · **Level:** 3 · **Difficulty:** ●● · **Requires:** L3-02

## Goal

Implement classic C-string routines (`STRLEN`, `STRCPY`, and `STRUPPER`) using post-increment byte loops (`MOVE.B (A0)+,(A1)+` + `BNE.S`).

## New concepts

- `MOVE.B (A0)+,(A1)+` copies a byte **and** sets `Z = 1` when that byte is the `$00` null terminator!
- Following `MOVE.B (A0)+,(A1)+` with `BNE.S .LOOP` copies an entire null-terminated string in a two-instruction loop.

## Predict

Before running, write down your answers:

1. Why does `MOVE.B (A0)+,(A1)+` followed by `BNE.S` also copy the terminating `$00` byte into the destination buffer?
2. What is the string length of `'m68k rocks!'` (excluding the `$00` byte)?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Inspect `$2000` in `View > Memory` after running `main.x68`: the copied and uppercased string `'M68K ROCKS!'` appears in ASCII.

## Explain

Because `MOVE` sets the `Z` flag according to the value moved, copying the final `$00` byte sets `Z = 1` and terminates the `BNE.S` loop right after writing the terminator.

## Common mistakes

- Forgetting to allocate `strlen + 1` bytes in the destination buffer for the trailing `$00` byte.

## Exercises

1. ●● Compute the length of `STR DC.B 'EASy68K',0` into `D0.L` (`7`). — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L3-05 (`searching`).
