# S-11 — Supervisor vs User Mode: Privilege Violations, USP/SSP, and System Calls

**Track:** `systems` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `S-08`

## Goal

Understand the 68000 S bit (`SR` bit 13), separate User (`USP`) and Supervisor (`SSP`) stack pointers, Vector 8 Privilege Violation exceptions, and `TRAP` system calls.

## New Concepts

- `SR` bit 13 (`$2000`) is the Supervisor bit: `1` = Supervisor (`A7` is `SSP`), `0` = User (`A7` is `USP`)
- Privileged instructions (`ORI/ANDI/EORI to SR`, `MOVE to SR`, `MOVE USP`, `STOP`, `RESET`, `RTE`) fault with Vector 8 (`$0020`) in User mode
- Vector 8 stacks the address of the faulting privileged instruction so the handler can inspect or skip it
- User programs request privileged operations via `TRAP #0..#15`

## Predict (Before You Run)

When user-mode code executes the 4-byte instruction `ORI.W #$2000,SR`, which vector fires and how many bytes must the handler add to the stacked `PC` to skip the offending instruction?

## Run & Observe

1. Open `main.x68` in **Edit68K**, assemble (`F9`), and launch in **Sim68K**.
2. Headless check: `easy68k-run examples/systems/S-11-supervisor-user`.
- Watch `A7` switch from `SSP` (`$01000000`) to `USP` (`$00FE0000`) when bit 13 of `SR` is cleared.
- Watch Vector 8 (`$0020`) catch `ORI.W #$2000,SR` (`D5 = 1`) and `TRAP #0` (`$0080`) return cleanly to supervisor mode (`D6 = 1`).

## Explain

Clearing bit 13 of `SR` drops the CPU into user mode. Attempting `ORI.W #$2000,SR` in user mode raises Vector 8 with the stacked `PC` pointing at the `ORI.W` instruction (4 bytes). Adding 4 to `2(A7)` skips it; later, `TRAP #0` legally transitions back to supervisor mode by setting bit 13 in the stacked `SR`.

## Common Mistakes

- Trying to return to supervisor mode from user mode with `ORI.W #$2000,SR` instead of a `TRAP` instruction.

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Drop to user mode (`ANDI.W #$DFFF,SR`), push `$12345678` onto the user stack (`-(A7)`), then invoke `TRAP #0` whose handler reads `USP` into `A0` and loads `(A0)` into `D0` before halting.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `S-12-memory-map` for combining user mode with `MEMORY PROTECTED`.
