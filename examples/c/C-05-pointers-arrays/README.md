# C-05 — Pointers, Arrays, and volatile Memory-Mapped I/O

**Track:** `c` | **Level:** 4 | **Difficulty:** ●● intermediate
**Prerequisites:** `C-02`, `L3-02`, `S-07`

## Goal

See how C pointer arithmetic (`*p++`, `a[i]`) maps directly to `(An)+` and `d(An,Xi)` addressing modes, and why `volatile` is mandatory for memory-mapped hardware registers (`$E00010`).

## New Concepts

- `p + 1` on `int32_t*` advances the address by `sizeof(int32_t) = 4` bytes
- Sequential pointer traversal `*p++` compiles to post-increment `(A0)+`
- `volatile uint8_t*` prevents the compiler from caching or dead-store-eliminating reads/writes to hardware registers like `EASY68K_LED_ADDR` (`$E00010`)

## Predict (Before You Run)

When we sum `{15, 30, 45, 60}` via `*p++` and write the result (`150 = $96`) to `*EASY68K_LED_ADDR`, what byte appears at `$E00010`?

## Run & Observe

1. Compare `main.c` and the annotated compiler output `main.x68` in **Edit68K**, or load `main.S68` in **Sim68K**.
2. Headless check: `easy68k-run examples/c/C-05-pointers-arrays`.
- Inspect `$E00010` (`96`) and `D0 = 150`.

## Explain

Without `volatile`, an optimising compiler seeing `*led = 0x55; *led = 0x96;` would delete the first write as a dead store. `volatile` forces every C access to emit a machine `MOVE` instruction.

## Common Mistakes

- Omitting `volatile` on hardware register pointers (causes polling loops on switch/button addresses to hang at `-O2`).

## Exercises

1. **Exercise 1** (`exercises/ex1.c` / `exercises/ex1.x68` / `exercises/ex1.toml`): Reverse the 4-element `uint16_t` array `{10, 20, 30, 40}` in-place at `$2000` (`{40, 30, 20, 10}`) and return the new first element (`40`) in `D0`.
   - Reference solution: `solutions/ex1.c` (`solutions/ex1.x68`).

## Going Further

See `C-09-interrupts-in-c` for `volatile` data shared between `main()` and an ISR.
