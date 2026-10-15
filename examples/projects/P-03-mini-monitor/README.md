# P-03 — Mini Monitor & Debugger: TRAP #0 Services, Memory Dump, Edit, and Checksum

## Goal
Implement a ROM-style machine-code monitor that installs a `TRAP #0` system call vector (`$0080`) and parses interactive hex commands (`W <addr> <byte>`, `M <addr>`, `Q`) to inspect and patch memory.

## Prerequisites
- `L3-08` (Bit manipulation & hex conversion), `S-08` (Exception vectors), `S-11` (`TRAP #n` dispatch)

## New Concepts
- Installing a custom `TRAP #0` handler at vector 32 (`$0080`) with function selector in `D0`
- Parsing hexadecimal addresses and byte values from command strings
- Formatting a hex word dump (`"2000: 4258\r\n"`)

## Predict
After executing monitor commands `W 2000 42`, `W 2001 58`, and `M 2000`, what 16-bit big-endian word sits at `$2000`, and what line is printed?

## Run & Observe
1. Run `main.x68` with scripted input `W 2000 42`, `W 2001 58`, `M 2000`, `Q`.
2. Observe output `2000: 4258` and `D0 = $4258` (`16984`), with bytes `42 58` at `$2000`.

## Explain
Each command invokes `TRAP #0` (`D0=1` to write a byte, `D0=0` to read a word). Because the 68000 is big-endian, writing `$42` to `$2000` and `$58` to `$2001` forms the 16-bit word `$4258` at `$2000`.

## Common Mistakes
- Using `RTS` instead of `RTE` at the end of the `TRAP #0` exception handler.
- Forgetting to convert `'A'..'F'` hex digits by subtracting `'A' - 10` (`55`) after checking `'0'..'9'`.

## Exercises
- **Exercise 1 (`exercises/ex1.x68`)**: Parse the 2-character hex string `"A5"` at `HEX_STR` into `D0 = $A5` (`165`) and store the byte to `$2000`.

## Going Further
- Add a `G <addr>` (Go) command that jumps to user code at `<addr>` with `JSR (A0)`.
