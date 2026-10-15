# Track O — Optimisation

Naive vs optimised pairs measured with the simulator's cycle counter; every example has a cycle budget.

Curriculum and rationale: [`EDUCATION_STRATEGY.md`](../../doc/examples/EDUCATION_STRATEGY.md).

Delivery: E7.

| ID | Example | Concepts | Status |
|----|---------|----------|--------|
| O-01 | measuring | Cycle counter (tasks 30/31), execution log, why measure first | done |
| O-02 | instruction-selection | MOVEQ/ADDQ/SUBQ/LEA/CLR choices, quick forms, EA costs | done |
| O-03 | addressing-costs | Register vs memory operands; d(An) vs (An)+ vs absolute | done |
| O-04 | loops | DBRA, loop inversion, unrolling, Duff's device | done |
| O-05 | block-memory | MOVEM for copy and fill, long vs word transfers | done |
| O-06 | multiply-divide-free | Shifts and adds, reciprocal multiplication, avoiding DIVS | done |
| O-07 | lookup-tables | Trade memory for cycles (sine, popcount, CRC) | done |
| O-08 | fixed-point | 16.16 maths, MULS-based scaling | done |
| O-09 | branchless | Scc, masks, conditional moves without branches | done |
| O-10 | register-allocation | Keeping hot values in registers, MOVEM save/restore minimisation | done |
| O-11 | compiler-vs-hand | Same algorithm: C at -O2/-Os vs hand asm; reading and beating the compiler | done |
| O-12 | c-optimisation | Writing C that compiles well on the 68000 (types, `restrict`, loop forms) | done |
| O-13 | graphics-fast-paths | Fast line/fill routines, double buffering, frame budgets | done |

Directory names follow `<ID>-<name>`, e.g. `L2-06-stack-frames`.
