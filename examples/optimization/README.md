# Track O — Optimisation

Naive vs optimised pairs measured with the simulator's cycle counter; every example has a cycle budget.

Curriculum and rationale: [`EDUCATION_STRATEGY.md`](../../doc/examples/EDUCATION_STRATEGY.md).

Delivery: E7.

| ID | Example | Concepts | Status |
|----|---------|----------|--------|
| O-01 | measuring | Cycle counter (tasks 30/31), execution log, why measure first | planned |
| O-02 | instruction-selection | MOVEQ/ADDQ/SUBQ/LEA/CLR choices, quick forms, EA costs | planned |
| O-03 | addressing-costs | Register vs memory operands; d(An) vs (An)+ vs absolute | planned |
| O-04 | loops | DBRA, loop inversion, unrolling, Duff's device | planned |
| O-05 | block-memory | MOVEM for copy and fill, long vs word transfers | planned |
| O-06 | multiply-divide-free | Shifts and adds, reciprocal multiplication, avoiding DIVS | planned |
| O-07 | lookup-tables | Trade memory for cycles (sine, popcount, CRC) | planned |
| O-08 | fixed-point | 16.16 maths, MULS-based scaling | planned |
| O-09 | branchless | Scc, masks, conditional moves without branches | planned |
| O-10 | register-allocation | Keeping hot values in registers, MOVEM save/restore minimisation | planned |
| O-11 | compiler-vs-hand | Same algorithm: C at -O2/-Os vs hand asm; reading and beating the compiler | planned |
| O-12 | c-optimisation | Writing C that compiles well on the 68000 (types, `restrict`, loop forms) | planned |
| O-13 | graphics-fast-paths | Fast line/fill routines, double buffering, frame budgets | planned |

Directory names follow `<ID>-<name>`, e.g. `L2-06-stack-frames`.
