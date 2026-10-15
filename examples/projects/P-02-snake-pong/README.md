# P-02 — Arcade Pong Loop: Double-Buffered Graphics, Collision, 7-Seg Score, and Sound

## Goal
Combine double-buffered 2D graphics (Trap #15 tasks 92/94), ball-and-paddle collision physics, 7-segment display score output (`$E00000`), and sound triggers within a bounded frame budget.

## Prerequisites
- `S-03..S-07` (Graphics, double buffering, sound, 7-segment display), `O-13` (Graphics fast paths)

## New Concepts
- Fixed-timestep arcade loop: Update Physics → Render Back Buffer → Repaint (`task 94`)
- Paddle/wall collision reflection (`NEG.W vx`) with score increment and 7-segment display update
- Keeping per-frame cycle cost deterministic

## Predict
Starting at `ball_x = 52, ball_y = 30, vx = 4, vy = 2` with a right paddle at `X = 60`, on which frame (0..3) does the ball hit the paddle and reverse `vx`?

## Run & Observe
1. Open the Output Window and Hardware Window (`View → Hardware`).
2. Run `main.x68`: after 4 frames, the ball bounces at `X = 60` on frame 2, steps back to `X = 52` on frame 4 (`D0 = 52, D1 = 38`), records `D2 = 1` bounce and `D3 = 10` score, and lights digit `1` (`$06`) on the 7-segment display at `$E00000`.

## Explain
Frame 1 moves `x: 52 -> 56`; Frame 2 moves `x: 56 -> 60`, hitting the paddle (`X >= 60`), which negates `vx` to `-4`, increments bounces (`D2 = 1`) and score (`D3 = 10`), and writes the 7-segment pattern; Frames 3 and 4 move `x: 60 -> 56 -> 52`.

## Common Mistakes
- Calling `task 94` (repaint) inside inner drawing loops instead of once at the end of each frame.
- Forgetting that 7-segment digits at `$E00000` are mapped at even byte addresses (`$E00000, $E00002, ...`).

## Exercises
- **Exercise 1 (`exercises/ex1.x68`)**: Simulate 5 frames of a paddle moving from `X = 20` toward `TARGET_X = 40` at `+4` pixels/frame; set `D0 = 40` (final `PADDLE_X`) and `D1 = 1` when `PADDLE_X == TARGET_X`.

## Going Further
- Hook Trap #15 task 19 (key state) to control the left paddle with `W`/`S` and run interactively.
