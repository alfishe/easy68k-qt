# L0-03 — Memory Layout, DC/DS, and Big-Endian Byte Order

**Track:** `core` · **Level:** 0 · **Difficulty:** ● · **Requires:** L0-02

## Goal

Declare constants and storage in memory with `DC` and `DS`, read and write memory, and observe Motorola 68000 big-endian byte order.

## New concepts

- The 68000 is **big-endian**: the most significant byte of a word or longword is stored at the lowest byte address.
- `DC.B`, `DC.W`, `DC.L` define initialized byte, word, and longword constants.
- `DS.B`, `DS.W`, `DS.L` reserve uninitialized storage.

## Predict

Before running, write down your answers:

1. When `$AABBCCDD` is written as a longword to `$2000`, which byte is at `$2000` and which byte is at `$2003`?
2. What value does `MOVE.W $2000,D1` read into the low 16 bits of `D1`?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Open `View > Memory` in Sim68K and set `Address` to `002000`.
2. Step through `main.x68` and watch bytes `AA BB CC DD` appear at `$2000..$2003`.

## Explain

Because the 68000 is big-endian, storing `D0.L = $AABBCCDD` at `$2000` places `$AA` at `$2000`, `$BB` at `$2001`, `$CC` at `$2002`, and `$DD` at `$2003`. Reading a word from `$2000` reads `$AABB`, and reading a byte from `$2003` reads `$DD`.

## Common mistakes

- Word (`.W`) or longword (`.L`) accesses at an **odd address** raise an Address Error exception on the 68000.

## Exercises

1. ● Store `$11223344` at `$2000` and load the middle word at `$2001`? Careful: `$2001` is odd! Instead, load the low word at `$2002` into `D0` (`$3344`). — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- L0-04 (`hello-world`), L1-01 (`sizes`).
