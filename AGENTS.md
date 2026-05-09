# EASy68K Port — Agent Guidance

This file governs how AI agents work on this project. Every instruction here is mandatory. Deviation produces broken builds, incorrect flags, or behavioral parity failures.

---

## 1. Core Principle: Incremental, Verified Progress

**Never change more than one concern at a time.** Each work session must produce a compilable, testable state. If you cannot compile and test after your change, the change is too broad.

The execution plan has 13 phases with 50+ tasks and explicit quality gates. **You work on ONE task at a time.** You do not skip ahead. You do not combine tasks.

### Mandatory Workflow Per Task

1. **Identify current task** — Read `doc/execution_plan.md`. Find the current task by number. Read its complete specification including quality gate.
2. **Create subtasks** — Break the task into concrete file-level operations using `todo_write`. Each subtask must be a single file edit, a single file creation, or a single test run.
3. **Implement one subtask** — Make the change. Do not touch unrelated files.
4. **Verify immediately** — Run the relevant test, compile check, or lint check before moving to the next subtask.
5. **Pass quality gate** — The task's quality gate must pass before you mark the task complete.

---

## 2. Source Authority

### Source Transition (Critical)

This project has a **hard source boundary** between Phase 2 and Phase 3:

| Phases | Primary Source | Secondary Reference |
|--------|---------------|-------------------|
| 0–2 | `EASy68K-qt/` (code transfer plan) | Original EASy68K (verification only) |
| 3–8 | **Original `EASy68K/`** | `EASy68K-qt/` (C++ idiom reference only, never trust logic) |
| 9–12 | Original `EASy68K/` (GUI/logic mapping) | N/A |

**Rules for Phases 3+:**
- Read the original `.CPP` file first. Understand what it does. Then write modern C++17.
- You may glance at `EASy68K-qt` for C++ syntax patterns, but **never copy its logic**. It has:
  - Sentinel EA addresses (`0xFF000000+`) — FATAL, replaced by `EffectiveAddr`
  - Incomplete flag computation (`UpdateFlags` only sets N/Z) — FATAL, must compute V/C/X
  - Linear instruction decoder — FATAL, replaced by hierarchical dispatch
  - Toy I/O callbacks — FATAL, replaced by 9 decomposed Trap #15 interfaces
  - Cpu-owns-Memory — FATAL, replaced by dependency injection

### Original Source File Map

When porting, use this mapping to find the authoritative source:

