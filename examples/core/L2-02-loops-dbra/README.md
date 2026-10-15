# L2-02 — Counted Loops with DBRA / DBF and DBcc

**Track:** `core` · **Level:** 2 · **Difficulty:** ●● · **Requires:** L2-01

## Goal

Write fast counted loops with `DBRA` (`DBF`) and conditional early-exit loops with `DBEQ`/`DBNE`.

## New concepts

- `DBRA Dn,label` (alias for `DBF`) decrements the **16-bit low word** `Dn.W` and branches to `label` unless `Dn.W` becomes `-1` (`$FFFF`).
- To execute a loop body `N` times (`N >= 1`), initialize `Dn.W` to `N - 1`!
- `DBEQ Dn,label` stops early when `Z = 1`, or when `Dn.W` reaches `-1`.

## Predict

Before running, write down your answers:

1. If `D1.W` is initialized to `4`, how many times does the `DBRA D1,LOOP` body execute?
2. What value is in `D1.W` when `DBRA` finally falls through?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Step through `main.x68` and watch `D1.W` count down `4, 3, 2, 1, 0, $FFFF` while `D0` accumulates `1 + 2 + 3 + 4 + 5 = 15`.

## Explain

Because `DBRA` branches until `Dn.W == -1` (`$FFFF`), initializing `Dn = N - 1` runs the body for `N - 1, N - 2, ..., 1, 0` — exactly `N` iterations.

## Common mistakes

- Initializing `Dn` to `N` instead of `N - 1` executes the loop `N + 1` times (an off-by-one bug)!
- Remember `DBRA` only decrements `Dn.W` (16 bits), not `Dn.L`.

## Exercises

1. ●● Compute $2^8 = 256$ in `D0` by starting with `D0 = 1` and doubling `D0` (`ADD.L D0,D0`) 8 times using a `DBRA D1,LOOP` with `D1 = 8 - 1`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L2-03 (`structured`), O-04 (`loops`).
