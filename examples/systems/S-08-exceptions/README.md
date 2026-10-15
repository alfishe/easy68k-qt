# S-08 — Exceptions and Vector Table: TRAP #n, CHK, TRAPV, and Divide by Zero

**Track:** `systems` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `L1-05`, `L2-04`

## Goal

Install exception handlers in the 68000 vector table (`$0000–$03FF`), enable exceptions, handle `TRAP #0`, `CHK`, `TRAPV`, and Divide-by-Zero, and return with `RTE`.

## New Concepts

- Vector table addresses: Vector `N` is at `4 * N` (e.g. Divide-by-Zero = Vector 5 at `$0014`, `CHK` = Vector 6 at `$0018`, `TRAPV` = Vector 7 at `$001C`, `TRAP #0` = Vector 32 at `$0080`)
- Group 1/2 exception frame pushes `[SSP+0] = SR` (word) and `[SSP+2] = PC` (long)
- `RTE` restores `SR` and `PC` from the supervisor stack

## Predict (Before You Run)

If `TRAP #0` sets bit 0 of `D7`, `CHK` sets bit 1, `TRAPV` sets bit 2, and `DIVU #0` sets bit 3, what is `D7` after all four exceptions fire and return via `RTE`?

## Run & Observe

1. Open `main.x68` in **Edit68K**, assemble (`F9`), and launch in **Sim68K**.
2. Headless check: `easy68k-run examples/systems/S-08-exceptions`.
- Watch `PC` jump through vectors `$0080`, `$0018`, `$001C`, and `$0014`.
- Inspect the 6-byte exception frame in the Stack window before each `RTE`, and `D7 = $0F` (`15`) at `SIMHALT`.

## Explain

When an exception occurs, the 68000 enters supervisor mode, pushes `PC` and `SR` onto `SSP`, and loads `PC` from vector `4 * N`. `RTE` pops `SR` and `PC` to resume execution.

## Common Mistakes

- Using `RTS` instead of `RTE` in an exception handler (leaves `SR` on the stack and corrupts the return address).

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Install a `TRAP #1` handler at Vector 33 (`$0084`) that doubles `D0` and returns with `RTE`. Invoke `TRAP #1` twice starting with `D0 = 15` (`D0 = 60`).
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `S-09-address-bus-errors` for 14-byte Group 0 exception frames.
