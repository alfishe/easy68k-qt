# C-02 — C Types on the 68000: Widths, Alignment, Big-Endian Layout, and sizeof

**Track:** `c` | **Level:** 4 | **Difficulty:** ●● intermediate
**Prerequisites:** `C-01`, `L0-03`, `L1-01`

## Goal

Inspect the sizes (`sizeof`) and big-endian memory representation of standard C types (`uint8_t`, `uint16_t`, `uint32_t`, `int`, `void*`) on the m68k ABI.

## New Concepts

- Under `m68k-elf-gcc` / `m68k-linux-gnu-gcc`: `sizeof(char)=1`, `sizeof(short)=2`, `sizeof(int)=4`, `sizeof(long)=4`, `sizeof(void*)=4`
- 16-bit and 32-bit integers are stored **big-endian** (MSB at the lowest address) and must be aligned to **even** addresses on the 68000

## Predict (Before You Run)

When `uint32_t magic = 0x12345678;` is written to `$2004`, what byte is at `*(uint8_t*)0x2004`?

## Run & Observe

1. Compare `main.c` and the annotated compiler output `main.x68` in **Edit68K**, or load `main.S68` in **Sim68K**.
2. Headless check: `easy68k-run examples/c/C-02-types-on-68000`.
- Inspect `$2000` (`01 02 04 04` = `sizeof(char)`, `sizeof(short)`, `sizeof(int)`, `sizeof(void*)`).
- Inspect `$2004` (`12 34 56 78`) and `D0 = $01020404`.

## Explain

On the big-endian 68000, a `uint32_t` value `0x12345678` places its most significant byte `0x12` at the base address.

## Common Mistakes

- Assuming `sizeof(int) == 2` (some vintage 16-bit m68k compilers used `-mshort`, whereas modern GCC defaults to 32-bit `int`).

## Exercises

1. **Exercise 1** (`exercises/ex1.c` / `exercises/ex1.x68` / `exercises/ex1.toml`): Read the high 16-bit word (`0xABCD`) and low 16-bit word (`0xEF01`) of the big-endian `uint32_t` at `$2000` (`0xABCDEF01`) and return their unsigned sum (`0xABCD + 0xEF01 = 0x00019ACE`) in `D0`.
   - Reference solution: `solutions/ex1.c` (`solutions/ex1.x68`).

## Going Further

See `C-06-structs-unions` for struct padding and alignment rules.
