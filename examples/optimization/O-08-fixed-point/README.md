# O-08 — Fixed-Point Arithmetic: Q8.8 and Q16.16 Scaling with SWAP

**Track:** `optimization` | **Level:** 4 | **Difficulty:** ●●●● expert
**Prerequisites:** `O-06`, `L1-07`

## Goal

Perform fast fractional physics and graphics maths on the integer-only 68000 using Q8.8 and Q16.16 fixed-point representation and the 4-cycle `SWAP` instruction.

## New Concepts

- Q16.16 stores the signed integer part in bits 31..16 and the 16-bit fraction (`1/65536` units) in bits 15..0
- Extracting the integer part of a Q16.16 register is a single 4-cycle `SWAP Dn` (instead of a 38-cycle `ASR.L #16`!)
- Q8.8 multiply `(a * b) >> 8` fits directly in a single 16×16 `MULS.W` followed by `ASR.L #8,D0`

## Predict (Before You Run)

In Q8.8 fixed point (`256` = `1.0`), what is `3.5 * 2.25` (`$0380 * $0240 >> 8`), and what is its integer part?

## Run & Observe

1. Compare `naive.x68` and `main.x68` in **Edit68K**, assemble (`F9`), and run in **Sim68K**.
2. Headless check: `easy68k-run examples/optimization/O-08-fixed-point`.
- Inspect `D2 = $000007E0` (`7.875` in Q8.8: integer `7`, fraction `.875 = 224/256 = $E0`) and `D0 = 7` (integer part).

## Explain

`3.5` in Q8.8 is `3.5 * 256 = 896` (`$0380`), and `2.25` is `2.25 * 256 = 576` (`$0240`). `MULS.W` produces `516096` (`$0007E000`); shifting right by 8 yields `$07E0` (`7.875`), and shifting right by 8 again extracts integer `7`.

## Common Mistakes

- Using `LSR.L #16,D0` on the 68000 when `SWAP D0` (4 cycles!) swaps the high and low 16-bit words immediately.

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Extract the signed 16-bit integer part of the Q16.16 value `$002A8000` (`42.5`) into `D0` (`42`) using `SWAP` + `EXT.L` in at most `32` cycles.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `P-07-demo-effect` for Q8.8 fixed-point rotozoom/plasma.
