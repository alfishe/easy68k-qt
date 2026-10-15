# L3-07 — Custom Stacks and Power-of-2 Ring Buffer Queues

**Track:** `core` · **Level:** 3 · **Difficulty:** ●●● · **Requires:** L3-02

## Goal

Implement a LIFO stack using a dedicated address register (`-(A2)` / `(A2)+`) and a FIFO circular ring buffer using head/tail indices masked by `CAPACITY - 1`.

## New concepts

- Any address register `A0–A6` can serve as a dedicated LIFO data stack with `-(An)` (push) and `(An)+` (pop).
- A FIFO ring buffer of size $2^k$ wraps its `head` and `tail` indices in one instruction: `ANDI.W #(CAP-1),Dn`.

## Predict

Before running, write down your answers:

1. If `10, 20, 30` are pushed onto a LIFO stack (`-(A2)`) and popped once (`(A2)+`), which value is popped (`30` or `10`)?
2. If `10, 20, 30` are enqueued into a FIFO ring buffer and dequeued once, which value is dequeued (`10` or `30`)?

## Run & observe

1. Assemble `main.x68` in Edit68K-Qt (`F9`) and open the `.S68` in Sim68K-Qt.
1. Step through `main.x68` and compare `D0 = 30` (LIFO pop) vs `D1 = 10` (FIFO dequeue).

## Explain

Keeping the ring buffer capacity a power of 2 (e.g. 8 bytes) turns modulo wraparound (`index % 8`) into a 4-cycle bitwise mask (`ANDI.W #7,Dn`).

## Common mistakes

- Using `MOD`/`DIVU` instead of bitwise `AND` for ring buffer wraparound wastes ~140 cycles per operation!

## Exercises

1. ●●● Push `100` and `200` onto a stack at `A2 = $2100` and pop both so `D0 = 200` and `D1 = 100`. — starter: `exercises/ex1.x68`, spec: `exercises/ex1.toml`

Solutions are in `solutions/` (try first!).

## Going further

- S-10 (`interrupts`), P-04 (`forth`).
