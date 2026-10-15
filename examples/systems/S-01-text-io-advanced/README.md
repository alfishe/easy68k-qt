# S-01 — Advanced Text I/O: Cursor, Colours, Echo, and Key State

**Track:** `systems` | **Level:** 4 | **Difficulty:** ●● intermediate
**Prerequisites:** `L0-04`, `L1-09`

## Goal

Control cursor coordinates (task 11), keyboard echo (task 12), input prompts (task 16), key state queries (task 19), and font properties (task 21).

## New Concepts

- Task 11: `D1.W = $CCRR` sets cursor to column `CC`, row `RR`; `D1.W = $00FF` reads cursor into `D1.W`; `D1.W = $FF00` clears screen
- Task 12: `D1.B = 0/1` disables/enables keyboard echo
- Task 16: `D1.B = 0..3` controls input prompt and linefeed display
- Task 19: `D1.L` queries up to 4 virtual key codes
- Task 21: `D1.L = $00BBGGRR` colour, `D2.L` font/style/size

## Predict (Before You Run)

After positioning the cursor at column 10 (`$0A`), row 4 (`$04`) with task 11 and reading it back with `D1.W = $00FF`, what value is returned in `D1.W`?

## Run & Observe

1. Open `main.x68` in **Edit68K**, assemble (`F9`), and launch in **Sim68K**.
2. Headless check: `easy68k-run examples/systems/S-01-text-io-advanced`.
- Watch the I/O window cursor jump to `(10, 4)` and print in bold green Courier New.
- Inspect `D2` (`$00000A04`) after reading back the cursor position.

## Explain

Task 11 packs column in bits 15..8 and row in bits 7..0 of `D1.W`. Passing `$00FF` queries the current cursor position using the same packed format.

## Common Mistakes

- Swapping row and column in `D1.W` (column is the high byte, row is the low byte).
- Using RGB instead of BGR in `D1.L` for task 21 (`$00BBGGRR`).

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Clear the screen (`D1.W = $FF00`), move the cursor to column 5, row 2 (`$0502`), and read the cursor position back into `D0.W`.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `S-03-graphics-basics` for pixel-level drawing and text placement on the graphics canvas.
