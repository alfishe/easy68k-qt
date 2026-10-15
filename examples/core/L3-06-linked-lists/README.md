# L3-06 — Linked Lists and Pointer Chasing

**Track:** `core` · **Level:** 3 · **Difficulty:** ●●● · **Requires:** L3-01

## Goal

Traverse a singly linked list of `(value, next_ptr)` nodes in memory using address register indirect with displacement (`4(A0)`).

## New concepts

- Each node has two 32-bit fields: `0(A0)` = `value` (`DC.L`) and `4(A0)` = `next` pointer (`DC.L`, `0` for `NULL`).
- `MOVEA.L 4(A0),A0` advances `A0` to the next node; `MOVE.L A0,D1` (or `CMPA.L #0,A0`) tests for `NULL`.
- Remember: `MOVEA.L` does **not** update condition codes on the 68000!

## Predict

Before running, write down your answers:

1. Why doesn't `MOVEA.L 4(A0),A0` followed immediately by `BEQ` detect when `A0` becomes `0` (`NULL`)?
2. What is the sum of the three linked nodes `N1(10) -> N2(20) -> N3(30) -> NULL`?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Step through `main.x68` and watch `A0` jump from `N1` to `N2` to `N3` (even though `N2` is placed after `N3` in memory!) and finally to `0`.

## Explain

Because `MOVEA` never modifies the CCR flags, you must test an address register explicitly with `CMPA.L #0,A0` (or `MOVE.L A0,Dn`) before branching on `BEQ`/`BNE`.

## Common mistakes

- Writing `MOVEA.L 4(A0),A0` followed by `BNE.S .LOOP` without `CMPA.L #0,A0`: the `Z` flag still holds the result of whatever instruction ran before `MOVEA`!

## Exercises

1. ●●● Unlink `NODE2` by changing `NODE1`'s `next` pointer (`4(A0)`) to point directly to `NODE3`, then sum the list (`10 + 30 = 40`). — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L3-07 (`stack-queue`), C-05 (`pointers-arrays`).
