# C-13 — Embedded C++ Patterns: Placement new, Static Allocation, and CRTP Hardware Drivers

**Track:** `c` | **Level:** 4 | **Difficulty:** ●●●● expert
**Prerequisites:** `C-11`, `C-12`, `S-07`

## Goal

Write bare-metal C++17 without a heap (`malloc`/`operator new`) using placement `new` into aligned static buffers and Curiously Recurring Template Pattern (`CRTP`) drivers for the EASy68K hardware window.

## New Concepts

- Placement `new`: `inline void* operator new(size_t, void* p) noexcept { return p; }` constructs a C++ object directly at a chosen memory address (`$2000`) with zero heap allocator
- CRTP (`template <typename Derived> struct HwDevice`) provides polymorphic driver interfaces resolved at compile time (no `vptr`, no indirect `JSR`)
- Type-safe MMIO register wrappers for `$E00000` (7-segment) and `$E00010` (LEDs)

## Predict (Before You Run)

When we placement-construct `DeviceState` at `$2000` (`magic = $68000C13`) and use the CRTP `Seg7LedDriver` to display `'7'` (`$07`) on 7-seg digit 0 (`$E00000`) and `$C3` on the LEDs (`$E00010`), what is at `$2000` and `$E00010`?

## Run & Observe

1. Compare `main.cpp` and the annotated compiler output `main.x68` in **Edit68K**, or load `main.S68` in **Sim68K**.
2. Headless check: `easy68k-run examples/c/C-13-cpp-embedded-patterns`.
- Inspect `$2000` (`68 00 0C 13`), `$E00000` (`07`), `$E00010` (`C3`), and `D0 = $C3` (`195`).

## Explain

Placement `new` gives full C++ constructor/destructor lifecycle guarantees over fixed physical memory addresses, while CRTP eliminates vtable indirection so hardware writes compile to single `MOVE.B` instructions.

## Common Mistakes

- Calling `delete p;` on an object created with placement `new` (instead, invoke the destructor explicitly `p->~T();` if needed).

## Exercises

1. **Exercise 1** (`exercises/ex1.cpp` / `exercises/ex1.x68` / `exercises/ex1.toml`): Placement-construct a 2-word `Packet{0x1234, 0x5678}` at `$2000`, write `$3F` (`'0'`) to `$E00000`, and return `0x1234 + 0x5678 = 0x68AC` in `D0`.
   - Reference solution: `solutions/ex1.cpp` (`solutions/ex1.x68`).

## Going Further

See `P-06-cpp-game-framework` for a complete freestanding C++17 game framework.
