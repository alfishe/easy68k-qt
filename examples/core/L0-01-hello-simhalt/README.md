# L0-01 — Program Skeleton: ORG, START, END, SIMHALT

**Track:** `core` · **Level:** 0 · **Difficulty:** ● · **Requires:** none

## Goal

Understand the minimal structure of an EASy68K assembly program and stop execution cleanly with SIMHALT.

## New concepts

- `ORG $1000` sets the starting memory address where instructions are placed.
- `START` is a label marking the entry point passed to `END START`.
- `NOP` (No Operation, opcode `$4E71`) advances `PC` by 2 bytes and takes 4 cycles.
- `SIMHALT` halts the simulator cleanly.

## Predict

Before running, write down your answers:

1. What address will `PC` start at when the program is loaded in Sim68K?
2. How many bytes does `NOP` advance `PC`?
3. How many machine cycles does a single `NOP` take?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Look at `PC` in the Registers panel: it starts at `00001000`.
2. Press Trace Into (`F7`) once: `PC` advances to `00001002` and `Cycles` shows `4`.
3. Press Trace Into (`F7`) again to execute `SIMHALT` and halt the CPU.

## Explain

`ORG $1000` tells the assembler to place the first instruction at `$001000`. `END START` writes an S8 termination record with entry address `$001000`. Executing `NOP` consumes 4 clock cycles and leaves all data and address registers unchanged.

## Common mistakes

- Omitting `SIMHALT` causes the CPU to continue fetching erased `$FFFF` words past the end of your program.
- Putting instructions in column 1 without leading whitespace makes the assembler treat the mnemonic as a label.

## Exercises

1. ● Add a second `NOP` before `SIMHALT` so two `NOP` instructions execute (8 cycles, final `PC = $1008`). — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L0-02 (`registers`), EASy68K Help -> Assembler Directives (`ORG`, `END`, `SIMHALT`).
