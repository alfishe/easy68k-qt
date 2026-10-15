# S-06 — Timing and Frame Pacing: Tasks 8 and 23

**Track:** `systems` | **Level:** 4 | **Difficulty:** ●● intermediate
**Prerequisites:** `L1-02`

## Goal

Read the simulator clock in hundredths of a second since midnight (task 8) and pace execution with `DelayHundredths` (task 23).

## New Concepts

- Task 8 returns hundredths of a second since midnight in `D1.L`
- Task 23 delays for `D1.L` hundredths of a second (`100` = 1.00 second)
- Computing elapsed time `end_time - start_time` for frame pacing

## Predict (Before You Run)

If we record `start_time` with task 8, delay `25` hundredths of a second with task 23, and read `end_time` with task 8, what is `end_time - start_time` in `D0`?

## Run & Observe

1. Open `main.x68` in **Edit68K**, assemble (`F9`), and launch in **Sim68K**.
2. Headless check: `easy68k-run examples/systems/S-06-timing`.
- Inspect `D2` (start time), `D3` (end time), and `D0 = 25` (elapsed hundredths).

## Explain

Task 23 pauses execution for `D1.L * 10 ms`. Subtracting the task 8 timestamps before and after yields the elapsed hundredths of a second.

## Common Mistakes

- Treating task 23's `D1.L` parameter as milliseconds instead of hundredths of a second (`1/100 s = 10 ms`).

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Perform two delays of `10` and `15` hundredths of a second (task 23) bracketed by task 8 calls, and store the total elapsed time (`25`) in `D0`.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `O-01-measuring` for cycle-exact timing with tasks 30 and 31.
