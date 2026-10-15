# L0-04 — Hello World with Trap #15 Text I/O

**Track:** `core` · **Level:** 0 · **Difficulty:** ● · **Requires:** L0-03

## Goal

Display null-terminated strings in the Sim68K Output Window using `LEA` and `TRAP #15` tasks 13 and 14.

## New concepts

- `LEA label,A1` loads the effective address of `label` into `A1`.
- `TRAP #15` with `D0 = 14` prints the null-terminated string at `(A1)` without CR/LF.
- `TRAP #15` with `D0 = 13` prints the null-terminated string at `(A1)` followed by CR/LF.

## Predict

Before running, write down your answers:

1. What happens if you forget the trailing `,0` null terminator on a `DC.B` string?
2. What is the difference between Trap #15 task 13 and task 14?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Run or step through `main.x68` and open `View > Output Window` in Sim68K.
2. Observe that task 14 leaves the cursor on the same line, while task 13 advances to the next line.

## Explain

Trap #15 is EASy68K's simulator I/O call. Task 14 outputs bytes starting at the address in `A1` until it reads a `$00` byte; task 13 does the same and appends carriage return + line feed (`\r\n`).

## Common mistakes

- Using `MOVE.L MSG,A1` instead of `LEA MSG,A1`: `MOVE.L` loads the first four ASCII characters of the string into `A1` rather than its address!

## Exercises

1. ● Print `EASy68K` followed by a newline using Trap #15 task 13. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L1-09 (`number-io`), S-01 (`text-io-advanced`).
