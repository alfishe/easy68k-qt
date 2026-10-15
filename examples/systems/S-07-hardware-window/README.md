# S-07 — Hardware Window: 7-Segment Display, LEDs, Switches, and Push Buttons

**Track:** `systems` | **Level:** 4 | **Difficulty:** ●● intermediate
**Prerequisites:** `L0-03`, `L1-06`

## Goal

Discover and drive the memory-mapped 7-segment display (`$E00000`), LED array (`$E00010`), toggle switches (`$E00012`), and push buttons (`$E00014`) via Trap #15 task 32.

## New Concepts

- Task 32 subtasks: `D1.B = 0` show hardware window, `1` get 7-seg address, `2` get LED address, `3` get switch address, `7` get push-button address
- 7-segment digits live at even byte offsets `base+0, base+2, ..., base+14` (bits `a..g` + decimal point)
- Writing a byte to the LED address lights the 8 LEDs

## Predict (Before You Run)

After querying the 7-segment base address (`$E00000`) and LED address (`$E00010`) via task 32 and writing `$7D` (`'6'`), `$7F` (`'8'`), and `$A5` to the LEDs, what byte is at `$E00010`?

## Run & Observe

1. Open `main.x68` in **Edit68K**, assemble (`F9`), and launch in **Sim68K**.
2. Headless check: `easy68k-run examples/systems/S-07-hardware-window`.
- Open `View → Hardware` to see `'68'` on the 7-segment display and `$A5` (`10100101`) on the LEDs.
- Inspect `D0 = $A5` and memory at `$E00000`.

## Explain

The hardware devices are memory-mapped I/O registers. Task 32 queries their configured base addresses so programs work even if the user relocates the devices in the Hardware window.

## Common Mistakes

- Writing 7-segment digits to consecutive bytes (`$E00000, $E00001`) instead of even byte offsets (`$E00000, $E00002, ...`).

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Query the 7-segment base address with task 32 (`D1.B = 1`) and write digit `'0'` (`$3F`) to offset `0(A0)` and digit `'1'` (`$06`) to offset `2(A0)`.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `S-10-interrupts` for hardware IRQ buttons and automatic timer interrupts.
