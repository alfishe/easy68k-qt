# L3-05 — Searching: Linear Search vs Binary Search

**Track:** `core` · **Level:** 3 · **Difficulty:** ●● · **Requires:** L3-04

## Goal

Implement binary search over a sorted word array and compare its step count against linear search.

## New concepts

- Binary search halves the search interval `[lo, hi]` on every iteration (`mid = (lo + hi) >> 1`).
- On an 8-element sorted array, binary search needs at most 4 comparisons vs up to 8 for linear search.

## Predict

Before running, write down your answers:

1. At what 0-based element index is `47` located in `[5, 12, 19, 26, 33, 47, 58, 71]`?
2. Which indices are probed by binary search when looking for `47`?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Step through `BIN_SEARCH` in Sim68K-Qt: watch `mid` (`D3`) probe index 3 (`26`), then index 5 (`47`), returning `D0 = 5` in just 2 probes!

## Explain

Computing `mid = (lo + hi) >> 1` and converting `mid` to a byte offset via `ADD.W D4,D4` (`mid * 2`) lets `0(A0,D4.W)` fetch the middle word in a single instruction.

## Common mistakes

- Running binary search on an unsorted array produces wrong results!

## Exercises

1. ●● Use linear search (`DBEQ`) to find the 0-based index of word `26` in `SORTED` and place the index (`3`) in `D0`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L3-06 (`linked-lists`).