| Component | Original File | What It Contains |
|-----------|--------------|-----------------|
| Effective address calc | `EASy68K/Sim68K/SCAN.CPP` | `eff_addr()` — all 12 addressing modes |
| Execution engine | `EASy68K/Sim68K/RUN.CPP` | `runprog()`, `one_inst()`, execution loop |
| Memory access | `EASy68K/Sim68K/UTILS.CPP` | `put()`, `value_of()`, `mem_put()`, `mem_req()` |
| Global state | `EASy68K/Sim68K/extern.h`, `var.h` | `D[]`, `A[]`, `PC`, `SR`, `memory[]` declarations |
| MOVE/MOVEA/MOVEQ | `EASy68K/Sim68K/CODE1.CPP` | `line1()`, `line2()`, `line3()`, `line9()` |
| ADD/SUB/arith | `EASy68K/Sim68K/CODE2.CPP`, `CODE3.CPP` | `line8()`, `line9()` arithmetic |
| AND/OR/EOR/logic | `EASy68K/Sim68K/CODE4.CPP` | `line8()`, `line0()` logic |
| BRA/Bcc/branch | `EASy68K/Sim68K/CODE5.CPP`, `CODE6.CPP` | `line6()` branches |
| CHK/TRAP/STOP/misc | `EASy68K/Sim68K/CODE7.CPP` | `line4()`, `line7()` miscellaneous |
| Shift/Rotate | `EASy68K/Sim68K/CODE8.CPP` | `lineE()` shifts |
| Trap #15 (ALL I/O) | `EASy68K/Sim68K/CODE9.CPP` | `TRAP()` — 50+ task labels, all I/O |
| Memory viewer | `EASy68K/Sim68K/Memory1.cpp` | Hex/ASCII viewer, editing, copy/fill |
| Stack viewer | `EASy68K/Sim68K/Stack1.cpp` | Stack display with A7 highlighting |
| Hardware window | `EASy68K/Sim68K/hardwareu.cpp` | 7-seg, LEDs, switches, push buttons |
| I/O console | `EASy68K/Sim68K/simIOu.cpp` | Trap #15 text I/O form |
| Logging | `EASy68K/Sim68K/logU.cpp` | Execution log + output log |
| Main simulator UI | `EASy68K/Sim68K/SIM68Ku.cpp` | Main form, listing display, controls |
| Memory ranges | `EASy68K/Sim68K/SIMOPS2.CPP` | Protected/Invalid/ROM range definitions |
| Assembler engine | `EASy68K/Edit68K/asm.h`, `ASSEMBLE.CPP` | Jump-table dispatch, two-pass assembly |
| Directives | `EASy68K/Edit68K/DIRECTIV.CPP` | ORG, EQU, DC, DS, INCLUDE, etc. |
| Code generation | `EASy68K/Edit68K/CODEGEN.CPP` | Instruction encoding |
| Expression eval | `EASy68K/Edit68K/EVAL.CPP` | Operator precedence, arithmetic |
| Macros | `EASy68K/Edit68K/MACRO.CPP` | MACRO/MEND, arguments, local labels |
| Structured flow | `EASy68K/Edit68K/STRUCTURED.CPP` | IF/THEN/ELSE, WHILE, FOR, SWITCH |
| Listing output | `EASy68K/Edit68K/LISTING.CPP` | .L68 listing format |
| Instruction table | `EASy68K/Edit68K/INSTTABL.CPP` | Opcode lookup, `INSTLOOK.CPP` |
| MOVEM encoding | `EASy68K/Edit68K/MOVEM.CPP` | Register list bitmask |
| Binary output | `EASy68K/Edit68K/BINFILE.CPP` | S-Record generation |

---

## 3. Scope Control Rules

### Rule 1: One Task, One Concern

When working on Task 6.2 (Arithmetic Instructions), you write arithmetic opcode handlers. You do NOT:
- Refactor the Memory class
- Change the EffectiveAddr struct
- Add new Trap #15 interface methods
- Reorganize the directory structure

If you discover something that needs fixing outside your current scope, **create a subtask note** but do not act on it now. Fix it when its own task comes up.

### Rule 2: Do Not Prematurely Generalize

Write exactly what the task requires. Do not add "helpful" abstractions, convenience methods, or future-proofing that the execution plan does not call for. The plan was designed with dependencies in mind — adding code early creates coupling that blocks later tasks.

### Rule 3: Respect Phase Dependencies

