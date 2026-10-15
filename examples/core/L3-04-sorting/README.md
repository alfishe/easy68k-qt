# L3-04 — Sorting in Memory: Insertion Sort on Word Arrays

**Track:** `core` · **Level:** 3 · **Difficulty:** ●●● · **Requires:** L3-02

## Goal

Sort an array of 16-bit signed integers in ascending order in memory using insertion sort.

## New concepts

- Insertion sort maintains a sorted prefix `arr[0..i-1]` and shifts elements greater than `key = arr[i]` one slot right.
- Address registers `A0` (outer cursor) and `A1` (inner scan cursor) make in-place array shifting concise.

## Predict

Before running, write down your answers:

1. Given `[42, 15, 8, 99, 23, 4]`, what are the 6 words at `$2000..$200B` after sorting?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Open `View > Memory` at `$2000` in Sim68K-Qt and watch the 6 words `[0004, 0008, 000F, 0017, 002A, 0063]` settle into sorted order.

## Explain

For small arrays (up to ~32 elements), insertion sort has very low instruction overhead on the 68000 and performs fewer swaps than bubble sort.

## Common mistakes

- Stepping `A1` by 1 byte instead of 2 bytes when shifting 16-bit `.W` elements!

## Exercises

1. ●●● Compare and swap the two words at `$2000` (`30`) and `$2002` (`10`) if out of order so `$2000` holds `10` and `$2002` holds `30`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L3-05 (`searching`).
