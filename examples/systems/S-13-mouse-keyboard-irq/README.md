# S-13 — Mouse and Keyboard Interrupts: Tasks 60, 61, and 62

**Track:** `systems` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `S-03`, `S-10`

## Goal

Configure peripheral interrupts for mouse button/move events (task 60) and keyboard press/release events (task 62), and read mouse coordinates and button state (task 61).

## New Concepts

- Task 60 (`MouseIRQ`): `D1.W` high byte = IRQ level (`1..7`, `0` = off), low byte = event mask (`bit 0` = button down, `bit 1` = button up, `bit 2` = move)
- Task 61 (`MouseRead`): `D1.B` mode (`0` = current, `1` = last up, `2` = last down) -> returns buttons in `D0.L` and `(Y << 16) | X` in `D1.L`
- Task 62 (`KeyIRQ`): `D1.W` high byte = IRQ level (`1..7`), low byte = event mask (`bit 0` = key down, `bit 1` = key up)

## Predict (Before You Run)

What value in `D1.W` enables Mouse IRQ at Level 1 for button-down events (`bit 0`)?

## Run & Observe

1. Open `main.x68` in **Edit68K**, assemble (`F9`), and launch in **Sim68K**.
2. Headless check: `easy68k-run examples/systems/S-13-mouse-keyboard-irq`.
- Inspect the autovector table entries at `$0064` (Level 1 Mouse ISR) and `$0068` (Level 2 Key ISR).
- In the GUI I/O window, clicking or pressing keys triggers the installed ISRs.

## Explain

Tasks 60 and 62 bind I/O window mouse and keyboard events to 68000 autovector interrupt levels. Inside the ISR, task 61 (`D1.B = 0..2`) reads the mouse button flags and packed `Y:X` coordinates.

## Common Mistakes

- Swapping the high byte (IRQ level) and low byte (event mask) in `D1.W` for tasks 60 and 62.

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Install a Level 3 autovector handler at `$006C`, enable keyboard key-down interrupts on Level 3 (`D1.W = $0301`) with task 62, then disable them (`D1.W = $0000`) and set `D0 = 62`.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `P-02-snake-pong` for interactive keyboard/mouse game controls.
