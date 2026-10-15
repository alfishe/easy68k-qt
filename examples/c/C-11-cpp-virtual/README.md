# C-11 — Virtual Dispatch and Vtables in Memory: How Polymorphism Works on the 68000

**Track:** `c` | **Level:** 4 | **Difficulty:** ●●●● expert
**Prerequisites:** `C-10`, `L2-08`

## Goal

Inspect a C++ virtual method table (`vtable`) and object `vptr` in 68000 memory, trace an indirect virtual call (`MOVEA.L (A0),A1` / `MOVEA.L 0(A1),A1` / `JSR (A1)`), and compare its cycle cost against a direct call.

## New Concepts

- Every polymorphic object starts with a 4-byte `vptr` at offset `0(this)` pointing to its class's `vtable` in `.rodata`
- Calling `shape->area()` loads `vptr` from `0(A0)`, loads the function pointer from slot `0(vptr)`, and executes `JSR (A1)`
- Extra cost vs direct `BSR`: two pointer loads + indirect `JSR` (~24 extra cycles per call on the 68000)

## Predict (Before You Run)

If a `Rect(6, 7)` object lives at `$2000` and a `Square(5)` object lives at `$2008`, what sits at `0($2000)` and `0($2008)`, and what is `rect->area() + sq->area()`?

## Run & Observe

1. Compare `main.cpp` and the annotated compiler output `main.x68` in **Edit68K**, or load `main.S68` in **Sim68K**.
2. Headless check: `easy68k-run examples/c/C-11-cpp-virtual`.
- Inspect `$2000` (`vptr` to `vtable_Rect` followed by `w=6, h=7`) and `$2008` (`vptr` to `vtable_Square` followed by `side=5`).
- Verify `D0 = 67` (`42 + 25`).

## Explain

C++ virtual dispatch is an array of function pointers (`vtable`) shared by all instances of a class, plus a single 4-byte `vptr` per instance.

## Common Mistakes

- Using `memset(obj, 0, sizeof(*obj))` on a polymorphic C++ object (wipes out the `vptr` at offset 0, causing the next virtual call to jump to `$00000000`!).

## Exercises

1. **Exercise 1** (`exercises/ex1.cpp` / `exercises/ex1.x68` / `exercises/ex1.toml`): Dispatch through the `vptr` at `0(A0)` (`A0 = $2000`) to call slot 0 of the vtable (`triple_fn`) with `this = A0` and return `3 * val` (`30`) in `D0`.
   - Reference solution: `solutions/ex1.cpp` (`solutions/ex1.x68`).

## Going Further

See `C-12-cpp-templates-constexpr` and `C-13-cpp-embedded-patterns` for zero-overhead compile-time polymorphism (templates and CRTP).
