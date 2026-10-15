# Core Path — Levels 0–3

Pure assembly, from your first instruction to data structures and program structure.

Curriculum and rationale: [`EDUCATION_STRATEGY.md`](../../doc/examples/EDUCATION_STRATEGY.md).

Delivery: E2 (Levels 0–1), E3 (Levels 2–3).

| ID | Example | Concepts | Status |
|----|---------|----------|--------|
| L0-01 | hello-simhalt | Program skeleton: ORG, START, END, SIMHALT | done |
| L0-02 | registers | MOVEQ/MOVE to D and A registers | done |
| L0-03 | memory | DC/DS, MOVE to and from memory, big-endian layout | done |
| L0-04 | hello-world | Trap #15 tasks 13/14, strings, LEA | done |
| L0-05 | reading-the-listing | .L68 listing, addresses, opcodes, S-records | done |
| L0-06 | stepping | Step / Trace / Run / breakpoints / Rewind | done |
| L1-01 | sizes | .B/.W/.L, sign extension, EXT, partial register updates | done |
| L1-02 | flags | CCR: N Z V C X; predict the flags for ADD/SUB/CMP | done |
| L1-03 | signed-unsigned | Two's complement, overflow vs carry | done |
| L1-04 | multiword | 64-bit add/sub with ADDX/SUBX and the X flag | done |
| L1-05 | mul-div | MULU/MULS/DIVU/DIVS, remainders, divide by zero (exception preview) | done |
| L1-06 | logic-bits | AND/OR/EOR/NOT, BTST/BSET/BCLR/BCHG, masks | done |
| L1-07 | shifts-rotates | ASL/ASR/LSL/LSR/ROL/ROR/ROXL/ROXR; multiply/divide by 2^n | done |
| L1-08 | bcd | ABCD/SBCD/NBCD, decimal display | done |
| L1-09 | number-io | Trap #15 tasks 3/4/15/17/20; radix conversion by hand vs task 15 | done |
| L2-01 | compare-branch | CMP/TST and all 14 Bcc conditions; signed vs unsigned branches | done |
| L2-02 | loops-dbra | DBRA/DBcc, loop counters, off-by-one traps | done |
| L2-03 | structured | EASy68K IF/ELSE/ENDI, WHILE, REPEAT, FOR, DBLOOP; inspect the generated code | done |
| L2-04 | subroutines | BSR/JSR/RTS, return addresses on the stack | done |
| L2-05 | parameters | Registers vs stack parameters; saving registers with MOVEM | done |
| L2-06 | stack-frames | LINK/UNLK, locals, frame pointer; Stack window | done |
| L2-07 | recursion | Factorial and Fibonacci; stack growth, cost | done |
| L2-08 | jump-tables | Computed JMP, d(PC,Xi), switch statements | done |
| L3-01 | addressing-modes | All 12 modes side by side with cycle costs | done |
| L3-02 | arrays | Indexing with (An)+, d(An,Xi); 2-D arrays | done |
| L3-03 | strings | Length, copy, compare, reverse, upper-case; null- vs length-prefixed | done |
| L3-04 | sorting | Bubble → insertion → quicksort, with cycle comparison | done |
| L3-05 | searching | Linear vs binary search | done |
| L3-06 | linked-lists | Nodes, insert/delete, pointer chasing | done |
| L3-07 | stack-queue | Ring buffers, -(An)/(An)+ stacks | done |
| L3-08 | macros | MACRO/ENDM, parameters, \@, NARG, building a mini-library | done |
| L3-09 | conditional-assembly | IFEQ/IFNE…, debug builds, OPT/listing control | done |
| L3-10 | multi-file | INCLUDE, INCBIN, SECTION/OFFSET for structs | done |
| L3-11 | position-independence | PC-relative code, relocatable routines | done |

Directory names follow `<ID>-<name>`, e.g. `L2-06-stack-frames`.
