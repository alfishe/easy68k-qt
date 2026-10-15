# O-01 — Measuring First: Cycle Counter (Tasks 30/31) and Baseline Profiling

**Track:** `optimization` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `L2-02`, `S-16`

## Goal

Measure exact 68000 execution cycles using Trap #15 tasks 30 (clear cycle counter) and 31 (read cycle counter) to compare a naive byte-by-byte array sum against a longword register-accumulated sum.

## New Concepts

- Task 30 resets the CPU cycle counter to 0; Task 31 returns the current 32-bit cycle count in `D1.L`
- Overhead of the measurement harness itself (`MOVEQ #31,D0` = 4 cycles)
- Comparing `D2` (naive cycle count) against `D3` (optimised cycle count) on identical input data

## Predict (Before You Run)

When summing 8 longwords (`10..80` = `360`), how many cycles does the optimised `ADD.L (A0)+,D0` loop save compared to reloading the base address and index every iteration?

## Run & Observe

1. Compare `naive.x68` and `main.x68` in **Edit68K**, assemble (`F9`), and run in **Sim68K**.
2. Headless check: `easy68k-run examples/optimization/O-01-measuring`.
- Compare `naive.x68` and `main.x68`.
- Inspect `D0 = 360` (sum), `D2` (naive cycles = 468), and `D3` (optimised cycles = 192).

## Explain

Recomputing `LEA ARR,A0` + `LSL.W #2,D1` + `ADD.L 0(A0,D1.W),D0` inside the loop wastes 34 cycles per element. Post-increment `ADD.L (A0)+,D0` with `DBRA` takes only 22 cycles per element.

## Common Mistakes

- Including console `TRAP #15` print calls inside the task 30/31 measurement window.

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Optimise the 8-longword sum in `exercises/ex1.x68` using `(A0)+` and `DBRA` so it finishes in at most `220` cycles (`max_cycles = 220`).
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `O-02-instruction-selection` for single-instruction cycle savings.
