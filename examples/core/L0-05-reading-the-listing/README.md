# L0-05 — Reading the .L68 Listing and S-Records

**Track:** `core` · **Level:** 0 · **Difficulty:** ● · **Requires:** L0-04

## Goal

Read an assembler `.L68` listing file: correlate addresses, machine-code hex words, `EQU` symbols, and the symbol cross-reference table.

## New concepts

- `EQU` assigns a constant value to a symbol at assembly time without emitting bytes.
- Each instruction in `.L68` shows its 24-bit address, 16-bit opcode word, and any extension words.
- The symbol table at the bottom of `.L68` lists every symbol and its value.

## Predict

Before running, write down your answers:

1. Does `COUNT EQU 5` emit any bytes into memory at `$1000`?
2. How many bytes long is `MOVEQ #COUNT,D0` vs `MOVE.L #BUFFER,A0`?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Assemble `main.x68` in Edit68K-Qt and click `Load L68` in the Assembler Status dialog.
2. Compare the `Address` and `Code` columns in the `.L68` window with the Source View in Sim68K-Qt.

## Explain

`EQU` lines show `=00000005` in the listing's code column and advance the location counter by 0 bytes. `MOVEQ #5,D0` encodes in a single 2-byte word (`7005`), whereas `LEA BUFFER,A0` uses a 2-byte opcode (`41F9`) followed by a 4-byte address (`00002000`).

## Common mistakes

- Confusing `EQU` (compile-time constant) with `DC` (bytes emitted in memory).

## Exercises

1. ● Define `MAGIC EQU $1234`, load it into `D0.W`, and store `D0.W` at `$2000`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L0-06 (`stepping`), EASyBIN-Qt (`View > EASyBIN`).
