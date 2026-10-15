# O-09 — Branchless Programming: Scc, SUBX Masks, and Branchless abs/min/max

**Track:** `optimization` | **Level:** 4 | **Difficulty:** ●●●● expert
**Prerequisites:** `O-02`, `L1-04`, `L2-01`

## Goal

Eliminate conditional branches (`Bcc` + `BRA`) using `Scc` (`$FF`/`$00`), `SUBX.L D1,D1` (`$FFFFFFFF`/`$00000000` from the `X` flag), and XOR-subtract branchless `abs`.

## New Concepts

- `ADD.L D0,D0` + `SUBX.L D1,D1` replicates the sign bit of `D0` across all 32 bits of `D1` (`-1` if negative, `0` if non-negative)
- Branchless `abs(x)`: `(x ^ mask) - mask` where `mask = x >> 31`
- `Scc Dn` sets `Dn.B = $FF` (`-1`) if `cc` is true or `$00` if false in 4–6 cycles without branching

## Predict (Before You Run)

For `x = -75`, what is `mask` (`$FFFFFFFF`), `x ^ mask` (`74`), and `(x ^ mask) - mask` (`75`)?

## Run & Observe

1. Compare `naive.x68` and `main.x68` in **Edit68K**, assemble (`F9`), and run in **Sim68K**.
2. Headless check: `easy68k-run examples/optimization/O-09-branchless`.
- Inspect `D0 = 75` and verify that the branchless `abs` completes without a single conditional branch instruction.

## Explain

When `x < 0`, `mask = -1` (`$FFFFFFFF`). XORing `x` with `-1` performs bitwise `NOT(x) = -x - 1`, and subtracting `-1` adds `1`, producing `-x`! When `x >= 0`, `mask = 0`, so `(x ^ 0) - 0 = x`.

## Common Mistakes

- Confusing `Scc`'s true result (`$FF = -1`) with C's boolean `1` (`NEG.B Dn` converts `$FF` to `1`).

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Convert a comparison `D1 < 100` (`D1 = 40`) into `D0 = 1` (true) or `0` (false) branchlessly using `CMPI.W #100,D1`, `SLT D0`, and `NEG.B D0` in at most `32` cycles.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `O-10-register-allocation` for keeping inner loops entirely in registers.
