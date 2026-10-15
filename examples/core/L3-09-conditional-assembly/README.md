# L3-09 — Conditional Assembly and Listing Control (IFEQ, IFNE, OPT)

**Track:** `core` · **Level:** 3 · **Difficulty:** ●● · **Requires:** L3-08

## Goal

Select code paths at assembly time using `IFEQ`, `IFNE`, `IFGT`, `IFLT`, `IFC`, `IFNC`, and `ENDC`.

## New concepts

- `IFNE expr` ... `ENDC` assembles its body only when `expr != 0` at assembly time.
- `IFEQ expr` ... `ENDC` assembles its body only when `expr == 0` at assembly time.
- Skipped conditional blocks emit **zero bytes** and take **zero runtime cycles**.

## Predict

Before running, write down your answers:

1. With `DEBUG EQU 1` and `WORD_MODE EQU 0`, which instructions are emitted into the program?
2. Note: Original EASy68K conditional assembly (`IFxx`/`ENDC`) does not have an `ELSE` keyword — use complementary `IFNE`/`IFEQ` blocks!

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Inspect `.L68` to see which lines inside `IFNE DEBUG` and `IFEQ WORD_MODE` produced machine code.

## Explain

Conditional assembly lets a single source file target multiple build configurations (e.g., debug assertions on/off, word vs longword precision) with zero runtime branch cost.

## Common mistakes

- Using `ELSE` inside `IFNE`/`ENDC`: in original EASy68K, `ELSE` belongs to structured runtime `IF`/`ELSE`/`ENDI`, not assembly-time `IFxx`/`ENDC`!

## Exercises

1. ●● Set `USE_ADD EQU 0` so the `IFEQ USE_ADD` block executes `MOVEQ #50,D0` instead of `MOVEQ #10,D0`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L3-10 (`multi-file`).
