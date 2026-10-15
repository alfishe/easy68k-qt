# P-05 — Multitasking Mini-Kernel: User/Supervisor Mode, Task Control Blocks, and Context Switching

## Goal
Build a two-task multitasking mini-kernel that runs tasks in User mode (`SR` bit 13 = 0) with isolated user stacks (`USP0 = $6000`, `USP1 = $7000`) and performs full context switches inside a `TRAP #1` scheduler handler (`$0084`).

## Prerequisites
- `S-08` (Vectors), `S-11` (`TRAP #n` system calls), `S-12` (Supervisor vs User mode & `USP`/`SSP`), `C-09` (ISRs)

## New Concepts
- Task Control Block (`TCB`) layout storing `PC`, `SR`, `USP`, and `D0-D7/A0-A6`
- Saving and restoring User Stack Pointer via privileged `MOVE.L USP, A0` / `MOVE.L A0, USP`
- Switching execution between two User-mode tasks by rewriting the exception frame on the Supervisor Stack before `RTE`

## Predict
Task 0 adds `10` to `$2000` on each of 3 slices; Task 1 adds `7` to `$2004` on each of 3 slices. After 6 context switches, what are the values at `$2000` (`D0`) and `$2004` (`D1`)?

## Run & Observe
1. Step through `TRAP1_SCHED` in `main.x68` and watch the `SR` (`$0000` in User tasks, `$2000` inside the kernel handler).
2. Observe `USP` toggle between `$6000` (Task 0) and `$7000` (Task 1) on every context switch.
3. Verify `D0 = 30`, `D1 = 21`, `D2 = 6` switches, and memory `$2000` = `00 00 00 1E 00 00 00 15`.

## Explain
When a User-mode task executes `TRAP #1` (`YIELD`), the CPU switches to Supervisor mode and pushes `PC` and `SR` onto `SSP`. The scheduler saves the outgoing task's `PC`, `SR`, `USP`, and registers into its `TCB`, loads the next task's `TCB` state, overwrites the stacked `SR`/`PC`, and executes `RTE` to resume the incoming task seamlessly.

## Common Mistakes
- Executing `MOVE.L USP, A0` from User mode (triggers a Privilege Violation, Vector 8).
- Forgetting to restore the incoming task's `USP` before returning via `RTE`.

## Exercises
- **Exercise 1 (`exercises/ex1.x68`)**: Complete a TCB state-swap routine that loads `D0 = 111` from `TCB_A` and `D1 = 222` from `TCB_B` and records the sum `333` in `D2`.

## Going Further
- Trigger the context switch from Vector 25 (`Level 1 Auto-Vector IRQ`) driven by the Hardware Window's auto-IRQ timer for true preemptive scheduling.
