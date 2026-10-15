# S-05 — Sound System: Standard and Multi-Voice Players (Tasks 70–77)

**Track:** `systems` | **Level:** 4 | **Difficulty:** ●● intermediate
**Prerequisites:** `L0-04`

## Goal

Load, play, loop, and stop WAV sounds using both the standard single-channel player (tasks 70–72, 76) and the multi-voice player (tasks 73–75, 77).

## New Concepts

- Standard player: Task 70 (play file), Task 71 (load slot `D1.B`), Task 72 (play slot `D1.B`), Task 76 (control `D2.L`: 0=play once, 1=loop, 2=stop slot, 3=stop all)
- Multi-voice player: Task 73 (play file), Task 74 (load slot), Task 75 (play slot), Task 77 (control)

## Predict (Before You Run)

Which control code in `D2.L` stops all sounds across all slots in tasks 76 and 77?

## Run & Observe

1. Open `main.x68` in **Edit68K**, assemble (`F9`), and launch in **Sim68K**.
2. Headless check: `easy68k-run examples/systems/S-05-sound`.
- Inspect `D2 = 3` (stop-all control code) and `D3 = 1` upon completion.

## Explain

Control code `3` stops all active sounds in the standard player (task 76) or multi-voice player (task 77) regardless of which slot index is in `D1.B`.

## Common Mistakes

- Confusing `D1.B` (sound slot index `0..255`) with `D2.L` (control command `0..3`) in tasks 76 and 77.

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Load sound slot 2 via task 71, send the stop-all command (`D2.L = 3`) via task 76, and place `76` in `D0`.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `P-02-snake-pong` for triggering sound effects on collision events.
