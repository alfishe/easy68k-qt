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

**Phase 8 (Assembler) — 4/6 tasks complete, 765 tests passing (505 sim + 260 asm).**

### Completed

| Phase | Description | Tests |
|-------|-------------|-------|
| ⋮ | *Phases 0–7 complete (infrastructure, simulator, all opcodes, Trap #15)* | 505 |
| 8.1 | Lexer (all opcodes + directives) | 100 |
| 8.2 | Parser + Symbol Table + Pratt-precedence evaluator | 73 |
| 8.3 | Expression evaluator (matching EVAL.CPP precedence) | 52 |
| 8.4 | Assembler core (two-pass, ORG/EQU/SET/DC/DS/EVEN/ODD/END, DC/DS word-alignment) | 32 |

Deliberate correctness improvements over the original are logged in `PROGRESS.md` — BCD X flag, CHK signed comparison, STOP full 5-step port, shift count modulo-64.

### In Progress

- **8.5** Missing assembler components (11 subtasks):
  - 8.5.1 Instruction table tests & assembler wiring
  - 8.5.2 Code generator: EA encoding + extension words
  - 8.5.3 Code generator: instruction encoding handlers (BUILD.CPP)
  - 8.5.4 MOVEM register list parsing & encoding
  - 8.5.5 Remaining directives (DCB, DC strings, INCLUDE, INCBIN, SECTION, OFFSET, OPT, etc.)
  - 8.5.6 Conditional assembly (IFC/IFNC/IFEQ/IFNE/IFLT/IFLE/IFGT/IFGE/ELSE/ENDC)
  - 8.5.7 Structured control flow (WHILE/FOR/REPEAT/IF/DBLOOP/UNLESS)
  - 8.5.8 Macro processor (MACRO/ENDM, parameter substitution, NARG, IFARG, MEXIT)
  - 8.5.9 Error reporter (43+ specific error codes)
  - 8.5.10 Listing generator (.L68 output)
  - 8.5.11 Object/S-Record output (S0/S1/S2/S3/S8)

### Next Up

- **8.6** Golden assembly tests
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

765 tests across 17 test suites covering memory, addressing modes, the decoder, all instruction groups, flag computation, Trap #15 I/O, lexer, parser, symbol table, expression evaluator, and assembler core.

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

### Scope Exclusions

The following original features are **not ported** (commented out or not applicable):
- MOVES/MOVEC (68010+ instructions) — commented out in original sources
- BINFILE.CPP binary output — dead code in original (not in project build, never called)
- 68020 bit-field instructions — optional, may be added later

## License

GPL-2.0-or-later — consistent with the original EASy68K project.

## Acknowledgments

- **Chuck Kelly** — original EASy68K author and maintainer
- **EASy68K contributor community** — two decades of educational use and improvement

---

*Phase 8 assembler in progress · 765 tests passing · Next: code generator & assembler wiring*
