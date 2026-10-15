# L1-03 — Signed vs Unsigned Comparisons and Overflow vs Carry

**Track:** `core` · **Level:** 1 · **Difficulty:** ●● · **Requires:** L1-02

## Goal

Distinguish signed two's-complement interpretation from unsigned interpretation of the exact same bit pattern (`$FF` = 255 unsigned or -1 signed).

## New concepts

- The CPU performs the **same binary addition/subtraction** for signed and unsigned numbers.
- Unsigned comparisons check `C` and `Z` (`BHI`/`BLS`/`BCC`/`BCS`).
- Signed comparisons check `N`, `V`, and `Z` (`BGT`/`BLE`/`BGE`/`BLT`).

## Predict

Before running, write down your answers:

1. For `D0.B = $FF` and `D1.B = $01`, is `D0 > D1` as unsigned bytes? What about as signed bytes?
2. After `CMP.B D1,D0` (`$FF - $01 = $FE`), what are `N`, `Z`, `V`, `C`?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Step through `main.x68` and watch `SHI` (Set if Higher, unsigned `>`) vs `SGT` (Set if Greater Than, signed `>`) after `CMP.B D1,D0`.

## Explain

Comparing `D0.B = $FF` against `D1.B = $01` computes `$FF - $01 = $FE`, which sets `N=1, Z=0, V=0, C=0`. Because there was no borrow (`C=0`) and the result is non-zero (`Z=0`), unsigned `HI` is **true** (`255 > 1`). Because `N != V` (`1 != 0`), signed `GT` is **false** (`-1 < +1`).

## Common mistakes

- Using `BGT`/`BLT` on memory addresses or unsigned byte values above `$7F` treats them as negative numbers!

## Exercises

1. ●● Compare `D0.B = $80` (-128 signed, 128 unsigned) with `D1.B = $7F` (+127 signed, 127 unsigned). Set `D2.B = $FF` if `D0 < D1` signed (`SLT`) and `D3.B = $FF` if `D0 > D1` unsigned (`SHI`). — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L2-01 (`compare-branch`).
