# EASy68K Port Execution Plan

This document is the step-by-step execution plan for the EASy68K cross-platform port. It is ordered for correctness: each task produces a compilable, testable state with explicit quality gates.

Companion documents:
- [Porting Proposal](../porting_proposal.md) — architecture, interfaces, and design decisions
- [Code Transfer Plan](../code_transfer_plan.md) — file-by-file transfer specifications from EASy68K-qt (Phases 1–2 only)

### Source Transition Point

This plan has a deliberate **source transition** at the boundary between Phase 2 and Phase 3:

- **Phases 0–2** transfer salvageable code from `EASy68K-qt` per the [Code Transfer Plan](../code_transfer_plan.md). Only types.h, the Memory class, and the S-Record loader are transferred — these are the components assessed as sound.

- **Phases 3–9** port directly from the **original EASy68K Borland sources** (`EASy68K/Sim68K/` and `EASy68K/Edit68K/`). EASy68K-qt is used only as a secondary cross-reference where noted. The original sources are the authoritative reference for behavioral parity — every opcode, every flag, every Trap #15 task must match what the original `CODE1.CPP`–`CODE9.CPP`, `RUN.CPP`, `SCAN.CPP`, and assembler files do.

**Rationale:** EASy68K-qt has fatal architectural flaws (sentinel EA addresses, linear instruction decoder, incomplete flag computation, toy-level I/O callbacks, Cpu-owns-Memory, stub assembler components). Continuing to use it as the primary source beyond the salvageable skeleton would propagate these bugs. The original Borland code, despite its VCL coupling and global state, contains proven-correct instruction logic that has been tested by thousands of students over two decades.

---
