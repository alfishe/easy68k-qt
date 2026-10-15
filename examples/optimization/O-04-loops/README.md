# O-04 — Loop Optimisation: DBRA, Loop Inversion, and Unrolling

**Track:** `optimization` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `O-03`, `L2-02`

## Goal

Cut loop control overhead on the 68000 by replacing `CMP`+`Bcc`+`BRA` with `DBRA` (10 cycles taken) and unrolling the loop body 4× to eliminate 75% of the `DBRA` branches.

## New Concepts

- Top-tested loop (`CMP` 6 + `BGE` 8 + body + `ADDQ` 4 + `BRA` 10) wastes 28 cycles of loop overhead per iteration
- Bottom-tested `DBRA Dn,label` costs only 10 cycles per iteration (14 on exit)
- 4× loop unrolling executes 4 operations per `DBRA`, reducing branch overhead from 10 cycles/item to 2.5 cycles/item

## Predict (Before You Run)

When summing 16 words (`1..16` = `136`), how many `DBRA` branches execute when the loop is unrolled 4×?

## Run & Observe

1. Compare `naive.x68` and `main.x68` in **Edit68K**, assemble (`F9`), and run in **Sim68K**.
2. Headless check: `easy68k-run examples/optimization/O-04-loops`.
- Inspect `D0 = 136`, `D4` (top-tested loop cycles = 556), and `D5` (4× unrolled `DBRA` loop cycles = 196).

## Explain

Unrolling 16 iterations by 4 reduces the loop count to `4` (`MOVEQ #4-1,D1`) with four `ADD.W (A0)+,D0` instructions per iteration, eliminating 12 taken branches (`120` cycles saved on `DBRA` alone).

## Common Mistakes

- Unrolling a loop whose trip count is not a multiple of the unroll factor without handling the remainder (or using Duff's device).

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Sum the 16 words at `ARR` (`1..16` = `136`) into `D0` using a 4× unrolled `DBRA` loop in at most `200` cycles.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `O-05-block-memory` for unrolled `MOVEM.L` block copies.
