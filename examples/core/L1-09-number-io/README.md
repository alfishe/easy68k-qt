# L1-09 — Number I/O and Base Conversion (Trap #15 Tasks 3, 4, 15)

**Track:** `core` · **Level:** 1 · **Difficulty:** ●● · **Requires:** L0-04, L1-05

## Goal

Read numbers from the keyboard (`TRAP #15` task 4) and print numbers in signed decimal (task 3) or any base 2–36 (task 15).

## New concepts

- Task 4 reads a signed 32-bit decimal number from the keyboard into `D1.L`.
- Task 3 displays `D1.L` as a signed decimal integer.
- Task 15 displays `D1.L` as an unsigned integer in base `D2.B` (e.g., `D2.B = 16` for hexadecimal, `D2.B = 2` for binary).

## Predict

Before running, write down your answers:

1. When the user enters `255`, what does task 15 print when `D2.B = 16` and when `D2.B = 2`?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Run `main.x68` in Sim68K-Qt, type `255` at the prompt, and check the decimal, hex (`FF`), and binary (`11111111`) output.

## Explain

Trap #15 task 4 parses the user's input string into a 32-bit two's-complement integer in `D1.L`. Task 15 formats `D1.L` unsigned in radix `D2.B` (`2..36`).

## Common mistakes

- Task 3 and task 15 read the number to print from `D1.L`, while `D0` holds the task number!

## Exercises

1. ●● Display `42` in hexadecimal (`2A`) using Trap #15 task 15 (`D1.L = 42`, `D2.B = 16`, `D0 = 15`). — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L2-01 (`compare-branch`), S-01 (`text-io-advanced`).
