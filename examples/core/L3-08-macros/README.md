# L3-08 — Assembler Macros: MACRO, ENDM, Parameters, and \@ Labels

**Track:** `core` · **Level:** 3 · **Difficulty:** ●● · **Requires:** L2-04

## Goal

Define reusable instruction templates with `MACRO`/`ENDM`, positional parameters `\1`..`\9`, size specifier `\0`, and unique per-expansion labels `\@`.

## New concepts

- `NAME MACRO` ... `ENDM` defines a macro; `\1`, `\2`, ... substitute caller arguments.
- `\0` substitutes the size suffix attached to the macro call (e.g., `CLAMP.W` -> `\0` is `W`).
- `\@` expands to a unique `_00000001` suffix on each macro invocation so internal branch labels never collide.

## Predict

Before running, write down your answers:

1. What happens if a macro defines a branch label `DONE:` without `\@` and is invoked twice in the same program?
2. What values are in `D0` and `D1` after clamping `D0 = 150` and `D1 = -5` to `[0, 100]`?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Enable `Expand Macros` (`OPT MEX`) or check `.L68` in Edit68K-Qt to see the expanded instructions and unique `\@` labels.

## Explain

Macros expand inline at assembly time (saving the `BSR`/`RTS` call/return overhead of a subroutine) while `\@` guarantees each expansion gets distinct local labels.

## Common mistakes

- Putting spaces between arguments in a macro call (`CLAMP D0, 0, 100` includes leading space in `\2` unless trimmed; write `CLAMP.L D0,#0,#100`).

## Exercises

1. ●● Define a `SWAP_REG MACRO` that exchanges two longword data registers `\1` and `\2` using `EXG \1,\2`, and use it to swap `D0 = 11` and `D1 = 22`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L3-09 (`conditional-assembly`).
