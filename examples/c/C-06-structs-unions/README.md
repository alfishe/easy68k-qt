# C-06 — Structs, Alignment Padding, Unions, and Assembly OFFSET Views

**Track:** `c` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `C-02`, `L3-10`

## Goal

Understand how the 68000 compiler inserts padding bytes inside `struct` definitions to keep 16-bit and 32-bit fields on even addresses, and how to match C structs with `OFFSET 0` in assembly.

## New Concepts

- On the 68000, `uint16_t` and `uint32_t` require 2-byte alignment (otherwise accessing them raises an Address Error)
- `struct Sprite { uint8_t id; int16_t x; int16_t y; uint32_t color; }` inserts 1 byte of padding after `id`, giving offsets `id=0`, `x=2`, `y=4`, `color=6` and `sizeof(struct Sprite) = 10`
- Matching `OFFSET 0` definitions in `.x68` files allow assembly routines to access C structs directly

## Predict (Before You Run)

What is `sizeof(struct Sprite)` on the 68000, and at what byte offset from `$2000` is `sprite->color` stored?

## Run & Observe

1. Compare `main.c` and the annotated compiler output `main.x68` in **Edit68K**, or load `main.S68` in **Sim68K**.
2. Headless check: `easy68k-run examples/c/C-06-structs-unions`.
- Inspect `$2000..$2009`: `07 00` (`id=7` + pad), `00 78` (`x=120`), `00 50` (`y=80`), `00 FF 80 00` (`color`).
- Verify `D0 = 10` (`sizeof(struct Sprite)`).

## Explain

Because `x` is a 16-bit word, the compiler aligns `x` to offset 2 by inserting 1 padding byte at offset 1. Total size is `1 + 1 + 2 + 2 + 4 = 10` bytes.

## Common Mistakes

- Assuming `sizeof(struct Sprite)` is `1 + 2 + 2 + 4 = 9` bytes or reading `x` at offset `1(A0)` (which triggers a 68000 Address Error!).

## Exercises

1. **Exercise 1** (`exercises/ex1.c` / `exercises/ex1.x68` / `exercises/ex1.toml`): Read `x` (`2(A0)`) and `y` (`4(A0)`) from the `Sprite` struct at `$2000` (`x = 120, y = 80`) and return `x + y` (`200`) in `D0`.
   - Reference solution: `solutions/ex1.c` (`solutions/ex1.x68`).

## Going Further

See `C-08-c-calls-asm` for passing a C struct pointer to an assembly function.
