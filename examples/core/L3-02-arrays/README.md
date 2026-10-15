# L3-02 — 1-D and 2-D Arrays: Sequential Scan and Row-Major Indexing

**Track:** `core` · **Level:** 3 · **Difficulty:** ●● · **Requires:** L3-01

## Goal

Traverse 1-D arrays with `(An)+` and compute 2-D row-major matrix offsets `(row * COLS + col) * elem_size`.

## New concepts

- Sequential array traversal with `(A0)+` advances the pointer automatically each element with zero extra instructions.
- A 2-D matrix of `ROWS x COLS` words in row-major order stores element `(r, c)` at byte offset `(r * COLS + c) * 2`.

## Predict

Before running, write down your answers:

1. In a 3×4 matrix of 16-bit words (`COLS = 4`), what byte offset from the start of the matrix holds element `(row = 2, col = 1)`?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Step through `main.x68` and verify that `(2 * 4 + 1) * 2 = 18` bytes selects row 2, column 1 (`99`).

## Explain

Because memory is 1-dimensional, 2-D arrays are flattened row by row. When `COLS` is a power of 2 (here `4`), `row * COLS` is a single fast left shift (`LSL.W #2,D1`).

## Common mistakes

- Forgetting to multiply the element index by the element byte size (`2` for `.W`, `4` for `.L`) when indexing word or longword arrays!

## Exercises

1. ●● Find the maximum word in `DATA DC.W 14, 52, 8, 91, 37` and return it in `D0.W` (`91`). — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L3-03 (`strings`), L3-04 (`sorting`).
