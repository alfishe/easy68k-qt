# C-10 — Freestanding C++17: Classes, Constructors, this Pointer, and RAII

**Track:** `c` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `C-03`, `C-06`

## Goal

Understand how freestanding C++17 (`-fno-exceptions -fno-rtti`) lowers member functions to normal 68000 subroutines taking `this` as a hidden first pointer argument, and how RAII destructors run deterministically on scope exit.

## New Concepts

- A non-static member function `int32_t Vec2D::manhattan() const` receives `const Vec2D* this` as its first stack argument (`4(A7)`)
- Constructors initialize object memory before use; destructors run automatically at the closing brace `}`
- RAII (`ScopedLed` / `InterruptGuard`) guarantees cleanup (e.g. writing status to `$E00010` or restoring `SR`) on return

## Predict (Before You Run)

When `ScopedLed led(0xA5, 0x5A)` is constructed at the start of `main()` and destroyed at `return`, what byte is left at `$E00010` after `main()` returns?

## Run & Observe

1. Compare `main.cpp` and the annotated compiler output `main.x68` in **Edit68K**, or load `main.S68` in **Sim68K**.
2. Headless check: `easy68k-run examples/c/C-10-cpp-classes`.
- Inspect `main.cpp` and `main.x68`.
- Watch `ScopedLed`'s constructor write `$A5` to `$E00010`, `Vec2D` methods compute `17 + 39 = 56`, and `~ScopedLed()` write `$5A` to `$E00010` right before `RTS`.

## Explain

C++ member functions have zero runtime magic: `v.manhattan()` simply passes `&v` (`this`) to `Vec2D_manhattan(const Vec2D* this)`. RAII destructors are ordinary calls inserted by the compiler at scope exit.

## Common Mistakes

- Using C++ `try`/`catch` or `dynamic_cast` in freestanding 68000 targets without a C++ runtime (`libstdc++` / `libsupc++`); always compile with `-fno-exceptions -fno-rtti`.

## Exercises

1. **Exercise 1** (`exercises/ex1.cpp` / `exercises/ex1.x68` / `exercises/ex1.toml`): Implement `Counter::add(int32_t delta)` (`this` at `4(A7)`, `delta` at `8(A7)`) and evaluate `c.add(30); c.add(12);` on a `Counter` initialized to `100` at `$2000`, returning `142` in `D0`.
   - Reference solution: `solutions/ex1.cpp` (`solutions/ex1.x68`).

## Going Further

See `C-11-cpp-virtual` for how `virtual` functions add a `vptr` and vtable.
