# S-04 — Double Buffering: Flicker-Free Animation with Tasks 92 and 94

**Track:** `systems` | **Level:** 4 | **Difficulty:** ●● intermediate
**Prerequisites:** `S-03`, `L2-02`

## Goal

Eliminate tearing and flicker during animation by enabling offscreen double buffering (task 92 mode 17) and presenting completed frames with task 94 (`Repaint`).

## New Concepts

- Task 92 `D1.B = 17` enables offscreen double buffering; `D1.B = 16` disables it
- Task 94 copies the offscreen back buffer to the visible window atomically
- Frame loop structure: clear back buffer → draw scene → `TRAP #15` task 94

## Predict (Before You Run)

Over 4 animation frames starting at `X = 0` and stepping by 16 pixels per frame, what is the final X coordinate in `D4` after 4 frames?

## Run & Observe

1. Open `main.x68` in **Edit68K**, assemble (`F9`), and launch in **Sim68K**.
2. Headless check: `easy68k-run examples/systems/S-04-double-buffering`.
- Inspect `D3 = 4` (frames rendered) and `D4 = 48` (last frame's X position).

## Explain

When double buffering is active, all drawing tasks (80–93, 95) render into the offscreen buffer. The user never sees a half-drawn frame; task 94 presents the finished frame all at once.

## Common Mistakes

- Forgetting to call task 94 (`Repaint`) at the end of each frame, leaving the visible screen blank.

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Enable double buffering (task 92, `D1 = 17`), draw a pixel at `(X, 25)` across 5 frames (`X = 10, 20, 30, 40, 50`) calling task 94 each frame, and leave the final `X` (`50`) in `D0`.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `O-13-graphics-fast-paths` and `P-02-snake-pong`.
