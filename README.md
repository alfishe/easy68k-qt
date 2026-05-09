# EASy68K Cross-Platform Port

A modern C++17 port of the [EASy68K](http://www.easy68k.com/) Motorola 68000 simulator and assembler ([original source](https://github.com/ProfKelly/EASy68K)), bringing the classic educational tool to contemporary platforms with improved architecture and comprehensive testing.

This repository is based on the unpacked Borland C++Builder sources from [alfishe/EASy68k](https://github.com/alfishe/EASy68k).

## Goals

- **Cross-platform** — Windows, macOS, Linux
- **Modern C++17** — clean architecture, no exceptions in core, RAII throughout
- **Verified correctness** — every instruction tested against original EASy68K sources
- **Qt6 GUI** — faithful to the original educational interface
- **Bug-for-bug parity by default** — deliberate fixes for known original bugs are logged in PROGRESS.md

## Current Status

**Phase 6 (Opcode Implementation) — 6/7 tasks complete, 336 tests passing.**

### Completed

| Phase | Description | Tests |
|-------|-------------|-------|
| 0 | Project infrastructure, CMake, clang-format | — |
| 1 | Types, Memory, EffectiveAddr tagged union | 22 |
| 2 | S-Record loader (.S68/.S19/.S3) | 16 |
| 3 | All 12 addressing modes, CpuState (SSP/USP) | 37 |
| 4 | Simulator core (Reset/Step/Run, HandleException) | 26 |
| 5 | Hierarchical instruction decoder (16 groups) | 14 |
| 6.1 | Move instructions (MOVE/MOVEA/MOVEQ/LEA/PEA/SWAP/EXG) | 37 |
| 6.2 | Arithmetic (ADD/SUB/MUL/DIV/NEG/CMP/TST/EXT/BCD/CHK) | 69 |
| 6.3 | Logic (OR/AND/EOR + immediate + CCR/SR, NOT, TAS) | 40 |
| 6.4 | Branch (BRA/BSR/Bcc/DBcc/Scc/JMP/JSR/RTS/RTE/RTR) | 32 |
| 6.5 | Miscellaneous (TRAP/TRAPV/ILLEGAL/RESET/STOP) | 19 |
| 6.6 | Shift/Rotate (ASL/ASR/LSL/LSR/ROL/ROR/ROXL/ROXR + BTST/BCHG/BCLR/BSET) | 25 |

Deliberate correctness improvements over the original are logged in `PROGRESS.md` — BCD X flag, CHK signed comparison, STOP full 5-step port, shift count modulo-64.

### Next Up

- **6.7** Flag computation verification suite
- **7** Trap #15 I/O dispatch
- **8** Assembler (lexer, parser, expression evaluator)
- **9** Golden simulation traces
- **10** Exception and interrupt tests
- **11** CI, code coverage, clang-tidy
- **12** Qt6 GUI (simulator, editor, binary utility)

## Architecture

- **Core libraries** (`src/sim/`, `src/asm/`) — zero Qt dependency, exception-free, `SimResult` error propagation
- **GUI** (`gui/`) — Qt6 (Widgets, Multimedia, SerialPort, Network, PrintSupport)
- **Tests** (`tests/`) — Google Test via FetchContent
- **Big-endian memory** with alignment-aware access
- **Tagged-union EffectiveAddr** replaces original's raw pointer arithmetic
- **10 typed flag functions** (Add/Sub/Logic/Move/Shift/Neg/Cmp/AddX/SubX/NegX) replace original's monolithic `cc_update()`

```
EASy68K-port/
├── include/easym68k/     # Public headers
├── src/sim/              # Simulator library
├── src/asm/              # Assembler library
├── gui/                  # Qt6 GUI applications
├── tests/                # Google Test suites
├── doc/                  # Design documents
└── cmake/                # Build helpers
```

## Building

```bash
cmake -B build -DBUILD_GUI=OFF    # core + tests (no Qt6 needed)
cmake --build build
ctest --test-dir build --output-on-failure
```

For the GUI, install Qt6 and drop `-DBUILD_GUI=OFF`. Google Test is fetched automatically via CMake FetchContent.

Sanitized builds:
```bash
cmake -B build-asan -DBUILD_GUI=OFF -DENABLE_ASAN=ON -DENABLE_UBSAN=ON
cmake --build build-asan && ctest --test-dir build-asan --output-on-failure
```

## Testing

336 tests across 13 test suites covering memory, addressing modes, the decoder, all implemented instruction groups, and flag computation.

```bash
ctest --test-dir build --output-on-failure         # all tests
build/tests/easym68k-sim-tests --gtest_filter="ShiftTest*"  # specific suite
```

## Documentation

- **[PROGRESS.md](PROGRESS.md)** — task-level status, commit hashes, decision log
- **[Execution Plan](doc/execution_plan.md)** — full implementation roadmap
- **[Porting Proposal](doc/porting_proposal.md)** — architecture and design decisions
- **[AGENTS.md](AGENTS.md)** — contributor guidelines

## Contributing

1. Read `PROGRESS.md` for current state and next task
2. Follow the execution plan — tasks are in dependency order
3. Write tests alongside every feature
4. Phases 0-2 derive from EASy68K-qt; Phases 3+ port from original Borland sources only
5. Behavioral parity with original EASy68K; deliberate fixes for known bugs are logged as decisions in `PROGRESS.md`
6. No autonomous commits — all changes require human review
7. No `Co-authored-by:` tags in commit messages

## License

GPL-2.0-or-later — consistent with the original EASy68K project.

## Acknowledgments

- **Chuck Kelly** — original EASy68K author and maintainer
- **EASy68K contributor community** — two decades of educational use and improvement

---

*Phase 6 nearly complete · 336 tests passing · Next: flag verification suite*
