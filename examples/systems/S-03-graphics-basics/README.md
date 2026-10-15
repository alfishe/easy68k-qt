# S-03 — Graphics Basics: Pixels, Lines, Shapes, Colours, and Pen Position

**Track:** `systems` | **Level:** 4 | **Difficulty:** ●● intermediate
**Prerequisites:** `L0-04`

## Goal

Draw pixels, lines, rectangles, and ellipses on the I/O canvas using Trap #15 tasks 80–96, and query pixel colour (task 83) and pen coordinates (task 96).

## New Concepts

- Task 80 (`SetPenColor`) and Task 81 (`SetFillColor`) take `$00BBGGRR` in `D1.L`
- Task 82 (`DrawPixel`) and Task 83 (`GetPixel` -> `D0.L`) at `(D1.W, D2.W)`
- Task 84 (`DrawLine`), Task 85 (`LineTo`), Task 86 (`MoveTo`)
- Task 87 (`DrawRectangle`), Task 88 (`DrawEllipse`), Task 96 (`GetXY` -> `D1.W, D2.W`)

## Predict (Before You Run)

After drawing a red pixel (`$000000FF`) at `(10, 20)` and a line from `(0, 0)` to `(100, 50)`, what do task 83 (`GetPixel(10, 20)`) and task 96 (`GetXY`) return?

## Run & Observe

1. Open `main.x68` in **Edit68K**, assemble (`F9`), and launch in **Sim68K**.
2. Headless check: `easy68k-run examples/systems/S-03-graphics-basics`.
- Open the I/O window (`View → Output Window`) to see the shapes.
- Inspect `D5 = $000000FF` (pixel colour) and `D6 = 100, D7 = 50` (pen X, Y).

## Explain

`DrawLine` updates the pen position to the line's endpoint `(D3.W, D4.W)`. `GetPixel` reads back the `$00BBGGRR` colour of any canvas pixel.

## Common Mistakes

- Confusing `DrawRectangle` (task 87, filled) with `DrawUnfilledRectangle` (task 90, outline only).

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Move the pen to `(40, 60)` using task 86, draw a line to `(80, 90)` using task 85, and read the final pen coordinates `(X, Y)` into `D1` and `D2` using task 96.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `S-04-double-buffering` for flicker-free animation.
