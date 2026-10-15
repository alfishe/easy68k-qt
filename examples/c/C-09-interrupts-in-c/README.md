# C-09 — Interrupts in C: ISR Wrappers, volatile Shared State, and Critical Sections

**Track:** `c` | **Level:** 4 | **Difficulty:** ●●●● expert
**Prerequisites:** `C-05`, `C-08`, `S-10`

## Goal

Write an Interrupt Service Routine in C safely by saving caller-clobbered registers (`D0–D1/A0–A1`) in an assembly wrapper (or `__attribute__((interrupt))`), marking shared state `volatile`, and guarding critical sections with `SR` interrupt masking.

## New Concepts

- A normal C function ends with `RTS` and freely clobbers `D0–D1/A0–A1`; an ISR must preserve **all** registers and end with `RTE`
- Either wrap the C handler in a 4-line assembly stub (`MOVEM.L D0-D1/A0-A1,-(A7)` / `JSR c_isr` / `MOVEM.L (A7)+,D0-D1/A0-A1` / `RTE`) or annotate it with `__attribute__((interrupt))`
- Shared variables updated by an ISR must be `volatile` so `main()` reloads them from memory

## Predict (Before You Run)

Why must an assembly ISR trampoline save `D0–D1/A0–A1` even though the C function it calls already saves `D2–D7/A2–A6`?

## Run & Observe

1. Compare `main.c` and the annotated compiler output `main.x68` in **Edit68K**, or load `main.S68` in **Sim68K**.
2. Headless check: `easy68k-run examples/c/C-09-interrupts-in-c`.
- Watch `irq2_trampoline` save `D0–D1/A0–A1`, call `c_timer_isr()` (which increments `g_ticks` at `$2000`), restore `D0–D1/A0–A1`, and execute `RTE`.
- Verify `D0 = 1` and `($2000) = 1`.

## Explain

Normal C functions obey the m68k calling convention where `D0–D1/A0–A1` are caller-saved. Because an interrupt can preempt `main()` between any two instructions, the ISR wrapper is the 'caller' and must save `D0–D1/A0–A1`.

## Common Mistakes

- Pointing an interrupt vector directly at a normal C function (clobbers `D0–D1/A0–A1` and executes `RTS` instead of `RTE`).

## Exercises

1. **Exercise 1** (`exercises/ex1.c` / `exercises/ex1.x68` / `exercises/ex1.toml`): Use a `TRAP #0` ISR trampoline at `$0080` that calls a C handler setting `g_shared = 100`, then add `50` inside a critical section (`ORI.W #$0700,SR` .. `ANDI.W #$F8FF,SR`) and return `150` in `D0`.
   - Reference solution: `solutions/ex1.c` (`solutions/ex1.x68`).

## Going Further

See `P-05-tiny-kernel` for full context switching on a timer interrupt.
