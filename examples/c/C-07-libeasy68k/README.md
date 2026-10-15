# C-07 — Writing libeasy68k: GCC Extended Inline Assembly for Trap #15

**Track:** `c` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `C-03`, `L0-04`, `L1-09`

## Goal

Bind C functions directly to specific 68000 registers (`d0`, `d1`, `d2`, `a1`) using GCC extended inline assembly (`__asm__ volatile`) to invoke Trap #15 tasks without function-call overhead.

## New Concepts

- Register-pinned local variables: `register uint32_t d0 __asm__("d0") = 15;`
- Output/input constraints `"+d"(d0), "+d"(d1)` and address constraint `"a"(a1)`
- Clobber list `"cc", "memory"` tells the compiler that `trap #15` may modify condition codes and memory

## Predict (Before You Run)

What text does `easy68k_puts("val=")`, `easy68k_print_int(255)`, `easy68k_puts(" hex=")`, `easy68k_print_radix(255, 16)`, `easy68k_println("")` print?

## Run & Observe

1. Compare `main.c` and the annotated compiler output `main.x68` in **Edit68K**, or load `main.S68` in **Sim68K**.
2. Headless check: `easy68k-run examples/c/C-07-libeasy68k`.
- Inspect the Output window (`val=255 hex=FF`) and `D0 = 255`.

## Explain

Using `static inline` wrappers with explicit register bindings in `easy68k.h` lets GCC schedule the `MOVEQ` and `LEA` instructions directly in the caller around `TRAP #15` with zero call/return overhead.

## Common Mistakes

- Listing `d0` only as an input (`"d"(d0)`) when a Trap #15 task modifies `D0` (causes GCC to assume `D0` still holds the task number after `trap #15`).

## Exercises

1. **Exercise 1** (`exercises/ex1.c` / `exercises/ex1.x68` / `exercises/ex1.toml`): Use `easy68k_print_radix(42, 2)` (Trap #15 task 15, base 2) to print `42` in binary (`"101010"`) and return `42` in `D0`.
   - Reference solution: `solutions/ex1.c` (`solutions/ex1.x68`).

## Going Further

See `examples/c/runtime/easy68k.h` for the complete `libeasy68k` header.
