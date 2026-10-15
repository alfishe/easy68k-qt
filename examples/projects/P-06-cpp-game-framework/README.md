# P-06 — Freestanding C++17 Game Framework with Hand-Optimised 68000 Assembly Hot Paths

## Goal
Combine freestanding C++17 entity-component structure (`EntityWorld<N>`) on `libeasy68k` with a hand-tuned 68000 assembly inner loop for velocity integration and AABB collision detection.

## Prerequisites
- `C-10..C-13` (Freestanding C++17), `O-08` (Fixed-point math), `O-11` (Compiler vs hand assembly)

## New Concepts
- Structuring high-level game state in zero-overhead C++17 templates while delegating per-frame hot loops to `.x68` assembly
- Contiguous array-of-structs / struct-of-arrays layout amenable to `(A0)+` and `DBRA`
- Bounding-box overlap detection without branches or `MULS`

## Predict
Entity 0 starts at `x = 40` moving at `vx = +5`/tick; Entity 1 starts at `x = 64` moving at `vx = -1`/tick (width = 8). After 4 ticks, where are Entity 0 and Entity 1, and do their 8-pixel bounding boxes overlap?

## Run & Observe
1. Inspect `main.cpp` and the hand-tuned assembly in `main.x68`.
2. Run the program: after 4 ticks, Entity 0 is at `x = 60` (`40 + 4*5`), Entity 1 is at `x = 60` (`64 - 4*1`), their boxes overlap (`hits = 1`), console prints `"Tick 4: x0=60 hits=1\r\n"`, and `D0 = 61` (`x0 + hits`).

## Explain
C++17 manages the world lifecycle and compile-time constants cleanly, while the inner loop uses `(A0)+` postincrement and `DBRA` to step all entities and check AABB overlap in under 2,000 cycles.

## Common Mistakes
- Using virtual calls per entity per frame when all entities share the same physics integration kernel.
- Mismatched struct field offsets between C++ and hand-written assembly routines.

## Exercises
- **Exercise 1 (`exercises/ex1.cpp` / `exercises/ex1.x68`)**: Step 2 entities for 1 tick (`x0 = 20 + 4 = 24`, `x1 = 28 - 2 = 26`, width 8) and check AABB overlap (`|x0 - x1| < 8`), setting `D0 = 1` (overlap) and `D1 = 50` (`x0 + x1`) within 1,000 cycles.

## Going Further
- Connect `EntityWorld::render()` to `e68k_set_double_buffer(true)` and `e68k_repaint()` from `easy68k.h`.
