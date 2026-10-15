# S-15 — Network I/O: UDP/TCP Client and Server Sockets (Tasks 100–107)

**Track:** `systems` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `L3-03`

## Goal

Understand EASy68K's non-blocking network tasks (100–107) for creating UDP/TCP clients and servers, sending/receiving datagrams, and querying local IP addresses.

## New Concepts

- Task 100 (`CreateClient`) and Task 101 (`CreateServer`): `D1.L` packs port in high word (`bits 31..16`) and protocol (`0=UDP, 1=TCP`) in low byte
- Task 102 (`Send`) / Task 103 (`Receive`) for stream/connected sockets; Task 106 (`SendAtPort`) / Task 107 (`ReceiveFromPort`) for UDP datagrams
- Task 104 (`CloseSocket`) and Task 105 (`GetLocalIP`)

## Predict (Before You Run)

How is port `8080` (`$1F90`) with UDP (`protocol = 0`) packed into `D1.L` for task 100/101?

## Run & Observe

1. Open `main.x68` in **Edit68K**, assemble (`F9`), and launch in **Sim68K**.
2. Headless check: `easy68k-run examples/systems/S-15-network`.
- Inspect `D2 = $1F900000` (port `8080` in upper word, `0` = UDP in lower word).

## Explain

Tasks 100 and 101 pack the 16-bit port number in the upper word of `D1.L` (`$1F900000` for UDP port 8080, or `$1F900001` for TCP port 8080).

## Common Mistakes

- Passing the port number in the low word of `D1.L` for tasks 100/101 instead of the high word.

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Pack TCP (`1`) port `4096` (`$1000`) into `D1.L` (`$10000001`) for task 100 and copy it to `D0`.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `tests/sim/network_io_test.cc` for live loopback UDP/TCP socket verification.
