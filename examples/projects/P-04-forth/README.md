# P-04 — Direct-Threaded Forth Interpreter in 68000 Assembly

## Goal
Implement a Direct-Threaded Code (DTC) Forth inner interpreter on the 68000 using `A4` as the Forth Instruction Pointer (`IP`), `A5` as the Parameter Stack Pointer (`PSP`), and `A7` as the Return Stack Pointer (`RSP`).

## Prerequisites
- `L2-08` (Indirect jump dispatch), `L3-03` (`(An)+` / `-(An)` stack discipline), `O-04` (Tight inner loops)

## New Concepts
- Direct-Threaded Code (`NEXT`: `MOVEA.L (A4)+, A0` + `JMP (A0)` — just 2 instructions!)
- Forth primitives (`LIT`, `DUP`, `ADD`, `SUB`, `MUL`, `SWAP`, `HALT_WORD`)
- Colon definitions (`DOCOL` pushes `IP` onto the return stack; `SEMIS` pops `IP` back)

## Predict
Trace the Forth word stream `LIT 7, SQUARE, LIT 5, ADD, HALT_WORD` where `SQUARE` is `: SQUARE DUP MUL ;`. What value is left on top of the data stack?

## Run & Observe
1. Step through `main.x68` watching `A4` (`IP`), `A5` (`PSP`), and `A7` (`RSP`).
2. When `SQUARE` (`DOCOL`) is entered, `A4` is pushed onto `-(A7)` and redirected to `SQUARE_BODY`.
3. When `SEMIS` runs, `A4` is restored from `(A7)+`, and `HALT_WORD` pops `54` (`7*7 + 5`) into `D0` and prints `"54\r\n"`.

## Explain
The 68000's postincrement address registers make it an ideal host for threaded interpreters: `MOVEA.L (A4)+, A0` followed by `JMP (A0)` fetches the next word's code address and jumps to it in 20 clock cycles with zero branch misprediction.

## Common Mistakes
- Using the same stack pointer for both the Forth data stack (`A5`) and return stack (`A7`), causing `DOCOL`/`SEMIS` return addresses to collide with operands.

## Exercises
- **Exercise 1 (`exercises/ex1.x68`)**: Implement the `OVER` primitive (`( a b -- a b a )`) in the DTC Forth engine and run `LIT 6, LIT 10, OVER, MUL, ADD, HALT_WORD` (`10 * 6 + 6 = 66`) into `D0 = 66`.

## Going Further
- Add `BRANCH` and `0BRANCH` (conditional relative IP offset) to support Forth `IF..THEN` and `BEGIN..UNTIL` loops.
