# S-10 — Interrupts: Autovectors, Interrupt Masks, Auto-IRQ, and STOP

**Track:** `systems` | **Level:** 4 | **Difficulty:** ●●● advanced
**Prerequisites:** `S-07`, `S-08`

## Goal

Install a Level 1–7 autovector interrupt service routine (`$0064–$007C`), configure the SR interrupt mask, wake the CPU from `STOP` on an interrupt, and return cleanly with `RTE`.

## New Concepts

- Autovector `L` (`1..7`) is stored at address `4 * (24 + L) = $60 + 4*L` (Level 1 = `$64`, ..., Level 7 = `$7C`)
- `SR` bits 10..8 hold the interrupt priority mask (`0..7`); levels `> mask` (and non-maskable Level 7) are serviced
- `STOP #imm` loads `SR` and halts the CPU until an unmasked interrupt arrives
- Task 32 subtask 6 (`D1.B = 6`, `D2.B = $80 | level`, `D3.L = interval_ms`) configures automatic interrupts

## Predict (Before You Run)

At what vector table address does the 68000 look up the Level 2 autovector ISR (`24 + 2 = 26`)?

## Run & Observe

1. Open `main.x68` in **Edit68K**, assemble (`F9`), and launch in **Sim68K**.
2. Headless check: `easy68k-run examples/systems/S-10-interrupts`.
- Inspect `$0068` (`26 * 4`) in the Memory window.
- Watch `STOP #$2000` wait until the Level 2 interrupt fires, runs `IRQ2_ISR` (`D6 = 1`), and resumes after `STOP`.

## Explain

When `STOP #$2000` executes, the interrupt mask is lowered to `0` and the processor waits. As soon as Level 2 IRQ fires, the CPU pushes `PC` and `SR`, raises the mask to `2`, runs `IRQ2_ISR` at `$0068`, and `RTE` resumes at the instruction after `STOP`.

## Common Mistakes

- Forgetting to save and restore scratch registers (`MOVEM.L`) in an asynchronous ISR.
- Leaving the `SR` interrupt mask at `7`, which blocks Levels 1–6.

## Exercises

1. **Exercise 1** (`exercises/ex1.x68` / `exercises/ex1.toml`): Install a Level 4 autovector ISR at `$0070` (`4 * (24 + 4)`) that sets `D0 = 42`, enable auto-IRQ Level 4 via task 32 subtask 6 (`D2.B = $84`), and execute `STOP #$2000`.
   - Reference solution: `solutions/ex1.x68`.

## Going Further

See `P-05-tiny-kernel` for preemptive context switching on a timer interrupt.
