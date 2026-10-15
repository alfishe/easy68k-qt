# L0-02 — Loading Data and Address Registers

**Track:** `core` · **Level:** 0 · **Difficulty:** ● · **Requires:** L0-01

## Goal

Load immediate constants into data registers (`D0`–`D7`) and address registers (`A0`–`A6`) using `MOVEQ`, `MOVE.L`, and `MOVEA.L`.

## New concepts

- `MOVEQ #imm8,Dn` loads a signed 8-bit constant (-128..+127) sign-extended to all 32 bits of `Dn` in 4 cycles.
- `MOVE.L #imm32,Dn` loads any 32-bit immediate value into `Dn`.
- `MOVEA.L #addr,An` loads a 32-bit address into address register `An` without modifying condition codes.

## Predict

Before running, write down your answers:

1. What 32-bit hex value is in `D1` after `MOVEQ #-1,D1`?
2. Why does `MOVEQ #42,D0` take 4 cycles while `MOVE.L #$12345678,D2` takes 12 cycles?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Step through each instruction (`F7`) while watching `D0`, `D1`, `D2`, `D3`, and `A0` in the Registers panel.
2. Observe that `MOVEQ #-1,D1` sets all 32 bits of `D1` to `FFFFFFFF`.

## Explain

`MOVEQ` packs an 8-bit signed constant inside the single 16-bit instruction word and sign-extends it to 32 bits. Full 32-bit immediates require two extension words after the opcode, taking 8 additional cycles to fetch.

## Common mistakes

- Trying `MOVEQ #200,D0`: 200 is outside the signed 8-bit range (-128..127); use `MOVE.L #200,D0` instead.

## Exercises

1. ● Load `100` into `D0`, `-5` into `D1`, and `$00003000` into `A1`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L0-03 (`memory`), EASy68K Help -> Instructions (`MOVE`, `MOVEQ`, `MOVEA`).
