# EASy68K Port — Agent Guidance

This file governs how AI agents work on this project. Deviation produces broken builds, incorrect flags, or behavioral parity failures.

## 1. Core Principles
* **One Concern at a Time:** The execution plan (`doc/execution_plan.md`) has explicit tasks and quality gates. Do not combine tasks. Do not skip ahead.
* **Pass Quality Gates:** If a task's quality gate fails, stop and fix it immediately. Errors compound.
* **No Premature Generalization:** Implement exactly what the task requires.
* **Track Progress:** Update `PROGRESS.md` after every task. Read it first when resuming work. It is the single source of truth for what is done and what to do next.
* **No Autonomous Commits:** Never commit code without explicit human review and confirmation. Present changes for review, wait for approval, then commit only when told to.
* **No Co-Authored Tags:** Never add `Co-authored-by:` or similar AI-attribution tags to commit messages.

## 2. Source Authority (CRITICAL)
This project has a **hard source boundary**:
* **Phases 0–2:** Transfer code from `EASy68K-qt/` as specified in the Code Transfer Plan.
* **Phases 3+:** Port **ONLY** from the original `EASy68K/` Borland sources. 

**WARNING:** Never copy logic from `EASy68K-qt` for Phases 3+. It contains fatal flaws:
* Sentinel EA addresses (`0xFF000000+`) — use `EffectiveAddr` instead.
* Incomplete flag computation — compute V/C/X per the M68000 manual.
* Linear instruction decoder — use hierarchical dispatch.
* Cpu-owns-Memory — use dependency injection.

## 3. Porting Procedure (Phases 3+)
1. Read the original Borland `.CPP` file. Understand the global state it reads/writes (`D[]`, `A[]`, `PC`, `SR`, `memory[]`).
2. Map to the new architecture:
   * `A[reg]` → `state_.a[reg]` (use `state_.GetSP()`/`SetSP()` for A7)
   * `eff_addr()` → `CalculateEA()` returning `EffectiveAddr`
   * `put()` / `value_of()` → `WriteEA()` / `ReadEA()` using `EffectiveAddr`
   * `mem_put()` / `mem_req()` → `memory_.Write()` / `memory_.Read()`
3. Write modern C++17. Eliminate `AnsiString`, `<vcl.h>`, and global variables.

## 4. Fatal Mistakes to Avoid
* **Trusting EASy68K-qt's Flags:** It only sets N and Z. You must implement V, C, and X correctly.
* **Using Raw Pointers for EA:** Never use `long*` pointing into registers or memory. Always use the `EffectiveAddr` tagged union.
* **Little-Endian Assumptions:** Memory is strictly big-endian. The Memory class handles byte-swapping internally. Do not cast memory pointers.

## 5. Architectural Mandates
* **Memory:** Big-endian.
* **Error Handling:** Use `SimResult` enum. No exceptions in core libraries.
* **Trap #15:** Decomposed into 9 interfaces (ITextIO, IFileIO, etc.).
* **Threading:** Simulator runs in a dedicated `QThread`. Use `BlockingQueuedConnection` for synchronous GUI ops.
* **Conventions:** Google C++ Style (`snake_case` methods) for core; Qt Style (`camelCase` methods) for GUI. C++17 minimum.

## 6. Testing & Validation
* Write tests immediately after porting a file using Google Test (`gtest`).
* **Flag Computation Testing is Critical.** Verify N/Z/V/C/X for all arithmetic/logic instructions with edge-case operands.
* Always run `clang-format` before concluding your work.

## 7. Build and Test Commands

All commands run from `/Users/dev/Projects/GitHub/EASy68k-port/`.

```bash
# Configure (first time or after CMakeLists changes)
cmake -B build -DBUILD_GUI=OFF

# Build everything
cmake --build build

# Build a specific target
cmake --build build --target easym68k-sim
cmake --build build --target easym68k-sim-tests

# Run all tests
ctest --test-dir build --output-on-failure

# Run a specific test suite by name
ctest --test-dir build -R sim-tests --output-on-failure
ctest --test-dir build -R asm-tests --output-on-failure

# Run a specific GoogleTest case directly (fastest)
build/tests/easym68k-sim-tests --gtest_filter="MemoryTest*"
build/tests/easym68k-asm-tests --gtest_filter="LexerTest*"

# Format check (clang-format is at /opt/homebrew/Cellar/llvm/21.1.8_1/bin/clang-format)
CFMT=/opt/homebrew/Cellar/llvm/21.1.8_1/bin/clang-format
find include/easym68k src tests \( -name '*.cc' -o -name '*.h' \) \
  | xargs $CFMT --dry-run --Werror

# Format in-place
find include/easym68k src tests \( -name '*.cc' -o -name '*.h' \) \
  | xargs $CFMT -i

# ASan + UBSan build
cmake -B build-asan -DBUILD_GUI=OFF -DENABLE_ASAN=ON -DENABLE_UBSAN=ON
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```
