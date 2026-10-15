# L3-10 — Multi-File Projects: INCLUDE, INCBIN, and OFFSET Structs

**Track:** `core` · **Level:** 3 · **Difficulty:** ●● · **Requires:** L3-09

## Goal

Split definitions across files with `INCLUDE`, embed binary data with `INCBIN`, and define C-style struct field offsets with `OFFSET`.

## New concepts

- `OFFSET 0` followed by `DS.B`/`DS.W`/`DS.L` labels defines struct field offsets (`PT_X = 0`, `PT_Y = 2`, `PT_SIZE = 4`) without emitting bytes.
- `INCLUDE 'defs.inc'` splices another source file into the assembly.
- `INCBIN 'payload.bin'` embeds raw binary bytes directly into the object image.

## Predict

Before running, write down your answers:

1. After `OFFSET 0`, `PT_X DS.W 1`, `PT_Y DS.W 1`, what are the values of `PT_X` and `PT_Y`?
2. Why must `ORG` be restored after an `OFFSET` block before emitting instructions?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Open `.L68` to see how `OFFSET 0` defines `PT_X = 0`, `PT_Y = 2`, `PT_Z = 4` and how `INCLUDE 'defs.inc'` is expanded.

## Explain

`OFFSET` switches the assembler's location counter into a relative offset counter so `DS` directives compute struct member displacements symbolically instead of reserving RAM.

## Common mistakes

- Forgetting `ORG $1000` after an `OFFSET` section!

## Exercises

1. ●● Include ` '../defs.inc'` and load `PT_Z(A0)` (`40`) into `D0.L`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L3-11 (`position-independence`), C-06 (`structs-unions`).
