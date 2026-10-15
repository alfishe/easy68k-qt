# Track C — C and C++ on the 68000

Freestanding C11 and C++17 compiled with the GCC cross-compiler, the libeasy68k Trap #15 runtime, calling conventions and reading compiler output.

Curriculum and rationale: [`EDUCATION_STRATEGY.md`](../../doc/examples/EDUCATION_STRATEGY.md).

Delivery: E5 (C), E6 (C++) — in parallel with the core path.

| ID | Example | Concepts | Status |
|----|---------|----------|--------|
| C-01 | toolchain | crt0 (stack setup, BSS clear, call main, exit via task 9), linker script, S-records | done |
| C-02 | types-on-68000 | int/short/long/pointer sizes, alignment, endianness, `sizeof` | done |
| C-03 | calling-convention | Arguments on the stack, return in D0, scratch D0–D1/A0–A1, saved D2–D7/A2–A6, frame pointer | done |
| C-04 | reading-compiler-output | `-S` at -O0/-O1/-O2/-Os; mapping C lines to instructions | done |
| C-05 | pointers-arrays | Pointer arithmetic ↔ addressing modes; `volatile` and memory-mapped I/O | done |
| C-06 | structs-unions | Layout and padding, bitfields vs masks, OFFSET-based asm views | done |
| C-07 | libeasy68k | Writing the Trap #15 runtime: inline asm with constraints | done |
| C-08 | c-calls-asm | Hand-written asm functions called from C (and back) | done |
| C-09 | interrupts-in-c | ISRs in C with an asm wrapper, critical sections, `volatile` | done |
| C-10 | cpp-classes | Freestanding C++17: classes, constructors, RAII; no exceptions/RTTI | done |
| C-11 | cpp-virtual | Virtual dispatch and vtables in memory; cost in cycles | done |
| C-12 | cpp-templates-constexpr | Templates, `constexpr`, zero-cost abstractions verified by cycle count | done |
| C-13 | cpp-embedded-patterns | Placement new, static allocation, CRTP drivers for the hardware window | done |

Directory names follow `<ID>-<name>`, e.g. `L2-06-stack-frames`.
