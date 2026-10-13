# Example Specification Format (`example.toml`)

Every example directory under `examples/` contains an `example.toml` that
tells the example runner how to build, run and check the example. The same
format is used for exercises (`exercises/<name>.toml`), whose reference
solution (`solutions/<name>.x68|.c|.cpp`) must satisfy it.

The file uses a strict **subset of TOML** so the runner can parse it without
external dependencies:

* `key = value` pairs, `[table]` headers and `[[array-of-tables]]` headers.
* Values: basic strings `"..."` (escapes `\\ \" \n \r \t \xHH`), integers
  (decimal, `0x` hex, `0b` binary, optional `_` separators), booleans,
  arrays of strings or integers, and inline tables of integers
  (`{ D0 = 0x78, A1 = 0x2000 }`).
* `#` starts a comment. No multi-line strings, floats or dates.

## Top level

| Key | Type | Required | Default | Meaning |
|-----|------|----------|---------|---------|
| `id` | string | yes | — | Stable ID, e.g. `"L2-06"` (matches the curriculum) |
| `title` | string | yes | — | Human-readable title |
| `track` | string | yes | — | `core`, `systems`, `c`, `optimization`, `projects` |
| `level` | integer | yes | — | 0–3 for the core path; 4 for tracks/capstones |
| `difficulty` | integer | yes | — | 1 ● basic … 4 ●●●● expert |
| `requires` | array of strings | no | `[]` | IDs that must be learned first |
| `source` | string | yes | — | Main file, relative to the example directory |
| `toolchain` | string | no | `"asm"` | `asm` (EASy68K assembler), `gcc-c`, `gcc-cpp` |
| `max_instructions` | integer | no | `1000000` | Safety limit; exceeding it fails the run |
| `simhalt` | boolean | no | `true` | SIMHALT enabled (`*[sim68k]simhalt_off` equivalent when false) |
| `exceptions` | boolean | no | `true` | Exception processing enabled |
| `interactive` | boolean | no | `false` | Needs a human (graphics/sound/hardware); runner only assembles it |

## `[expect]`

All present keys must match; absent keys are not checked.

| Key | Type | Meaning |
|-----|------|---------|
| `halted` | boolean | Program ended by SIMHALT or Trap #15 task 9 |
| `result` | string | Final `SimResult` name, e.g. `"kHalted"`, `"kAddressError"` |
| `output` | string | Exact Trap #15 text output (CR LF as `\r\n`) |
| `output_contains` | array of strings | Substrings that must appear in the output |
| `registers` | inline table | Expected values: `D0`–`D7`, `A0`–`A7`, `PC`, `SR`, `USP`, `SSP` |
| `max_cycles` | integer | Upper bound on the cycle count (required in Track O) |
| `min_cycles` | integer | Lower bound (rarely needed; guards "too good to be true") |

### `[[expect.memory]]`

| Key | Type | Meaning |
|-----|------|---------|
| `address` | integer | Start address |
| `bytes` | string | Expected bytes in hex, spaces optional: `"00 78 FF"` |
| `text` | string | Alternatively: expected ASCII text (no terminator implied) |

## `[input]`

Scripted keyboard input for Trap #15 input tasks.

| Key | Type | Meaning |
|-----|------|---------|
| `lines` | array of strings | Lines returned in order to tasks 2, 4 and 18 (no CR/LF) |
| `chars` | string | Characters returned in order to task 5 |

## Example

```toml
# L2-06 — Stack frames with LINK/UNLK
id = "L2-06"
title = "Stack frames with LINK/UNLK"
track = "core"
level = 2
difficulty = 2
requires = ["L2-04", "L2-05"]
source = "main.x68"

[expect]
halted = true
output = "5! = 120\r\n"
registers = { D0 = 0x78 }
max_cycles = 2400

[[expect.memory]]
address = 0x2000
bytes = "00 00 00 78"

[input]
lines = ["5"]
```

## Runner behaviour

1. Build: assemble `source` with the port's assembler (`asm`), or compile
   and link with the GCC cross-toolchain (`gcc-c`, `gcc-cpp`; skipped with a
   notice when the toolchain is not installed).
2. Load the S-records and start the program like Sim68K's *File → Open*
   (`InitProgram()`: PC = start address, SR = $2000, USP = $00FF0000,
   SSP = $01000000).
3. Run headlessly with the scripted input until halt or `max_instructions`.
4. Compare against `[expect]`; report every mismatch, plus cycles used.
5. Repeat for every `exercises/<name>.toml` with its `solutions/` file.
