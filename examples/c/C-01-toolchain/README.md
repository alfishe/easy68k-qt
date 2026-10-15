# C-01 — Freestanding C Toolchain: crt0, Linker Script, and S-Records

**Track:** `c` | **Level:** 4 | **Difficulty:** ●● intermediate
**Prerequisites:** `L2-04`, `L2-06`

## Goal

Understand how freestanding C code starts on a bare 68000: `crt0.S` sets `SP`, zeroes `.bss`, calls `main()`, and halts with the return value in `D0`.

## New Concepts

- Entry point `_start` at `$1000` (`examples/c/runtime/crt0.S` and `easy68k.ld`)
- `.text` (code), `.rodata` (constants), `.data` (initialised globals), and `.bss` (zero-initialized globals)
- `objcopy -O srec` converts the linked ELF binary into an `.S68` S-record image

## Predict (Before You Run)

If `int g_uninit;` lives in `.bss` and `int g_base = 40;` lives in `.data`, what does `main()` return in `D0` when it computes `g_base + g_uninit + 2`?

## Run & Observe

1. Compare `main.c` and the annotated compiler output `main.x68` in **Edit68K**, or load `main.S68` in **Sim68K**.
2. Headless check: `easy68k-run examples/c/C-01-toolchain`.
- Inspect `main.c` alongside `main.x68` (annotated compiler output) and `main.S68`.
- Watch `crt0` clear `.bss` at `$2004`, call `main`, print `C-01: 42`, and return `D0 = 42`.

## Explain

C guarantees that uninitialised globals with static storage duration start at `0`. On bare metal there is no OS loader, so `crt0` explicitly zeroes `[__bss_start, __bss_end)` before calling `main()`.

## Common Mistakes

- Jumping straight to `main()` at `$1000` without running `crt0` (leaves `.bss` filled with `$FF` in Sim68K).

## Exercises

1. **Exercise 1** (`exercises/ex1.c` / `exercises/ex1.x68` / `exercises/ex1.toml`): Return `g_start + g_zero + 23` (`123`) in `D0` where `g_start = 100` (`.data`) and `g_zero` is zeroed in `.bss`.
   - Reference solution: `solutions/ex1.c` (`solutions/ex1.x68`).

## Going Further

See `C-02-types-on-68000` for C integer sizes, alignment, and endianness on the 68000.
