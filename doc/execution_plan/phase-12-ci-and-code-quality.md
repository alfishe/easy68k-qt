## Phase 12: CI and Code Quality

### Task 12.1 — Enable CI

Push CI configs from Task 0.5. Verify:
- Linux (gcc + clang) builds and tests pass
- macOS (clang) builds and tests pass
- Windows (MSVC) builds and tests pass
- ASan/UBSan builds pass on all platforms

**Quality Gate 12.1:** All CI pipelines green.

---

### Task 12.2 — Code Coverage

Add `gcov`/`lcov` to CI. Target:
- `libeasym68k-sim`: ≥90% line coverage
- `libeasym68k-asm`: ≥85% line coverage

**Quality Gate 12.2:** Coverage meets targets on all platforms.

---

### Task 12.3 — clang-tidy

Add `clang-tidy` checks to CI using the config in `ci/clang-tidy.cfg`.

**Quality Gate 12.3:** Zero `clang-tidy` warnings on core library code.
