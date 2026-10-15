# O-07 — Lookup Tables: Trading Memory for Cycles (Nybble Popcount & Squares)

**Track:** `optimization` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `O-03`, `L3-02`

## Goal

Replace multi-iteration bit-counting or math loops with O(1) table lookups indexed by `0(A0,Dn.W)`.

## New Concepts

- Bit-by-bit popcount of an 8-bit byte takes 8 loop iterations (~140 cycles)
- A 16-byte nybble popcount table (`POP4_TAB`) counts set bits in any byte in two table lookups (~40 cycles)
- Precomputed sine/square/CRC tables turn runtime arithmetic into a single indexed `MOVE`

## Predict (Before You Run)

How many set bits are in `$D5` (`%11010101`), and how many cycles does a nybble-table lookup take compared to an 8-iteration `LSR.B`+`BCC` loop?

## Run & Observe

1. Compare `naive.x68` and `main.x68` in **Edit68K**, assemble (`F9`), and run in **Sim68K**.
2. Headless check: `easy68k-run examples/optimization/O-07-lookup-tables`.
- Inspect `D0 = 5` (bits set in `$D5`), `D4` (loop cycles = 158), and `D5` (lookup table cycles = 52).

## Explain

Splitting `$D5` into high nybble `$0D` (`3` bits) and low nybble `$05` (`2` bits) indexes directly into the 16-byte `POP4` table (`3 + 2 = 5`).

## Common Mistakes

- Forgetting to mask or clear upper bits of the index register `Dn.W` before `0(A0,Dn.W)` (since `.W` sign-extends bit 15).

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Look up `13^2` (`169`) from the precomputed word table `SQ_TAB` using `D1 = 13` in at most `48` cycles (`MULU` exceeds the budget!).
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `P-07-demo-effect` for sine/cosine lookup tables in real-time plasma rendering.
