# S-16 — Logging and Advanced Debugging: Cycle Counters, Directives, and Breakpoints

**Track:** `systems` | **Level:** 4 | **Difficulty:** ●● intermediate
**Prerequisites:** `L0-06`, `L2-06`

## Goal

Use `*[sim68k]break`, Trap #15 tasks 30/31 (cycle counter reset/read), and structured memory watchpoints to profile and debug 68000 code.

## New Concepts

- `*[sim68k]break` comment directive sets an automatic PC breakpoint when the `.L68` listing is loaded in Sim68K
- Task 30 clears the CPU cycle counter; Task 31 reads the 32-bit cycle counter into `D1.L`
- Sim68K Advanced Break Points (`View → Break Points`) trigger on register values (`D0 == $10`), memory reads/writes (`$2000 Write`), and boolean expressions (`R1 AND A1`)

## Predict (Before You Run)

If we clear the cycle counter with task 30 and immediately execute `MOVEQ #42,D2` (4 cycles) and `ADDQ.L #8,D2` (8 cycles) before reading the cycle counter with task 31, what cycle count is returned in `D1.L` (including `MOVEQ #31,D0` before `TRAP #15`)?

## Run & Observe

1. Open `main.x68` in **Edit68K**, assemble (`F9`), and launch in **Sim68K**.
2. Headless check: `easy68k-run examples/systems/S-16-logging-debugging`.
- Use `Options → Log Output` and `Log Start` (`Ctrl+F4`) to record an instruction trace.
- Inspect `D2 = 50` and `D3 = 16` (`4 + 8 + 4` cycles before `TRAP #15`).

## Explain

Task 30 zeroes `cycles`. Next, `MOVEQ #42,D2` takes 4 cycles, `ADDQ.L #8,D2` takes 8 cycles, and `MOVEQ #31,D0` takes 4 cycles (total 16 cycles) at the moment `TRAP #15` reads `cycles` into `D1.L`.

## Common Mistakes

- Forgetting that the `MOVEQ #31,D0` instruction preparing the task 31 call also consumes 4 cycles.

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Clear the cycle counter (task 30), execute `NOP` (4 cycles), and read the cycle counter (task 31, after `MOVEQ #31,D0`) into `D0` (`8` cycles).
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `O-01-measuring` to begin Track O (Optimisation).
