# O-03 — Addressing Mode Costs: (An)+ vs d(An,Xi) vs Absolute Long

**Track:** `optimization` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `O-02`, `L3-01`

## Goal

Understand the exact bus-cycle cost of 68000 effective addresses (`(An)` = 4/8 cycles, `(An)+` = 4/8 cycles, `d(An,Xi)` = 10/14 cycles, `xxx.L` = 12/16 cycles) and replace indexed/absolute addressing in loops with post-increment pointers.

## New Concepts

- Every extension word in an effective address adds 4 cycles (and `+2` cycles for index calculation in `d(An,Xi)`)
- Reading a word via `(A0)+` costs 8 cycles total (`MOVE.W (A0)+,D0`), whereas `MOVE.W ARR(D1.W),D0` costs 14–18 cycles plus index maintenance
- Absolute short `($2000).W` (1 extension word, 8 cycles) vs absolute long `($00002000).L` (2 extension words, 12 cycles)

## Predict (Before You Run)

When copying 8 words from `SRC` to `DST`, how many cycles does `MOVE.W (A0)+,(A1)+` save compared to indexed `MOVE.W 0(A0,D1.W),0(A1,D1.W)`?

## Run & Observe

1. Compare `naive.x68` and `main.x68` in **Edit68K**, assemble (`F9`), and run in **Sim68K**.
2. Headless check: `easy68k-run examples/optimization/O-03-addressing-costs`.
- Inspect `D4` (naive indexed copy cycles = 368) vs `D5` (post-increment `MOVE.W (A0)+,(A1)+` cycles = 200).

## Explain

`MOVE.W 0(A0,D1.W),0(A1,D1.W)` fetches two index extension words and performs two index additions per word (`28` cycles), plus `ADDQ.W #2,D1` (`4` cycles). `MOVE.W (A0)+,(A1)+` is a single 2-byte instruction taking `12` cycles.

## Common Mistakes

- Using `0(An)` (16-bit displacement `0`, +4 cycles and +2 bytes) when `(An)` (0 extension words) is meant.

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Copy 8 words from `SRC` to `$2000` using `(A0)+` and `(A1)+` in at most `200` cycles.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `O-05-block-memory` for copying 32–60 bytes per iteration with `MOVEM.L`.
