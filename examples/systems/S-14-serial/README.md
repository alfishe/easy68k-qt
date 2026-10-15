# S-14 — Serial I/O: COM Port Initialisation, Parameters, and Framing (Tasks 40–43)

**Track:** `systems` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `L3-03`

## Goal

Understand how EASy68K programs initialise serial channels (task 40), pack baud/parity/bits/flow parameters (task 41), and transmit/receive buffers (tasks 42–43).

## New Concepts

- Channel ID (`CID`, `0..15`) is passed in the high word of `D0.L` (`D0[31:16]`)
- Task 40 (`SerialInit`): port name string at `(A1)`
- Task 41 (`SerialSetParams`): `D1.L` packs speed code (`bits 7..0`), parity (`bits 10..8`), data bits (`bits 12..11`), stop bits (`bits 14..13`), flow control (`bit 15`)
- Tasks 42 (`SerialRead`) and 43 (`SerialSend`): transfer `D1.B` bytes via `(A1)` and return status in `D0.W`

## Predict (Before You Run)

To select `CID = 1` with task 41 (`SerialSetParams`), what value is placed in `D0.L` before `TRAP #15`?

## Run & Observe

1. Open `main.x68` in **Edit68K**, assemble (`F9`), and launch in **Sim68K**.
2. Headless check: `easy68k-run examples/systems/S-14-serial`.
- Inspect `D0 = $00010029` (`CID = 1` in upper word, task `41 = $29` in lower byte) and the packed 9600-8N1 configuration word in `D2`.

## Explain

Serial tasks multiplex up to 16 open serial ports by storing the channel ID in `D0`'s upper word while `D0.B` holds the Trap #15 task number (`40..43`).

## Common Mistakes

- Using `MOVE.L #41,D0` when `CID > 0`, which overwrites the upper word of `D0` and resets `CID` to `0`.

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Build the `D0.L` command word for `CID = 3` and task `43` (`SerialSend`, `$2B`), and the 9600-8N1 parameter word (`speed=5`, `8 data bits = 3<<11`, `$1805`) in `D1.L`.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `S-15-network` for UDP/TCP socket communication.
