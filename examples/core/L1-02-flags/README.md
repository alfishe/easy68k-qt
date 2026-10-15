# L1-02 — Condition Codes (CCR): X, N, Z, V, C

**Track:** `core` · **Level:** 1 · **Difficulty:** ● · **Requires:** L1-01

## Goal

Predict and inspect the five Condition Code Register (CCR) flags (`X`, `N`, `Z`, `V`, `C`) across `ADD`, `SUB`, and `CMP`.

## New concepts

- `Z` (Zero, bit 2) = 1 when the operation result is 0.
- `N` (Negative, bit 3) = 1 when the most significant bit of the result (`.B` bit 7, `.W` bit 15, `.L` bit 31) is 1.
- `C` (Carry, bit 0) = 1 on unsigned carry/borrow out of the MSB; `X` (Extend, bit 4) tracks `C` on arithmetic instructions.
- `V` (Overflow, bit 1) = 1 on signed two's-complement overflow.

## Predict

Before running, write down your answers:

1. Which flags are set by `ADD.B #1,D0` when `D0.B = $FF`?
2. Which flags are set by `ADD.B #1,D1` when `D1.B = $7F` (+127)?
3. Does `CMP.B #5,D2` modify `D2`, or only the CCR flags?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Watch the `SR` binary display under `T S INT XNZVC` in the Registers panel after each step.
2. Note how `MOVE` from `SR` saves the flags after each test into `D3`, `D4`, and `D5`.

## Explain

Adding `1` to `$FF` wraps to `$00`, setting `Z=1`, `C=1`, `X=1` (`CCR = $15`). Adding `1` to `$7F` (+127) produces `$80` (-128), setting `N=1` and `V=1` (signed overflow, `CCR = $0A`). `CMP.B #5,D2` when `D2 = 5` computes `D2 - 5 = 0` and sets `Z=1` without changing `D2` or `X`.

## Common mistakes

- Remember that `CMP` does **not** affect the `X` (Extend) flag!

## Exercises

1. ● Compute `SUBQ.B #1,D0` when `D0 = 0` (unsigned borrow: `X=1, N=1, Z=0, V=0, C=1 -> $19`) and store the low 5 bits of `SR` in `D1`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L1-03 (`signed-unsigned`), L1-04 (`multiword`), L2-01 (`compare-branch`).
