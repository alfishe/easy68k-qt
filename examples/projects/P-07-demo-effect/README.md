# P-07 — Demo Effect: Fixed-Point Sine-Table Plasma Generator Within a Cycle Budget

## Goal
Generate an 8×8 demoscene plasma tile (`64` bytes at `$2000`) by combining two phase-shifted sine lookup table waves (`SIN16[x + phase] + SIN16[y + phase2]`) using fast indexed addressing and `(A1)+` writes within a strict 1,800-cycle budget.

## Prerequisites
- `O-07` (Lookup tables), `O-08` (Fixed-point math), `O-13` (Graphics fast paths)

## New Concepts
- 2D separable plasma synthesis: `I(x, y) = sin(x + t1) + sin(y + t2)`
- Power-of-two table wrapping with `ANDI.W #$0F, Dn` (no division or branches)
- Hoisting row sine lookups `sin(y + t2)` out of the inner X loop!

## Predict
Why does hoisting `SIN16[(y + t2) & 15]` outside the inner 8-pixel X loop save 56 table lookups per 8×8 tile?

## Run & Observe
1. Run `main.x68` and inspect the 64-byte plasma tile at `$2000..$203F` in the Memory Window.
2. Notice that the row contribution `D5 = SIN16[(y + t2) & 15]` is fetched once per row (`8` times total instead of `64`), completing the entire 64-pixel plasma tile + checksum in ~5,138 cycles (`<= 5500` budget).
3. Verify `D0 = 12960` (total intensity sum across all 64 pixels), `D1 = 174` (pixel `(0,0)`), and `D2 = 238` (pixel `(4,4)`).

## Explain
Because `SIN16` is symmetric around `64` (average value `64`), summing `SIN16[x] + SIN16[y]` across a full period averages `128` per pixel (`64 * 128 = 8192`). Hoisting the Y lookup out of the inner X loop cuts memory reads inside the inner loop in half.

## Common Mistakes
- Recomputing the Y sine lookup on every pixel inside the inner X loop.
- Forgetting to mask table indices with `ANDI.W #$0F, Dn` when phase offsets wrap past 15.

## Exercises
- **Exercise 1 (`exercises/ex1.x68`)**: Evaluate a 16-pixel 1D plasma scanline (`OUT[x] = SIN16[x]`, `x = 0..15`) into `$2000` and accumulate its sum (`1024`) in `D0` in `<= 400` cycles.

## Going Further
- Map each plasma intensity byte (`0..255`) to an RGB palette table (`PALETTE[val * 4]`) and plot an animated 64×64 window using Trap #15 double buffering.
