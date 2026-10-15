# L1-04 — 64-Bit Multiword Arithmetic with ADDX and SUBX

**Track:** `core` · **Level:** 1 · **Difficulty:** ●● · **Requires:** L1-02

## Goal

Perform 64-bit addition and subtraction across register pairs (`D0:D1` and `D2:D3`) using `ADD.L`/`ADDX.L` and `SUB.L`/`SUBX.L`.

## New concepts

- `ADD.L D3,D1` adds the low 32-bit halves and sets `X = 1` on unsigned carry out.
- `ADDX.L D2,D0` adds the high 32-bit halves **plus the `X` flag** (`D0 = D0 + D2 + X`).
- `ADDX`/`SUBX` only clear `Z` when the result is non-zero (leaving `Z` unchanged if the high word is 0), so `Z` after the full 64-bit operation tests whether the entire 64-bit result is zero!

## Predict

Before running, write down your answers:

1. When adding `0x00000001_FFFFFFFF` (`D0:D1`) and `0x00000002_00000002` (`D2:D3`), what are `D0` (high 32 bits) and `D1` (low 32 bits)?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Step through `ADD.L D3,D1` in Sim68K-Qt: watch `D1` become `$00000001` and `X` become `1`.
2. Step through `ADDX.L D2,D0`: watch `D0` become `1 + 2 + 1 = 4`.

## Explain

The low-half addition `$FFFFFFFF + $00000002` produces `$00000001` with `X=1`. `ADDX.L D2,D0` then adds the carry (`X=1`) into the high 32 bits: `$00000001 + $00000002 + 1 = $00000004`.

## Common mistakes

- Executing an instruction that modifies `X` between `ADD.L` and `ADDX.L` corrupts the carry propagation (`MOVE` does not touch `X`, which is why the 68000 has a separate `X` flag in addition to `C`!).

## Exercises

1. ●● Subtract 64-bit `D2:D3 = $00000001_00000002` from `D0:D1 = $00000005_00000001` using `SUB.L` and `SUBX.L` (result: `D0 = 3`, `D1 = $FFFFFFFF`). — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L1-05 (`mul-div`).
