# EASy68K Port Execution Plan

<!-- mdsplit-index -->
| Section | Description |
|---|---|
| [Preamble](execution_plan/_preamble.md) | Preamble |
| [Feature Coverage Matrix](execution_plan/feature-coverage-matrix.md) | Maps every EASy68K simulator feature to execution plan tasks. Gaps identified during analysis are marked with remediation tasks. |
| [Phase 0: Project Infrastructure](execution_plan/phase-0-project-infrastructure.md) | Create the project root and every directory. No source files yet — just the skeleton. |
| [Phase 1: Types and Memory — EASy68K-qt Transfer (Code Transfer Plan Sections 3.1, 3.2)](execution_plan/phase-1-types-and-memory-easy68k-qt-transfer-code-transfer-plan-sections-31-32.md) | Source: `EASy68K-qt/src/core/simulator/types.h` → Target: `include/easym68k/sim/types.h` |
| [Phase 2: S-Record Loader — EASy68K-qt Transfer (Code Transfer Plan Section 3.7)](execution_plan/phase-2-s-record-loader-easy68k-qt-transfer-code-transfer-plan-section-37.md) | Source: `EASy68K-qt/src/core/simulator/srecord_loader.cc` |
| [Phase 3: Addressing Modes — Port from Original (EASy68K/Sim68K/SCAN.CPP)](execution_plan/phase-3-addressing-modes-port-from-original-easy68ksim68kscancpp.md) | New file: `include/easym68k/sim/addressing_mode.h` |
| [Phase 4: Simulator Core — Port from Original (EASy68K/Sim68K/RUN.CPP, extern.h, var.h)](execution_plan/phase-4-simulator-core-port-from-original-easy68ksim68kruncpp-externh-varh.md) | New file: `include/easym68k/sim/simulator.h` |
| [Phase 5: Instruction Decoder — Port from Original (EASy68K/Sim68K/CODE1-CODE8.CPP)](execution_plan/phase-5-instruction-decoder-port-from-original-easy68ksim68kcode1-code8cpp.md) | New file: `src/sim/decode.cc` |
| [Phase 6: Opcode Implementation — Port from Original (EASy68K/Sim68K/CODE1-CODE9.CPP)](execution_plan/phase-6-opcode-implementation-port-from-original-easy68ksim68kcode1-code9cpp.md) | All opcode implementations ported from the original EASy68K Borland sources. `CODE1.CPP`–`CODE9.CPP` is the authoritative reference for behavioral parity. |
| [Phase 7: Trap #15 Dispatch — Port from Original (EASy68K/Sim68K/CODE9.CPP)](execution_plan/phase-7-trap-15-dispatch-port-from-original-easy68ksim68kcode9cpp.md) | Create all 9 interface headers in `include/easym68k/sim/`. |
| [Phase 8: Assembler — Port from Original (EASy68K/Edit68K/)](execution_plan/phase-8-assembler-port-from-original-easy68kedit68k.md) | Primary source: `EASy68K/Edit68K/asm.h` — original token definitions and lexer structures. |
| [Phase 9: Simulator Core Library — Full Parity & Headless Readiness](execution_plan/phase-9-simulator-core-library-full-parity-headless-readiness.md) | Closes all remaining gaps between the ported simulator core and full behavioral parity. |
| [Phase 10: Golden Simulation Traces (Proposal Section 5)](execution_plan/phase-10-golden-simulation-traces-proposal-section-5.md) | New file: `tests/sim/goldentracetest.cc` |
| [Phase 11: Exception and Interrupt Tests](execution_plan/phase-11-exception-and-interrupt-tests.md) | New file: `tests/sim/exception_test.cc` |
| [Phase 12: CI and Code Quality](execution_plan/phase-12-ci-and-code-quality.md) | Push CI configs from Task 0.5 and verify pipeline. |
| [Phase 13: GUI Implementation](execution_plan/phase-13-gui-implementation.md) | Implements the three Qt GUI applications. Simulator GUI is the largest; every task references the Feature Coverage Matrix. |
| [Summary: Quality Gate Checklist](execution_plan/summary-quality-gate-checklist.md) | Progression rule: a quality gate must pass before proceeding to the next task. Failures must be fixed before any subsequent work begins. |
<!-- /mdsplit-index -->

<!-- trailing-newline:true -->
