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

See [PROGRESS.md](PROGRESS.md) for task status and
[doc/execution_plan/parity-audit.md](doc/execution_plan/parity-audit.md) for
the feature-by-feature parity snapshot.

- **Simulator core** (`src/sim/`) — all 68000 instructions (including bit-field
  instructions), exceptions (including class 0 address/bus error frames),
  autovectored interrupts, cycle counting, memory map semantics, advanced
  register/memory/expression breakpoints, Step Over / Run To, Trap #15
  dispatch, logging, and observer hooks (Phases 1–11 complete).
- **Assembler** (`src/asm/`) — a function-by-function port of the original
  Edit68K assembler (`src/asm/core/`). Listings (.L68) and S-records (.S68)
  match the original on the golden programs and corpus; the instruction matrix
  (98,583 instruction × addressing mode × size cases) matches the original
  assembler (Phase 8 complete).
- **GUI applications** (`gui/`, Phase 13 complete — **69/69** parity items and
  **30/30** Feature Coverage Matrix items verified):
  - `easy68k-editor` (**Edit68K-Qt**): MDI `.X68`/`.L68` editor with 8-category
    syntax highlighting, Assembly/Fixed tab stops, integrated assembler (`F9`),
    clickable error list, simulator launch, printing, and context-sensitive
    `F1` help.
  - `easy68k-simulator` (**Sim68K-Qt**): full simulator GUI with register
    editor, source listing view with breakpoint gutter, I/O console & 640×480
    graphics window (with full-screen mode), Hardware peripheral window (7-seg,
    LEDs, switches, buttons, auto-IRQ, memory map), Memory viewer/editor,
    Stack viewer, Advanced Break Points dialog, Auto Trace, Execution/Output
    logging, and all 9 Qt Trap #15 backends.
  - `easybin` (**EASyBIN-Qt**): S-Record and binary utility with 1/2/4-file
    interleave split modes and color-coded hex/ASCII editor.
  - `easy68k-help` (**HelpViewer**): offline HTML help browser (`contents.hhc`,
    `index.hhk`, `ez68k.ali`, and Quick Reference PDF) shared across all three
    applications.

Correctness is checked against *reference oracles*: the original Edit68K
assembler and Sim68K simulator built on Linux from the original sources
(`-DBUILD_REFERENCE_TOOLS=ON`, `tools/reference/`).

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

Simulator unit tests, assembler tests (unit, golden programs, corpus and
instruction matrix compared with the original assembler) and asm + sim
integration tests. To compare the assembler with the original directly:

```bash
cmake -B build-ref -DBUILD_GUI=OFF -DBUILD_REFERENCE_TOOLS=ON -DBUILD_TESTING=OFF
cmake --build build-ref
tools/reference/compare_asm.sh build-ref program.x68 [-- --mex --sex --cex]
```

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

## License

EASy68K-Qt is licensed under the GNU General Public License, version 2 or
(at your option) any later version (SPDX `GPL-2.0-or-later`). The full GPLv2
text is in [`LICENSE`](LICENSE).

This follows the original EASy68K, which is distributed under the GNU GPL
without a version number (it ships the GPLv2 text; GPLv2 §9 lets the
licensee choose any published version) and includes code explicitly under
GPL-2.0-or-later. The "or later" option allows a move to GPLv3 if needed.

The learning examples under `examples/` are CC0-1.0 (see `examples/LICENSE`).

## Acknowledgments

- **Chuck Kelly** — original EASy68K author and maintainer
- **EASy68K contributor community** — two decades of educational use and improvement

---

*Phases 0–11 & Phase 13 (GUI) complete · 69/69 parity items & 30/30 feature matrix items verified · Next: Examples & Education (E1–E9)*
