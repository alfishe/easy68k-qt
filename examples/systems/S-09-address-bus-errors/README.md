# S-09 — Address and Bus Errors: Odd Addresses, Group 0 Stack Frames, and Recovery

**Track:** `systems` | **Level:** 4 | **Difficulty:** ●●●● expert
**Prerequisites:** `S-08`

## Goal

Understand 68000 Group 0 exceptions (Vector 2 Bus Error and Vector 3 Address Error), inspect the 14-byte diagnostic stack frame, and clean up the stack to recover.

## New Concepts

- Word or long accesses at an odd memory address trigger Vector 3 (`Address Error`, `$000C`)
- Group 0 stack frame (14 bytes on `SSP`):
  - `0(A7)` = Function Code / R/W info word (2 bytes)
  - `2(A7)` = Faulting access address (4 bytes)
  - `6(A7)` = Instruction register / opcode (2 bytes)
  - `8(A7)` = Saved `SR` (2 bytes)
  - `10(A7)` = Saved `PC` (4 bytes)
- Popping the extra 8 bytes (`info`, `fault_addr`, `IR`) leaves a standard 6-byte `SR+PC` frame for `RTE`

## Predict (Before You Run)

When `MOVE.W (A0),D0` executes with `A0 = $00002001`, what faulting address is pushed at `2(A7)` on the supervisor stack?

## Run & Observe

1. Open `main.x68` in **Edit68K**, assemble (`F9`), and launch in **Sim68K**.
2. Headless check: `easy68k-run examples/systems/S-09-address-bus-errors`.
- Step into the `MOVE.W (A0),D0` instruction and inspect the 14-byte Group 0 frame in the Stack window.
- Inspect `D5` (info word `$0015`), `D6 = $00002001` (faulting address), and `D7 = $3010` (`MOVE.W (A0),D0` opcode).

## Explain

Unlike Group 1/2 exceptions (6-byte frame), Group 0 faults push 8 extra bytes describing the aborted bus cycle (`info`, `access address`, `instruction word`). Removing those 8 bytes allows `RTE` to return to the next instruction.

## Common Mistakes

- Executing `RTE` directly inside an Address/Bus Error handler without popping the 8 extra Group 0 words first.

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Install an Address Error handler at `$000C` that extracts the faulting address into `D0`, cleans up the 8 extra stack bytes, and returns via `RTE` after `MOVE.L (A1),D1` faults at `A1 = $00003003`.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `S-12-memory-map` for triggering Bus Errors via `MEMORY INVALID` and `MEMORY PROTECTED`.