| Phase | Depends On | Cannot Start Until |
|-------|-----------|-------------------|
| 1 (Types/Memory) | Phase 0 | Directory tree + CMake exist |
| 2 (S-Record) | Phase 1 | Memory class compiles + tests pass |
| 3 (Addressing) | Phase 1 | EffectiveAddr + Memory exist |
| 4 (Simulator core) | Phase 1, 3 | CpuState, Memory, addressing modes work |
| 5 (Decoder) | Phase 4 | M68kSimulator::Step() works |
| 6 (Opcodes) | Phase 5 | Decoder dispatches correctly |
| 7 (Trap #15) | Phase 4, 6 | Simulator executes instructions |
| 8 (Assembler) | Phase 0 | Independent of simulator phases |
| 9 (Golden traces) | Phase 6, 7 | Full instruction + Trap #15 coverage |
| 10 (Exceptions) | Phase 6 | Opcode handlers generate exceptions |
| 11 (CI/Quality) | Phase 9 | All tests pass |
| 12 (GUI) | Phase 9 | All core libraries proven correct |

### Rule 4: Quality Gates Are Non-Negotiable

Every task ends with a quality gate. If the gate fails, **stop and fix**. Do not proceed to the next task. The progression rule from the execution plan:

> A quality gate must pass before proceeding to the next task. If a gate fails, the failure must be fixed before any subsequent work begins. This prevents compounding errors.

---

## 4. Porting Procedure (Phases 3–8)

When porting from the original Borland sources, follow this exact procedure for every function:

### Step 1: Read the Original

Read the entire original `.CPP` function. Understand:
- What global state it reads (`D[]`, `A[]`, `PC`, `SR`, `memory[]`)
- What parameters it takes
- What it writes back
- What flags it sets
- What edge cases it handles

### Step 2: Map to New Architecture

Translate each pattern:

| Original Pattern | New Pattern |
|-----------------|-------------|
| `D[reg]` | `state_.d[reg]` |
| `A[reg]` | `state_.a[reg]` (for A0–A6); `state_.GetSP()` / `state_.SetSP()` (for A7) |
| `A[8]` (SSP) | `state_.ssp` |
| `PC` | `state_.pc` |
| `SR` | `state_.sr` (use `SetFlag`/`GetFlag` helpers) |
| `memory[addr]` | `memory_.ReadByte(addr)` / `memory_.WriteByte(addr, val)` |
| `eff_addr(mode, reg, size)` | `CalculateEA(mode, reg, size)` → returns `EffectiveAddr` |
| `put(ea, value, size)` | `WriteEA(ea, value, size)` — switches on `ea.target` |
| `value_of(ea, size)` | `ReadEA(ea, size)` — switches on `ea.target` |
| `mem_put(loc, val, size)` | `memory_.Write(loc, val, size)` |
| `mem_req(loc, size)` | `memory_.Read(loc, size)` |
| `long*` EA pointer | **NEVER.** Use `EffectiveAddr` tagged union. |
| `IsDataRegEA(ea)` / sentinel | `ea.target == EaTarget::kDataReg` |
| `AnsiString` | `std::string` or `std::string_view` |
| `#include <vcl.h>` | Delete entirely |
| `Application->ProcessMessages()` | Not needed (QThread model) |
| `Sleep(n)` | `std::this_thread::sleep_for` or `QThread::msleep` |
| `__fastcall` | Delete (no Borland calling convention) |
| `TColor(0x00BBGGRR)` | `uint32_t` RGB value; convert to `QColor` at GUI boundary |
| `StrToInt("0x" + s)` | `std::stoul(s, nullptr, 16)` |

### Step 3: Write New Code

Write the function from understanding, not by copy-pasting the original and doing search-replace. The original's patterns (pointer-range discrimination, global state, VCL types) are fundamentally incompatible with the new architecture. Direct copy-paste will produce code that looks right but is wrong.

### Step 4: Cross-Reference EASy68K-qt (Optional)

If you are unsure about C++ idioms for a particular operation, you may read the corresponding `EASy68K-qt` file. But:
- Do NOT trust its flag computation
- Do NOT copy its sentinel EA patterns
- Do NOT copy its `WriteToEA` free function
- Treat it as a rough draft, not a reference implementation

### Step 5: Write Tests

Every ported function must have at least one test before the quality gate is checked. See Section 5.

---

## 5. Testing Requirements

### When to Write Tests

- **After each file is ported** — Before moving to the next file in the same task.
- **Before declaring a task complete** — The quality gate requires it.

### Test Framework

- **Google Test** (`gtest`) for all core library tests.
- Test files go in `tests/sim/` or `tests/asm/` matching the source structure.
- Each test file is named after the source file it tests: `memory_test.cc` tests `memory.cc`.

### Test Categories Per Phase

| Phase | Minimum Tests | Critical Focus |
|-------|-------------|----------------|
| 1 | Memory: 10+, EffectiveAddr: 5+ | Big-endian byte ordering, alignment, invalid access |
| 2 | S-Record: 14+ | Checksum validation, all record types, malformed input |
| 3 | Addressing: 20+ | Every mode produces correct `EaTarget`, register vs memory |
| 4 | Simulator: 18+ | Reset vectors, exception stack frames, flag computation |
| 5 | Decoder: 12+ | All opcode groups, SIMHALT, unknown opcodes |
| 6 | Opcodes: 165+ | **Most critical phase** — every instruction, every flag |
| 7 | Trap #15: 50+ | Every task number with mock interfaces |
| 8 | Assembler: 50+ | Every directive, golden assembly tests |

### Flag Computation Testing (Phase 6 — CRITICAL)

Incorrect flags will silently produce wrong program behavior. For every arithmetic/logic instruction, test with operand combinations that exercise:

- **Carry (C):** Result exceeds size mask
- **Overflow (V):** Signed result exceeds representable range
- **Extend (X):** Matches carry for arithmetic, unchanged for logic
- **Negative (N):** MSB of result is set
- **Zero (Z):** Result is zero

The `flag_computation_test.cc` file (Task 6.7) with 60+ test cases is the single most important quality gate in the entire project.

---

## 6. Build and Lint Commands

These commands must pass after every subtask that modifies source files:

```bash
# Configure (run once after CMakeLists changes)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build (run after every source change)
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure

# Run specific test suite
ctest --test-dir build -R memory_test
ctest --test-dir build -R opcodes_arithmetic_test

# Format check (must pass before commit)
find include/easym68k src tests -name '*.cc' -o -name '*.h' | xargs clang-format --dry-run --Werror

# Format fix
find include/easym68k src tests -name '*.cc' -o -name '*.h' | xargs clang-format -i

# ASan build and test
cmake -B build-asan -DENABLE_ASAN=ON -DENABLE_UBSAN=ON
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

---

## 7. Coding Conventions

### Dual Convention System

| Layer | Style Guide | Naming |
|-------|-----------|--------|
| `include/easym68k/`, `src/`, `tests/` | Google C++ Style Guide | `snake_case` functions/variables, `PascalCase` types, `kConstant` |
| `gui/` | Qt conventions | `PascalCase` methods, `camelCase` variables, `m_` prefix, signals/slots |

### Key Rules

- **C++17 minimum** — Use `std::variant`, `std::optional`, `std::string_view`, structured bindings, `if constexpr`.
- **No exceptions in core libraries** — Error propagation uses `SimResult` enum and `MemoryAccessResult` struct.
- **No RTTI in hot paths** — The `EffectiveAddr` tagged union uses `EaTarget` enum discriminator, not `dynamic_cast`.
- **Namespace:** `easym68k::sim` for simulator, `easym68k::asm` for assembler.
- **Include guards:** `EASY68K_SIM_TYPES_H_` format (project + path + file + `_H_`).
- **File extensions:** `.h` for headers, `.cc` for source.
- **clang-format** — Always run before committing. Config is in `.clang-format` at project root.

---

## 8. Subtask Tracking Template

When starting a task, create a `todo_write` with subtasks like this:

```
Task 3.2 — Implement addressing_mode.cc
├── 3.2a: Write CalculateEA for modes 0-1 (Dn, An)
├── 3.2b: Write CalculateEA for modes 2-4 (indirect, postinc, predec)
├── 3.2c: Write CalculateEA for modes 5-6 (displacement, indexed)
├── 3.2d: Write CalculateEA for mode 7 (absolute, PC-rel, immediate)
├── 3.2e: Write ReadEA for all EaTarget variants
├── 3.2f: Write WriteEA for all EaTarget variants
├── 3.2g: Compile and fix errors
├── 3.2h: Run addressing_mode_test.cc
└── 3.2i: Pass Quality Gate 3.2
```

Each subtask is atomic — it compiles or it tests. Never combine two subtasks into one edit.

---

## 9. Common Mistakes to Avoid

### Mistake 1: Copying from EASy68K-qt

**Wrong:** Copy `opcodes_arithmetic.cc` from EASy68K-qt, then try to fix sentinel EA and incomplete flags.

**Right:** Read `EASy68K/Sim68K/CODE2.CPP` + `CODE3.CPP`. Understand the original logic. Write new `opcodes_arithmetic.cc` that uses `EffectiveAddr`, complete flag computation, and `CpuState` methods.

### Mistake 2: Trusting EASy68K-qt's Flag Computation

EASy68K-qt's `UpdateFlags()` only sets N and Z. It does NOT compute V, C, or X. Any test that passes against EASy68K-qt's flags is not a valid test. You must verify against the M68000 Programmers Reference Manual.

### Mistake 3: Using Raw Pointers for EA

The original uses `long*` that points into `D[]`, `A[]`, or `memory[]`. EASy68K-qt uses sentinel addresses `0xFF000000+`. Both patterns are **banned**. Always use `EffectiveAddr` with `EaTarget` discriminator.

### Mistake 4: Forgetting Big-Endian Memory

The Memory class stores bytes in big-endian (68000 native) order. On little-endian hosts (x86, most ARM), `ReadWord`/`ReadLong` must byte-swap. The Memory class handles this internally. Never cast `&memory[addr]` to `uint16_t*` or `uint32_t*` — that is a strict aliasing violation and produces wrong results on ARM64.

### Mistake 5: Touching Multiple Files in One Subtask

If a change to `types.h` requires updating `memory.h`, that is TWO subtasks:
1. Edit `types.h` → compile → verify in isolation
2. Edit `memory.h` → compile → verify tests pass

### Mistake 6: Skipping Quality Gates

Quality gates exist because errors compound. A wrong `SimResult` enum in Phase 1 will produce impossible-to-debug failures in Phase 6. Fix it NOW.

---

## 10. File Change Protocol

Before modifying any file, follow this checklist:

1. **Is this file within the current task's scope?** If not, stop.
2. **Have I read the execution plan task specification?** If not, read it now.
3. **Do I understand what the original source does?** If not, read it now.
4. **Am I changing one concern?** If the edit touches two unrelated things, split it.
5. **Will this compile after my change?** If uncertain, compile after the edit.
6. **Does the relevant test still pass?** If not, fix before proceeding.
7. **Does clang-format pass?** If not, format before proceeding.

---

## 11. Key Architectural Decisions (Immutable)

These decisions are made and documented in the porting proposal. Do not revisit them during implementation:

1. **Memory byte order:** Big-endian (68000 native). All `Read`/`Write` methods byte-swap on LE hosts.
2. **EffectiveAddr:** Tagged union with `EaTarget` discriminator. No sentinel addresses. No raw pointer EA.
3. **SimResult:** Error propagation enum. No exceptions in core libraries.
4. **Trap #15 decomposition:** 9 focused interfaces (ITextIO, IFileIO, ISerialIO, INetworkIO, IGraphicsIO, ISoundIO, IPeripheralIO, ISimulatorEnv, IPrintIO) + ILogger.
5. **Threading model:** Dedicated QThread with `BlockingQueuedConnection` for synchronous ops (text input, file dialogs) and `QueuedConnection` for async ops (text output, graphics, sound).
6. **C++ standard:** C++17 minimum.
7. **Help system:** Out of scope. Use `QDesktopServices::openUrl()` only.
8. **Sound:** Qt-only. `QMediaPlayer` for single-voice, `QSoundEffect` for multi-voice. No DirectX, no SDL.
9. **USP/SSP:** Separate `state_.usp` and `state_.ssp` fields, not `a[8]` index hack.
10. **SIMHALT:** Opcode `0xFFFF` is a first-class instruction returning `SimResult::kHalted`.

---

## 12. Project Directory Reference

```
EASy68k-port/
├── AGENTS.md              ← You are here
├── doc/
│   ├── porting_proposal.md    ← Architecture, interfaces, design decisions
│   ├── execution_plan.md      ← Task-by-task plan with quality gates
│   ├── code_transfer_plan.md  ← EASy68K-qt transfer spec (Phases 0–2 only)
│   └── original-ui/           ← Screenshots of original Windows UI
└── (easym68k/ project root will be created in Phase 0)

EASy68K/                    ← Original Borland source (authoritative for Phases 3+)
├── Sim68K/                    ← Simulator source
└── Edit68K/                   ← Assembler source

EASy68K-qt/                 ← Prior porting attempt (secondary reference only)
```
