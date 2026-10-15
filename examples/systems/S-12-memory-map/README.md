# S-12 — Memory Protection: ROM, Read-Only, Protected, and Invalid Ranges

**Track:** `systems` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `S-09`, `S-11`

## Goal

Declare memory protection regions with the `MEMORY` directive (`ROM`, `READ`, `PROTECTED`, `INVALID`) and observe how the simulator enforces them via Bus Errors (Vector 2).

## New Concepts

- `MEMORY ROM start,end`: writes are silently ignored (no fault)
- `MEMORY READ start,end`: writes fault with Bus Error (Vector 2, `$0008`)
- `MEMORY PROTECTED start,end`: only accessible in Supervisor mode; User mode accesses fault with Bus Error
- `MEMORY INVALID start,end`: any read or write faults with Bus Error
- Assembler emits EASy68K `S0` records so Sim68K automatically configures the Hardware window Memory Map on load

## Predict (Before You Run)

If `$2000–$20FF` is `ROM` holding `$12345678` and `$2300–$23FF` is `INVALID`, what happens when we execute `MOVE.L #$DEADBEEF,$2000` followed by `MOVE.W (A0),D0` with `A0 = $2300`?

## Run & Observe

1. Open `main.x68` in **Edit68K**, assemble (`F9`), and launch in **Sim68K**.
2. Headless check: `easy68k-run examples/systems/S-12-memory-map`.
- Inspect `$2000`: it still contains `$12345678` because ROM writes are ignored.
- Watch `MOVE.W (A0),D0` at `$2300` trigger the Vector 2 Bus Error handler (`D6 = $00002300`, `D7 = 1`).

## Explain

The `MEMORY` directive embeds memory-map `S0` records in the `.S68` file. Writes to `ROM` are dropped without faulting, while accesses to `INVALID` (or User-mode accesses to `PROTECTED`) raise a Group 0 Bus Error at Vector 2 (`$0008`).

## Common Mistakes

- Confusing `ROM` (writes ignored silently) with `READ` (writes raise a Bus Error).

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Declare `MEMORY READ $2100,$21FF`, install a Bus Error handler at `$0008` that records the faulting address in `D0` and recovers via `RTE`, and attempt `MOVE.W D1,(A0)` with `A0 = $2100`.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `P-05-tiny-kernel` for isolating user tasks from kernel memory with `MEMORY PROTECTED`.
