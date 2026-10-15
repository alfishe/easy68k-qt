# P-01 — Calculator: Reverse Polish Notation (RPN) Stack Engine

## Goal
Build a complete Reverse Polish Notation (RPN) calculator that tokenizes an input line, pushes decimal integers onto a dedicated operand stack (`A3`), and dispatches operators (`+`, `-`, `*`, `/`, `=`) to compute and print results.

## Prerequisites
- `L2-06` (Stack frames), `L2-08` (Jump tables), `S-01` (Advanced text I/O)

## New Concepts
- Dedicated operand stack pointer (`A3`) separate from the hardware call stack (`A7`)
- Single-pass whitespace-delimited tokenizer and decimal accumulator
- Operator dispatch for binary arithmetic (`+`, `-`, `*`, `/`) and output (`=`)

## Predict
For the RPN expression `"12 8 + 5 * 4 - ="`, trace the operand stack after each token. What value is on top when `=` executes?

## Run & Observe
1. Run `main.x68` with input `12 8 + 5 * 4 - =`.
2. Observe `96` printed on the console and `D0 = 96` (`$60`) on halt.
3. Inspect the operand stack buffer at `$3000` in the Memory Window.

## Explain
`12` and `8` are pushed; `+` pops both and pushes `20`; `5` is pushed and `*` replaces `20, 5` with `100`; `4` is pushed and `-` computes `100 - 4 = 96`; `=` pops `96`, prints it via Trap #15 task 3, and leaves it in `D0`.

## Common Mistakes
- Reversing operand order on non-commutative `-` and `/`: when popping `b` then `a`, compute `a - b` (`SUB.L b, a`), not `b - a`.
- Using `A7` for the RPN data stack while inside `BSR` subroutines (which overwrites return addresses).

## Exercises
- **Exercise 1 (`exercises/ex1.x68`)**: Add the unary `n` (negate top-of-stack) operator to the RPN evaluator and evaluate `"15 3 * n 50 + ="` (`-(15 * 3) + 50 = 5`) into `D0 = 5` and output `"5\r\n"`.

## Going Further
- Add fixed-point Q16.16 decimal parsing (`.` decimal point) or `d` (dup) and `s` (swap) stack words.
