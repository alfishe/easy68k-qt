# EASy68K Port Execution Plan

This document is the step-by-step execution plan for the EASy68K cross-platform port. It is ordered for correctness: each task produces a compilable, testable state with explicit quality gates.

Companion documents:
- [Porting Proposal](porting_proposal.md) — architecture, interfaces, and design decisions
- [Code Transfer Plan](code_transfer_plan.md) — file-by-file transfer specifications from EASy68K-qt (Phases 1–2 only)

### Source Transition Point

This plan has a deliberate **source transition** at the boundary between Phase 2 and Phase 3:

- **Phases 0–2** transfer salvageable code from `EASy68K-qt` per the [Code Transfer Plan](code_transfer_plan.md). Only types.h, the Memory class, and the S-Record loader are transferred — these are the components assessed as sound.

- **Phases 3–9** port directly from the **original EASy68K Borland sources** (`EASy68K/Sim68K/` and `EASy68K/Edit68K/`). EASy68K-qt is used only as a secondary cross-reference where noted. The original sources are the authoritative reference for behavioral parity — every opcode, every flag, every Trap #15 task must match what the original `CODE1.CPP`–`CODE9.CPP`, `RUN.CPP`, `SCAN.CPP`, and assembler files do.

**Rationale:** EASy68K-qt has fatal architectural flaws (sentinel EA addresses, linear instruction decoder, incomplete flag computation, toy-level I/O callbacks, Cpu-owns-Memory, stub assembler components). Continuing to use it as the primary source beyond the salvageable skeleton would propagate these bugs. The original Borland code, despite its VCL coupling and global state, contains proven-correct instruction logic that has been tested by thousands of students over two decades.

---

## Feature Coverage Matrix

This matrix maps every feature of the original EASy68K simulator to the execution plan tasks that implement it. **Every feature must have at least one task.** Gaps identified during analysis are marked with NEW.

| # | Original Feature | Original Source | Execution Plan Task(s) | Status |
|---|-----------------|-----------------|----------------------|--------|
| 1 | 68000 register display (D0-D7, A0-A7, PC, USP, SSP, SR, Cycles) | SIM68Ku.cpp | Task 13.3 RegisterWidget | Covered |
| 2 | Source code display (.L68 listing mapped to PC) | SIM68Ku.cpp ListBox1, highlight() | Task 13.4 SourceView | **NEW** |
| 3 | Stack viewer (A7 highlight, address register selection, scroll) | Stack1.cpp | Task 13.5 StackWindow | **NEW** |
| 4 | Memory display (hex/ASCII viewer, address jump) | Memory1.cpp FormPaint | Task 13.6 MemoryWindow | Covered |
| 5 | Direct memory editing (click hex/ASCII byte, type new value) | Memory1.cpp FormKeyPress | Task 13.7 Memory Editing | **NEW** |
| 6 | Memory block copy (From/To/Bytes fields, overlap-safe) | Memory1.cpp CopyClick | Task 13.7 Memory Editing | **NEW** |
| 7 | Memory block fill (From/To/FillValue) | Memory1.cpp FillClick | Task 13.7 Memory Editing | **NEW** |
| 8 | Hardware display (7-segment, LEDs, switches, push buttons) | hardwareu.cpp | Task 13.8 HardwareWindow | Covered |
| 9 | Output results (Trap #15 text I/O console) | simIOu.cpp | Task 13.9 ConsoleWidget | Covered |
| 10 | Graphics output (QPainter, 16 raster ops, double buffer) | CODE9.CPP tasks 80-96 | Task 13.10 GraphicsIO | Covered |
| 11 | Sound output (single-voice + multi-voice) | CODE9.CPP tasks 70-77 | Task 13.11 SoundIO | Covered |
| 12 | Interrupts (auto-IRQ, hardware IRQ, autovector) | hardwareu.cpp autoIRQ | Task 13.8 HardwareWindow + Task 11.2 | Covered |
| 13 | Log execution output (instruction + registers to text file) | logU.cpp ElogFile | Task 13.12 LogWindow | **NEW** |
| 14 | Log Trap #15 output (binary file) | logU.cpp OlogFile | Task 13.12 LogWindow | **NEW** |
| 15 | Log memory range dump (configurable from/bytes) | logU.cpp MemFrom/MemBytes | Task 13.12 LogWindow | **NEW** |
| 16 | Load S-Record files (.S68, .S19, .S3) | SIM68Ku.cpp loadSrec | Task 2.1 SRecordLoader + Task 12.2 | Covered |
| 17 | Serial I/O (COM port init, read, send) | CODE9.CPP tasks 40-43 | Task 13.13 SerialIO | Covered |
| 18 | TCP/UDP network I/O (client/server, send/receive) | CODE9.CPP tasks 100-107 | Task 13.14 NetworkIO | Covered |
| 19 | Definable memory ranges (Protected/Invalid/ROM with start/end) | SIMOPS2.CPP + hardwareu.cpp | Task 13.15 MemoryRangeDialog | **NEW** |
| 20 | Breakpoints (PC breakpoints with toggle, clear all) | SIM68Ku.cpp BreaksFrm | Task 4.1 M68kSimulator + Task 13.4 SourceView | Covered |
| 21 | Run/Step/Trace/Pause/Reset controls | SIM68Ku.cpp Run/Step/Trace | Task 12.2 MainWindow | Covered |
| 22 | Run-to-cursor | SIM68Ku.cpp RunToCursorExecute | Task 13.4 SourceView | Covered |
| 23 | *[sim68k] listing commands (break, bitfield, SIMHALT_OFF) | SIM68Ku.cpp lines 310-337 | Task 13.4 SourceView | **NEW** |
| 24 | Printing (character output, form feed) | CODE9.CPP task 10 | Task 13.16 PrintIO | Covered |
| 25 | File I/O (open, read, write, seek, close, delete, dialog) | CODE9.CPP tasks 50-59 | Task 13.17 FileIO | Covered |
| 26 | Peripheral I/O (mouse/keyboard IRQ enable, state read) | CODE9.CPP tasks 60-62 | Task 13.8 HardwareWindow | Covered |
| 27 | Full-screen output window | CODE9.CPP task 33 | Task 13.10 GraphicsIO | Covered |
| 28 | Drag-and-drop file open | SIM68Ku.cpp WmDropFiles | Task 12.2 MainWindow | Covered |
| 29 | Settings persistence (QSettings for window positions, fonts, options) | SIM68Ku.cpp SaveSettings | Task 13.18 SettingsPersistence | Covered |
| 30 | Cycle counter with clear button | SIM68Ku.cpp | Task 13.3 RegisterWidget | Covered |

---

## Phase 0: Project Infrastructure

### Task 0.1 — Create Repository and Directory Tree

Create the project root and every directory. No source files yet — just the skeleton.

```
easym68k/
├── CMakeLists.txt
├── cmake/
│   └── CompilerWarnings.cmake
├── include/
│   └── easym68k/
│       ├── asm/
│       │   ├── assembler.h
│       │   ├── lexer.h
│       │   ├── parser.h
│       │   ├── symbol_table.h
│       │   ├── expression.h
│       │   ├── error_reporter.h
│       │   ├── code_generator.h
│       │   ├── directive_types.h
│       │   ├── instruction_table.h
│       │   ├── listing_generator.h
│       │   └── macro_processor.h
│       └── sim/
│           ├── types.h
│           ├── memory.h
│           ├── effective_addr.h
│           ├── addressing_mode.h
│           ├── simulator.h
│           ├── decode.h
│           ├── srecord_loader.h
│           ├── trap15.h
│           ├── itext_io.h
│           ├── ifile_io.h
│           ├── iserial_io.h
│           ├── inetwork_io.h
│           ├── igraphics_io.h
│           ├── isound_io.h
│           ├── iperipheral_io.h
│           ├── isimulator_env.h
│           ├── iprint_io.h
│           └── ilogger.h
├── src/
│   ├── asm/
│   │   ├── CMakeLists.txt
│   │   ├── assembler.cc
│   │   ├── lexer.cc
│   │   ├── parser.cc
│   │   ├── symbol_table.cc
│   │   ├── expression_evaluator.cc
│   │   ├── error_reporter.cc
│   │   ├── code_generator.cc
│   │   ├── directives.cc
│   │   ├── instruction_table.cc
│   │   ├── listing_generator.cc
│   │   └── macro_processor.cc
│   └── sim/
│       ├── CMakeLists.txt
│       ├── memory.cc
│       ├── addressing_mode.cc
│       ├── simulator.cc
│       ├── decode.cc
│       ├── srecord_loader.cc
│       ├── trap15_dispatch.cc
│       ├── opcodes_move.cc
│       ├── opcodes_arithmetic.cc
│       ├── opcodes_logic.cc
│       ├── opcodes_branch.cc
│       ├── opcodes_misc.cc
│       └── opcodes_shift.cc
├── gui/
│   ├── CMakeLists.txt
│   ├── common/
│   │   ├── hex_spinbox.h
│   │   └── hex_spinbox.cc
│   ├── editor/
│   │   ├── CMakeLists.txt
│   │   ├── main.cc
│   │   ├── mainwindow.h
│   │   ├── mainwindow.cc
│   │   ├── codeeditor.h
│   │   ├── codeeditor.cc
│   │   ├── highlighter.h
│   │   └── highlighter.cc
│   ├── simulator/
│   │   ├── CMakeLists.txt
│   │   ├── main.cc
│   │   ├── mainwindow.h
│   │   ├── mainwindow.cc
│   │   ├── register_widget.h
│   │   ├── register_widget.cc
│   │   ├── source_view.h
│   │   ├── source_view.cc
│   │   ├── stack_window.h
│   │   ├── stack_window.cc
│   │   ├── disassembly_view.h
│   │   ├── disassembly_view.cc
│   │   ├── console_widget.h
│   │   ├── console_widget.cc
│   │   ├── memory_window.h
│   │   ├── memory_window.cc
│   │   ├── log_window.h
│   │   ├── log_window.cc
│   │   ├── memory_range_dialog.h
│   │   ├── memory_range_dialog.cc
│   │   ├── hardware_window.h
│   │   ├── hardware_window.cc
│   │   ├── qt_text_io.h
│   │   ├── qt_text_io.cc
│   │   ├── qt_file_io.h
│   │   ├── qt_file_io.cc
│   │   ├── qt_serial_io.h
│   │   ├── qt_serial_io.cc
│   │   ├── qt_network_io.h
│   │   ├── qt_network_io.cc
│   │   ├── qt_graphics_io.h
│   │   ├── qt_graphics_io.cc
│   │   ├── qt_sound_io.h
│   │   ├── qt_sound_io.cc
│   │   ├── qt_peripheral_io.h
│   │   ├── qt_peripheral_io.cc
│   │   ├── qt_simulator_env.h
│   │   ├── qt_simulator_env.cc
│   │   ├── qt_print_io.h
│   │   └── qt_print_io.cc
│   └── easybin/
│       ├── CMakeLists.txt
│       ├── main.cc
│       ├── mainwindow.h
│       └── mainwindow.cc
├── tests/
│   ├── CMakeLists.txt
│   ├── sim/
│   │   ├── memory_test.cc
│   │   ├── effective_addr_test.cc
│   │   ├── addressing_mode_test.cc
│   │   ├── simulator_test.cc
│   │   ├── decode_test.cc
│   │   ├── srecord_loader_test.cc
│   │   ├── opcodes_move_test.cc
│   │   ├── opcodes_arithmetic_test.cc
│   │   ├── opcodes_logic_test.cc
│   │   ├── opcodes_branch_test.cc
│   │   ├── opcodes_misc_test.cc
│   │   ├── opcodes_shift_test.cc
│   │   ├── flag_computation_test.cc
│   │   ├── exception_test.cc
│   │   ├── trap15_mock_test.cc
│   │   └── golden_trace_test.cc
│   └── asm/
│       ├── lexer_test.cc
│       ├── parser_test.cc
│       ├── symbol_table_test.cc
│       ├── expression_test.cc
│       ├── assembler_test.cc
│       ├── directives_test.cc
│       ├── macro_test.cc
│       ├── structured_test.cc
│       ├── listing_test.cc
│       └── golden_assembly_test.cc
├── data/
│   └── golden/
│       ├── asm/
│       │   ├── basic_move.x68
│       │   ├── basic_move.s68
│       │   ├── basic_move.l68
│       │   ├── ... (one triplet per test program)
│       └── sim/
│           ├── isa_traces/    (execution trace JSON files)
│           └── trap15_traces/ (Trap #15 I/O trace files)
├── ci/
│   ├── github/
│   │   ├── linux.yml
│   │   ├── macos.yml
│   │   └── windows.yml
│   └── clang-tidy.cfg
├── resources/
│   ├── easym68k.qrc
│   └── icons/
├── .clang-format
└── .gitignore
```

**Quality Gate 0.1:** Directory tree exists, no missing directories.

---

### Task 0.2 — Top-Level CMakeLists.txt

Write the root `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)
project(easym68k VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/CompilerWarnings.cmake)

option(BUILD_TESTING "Build unit and golden tests" ON)
option(BUILD_GUI "Build Qt 6 GUI applications" ON)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

# Core libraries — zero Qt dependency
add_subdirectory(src/sim)
add_subdirectory(src/asm)

# Qt GUI applications
if(BUILD_GUI)
  find_package(Qt6 REQUIRED COMPONENTS
    Widgets Multimedia SerialPort Network PrintSupport
  )
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)
  add_subdirectory(gui)
endif()

# Tests
if(BUILD_TESTING)
  enable_testing()
  include(FetchContent)
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  add_subdirectory(tests)
endif()
```

Write `cmake/CompilerWarnings.cmake`:

```cmake
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
  if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
  endif()
  if(ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  add_compile_options(/W4 /WX)
endif()
```

**Quality Gate 0.2:** `cmake -B build` succeeds with no errors. Empty library targets compile.

---

### Task 0.3 — Library CMakeLists.txt Files

Write `src/sim/CMakeLists.txt`:

```cmake
add_library(easym68k-sim STATIC
  memory.cc
  addressing_mode.cc
  simulator.cc
  decode.cc
  srecord_loader.cc
  trap15_dispatch.cc
  opcodes_move.cc
  opcodes_arithmetic.cc
  opcodes_logic.cc
  opcodes_branch.cc
  opcodes_misc.cc
  opcodes_shift.cc
)

target_include_directories(easym68k-sim PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
target_compile_features(easym68k-sim PUBLIC cxx_std_17)
```

Write `src/asm/CMakeLists.txt`:

```cmake
add_library(easym68k-asm STATIC
  assembler.cc
  lexer.cc
  parser.cc
  symbol_table.cc
  expression_evaluator.cc
  error_reporter.cc
  code_generator.cc
  directives.cc
  instruction_table.cc
  listing_generator.cc
  macro_processor.cc
)

target_include_directories(easym68k-asm PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
target_compile_features(easym68k-asm PUBLIC cxx_std_17)
target_link_libraries(easym68k-asm PRIVATE easym68k-sim)
```

Write `tests/CMakeLists.txt`:

```cmake
add_executable(easym68k-sim-tests
  sim/memory_test.cc
  sim/effective_addr_test.cc
  sim/addressing_mode_test.cc
  sim/simulator_test.cc
  sim/decode_test.cc
  sim/srecord_loader_test.cc
  sim/opcodes_move_test.cc
  sim/opcodes_arithmetic_test.cc
  sim/opcodes_logic_test.cc
  sim/opcodes_branch_test.cc
  sim/opcodes_misc_test.cc
  sim/opcodes_shift_test.cc
  sim/flag_computation_test.cc
  sim/exception_test.cc
  sim/trap15_mock_test.cc
  sim/golden_trace_test.cc
)
target_link_libraries(easym68k-sim-tests PRIVATE easym68k-sim GTest::gtest_main)
add_test(NAME sim-tests COMMAND easym68k-sim-tests)

add_executable(easym68k-asm-tests
  asm/lexer_test.cc
  asm/parser_test.cc
  asm/symbol_table_test.cc
  asm/expression_test.cc
  asm/assembler_test.cc
  asm/directives_test.cc
  asm/macro_test.cc
  asm/structured_test.cc
  asm/listing_test.cc
  asm/golden_assembly_test.cc
)
target_link_libraries(easym68k-asm-tests PRIVATE easym68k-asm GTest::gtest_main)
add_test(NAME asm-tests COMMAND easym68k-asm-tests)
```

**Quality Gate 0.3:** `cmake --build build` compiles all empty/stub targets with zero errors and zero warnings.

---

### Task 0.4 — .clang-format Configuration

Write `.clang-format` conforming to Google C++ Style Guide:

```yaml
BasedOnStyle: Google
IndentWidth: 2
ColumnLimit: 100
AllowShortFunctionsOnASingleLine: Inline
AllowShortIfStatementsOnASingleLine: false
AllowShortLoopsOnASingleLine: false
BreakBeforeBraces: Attach
NamespaceIndentation: None
SortIncludes: CaseInsensitive
Standard: c++17
```

**Quality Gate 0.4:** `clang-format --dry-run --Werror include/easym68k/sim/types.h` passes once the file exists.

---

### Task 0.5 — CI Pipeline Configuration

Write `ci/github/linux.yml`, `macos.yml`, `windows.yml` — each with:

1. Checkout
2. Install dependencies (Qt 6, CMake, ninja)
3. `cmake -B build -DENABLE_ASAN=ON -DENABLE_UBSAN=ON`
4. `cmake --build build`
5. `ctest --test-dir build --output-on-failure`
6. Upload test results

**Quality Gate 0.5:** All three CI configs pass on their respective platforms (verified manually before committing).

---

### Task 0.6 — Golden Test Data Infrastructure

Create `data/golden/` structure and a script to generate reference data from the original Windows binaries:

1. Identify 20+ representative `.X68` test programs from the original EASy68K distribution covering:
   - Every instruction category (move, arithmetic, logic, branch, shift, BCD, bit, system)
   - Every addressing mode
   - Every Trap #15 task
   - Exception handling
   - Edge cases (overflow, boundary addresses, zero-length operations)
2. For each program, run the original `Edit68K.exe` to produce Golden `.S68` and `.L68` files
3. For each program, run the original `Sim68K.exe` with register/memory logging to produce Golden execution traces (JSON format: array of `{pc, d[0..7], a[0..7], sr, cycles, memory_writes: [{addr, size, value}]}` per step)
4. Place all golden files in `data/golden/asm/` and `data/golden/sim/`

**Quality Gate 0.6:** At least 20 golden triplets (`.x68` + `.s68` + `.l68`) and 10 golden execution trace files exist and are committed.

---

## Phase 1: Types and Memory — EASy68K-qt Transfer (Code Transfer Plan Sections 3.1, 3.2)

### Task 1.1 — Transfer and Adapt `types.h`

Source: `EASy68K-qt/src/core/simulator/types.h` → Target: `include/easym68k/sim/types.h`

Steps:

1. Copy the file to the new location
2. Change include guard to `EASY68K_SIM_TYPES_H_`
3. Change namespace from `sim68k` to `easym68k::sim`
4. Rename `ExecResult` to `SimResult`:
   ```cpp
   enum class SimResult : uint8_t {
     kOk = 0,
     kBadInstruction,
     kPrivilegeViolation,
     kAddressError,
     kBusError,
     kDivideByZero,
     kChkException,
     kTrapVException,
     kTraceException,
     kLine1010,
     kLine1111,
     kStopInstruction,
     kTrapException,
     kHalted
   };
   ```
5. Remove `kBreakpoint` from the enum (breakpoints are a GUI concern)
6. Change `kAddrRegCount = 9` to `kAddrRegCount = 8` and add `constexpr int kSspIndex = 8;`
7. Move addressing mode constants (`kAddrModeDn` through `kAddrModeAll`) out — they go in `addressing_mode.h`
8. Run `clang-format`

**Quality Gate 1.1:** File compiles in isolation. All enum values are present and correctly named. No addressing mode constants remain in this file.

---

### Task 1.2 — Create `effective_addr.h`

New file: `include/easym68k/sim/effective_addr.h`

This is the core type that replaces the sentinel-address pattern:

```cpp
#ifndef EASY68K_SIM_EFFECTIVE_ADDR_H_
#define EASY68K_SIM_EFFECTIVE_ADDR_H_

#include <cstdint>

namespace easym68k::sim {

enum class EaTarget : uint8_t {
  kDataReg,    // Dn
  kAddrReg,    // An
  kMemory,     // (An), (An)+, -(An), d(An), d(An,Xi), xxx.W, xxx.L, d(PC), d(PC,Xi)
  kImmediate   // #imm
};

struct EffectiveAddr {
  EaTarget target;

  // For kDataReg/kAddrReg: register number (0–7)
  // For kMemory: unused (use .address)
  // For kImmediate: unused (use .address for the immediate value)
  uint8_t index;

  // For kMemory: the computed 24-bit memory address
  // For kImmediate: the immediate value itself
  uint32_t address;

  // The original mode/reg fields from the instruction word,
  // needed by some instructions (e.g., MOVEM needs to know
  // predecrement vs postincrement for register ordering)
  uint8_t mode;
  uint8_t reg;
};

// Convenience constructors
inline EffectiveAddr MakeDataRegEa(int reg) {
  return {EaTarget::kDataReg, static_cast<uint8_t>(reg), 0, 0, static_cast<uint8_t>(reg)};
}

inline EffectiveAddr MakeAddrRegEa(int reg) {
  return {EaTarget::kAddrReg, static_cast<uint8_t>(reg), 0, 1, static_cast<uint8_t>(reg)};
}

inline EffectiveAddr MakeMemoryEa(uint32_t address, uint8_t mode, uint8_t reg) {
  return {EaTarget::kMemory, 0, address, mode, reg};
}

inline EffectiveAddr MakeImmediateEa(uint32_t value) {
  return {EaTarget::kImmediate, 0, value, 7, 4};
}

}  // namespace easym68k::sim

#endif  // EASY68K_SIM_EFFECTIVE_ADDR_H_
```

**Quality Gate 1.2:** File compiles. Unit test `effective_addr_test.cc` verifies all four constructors produce correct `EaTarget` values and fields.

---

### Task 1.3 — Transfer and Adapt Memory Class

Source: `EASy68K-qt/src/core/simulator/memory.h` + `memory.cc`
Target: `include/easym68k/sim/memory.h` + `src/sim/memory.cc`

Steps:

1. Copy both files to new locations
2. Change include guard to `EASY68K_SIM_MEMORY_H_`
3. Change namespace to `easym68k::sim`
4. Replace `#include "types.h"` with `#include "easym68k/sim/types.h"`
5. Replace raw pointer members:
   ```cpp
   // Before:
   uint8_t* data_;
   uint8_t* flags_;
   // After:
   std::vector<uint8_t> data_;
   std::vector<uint8_t> flags_;
   ```
6. Remove the destructor (vector cleans up)
7. Remove the move constructor/assignment (vector handles it) — use `= default`
8. Keep copy deleted (16MB is expensive to copy)
9. Change `ReadByte` signature:
   ```cpp
   // Before: uint8_t ReadByte(uint32_t address) const;
   // After:  MemoryAccessResult ReadByte(uint32_t address) const;
   ```
   Update implementation to check `kMemInvalid` flag and return `MemoryAccessResult`
10. Add `error_message` field to `MemoryAccessResult`:
    ```cpp
    struct MemoryAccessResult {
      bool success;
      uint32_t value;
      SimResult error;
      std::string error_message;
    };
    ```
11. Update all `ReadByte` callers in `memory.cc` (none internal — it's the leaf)
12. Update `MaskAddress` to use `data_.size()` bounds check
13. Run `clang-format`

**Quality Gate 1.3:**
- `memory_test.cc` ported from EASy68K-qt and passes (read/write, alignment, endianness, ROM protection)
- `ReadByte` returns `MemoryAccessResult` with proper error on invalid memory
- No raw `new`/`delete` in the file
- `sizeof(Memory)` compiles without warnings
- ASan build passes all memory tests

---

### Task 1.4 — Port Memory Tests

Source: `EASy68K-qt/tests/simulator/memory_test.cc` → Target: `tests/sim/memory_test.cc`

Steps:

1. Copy the file
2. Change namespace to `easym68k::sim`
3. Update `ReadByte` calls to handle `MemoryAccessResult` return type
4. Add new tests:
   - `ReadByteInvalidAddress` — verify `kMemInvalid` returns error
   - `ReadByteBoundary` — read from address 0 and address 0xFFFFFF
   - `WriteThenReadWordEndian` — verify big-endian byte ordering in storage
   - `WriteThenReadLongEndian` — same for long
   - `DoubleAlignmentError` — odd address word+long both fail
   - `ProtectedThenUnprotect` — set then clear protection flags
5. Run tests: `ctest --test-dir build -R memory_test`

**Quality Gate 1.4:** All memory tests pass. No ASan errors.

---

## Phase 2: S-Record Loader — EASy68K-qt Transfer (Code Transfer Plan Section 3.7)

### Task 2.1 — Transfer and Extract SRecordLoader

Source: `EASy68K-qt/src/core/simulator/srecord_loader.cc`
Target: `include/easym68k/sim/srecord_loader.h` + `src/sim/srecord_loader.cc`

Steps:

1. Create new header with standalone class:
   ```cpp
   class SRecordLoader {
    public:
     struct LoadResult {
       bool success;
       uint32_t start_address;
       bool has_start_address;
       std::string error_message;
       int error_line;
     };

     LoadResult LoadFromFile(const std::string& filename, Memory& memory);
     LoadResult LoadFromStream(std::istream& stream, Memory& memory);
     LoadResult LoadFromLines(const std::vector<std::string>& lines, Memory& memory);
   };
   ```
2. Copy the hex parsing helpers (`HexCharToInt`, `ParseHexByte`, `ParseHexValue`)
3. Add checksum validation:
   - Accumulate all bytes (byte count + address bytes + data bytes + checksum byte)
   - Verify `accumulated & 0xFF == 0xFF` (one's complement property)
   - On failure, set `LoadResult.success = false` with line number and error message
4. Change from writing to `memory_` member to writing through `Memory&` parameter
5. Return structured `LoadResult` instead of `bool`
6. Add `LoadFromStream` overload
7. Run `clang-format`

**Quality Gate 2.1:**
- SRecordLoader compiles with no dependency on Cpu or Simulator classes
- `srecord_loader_test.cc` passes with valid S1/S2/S3/S7/S8/S9 records
- Checksum validation correctly rejects corrupted records
- Error messages include line numbers

---

### Task 2.2 — S-Record Loader Tests

Source: `EASy68K-qt/tests/simulator/srecord_test.cc` → Target: `tests/sim/srecord_loader_test.cc`

Steps:

1. Port existing tests
2. Add new tests:
   - `ChecksumValid` — known-good S-Record with correct checksum
   - `ChecksumInvalid` — same record with corrupted checksum byte → `success == false`
   - `S1Record` — 16-bit address data record
   - `S2Record` — 24-bit address data record
   - `S3Record` — 32-bit address data record
   - `S7StartAddress` — 32-bit start address
   - `S8StartAddress` — 24-bit start address
   - `S9StartAddress` — 16-bit start address
   - `EmptyFile` — returns error
   - `MissingSRecordHeader` — lines without 'S' prefix are skipped
   - `TruncatedRecord` — byte count exceeds available hex digits → error
   - `MultipleDataRecords` — sequential S1 records load to correct addresses
   - `LoadFromStream` — `std::istringstream` input works

**Quality Gate 2.2:** All 14 S-Record tests pass. No ASan errors.

---

## Phase 3: Addressing Modes — Port from Original (EASy68K/Sim68K/SCAN.CPP)

### Task 3.1 — Create `addressing_mode.h`

New file: `include/easym68k/sim/addressing_mode.h`

Contains:
- Addressing mode constants (moved from `types.h`)
- `EffectiveAddr CalculateEA(Memory& mem, uint32_t& pc, int mode, int reg, DataSize size)`
- Declarations for read/write helper functions that operate on `EffectiveAddr`

```cpp
#ifndef EASY68K_SIM_ADDRESSING_MODE_H_
#define EASY68K_SIM_ADDRESSING_MODE_H_

#include "easym68k/sim/effective_addr.h"
#include "easym68k/sim/types.h"

namespace easym68k::sim {

// Calculate effective address from mode/reg fields.
// May advance PC (for extension words: displacement, index, absolute address, immediate).
// Returns EffectiveAddr with appropriate EaTarget.
// Does NOT read the value — that's a separate operation.
EffectiveAddr CalculateEA(Memory& memory, uint32_t& pc, int mode, int reg, DataSize size);

// Read a value from the given EA.
uint32_t ReadEA(Memory& memory, const CpuState& state, const EffectiveAddr& ea, DataSize size);

// Write a value to the given EA.
SimResult WriteEA(Memory& memory, CpuState& state, const EffectiveAddr& ea, uint32_t value, DataSize size);

}  // namespace easym68k::sim

#endif
```

**Quality Gate 3.1:** Header compiles. No sentinel addresses. Clean separation: CalculateEA produces address, ReadEA/WriteEA perform data movement.

---

### Task 3.2 — Implement `addressing_mode.cc`

New file: `src/sim/addressing_mode.cc`

Primary source: `EASy68K/Sim68K/SCAN.CPP` — the original `eff_addr()` function that computes effective addresses for all 12 modes.
Secondary reference: `EASy68K-qt/src/core/simulator/addressing_modes.cc` — for the mode/register switch structure only. Do NOT copy sentinel address patterns.

Steps:

1. Implement `CalculateEA` — switch on mode 0–7:
   - Mode 0 (Dn): return `MakeDataRegEa(reg)`
   - Mode 1 (An): return `MakeAddrRegEa(reg)` — reject byte size
   - Mode 2 ((An)): return `MakeMemoryEa(state.a[reg], mode, reg)`
   - Mode 3 ((An)+): compute EA from An, then advance An by size increment (SP alignment rule for byte), return `MakeMemoryEa`
   - Mode 4 (-(An)): decrement An by size, then return `MakeMemoryEa`
   - Mode 5 (d(An)): fetch displacement word from PC, compute address, return `MakeMemoryEa`
   - Mode 6 (d(An,Xi)): fetch extension word from PC, decode index register + size + displacement, compute address, return `MakeMemoryEa`
   - Mode 7 sub-cases (7.0 absolute.W, 7.1 absolute.L, 7.2 d(PC), 7.3 d(PC,Xi), 7.4 #imm): return appropriate `MakeMemoryEa` or `MakeImmediateEa`
2. Implement `ReadEA` — switch on `ea.target`:
   - `kDataReg`: read from `state.d[ea.index]` with size masking
   - `kAddrReg`: read from `state.a[ea.index]` with size masking (word = sign-extend)
   - `kMemory`: read from `memory.Read(ea.address, size)`
   - `kImmediate`: return `ea.address` (already the value)
3. Implement `WriteEA` — switch on `ea.target`:
   - `kDataReg`: write to `state.d[ea.index]` preserving upper bits for byte/word
   - `kAddrReg`: write to `state.a[ea.index]` (full 32-bit for long, sign-extend for word)
   - `kMemory`: write via `memory.Write(ea.address, value, size)`
   - `kImmediate`: return `SimResult::kBadInstruction` (can't write to immediate)

**Critical difference from EASy68K-qt:** No sentinel addresses. No `0xFF000000` constants. Every branch of every switch uses `EffectiveAddr.target` as the discriminator.

**Quality Gate 3.2:**
- `addressing_mode_test.cc` passes for all 12 addressing modes
- Every `EffectiveAddr` returned has the correct `EaTarget`
- No sentinel address constants anywhere in the file
- ASan build passes

---

### Task 3.3 — Addressing Mode Tests

New file: `tests/sim/addressing_mode_test.cc`

Test cases for every addressing mode:

1. **Dn** (mode 0): EA target is kDataReg, index matches reg
2. **An** (mode 1): EA target is kAddrReg; byte size returns error
3. **(An)** (mode 2): EA target is kMemory, address matches An value
4. **(An)+** (mode 3): EA address is pre-increment An; An advanced by 1/2/4; SP byte increments by 2
5. **-(An)** (mode 4): An decremented first; EA address is post-decrement An
6. **d(An)** (mode 5): Extension word fetched from PC; EA address = An + d
7. **d(An,Xi)** (mode 6): Extension word decoded; index register value read; EA address = An + d + Xi
8. **xxx.W** (mode 7.0): 16-bit address sign-extended; PC advanced by 2
9. **xxx.L** (mode 7.1): 32-bit address; PC advanced by 4
10. **d(PC)** (mode 7.2): PC-relative displacement; EA address = PC + d (where PC points past extension word)
11. **d(PC,Xi)** (mode 7.3): PC-relative indexed
12. **#imm** (mode 7.4): EA target is kImmediate; value is the fetched immediate; PC advanced by 2 or 4

Plus ReadEA/WriteEA tests:
13. ReadEA from data register (byte/word/long masking)
14. ReadEA from address register (word sign-extends)
15. ReadEA from memory (big-endian read)
16. ReadEA from immediate (returns stored value)
17. WriteEA to data register (preserves upper bits for byte/word)
18. WriteEA to address register (word sign-extends, long stores full)
19. WriteEA to memory (big-endian write)
20. WriteEA to immediate returns error

**Quality Gate 3.3:** All 20 addressing mode tests pass.

---

## Phase 4: Simulator Core — Port from Original (EASy68K/Sim68K/RUN.CPP, extern.h, var.h)

### Task 4.1 — Create Simulator Header

New file: `include/easym68k/sim/simulator.h`

```cpp
#ifndef EASY68K_SIM_SIMULATOR_H_
#define EASY68K_SIM_SIMULATOR_H_

#include <atomic>
#include <cstdint>
#include <vector>

#include "easym68k/sim/types.h"
#include "easym68k/sim/effective_addr.h"
#include "easym68k/sim/memory.h"

namespace easym68k::sim {

// Forward declarations for Trap #15 interfaces
class ITextIO;
class IFileIO;
class ISerialIO;
class INetworkIO;
class IGraphicsIO;
class ISoundIO;
class IPeripheralIO;
class ISimulatorEnv;
class IPrintIO;
class ILogger;

struct CpuState {
  uint32_t d[8];     // Data registers D0-D7
  uint32_t a[8];     // Address registers A0-A7
  uint32_t usp;      // User Stack Pointer (separate from A7)
  uint32_t ssp;      // Supervisor Stack Pointer
  uint32_t pc;       // Program Counter
  uint16_t sr;       // Status Register
  uint64_t cycles;   // Cycle counter
  bool halted;
  bool stopped;

  void Reset(Memory& memory);

  bool IsSupervisor() const { return (sr & kSrSupervisor) != 0; }
  void SetSupervisor(bool value);
  int GetInterruptMask() const { return (sr & kSrIntMask) >> 8; }
  void SetInterruptMask(int level);
  bool GetFlag(uint16_t flag) const { return (sr & flag) != 0; }
  void SetFlag(uint16_t flag, bool value);
  uint32_t GetSP() const { return IsSupervisor() ? ssp : a[7]; }
  void SetSP(uint32_t value);
};

struct Breakpoint {
  uint32_t address;
  bool enabled;
};

class M68kSimulator {
 public:
  struct Config {
    ITextIO* text_io = nullptr;
    IFileIO* file_io = nullptr;
    ISerialIO* serial_io = nullptr;
    INetworkIO* network_io = nullptr;
    IGraphicsIO* graphics_io = nullptr;
    ISoundIO* sound_io = nullptr;
    IPeripheralIO* peripheral_io = nullptr;
    ISimulatorEnv* sim_env = nullptr;
    IPrintIO* print_io = nullptr;
    ILogger* logger = nullptr;
  };

  explicit M68kSimulator(Config config);
  ~M68kSimulator() = default;

  // Non-copyable, movable
  M68kSimulator(const M68kSimulator&) = delete;
  M68kSimulator& operator=(const M68kSimulator&) = delete;

  // Execution
  void Reset();
  SimResult Step();
  SimResult Run(int max_instructions = -1);
  void RequestStop();

  // State access
  CpuState& State() { return state_; }
  const CpuState& State() const { return state_; }
  Memory& GetMemory() { return memory_; }
  const Memory& GetMemory() const { return memory_; }

  // Breakpoints
  int AddBreakpoint(uint32_t address);
  void RemoveBreakpoint(int id);
  void ClearBreakpoints();
  bool HasBreakpoint(uint32_t address) const;

  // Program loading
  bool LoadSRecord(const std::string& filename);
  bool LoadSRecordLines(const std::vector<std::string>& lines);

 private:
  // Fetch
  uint16_t FetchWord();
  uint32_t FetchLong();

  // Instruction dispatch
  SimResult ExecuteInstruction(uint16_t opcode);

  // Addressing mode
  EffectiveAddr CalculateEA(int mode, int reg, DataSize size);
  uint32_t ReadEA(const EffectiveAddr& ea, DataSize size);
  SimResult WriteEA(const EffectiveAddr& ea, uint32_t value, DataSize size);

  // Condition codes
  bool CheckCondition(Condition cond) const;

  // Exceptions
  void HandleException(ExceptionVector vector);

  // Flag computation (per-operation)
  void UpdateFlagsAdd(uint32_t src, uint32_t dst, uint32_t result, DataSize size);
  void UpdateFlagsSub(uint32_t src, uint32_t dst, uint32_t result, DataSize size);
  void UpdateFlagsLogic(uint32_t result, DataSize size);
  void UpdateFlagsMove(uint32_t result, DataSize size);
  void UpdateFlagsShift(uint32_t result, DataSize size, bool carry, int count);

  // Opcode handlers (declared in opcodes_*.cc)
  // ...

  CpuState state_;
  Memory memory_;
  Config config_;
  std::vector<Breakpoint> breakpoints_;
  std::atomic<bool> stop_requested_;
  uint32_t start_address_;
};

}  // namespace easym68k::sim

#endif
```

**Quality Gate 4.1:** Header compiles. No raw I/O callbacks. All Trap #15 interfaces are injected. `CpuState` has separate `usp`/`ssp` instead of `a[8]` hack.

---

### Task 4.2 — Implement Simulator Core

New file: `src/sim/simulator.cc`

Primary source: `EASy68K/Sim68K/RUN.CPP` — the original `runprog()`, `one_inst()`, and execution loop.
Primary source: `EASy68K/Sim68K/extern.h` + `var.h` — global state declarations that must be encapsulated into `M68kSimulator`.
Primary source: `EASy68K/Sim68K/UTILS.CPP` — `put()`, `value_of()`, `mem_put()`, `mem_req()`, exception handling.
Secondary reference: `EASy68K-qt/src/core/simulator/cpu.cc` for `CpuState::Reset()`, `CheckCondition()`, `HandleException()` — these are correct in EASy68K-qt and can be used as a starting point, but must be verified against the original.

Write from scratch:
- `M68kSimulator::Reset()` — uses injected Memory, reads vectors
- `M68kSimulator::Step()` — fetch opcode, dispatch, return `SimResult`
- `M68kSimulator::Run()` — loop with `stop_requested_` check and breakpoint check
- `M68kSimulator::RequestStop()` — sets atomic flag
- All flag computation methods (`UpdateFlagsAdd`, `UpdateFlagsSub`, etc.) — **complete** V/C/X/N/Z
- `FetchWord()` / `FetchLong()` — advance PC, return values from Memory

**Quality Gate 4.2:**
- `simulator_test.cc` passes: Reset, register access, Step (NOP), Run, RequestStop, breakpoints
- `CheckCondition` test: all 16 conditions verified
- `HandleException` test: stack frame format correct (PC then SR pushed)
- `UpdateFlagsAdd` test: V/C/X computed correctly for multiple operand combinations
- `UpdateFlagsSub` test: borrow/V computed correctly
- ASan build passes

---

### Task 4.3 — Simulator Unit Tests

New file: `tests/sim/simulator_test.cc`

Minimum test cases:

1. `ResetInitializesFromVectors` — SSP from $0, PC from $4
2. `SetGetRegisters` — D0, A1, PC, SR
3. `SupervisorMode` — IsSupervisor true after reset, SetSupervisor toggles
4. `InterruptMask` — Get/SetInterruptMask
5. `SPSwitching` — GetSP returns SSP in supervisor, USP in user mode
6. `StepNop` — Step executes NOP, PC advances by 2
7. `StepIllegal` — Step on illegal opcode returns `kBadInstruction`
8. `RunStopRequested` — Run until RequestStop from another thread
9. `RunBreakpoint` — Run stops at breakpoint
10. `RunMaxInstructions` — Run respects max_instructions limit
11. `CheckConditionAll16` — verify every condition code with appropriate flag setup
12. `HandleExceptionStackFrame` — PC and SR pushed in correct order, PC loaded from vector
13. `HandleExceptionSupervisorSwitch` — switches to supervisor mode on exception
14. `FlagAddOverflow` — ADD sets V correctly
15. `FlagAddCarry` — ADD sets C and X correctly
16. `FlagSubBorrow` — SUB sets C (borrow) correctly
17. `FlagLogicClearsVC` — AND/OR/EOR clear V and C, leave X unchanged
18. `FlagMoveClearsVC` — MOVE clears V and C, leaves X unchanged

**Quality Gate 4.3:** All 18 simulator tests pass.

---

## Phase 5: Instruction Decoder — Port from Original (EASy68K/Sim68K/CODE1-CODE8.CPP)

### Task 5.1 — Implement Hierarchical Decoder

New file: `src/sim/decode.cc`

Primary source: `EASy68K/Sim68K/CODE1.CPP`–`CODE8.CPP` — the original instruction dispatch functions (`line0()` through `lineF()`) which use bit-field switch/case dispatch.
Secondary reference: `EASy68K-qt/src/core/simulator/instruction_decoder.cc` for the complete list of mask/value/handler entries. Do NOT copy the linear search pattern.

Implement two-level dispatch:

```cpp
SimResult M68kSimulator::ExecuteInstruction(uint16_t opcode) {
  int group = (opcode >> 12) & 0xF;

  switch (group) {
    case 0x0: return DispatchGroup0(opcode);  // ORI/ANDI/SUBI/ADDI/EORI/CMPI/BTST/BCHG/BCLR/BSET/MOVEP
    case 0x1: return OpMove(opcode);          // MOVE.B
    case 0x2: return DispatchMoveOrMovea(opcode); // MOVE.L / MOVEA.L
    case 0x3: return DispatchMoveOrMovea(opcode); // MOVE.W / MOVEA.W
    case 0x4: return DispatchGroup4(opcode);  // CHK/LEA/CLR/NEG/NEGX/NOT/TST/EXT/NBCD/SWAP/PEA/MOVEM/JSR/JMP/...
    case 0x5: return DispatchGroup5(opcode);  // ADDQ/SUBQ/Scc/DBcc
    case 0x6: return DispatchGroup6(opcode);  // BRA/BSR/Bcc
    case 0x7: return OpMoveq(opcode);         // MOVEQ
    case 0x8: return DispatchGroup8(opcode);  // OR/DIVU/DIVS/SBCD
    case 0x9: return DispatchGroup9(opcode);  // SUB/SUBA/SUBX
    case 0xA: return OpLine1010(opcode);      // Line A
    case 0xB: return DispatchGroupB(opcode);  // CMP/CMPA/CMPM/EOR
    case 0xC: return DispatchGroupC(opcode);  // AND/MULU/MULS/ABCD/EXG
    case 0xD: return DispatchGroupD(opcode);  // ADD/ADDA/ADDX
    case 0xE: return DispatchGroupE(opcode);  // ASd/LSd/ROXd/ROd
    case 0xF: return DispatchSimhalt(opcode); // SIMHALT or Line F
  }
  return SimResult::kBadInstruction;
}
```

Each `DispatchGroupN` function contains a secondary switch or if-chain for sub-opcodes within that group, checking specific bit patterns.

**Special case — SIMHALT:** Opcode `0xFFFF` matches group 0xF. Check `opcode == 0xFFFF` first → return `SimResult::kHalted`. Any other Line-F opcode → `SimResult::kLine1111`.

**Quality Gate 5.1:**
- `decode_test.cc` passes: every opcode in the EASy68K-qt instruction table maps to the correct handler
- SIMHALT (`0xFFFF`) returns `SimResult::kHalted`
- Unknown opcodes return `SimResult::kBadInstruction`
- Decoder is O(1) — no linear search through a table

---

### Task 5.2 — Decoder Tests

New file: `tests/sim/decode_test.cc`

1. `NopDispatches` — opcode `0x4E71` → handler for NOP
2. `RtsDispatches` — opcode `0x4E75` → handler for RTS
3. `MoveqDispatches` — opcode `0x7042` → handler for MOVEQ
4. `SimhaltDispatches` — opcode `0xFFFF` → returns `SimResult::kHalted`
5. `IllegalDispatches` — opcode `0x4AFC` → handler for ILLEGAL
6. `LineADispatches` — opcode `0xA000` → returns `SimResult::kLine1010`
7. `LineFDispatches` — opcode `0xF000` (not SIMHALT) → returns `SimResult::kLine1111`
8. `OriToCcr` — opcode `0x003C` dispatches correctly (not confused with generic ORI)
9. `ExgDnDn` — opcode `0xC140` dispatches correctly
10. `ExgAnAn` — opcode `0xC148` dispatches correctly
11. `MovemRegister` — opcode `0x4880` dispatches correctly
12. `BtstDynamic` — opcode `0x0100` dispatches correctly (not confused with ORI)

**Quality Gate 5.2:** All 12 decode tests pass.

---

## Phase 6: Opcode Implementation — Port from Original (EASy68K/Sim68K/CODE1-CODE9.CPP)

All opcode implementations in this phase are ported from the **original EASy68K Borland sources**. The instruction logic in `CODE1.CPP`–`CODE9.CPP` is the authoritative reference for behavioral parity. EASy68K-qt's opcode files may be consulted as a secondary reference for C++ code structure, but every handler must be verified against the original and must not inherit EASy68K-qt's sentinel EA patterns or incomplete flag computation.

### Task 6.1 — Move Instructions

Primary source: `EASy68K/Sim68K/CODE1.CPP` — original MOVE/MOVEA/MOVEQ/MOVEM/MOVEP/LEA/PEA/EXG/LINK/UNLK/SWAP implementations.
Secondary reference: `EASy68K-qt/src/core/simulator/opcodes_move.cc` — for C++ code structure only, not for logic correctness.

Port procedure (apply to ALL opcode files — porting from original, not copying from EASy68K-qt):

1. Read the original `CODE*.CPP` function for the instruction
2. Translate Borland C++ to modern C++17: replace `AnsiString` with `std::string`, remove `#include <vcl.h>`, replace global arrays (`D[]`, `A[]`, `memory[]`) with `CpuState` and `Memory` class methods
3. Replace `put()` / `value_of()` calls with `ReadEA()` / `WriteEA()` on `EffectiveAddr`
4. Replace `eff_addr()` calls with `CalculateEA()` returning `EffectiveAddr`
5. Replace `mem_put()` / `mem_req()` with `Memory::Write()` / `Memory::Read()`
6. Complete flag computation per M68000 Programmers Reference Manual (the original delegates to `UpdateFlags*` helpers)
7. Cross-reference EASy68K-qt's implementation if needed for C++ idioms, but do NOT trust its flag computation
8. Write tests
9. Run `clang-format`

**Instructions in this file:**
- `OpMove` (byte/word/long)
- `OpMovea` (word/long with sign extension)
- `OpMoveq` (8-bit immediate with sign extension)
- `OpMovem` (register list to/from memory)
- `OpMovep` (register ↔ memory byte-swapped)
- `OpMoveFromSr`, `OpMoveToCcr`, `OpMoveToSr`
- `OpMoveUsp`
- `OpLea`
- `OpPea`
- `OpExg`
- `OpLink`
- `OpUnlk`
- `OpSwap`

**Quality Gate 6.1:**
- `opcodes_move_test.cc` passes with at least 30 test cases
- MOVE.B/W/L to/from all addressing modes
- MOVEA.W sign-extends, MOVEA.L does not
- MOVEQ positive, negative, zero values with correct flags
- MOVEM with all register list patterns
- EXG all three variants
- LINK/UNLK frame pointer behavior
- All flag values verified per M68000 Programmers Reference Manual

---

### Task 6.2 — Arithmetic Instructions

Primary source: `EASy68K/Sim68K/CODE2.CPP` + `CODE3.CPP` — original ADD/ADDI/ADDQ/ADDX/SUB/SUBI/SUBQ/SUBX/MULU/MULS/DIVU/DIVS/NEG/NEGX/CLR/CMP/TST/EXT implementations.
Secondary reference: `EASy68K-qt/src/core/simulator/opcodes_arithmetic.cc` — for C++ code structure only.

Same port procedure as Task 6.1, plus:

**Critical:** Complete V/C/X flag computation. Every arithmetic handler must call the appropriate flag method:

- ADD/ADDI/ADDQ/ADDX → `UpdateFlagsAdd(src, dst, result, size)`
- SUB/SUBI/SUBQ/SUBX → `UpdateFlagsSub(src, dst, result, size)`
- MULU/MULS → V = overflow, C = 0, X unchanged
- DIVU/DIVS → V = overflow on divide-by-zero or quotient overflow, C = 0

**Instructions in this file:**
- `OpAdd`, `OpAdda`, `OpAddi`, `OpAddq`, `OpAddx`
- `OpSub`, `OpSuba`, `OpSubi`, `OpSubq`, `OpSubx`
- `OpMuls`, `OpMulu`, `OpDivs`, `OpDivu`
- `OpNeg`, `OpNegx`
- `OpClr`
- `OpCmp`, `OpCmpa`, `OpCmpi`, `OpCmpm`
- `OpTst`
- `OpExt`

**Quality Gate 6.2:**
- `opcodes_arithmetic_test.cc` passes with at least 50 test cases
- Every ADD/SUB variant tested with values that trigger: carry, overflow, zero, negative
- MULU overflow (result > 0xFFFF) sets V correctly
- DIVU divide-by-zero generates exception
- NEG of zero sets C but not X (this is a known subtle case)
- CLR always clears N/Z/V/C (X unchanged)

---

### Task 6.3 — Logic Instructions

Primary source: `EASy68K/Sim68K/CODE4.CPP` — original AND/ANDI/OR/ORI/EOR/EORI/NOT/TAS/BTST/BCHG/BCLR/BSET implementations.
Secondary reference: `EASy68K-qt/src/core/simulator/opcodes_logic.cc` — for C++ code structure only.

Same port procedure. All logic ops: V = 0, C = 0, X unchanged, N/Z from result.

**Instructions:**
- `OpAnd`, `OpAndi`, `OpAndiToCcr`, `OpAndiToSr`
- `OpOr`, `OpOri`, `OpOriToCcr`, `OpOriToSr`
- `OpEor`, `OpEori`, `OpEoriToCcr`, `OpEoriToSr`
- `OpNot`
- `OpTas` (move to misc or keep here)
- `OpBitOp` (BTST/BCHG/BCLR/BSET — static and dynamic)

**Quality Gate 6.3:**
- `opcodes_logic_test.cc` passes with at least 30 test cases
- ANDI/ORI/EORI to CCR: verify individual flag bit manipulation
- ANDI/ORI/EORI to SR: privileged instruction check
- BTST/BCHG/BCLR/BSET: both static (immediate) and dynamic (register) bit numbers
- NOT: verify flags (N/Z from result, V=0, C=0)

---

### Task 6.4 — Branch Instructions

Primary source: `EASy68K/Sim68K/CODE5.CPP` + `CODE6.CPP` — original BRA/BSR/Bcc/DBcc/Scc/JMP/JSR/RTS/RTE/RTR implementations.
Secondary reference: `EASy68K-qt/src/core/simulator/opcodes_branch.cc` — for C++ code structure only.

**Instructions:**
- `OpBra`, `OpBsr`
- `OpBcc` (all 14 conditional branches)
- `OpDbcc`
- `OpScc`
- `OpJmp`, `OpJsr`
- `OpRts`, `OpRte`, `OpRtr`

**Quality Gate 6.4:**
- `opcodes_branch_test.cc` passes with at least 25 test cases
- BRA short (byte displacement) and word displacement
- BSR pushes return address correctly
- Bcc taken/not-taken with all 14 conditions
- DBcc loop count decrement and branch/fallthrough
- Scc sets byte to $00 or $FF
- RTE restores SR and PC from stack

---

### Task 6.5 — Miscellaneous Instructions

Primary source: `EASy68K/Sim68K/CODE7.CPP` — original CHK/TRAP/TRAPV/ILLEGAL/NOP/RESET/STOP/LINE1010/LINE1111 implementations.
Secondary reference: `EASy68K-qt/src/core/simulator/opcodes_misc.cc` — for C++ code structure only.

**Instructions:**
- `OpChk`
- `OpTrap`
- `OpTrapv`
- `OpIllegal`
- `OpNop`
- `OpReset`
- `OpStop`
- `OpLine1010`
- `OpLine1111`
- `OpSimhalt` (new — opcode `0xFFFF`)

**Quality Gate 6.5:**
- `opcodes_misc_test.cc` passes with at least 15 test cases
- CHK exception when Dn < 0 or Dn > bound
- TRAP generates exception with correct vector
- TRAPV traps only when V flag is set
- STOP loads SR and enters stopped state
- SIMHALT returns `SimResult::kHalted`

---

### Task 6.6 — Shift/Rotate Instructions

Primary source: `EASy68K/Sim68K/CODE8.CPP` — original ASL/ASR/LSL/LSR/ROL/ROR/ROXL/ROXR implementations.
Secondary reference: `EASy68K-qt/src/core/simulator/opcodes_logic.cc` (shift section) — for C++ code structure only.

**Instructions:**
- `OpShiftRotate` — register and memory variants
- ASL/ASR (arithmetic shift left/right)
- LSL/LSR (logical shift left/right)
- ROL/ROR (rotate left/right)
- ROXL/ROXR (rotate through extend left/right)

**Quality Gate 6.6:**
- `opcodes_shift_test.cc` passes with at least 20 test cases
- Shift count from immediate (1–8) and register (0–63, modulo 64 for long, modulo 8 for byte/word)
- C flag = last bit shifted out
- V flag set for ASL if sign changes during shift
- Memory shifts always shift by 1
- ROXL/ROXR include X flag in rotation

---

### Task 6.7 — Flag Computation Verification Suite

New file: `tests/sim/flag_computation_test.cc`

Systematic verification of N/Z/V/C/X for every instruction category with specific operand values:

| Instruction | Size | Operands | Expected N | Z | V | C | X |
|-------------|------|----------|-----------|---|---|---|---|
| ADD.L | Long | $7FFFFFFF + $00000001 | 1 | 0 | 1 | 0 | 0 |
| ADD.L | Long | $80000000 + $80000000 | 1 | 0 | 0 | 1 | 1 |
| SUB.W | Word | $0000 - $0001 | 1 | 0 | 0 | 1 | 1 |
| SUB.W | Word | $8000 - $0001 | 0 | 0 | 1 | 0 | 0 |
| AND.B | Byte | $FF & $0F | 0 | 0 | 0 | 0 | unchanged |
| MOVE.W | Word | $FFFF | 1 | 0 | 0 | 0 | unchanged |
| ASL.W | Word | $4000, count=1 | 0 | 0 | 1 | 0 | unchanged |
| ASL.W | Word | $C000, count=1 | 1 | 0 | 1 | 0 | unchanged |
| ... (60+ total rows) | | | | | | | |

**Quality Gate 6.7:** All 60+ flag computation test cases pass. This is the single most important quality gate — incorrect flags will silently produce wrong program behavior.

---

## Phase 7: Trap #15 Dispatch — Port from Original (EASy68K/Sim68K/CODE9.CPP)

### Task 7.1 — Define Trap #15 Interface Headers

Create all 9 interface headers in `include/easym68k/sim/`:

| Header | Interface | Tasks |
|--------|-----------|-------|
| `itext_io.h` | `ITextIO` | 0-2, 5-6, 11-14, 16-18, 22, 24-25 |
| `ifile_io.h` | `IFileIO` | 50-59 |
| `iserial_io.h` | `ISerialIO` | 40-43 |
| `inetwork_io.h` | `INetworkIO` | 100-107 |
| `igraphics_io.h` | `IGraphicsIO` | 80-96 |
| `isound_io.h` | `ISoundIO` | 70-77 |
| `iperipheral_io.h` | `IPeripheralIO` | 60-62 |
| `isimulator_env.h` | `ISimulatorEnv` | 8, 23, 30-33 |
| `iprint_io.h` | `IPrintIO` | 10 |

Copy interface definitions from proposal Section 6.4. Also create:
- `include/easym68k/sim/ilogger.h` — `ILogger` interface

**Quality Gate 7.1:** All 10 interface headers compile. No implementation dependencies.

---

### Task 7.2 — Implement Trap #15 Dispatch

New file: `src/sim/trap15_dispatch.cc`

Port the task-number dispatch logic from `EASy68K/Sim68K/CODE9.CPP`. This is the original `TRAP()` function containing the complete switch statement mapping task numbers (read from D0.B) to interface method calls:

```cpp
SimResult M68kSimulator::DispatchTrap15() {
  uint8_t task = static_cast<uint8_t>(state_.d[0] & 0xFF);

  switch (task) {
    case 0: config_.text_io->TextOut(ReadStringFromEA(...)); break;
    case 1: config_.text_io->TextOutCRLF(ReadStringFromEA(...)); break;
    case 2: config_.text_io->TextIn(...); break;
    // ... all 50+ cases
    case 70: config_.sound_io->PlaySound(...); break;
    case 80: config_.graphics_io->SetPenColor(...); break;
    // ...
    default: /* unknown task — log warning */ break;
  }
  return SimResult::kOk;
}
```

**Reference `EASy68K/Sim68K/CODE9.CPP` for every task number and its D0/D1/A0/A1 register usage pattern.** Each task reads its parameters from specific registers or memory locations — this mapping must be exact. The original is the single authoritative source for all Trap #15 semantics.

**Quality Gate 7.2:**
- `trap15_mock_test.cc` passes with mock implementations of all 9 interfaces
- Every task number from 0–25, 30–33, 40–43, 50–59, 60–62, 70–77, 80–96, 100–107 dispatches to the correct interface method
- Unknown task numbers log a warning and do not crash
- Register parameter mapping verified per original CODE9.CPP

---

### Task 7.3 — Trap #15 Mock Tests

New file: `tests/sim/trap15_mock_test.cc`

Create mock implementations of all 9 interfaces using gtest `MOCK_METHOD`. For each Trap #15 task:

1. Set up register state with expected parameters
2. Execute `TRAP #15`
3. Verify the mock received the expected call with the expected arguments
4. Verify register result values are written back correctly

Minimum test coverage: one test per task number (~50 tests), plus:
- Multiple parameter combinations for complex tasks (graphics modes, sound control values)
- Edge cases: null interface pointer → task is a no-op (log warning)
- SIMHALT verification within Trap context

**Quality Gate 7.3:** All 50+ Trap #15 mock tests pass.

---

## Phase 8: Assembler — Port from Original (EASy68K/Edit68K/)

### Task 8.1 — Transfer Lexer

Primary source: `EASy68K/Edit68K/asm.h` — original token definitions and lexer structures.
Secondary reference: `EASy68K-qt/src/core/assembler/lexer.h` + `lexer.cc` — good tokenizer foundation, but missing EASy68K-specific tokens.

Steps:
1. Copy files
2. Change namespace to `easym68k::asm`
3. Change include guard
4. Add EASy68K-specific tokens: `ORG`, `EQU`, `SET`, `DC`, `DS`, `SECTION`, `INCLUDE`, `MACRO`, `MEND`, `IF`, `THEN`, `ELSE`, `ENDI`, `WHILE`, `ENDW`, `FOR`, `ENDF`, `REPEAT`, `UNTIL`, `SWITCH`, `ENDSW`, `COMMENT`, `LOAD`, `END`, `OPT`, `XREF`
5. Run `clang-format`

**Quality Gate 8.1:** `lexer_test.cc` passes with tokens for every EASy68K directive and instruction.

---

### Task 8.2 — Transfer Parser and Symbol Table

Primary source: `EASy68K/Edit68K/asm.h` + `ASSEMBLE.CPP` + `OPPARSE.CPP` — original parser and instruction operand parsing.
Secondary reference: `EASy68K-qt/src/core/assembler/parser.h` + `parser.cc` — instruction/directive parsing framework, needs EASy68K compatibility.

Steps:
1. Copy files, change namespace/guards
2. Add directive parsing for all EASy68K directives (currently stubs)
3. Add EASy68K instruction mnemonic recognition
4. Run `clang-format`

**Quality Gate 8.2:** `parser_test.cc` and `symbol_table_test.cc` pass.

---

### Task 8.3 — Transfer Expression Evaluator

Primary source: `EASy68K/Edit68K/EVAL.CPP` — the original expression evaluator with operator precedence.
Secondary reference: `EASy68K-qt/src/core/assembler/expression.h` + `expression_evaluator.cc` — reasonable base, but verify operator precedence matches original.

Steps:
1. Copy files, change namespace/guards
2. Verify operator precedence matches EASy68K original (`EVAL.CPP`)
3. Add EASy68K-specific operators if missing
4. Run `clang-format`

**Quality Gate 8.3:** `expression_test.cc` passes with comprehensive arithmetic, logical, and relational expressions.

---

### Task 8.4 — Transfer Assembler Core

Primary source: `EASy68K/Edit68K/asm.h` + `ASSEMBLE.CPP` — the original assembler engine with jump-table dispatch.
Secondary reference: `EASy68K-qt/src/core/assembler/assembler.h` + `assembler.cc` — framework is reasonable but many methods are stubs.

Steps:
1. Copy files, change namespace/guards
2. Wire up lexer, parser, symbol table, expression evaluator
3. Verify two-pass assembly structure
4. Run `clang-format`

**Quality Gate 8.4:** `assembler_test.cc` passes for NOP, ORG, labels, MOVEQ, DC.W (existing EASy68K-qt test cases).

---

### Task 8.5 — Implement Missing Assembler Components

The assembler infrastructure (lexer, parser, expression evaluator, symbol table, instruction table metadata) is complete. This task fills in the ~4,100 remaining lines across 9 original source files that make the assembler functionally complete. The original assembler is approximately 5,900 lines; approximately 1,800 lines (31%) have been ported. This task covers the remaining 69%.

**Current capability gap:** Only NOP and MOVEQ produce machine code. Every other instruction produces "Unknown instruction" errors. No macros, no conditional assembly, no INCLUDE, no S-record output, no listing generation. A program as simple as `MOVE.L #$1234,D0 / ADD.L D0,D1 / RTS` cannot be assembled.

**Source transition rule:** All code is ported from the **original Borland sources** (`EASy68K/Edit68K/`), not from EASy68K-qt stubs.

**Porting checklist for each subtask:**
1. Read the original `.CPP` file
2. Replace `AnsiString` with `std::string`
3. Replace `TColor`/`FontStyle` with `uint32_t` RGB
4. Remove `#include <vcl.h>`
5. Implement in the target file following the class structure
6. Write tests

---

#### Task 8.5.1 — Instruction Table Tests and Assembler Wiring

The instruction table (`instruction_table.cc`, 254 lines) already exists with 124 entries covering the full 68000 ISA. It is **not wired into the assembler** — `HandleInstruction()` only handles NOP/MOVEQ; `InstructionSize()` returns 2 for everything.

Source: `EASy68K/Edit68K/INSTTABL.CPP` (907 lines) + `INSTLOOK.CPP` (155 lines) — already ported as metadata only.

Steps:
1. Write `tests/asm/instruction_table_test.cc`:
   - **Lookup tests:** Verify `InstructionTable::Lookup()` returns the correct `InstrEntry` for all 124 instruction mnemonics (name, encoding type, condition code, size flags, operand count, mode masks, base opcode)
   - **Lookup negative tests:** Verify unknown mnemonics return `std::nullopt`
   - **ExtWordCount tests:** Verify extension word count for every addressing mode + size combination (13 modes × 3 sizes + immediate/long = ~42 cases)
   - **SizeField tests:** Verify B→0, W→1, L→2 mapping
   - **Mode mask tests:** Verify `kModeD`, `kModeA`, `kModeEA`, `kModeCtrl`, `kModeAlt`, `kModeData`, `kModeUSP` bit masks match original `asm.h` definitions
   - **Table completeness:** Verify every 68000 instruction from INSTTABL.CPP is present (no gaps)
2. Wire `InstructionTable` into `Assembler` class:
   - Add `InstructionTable instruction_table_;` member to `assembler.h`
   - Replace `InstructionSize()` hardcoded logic with table lookup: look up the mnemonic, compute size from base opcode words + ExtWordCount for each operand
   - Replace `HandleInstruction()` NOP/MOVEQ special-casing with table-driven dispatch: look up mnemonic → get `InstrEncoding` → dispatch to the appropriate code generator handler
3. Remove all `// TODO(Task 8.5)` comments in `assembler.cc`

**Quality Gate 8.5.1:** `instruction_table_test.cc` passes with 50+ test cases. `InstructionSize()` returns correct values for all instruction types (verified by existing assembler tests + new size-specific tests). `HandleInstruction()` no longer falls through to "Unknown instruction" for any valid 68000 opcode — it dispatches to the code generator (which may still be stub-level at this point, but the dispatch path must be correct).

---

#### Task 8.5.2 — Code Generator: EA Encoding and Extension Words

Port the foundational encoding functions from CODEGEN.CPP. These are the building blocks used by every instruction encoding handler.

Source: `EASy68K/Edit68K/CODEGEN.CPP` (212 lines) — `effAddr()`, `extWords()`, `output()`.

Steps:
1. Port `effAddr()` (CODEGEN.CPP:44-95) → `CodeGenerator::EffAddr()`:
   - Maps each `AddressMode` to a 6-bit effective address code: Dn=0x00|reg, An=0x08|reg, (An)=0x10|reg, (An)+=0x18|reg, -(An)=0x20|reg, d(An)=0x28|reg, d(An,Xi)=0x30|reg, Abs.W=0x38, Abs.L=0x39, d(PC)=0x3A, d(PC,Xi)=0x3B, #imm=0x3C
   - Returns `uint16_t` with mode and register fields packed into the low 6 bits
   - Error on invalid register numbers (An register > 7, etc.)
2. Port `extWords()` (CODEGEN.CPP:97-196) → `CodeGenerator::ExtWords()`:
   - Emit extension words for displacement, address, and immediate operands
   - Range checking with specific error codes: `INV_16_BIT_DATA`, `INV_8_BIT_DATA`, `INV_OFFSET`
   - Symbol reference handling: `backRef` flag determines whether value is resolved (pass 2) or placeholder (pass 1)
   - PC-relative displacement calculation: `disp = value - (loc + 2)`
   - Indexed addressing: extension word with scale/disp/reg fields
   - Immediate values: byte/word/long size-appropriate encoding
3. Port `output()` (CODEGEN.CPP:28-42) → `CodeGenerator::Output()`:
   - Writes a 16-bit instruction word to the output buffer
   - Advances location counter by 2
   - Triggers listing generator callback (if active)
4. Write `tests/asm/code_generator_test.cc`:
   - **EffAddr tests:** All 13 addressing modes produce correct 6-bit codes (13 test cases minimum)
   - **ExtWords displacement tests:** d(An) with positive/negative/out-of-range displacements
   - **ExtWords indexed tests:** d(An,Xi) with all register combinations, scale factors
   - **ExtWords absolute tests:** Abs.W vs Abs.L selection based on value range
   - **ExtWords immediate tests:** #imm for B/W/L sizes, range checking errors
   - **ExtWords PC-relative tests:** forward and backward references, displacement calculation
   - **Output tests:** Instruction word written correctly, location counter advanced

**Quality Gate 8.5.2:** `code_generator_test.cc` passes with 40+ test cases. EA encoding produces correct bit patterns for all 13 addressing modes. Extension words are emitted correctly for all mode+size combinations. Range checking produces appropriate error codes. PC-relative displacement calculation is correct.

---

#### Task 8.5.3 — Code Generator: Instruction Encoding Handlers

Port all instruction encoding handler functions from BUILD.CPP. This is the core of the assembler — the code that produces actual machine code.

Source: `EASy68K/Edit68K/BUILD.CPP` (889 lines) — 20+ handler functions.

Steps:
1. Port each encoding handler as a `CodeGenerator` method:

   | Handler | Instructions | Lines | Key Logic |
   |---------|-------------|-------|----------|
   | `zeroOp` | ILLEGAL, NOP, RESET, RTE, RTR, RTS, TRAPV | 103-111 | Emit mask word only |
   | `oneOp` | ASd/LSd/ROd/ROXd \u003cea\u003e, CLR, JMP, JSR, NBCD, NEG, NEGX, NOT, PEA, TAS, TST | 136-146 | Emit mask | EA(source) + ext words |
   | `arithReg` | ADD/ADDA/AND/CHK/CMP/CMPA/DIVS/DIVU/LEA/MULS/MULU/OR/SUB/SUBA \u003cea\u003e,Dn | 169-205 | Emit mask | EA(src) | (Dn\u003c\u003c9) + ext words; **peephole: ADDA/SUBA #1-8 → ADDQ/SUBQ** |
   | `arithAddr` | ADD/AND/BCHG/BCLR/BSET/BTST/EOR/OR/SUB Dn,\u003cea\u003e | 224-234 | Emit mask | EA(dest) | (Sn\u003c\u003c9) + ext words |
   | `immedInst` | ADDI/ANDI/CMPI/EORI/ORI/SUBI | 299-329 | Emit mask | EA(dest) + ext words for both; **peephole: ADDI/SUBI #1-8 → ADDQ/SUBQ** |
   | `quickMath` | ADDQ/SUBQ | 341-354 | Emit mask | (data&7)\u003c\u003c9 | EA(dest) + ext words; validate data 1-8 |
   | `shiftReg` | ASd/LSd/ROd/ROXd register | 746-763 | Emit mask | (count\u003c\u003c9) | Dn; validate shift count 1-8 |
   | `branch` | Bcc/BRA/BSR | 562-593 | Short: mask | (disp&0xFF); Word: mask + disp word; size hint and range logic |
   | `dbcc` | DBcc | 685-706 | Emit mask | Dn + disp word |
   | `scc` | Scc | 720-729 | Emit mask | EA(source) + ext words |
   | `move` | MOVE/MOVEA | 62-87 | Dest EA re-mapped to bits 11-9/5-3; **peephole: MOVE #imm,Dn → MOVEQ** |
   | `moveq` | MOVEQ | 603-614 | Emit mask | (Dn\u003c\u003c9) | (data&0xFF); validate -128..255 |
   | `immedToCCR` | ANDI/EORI/ORI to CCR | 627-642 | Emit mask + immediate byte word |
   | `immedWord` | ANDI/EORI/ORI to SR, RTD, STOP | 657-672 | Emit mask + immediate word |
   | `staticBit` | BCHG/BCLR/BSET/BTST #n,\u003cea\u003e | 469-483 | Emit mask | EA(dest) + data byte word + ext words |
   | `exg` | EXG | 773-792 | DnxDn/DnxAx/AxxAx mode bits; An register goes in bottom 3 bits for mixed-mode |
   | `twoReg` | ABCD/ADDX/CMPM/SBCD/SUBX | 807-816 | Emit mask | (Dn\u003c\u003c9) | Sn |
   | `oneReg` | EXT/SWAP/UNLK | 829-838 | Emit mask | Dn/An |
   | `moveUSP` | MOVE USP,An / MOVE An,USP | 850-862 | Emit mask | An register number |
   | `link` | LINK | 871-886 | Emit mask | An + disp word; validate -32768..32767 |
   | `trap` | TRAP | 537-548 | Emit mask | (vector&0xF); validate 0-15 |
   | `movep` | MOVEP | 364-395 | Dn↔d(An); 4 sub-forms (word/long × reg→mem/mem→reg); direction bit at bit 7, size bit at bit 6 |
   | `bitField` | BFCHG/BFCLR/BFEXTS/BFEXTU/BFFFO/BFINS/BFSET/BFTST | 253-284 | 68020 bit-field instructions (optional, guarded by `#if 0` or config flag) |

2. Wire all handlers into `HandleInstruction()` dispatch via the `InstrEncoding` enum from `instruction_table.h`:
   ```cpp
   switch (entry.encoding) {
     case InstrEncoding::kFixed:     return code_gen_.zeroOp(entry.base_opcode, ...);
     case InstrEncoding::kEaBidirect: return code_gen_.arithReg(entry.base_opcode, ...);
     case InstrEncoding::kMove:      return code_gen_.move(entry.base_opcode, ...);
     case InstrEncoding::kBranch:    return code_gen_.branch(entry.base_opcode, ...);
     // ... all 26 encoding types
   }
   ```
3. Implement peephole optimizations from BUILD.CPP:
   - **ADDI #1-8 → ADDQ** (BUILD.CPP:309-320, `immedInst()`): immediate value 1-8 and not forced long
   - **ADDA/SUBA #1-8 → ADDQ/SUBQ** (BUILD.CPP:178-197, `arithReg()`): same for address register arithmetic
   - **MOVE #imm,Dn → MOVEQ** (BUILD.CPP:68-74, `move()`): immediate fits 8 bits, dest is Dn, size is .L
4. Write `tests/asm/code_generator_test.cc` (extended):
   - **Per-handler tests:** At least 2 test cases per handler (positive + boundary)
   - **Peephole optimization tests:** Verify ADDI #3,D0 encodes as ADDQ; verify MOVE.L #5,D0 encodes as MOVEQ
   - **Branch displacement tests:** Short (±127), word (±32767), size hint (.B/.W/.L), BSR
   - **MOVEM placeholder tests:** Skip until Task 8.5.4
   - **End-to-end instruction tests:** Assemble `ADD.L D0,D1` and verify opcode bytes `D081`; assemble `MOVE.W (A0)+,D2` and verify `3018`; assemble `BRA label` and verify displacement

**Quality Gate 8.5.3:** `code_generator_test.cc` passes with 60+ test cases. Every 68000 instruction (except MOVEM and bit-field) produces correct machine code, including MOVEP with all 4 sub-forms. Peephole optimizations fire correctly. Branch displacement calculation handles short and word cases. `HandleInstruction()` dispatches all encoding types without "Unknown instruction" errors.

---

#### Task 8.5.4 — MOVEM Register List Parsing and Encoding

Port the MOVEM register list parser and encoder. This is a separate concern because the register list syntax is unique to MOVEM and requires its own parser.

Source: `EASy68K/Edit68K/MOVEM.CPP` (373 lines).

Steps:
1. Port register list parsing:
   - Syntax: `D0-D7/A0-A7` with ranges (`D0-D3`), individual registers (`D5`), and combinations (`D0-D3/D5/A0-A2`)
   - Register list symbol expansion: labels defined with REG directive
   - Register mask computation: 16-bit mask where bit N = register N (D0=bit0, D7=bit7, A0=bit8, A7=bit15)
   - Pre-decrement mode: register bit order is reversed (A7=bit0, D0=bit15) per M68000 manual
2. Port MOVEM instruction encoding:
   - Two forms: `MOVEM register_list,\u003cea\u003e` (register → memory) and `MOVEM \u003cea\u003e,register_list` (memory → register)
   - Size: .W (word) or .L (long)
   - Base opcodes: 0x4880 (register→memory .W), 0x48C0 (register→memory .L), 0x4C80 (memory→register .W), 0x4CC0 (memory→register .L)
3. Write `tests/asm/movem_test.cc`:
   - **Parsing tests:** Single register, range, combined, all-registers
   - **Mask computation tests:** D0-D7 = 0x00FF, A0-A7 = 0xFF00, D0/A0 = 0x0101, etc.
   - **Pre-decrement reversal tests:** D0-D7/A0 in -(An) mode = reversed mask
   - **Encoding tests:** MOVEM.W D0-D3,(A0) produces correct opcode + mask; MOVEM.L (A1)+,D0-D7/A0-A7 produces correct opcode + mask
   - **REG directive integration:** `REG D0-D3/A0-A2` defines a named register list symbol
4. Wire MOVEM handler into `HandleInstruction()` dispatch

**Quality Gate 8.5.4:** `movem_test.cc` passes with 25+ test cases. Register list parsing handles all valid syntax including ranges and register list symbols. Pre-decrement mode reverses register bit order. MOVEM.W and MOVEM.L produce correct opcodes and register masks.

---

#### Task 8.5.5 — Remaining Directives

Port the directives not yet implemented. Currently working: ORG, EQU, SET, DC.B/W/L, DS.B/W/L, EVEN, ODD, END. Missing from the original DIRECTIV.CPP:

Source: `EASy68K/Edit68K/DIRECTIV.CPP` (1043 lines).

Steps:
1. **DCB — Define Constant Block** (DIRECTIV.CPP:363-432):
   - Syntax: `DCB.size count,value`
   - Fill `count` copies of `value` (B/W/L size)
   - Word-align before DCB.W/L (same as DC alignment)
2. **DC string literals** (DIRECTIV.CPP:355-361):
   - Syntax: `DC.B 'Hello',0` or `DC.B "World"`
   - Single-quoted and double-quoted strings in DC.B
   - Each character becomes one byte; closing quote is not stored
3. **INCLUDE** (DIRECTIV.CPP:638-843):
   - Syntax: `INCLUDE 'filename'`
   - Recursive file inclusion: the included file is assembled as if its lines appeared inline
   - Nesting depth limit (original: no explicit limit, but practical limit via file handles)
   - Search path: current directory first, then include paths
4. **INCBIN** (DIRECTIV.CPP:845-870):
   - Syntax: `INCBIN 'filename'`
   - Raw binary file inclusion: file bytes are emitted directly into the output
5. **SECTION** (DIRECTIV.CPP:45-87):
   - Syntax: `SECTION name,flags`
   - Section support (CODE, DATA, BSS)
   - Affects listing and S-record output
6. **OFFSET** (DIRECTIV.CPP:89-107):
   - Syntax: `OFFSET expression`
   - Sets assembly mode where labels are offsets rather than absolute addresses
7. **ORG odd-address auto-alignment** (DIRECTIV.CPP:112-115):
   - If ORG target is odd, auto-increment to next even address with ODD_ADDRESS warning
   - Currently a TODO in `assembler.cc:109`
8. **OPT** (DIRECTIV.CPP:564-630):
   - Syntax: `OPT option[,option...]`
   - Options from original: `CRE` (cross-reference enable), `MEX`/`NOMEX` (macro expansion listing), `SEX`/`NOSEX` (structured expansion listing), `WAR`/`NOWAR` (warning messages), `CEX`/`NOCEX` (constant expansion in listing), `BIT` (bit-field instruction support)
9. **FAIL** (DIRECTIV.CPP:902-915):
   - Syntax: `FAIL message`
   - Forces an assembly error with a custom message
10. **LIST/NOLIST** (DIRECTIV.CPP:917-960):
    - Control listing output on/off
11. **PAGE** (DIRECTIV.CPP:962-975):
    - Page eject in listing output
12. **MEMORY** (DIRECTIV.CPP:899-991):
    - Define memory range attributes (ROM, READ, PROTECTED, INVALID) with start/end addresses
    - Memory map data is written as additional S0 records in the S-record output file (OBJECT.CPP:326-341)
13. **SIMHALT** (DIRECTIV.CPP:1002-1020):
    - Emit SIMHALT opcode (0xFFFF) as a simulator halt marker
14. **REG** (DIRECTIV.CPP:1022-1043):
    - Syntax: `label REG register_list`
    - Define a named register list symbol for use with MOVEM
15. **XDEF/XREF** — full implementation:
    - Currently no-ops; emit proper linker metadata in S-record output
16. Write `tests/asm/directives_test.cc`:
    - **DCB tests:** DCB.B 10,$FF; DCB.W 5,$1234; DCB.L 3,$DEADBEEF; zero count; negative count error
    - **DC string tests:** `DC.B 'Hello'`, `DC.B "World",0`, empty string, string with escape chars
    - **INCLUDE tests:** Simple include, nested include, include of non-existent file (error), include cycle detection
    - **INCBIN tests:** Binary file inclusion, non-existent file error
    - **SECTION tests:** CODE/DATA/BSS sections, section switching
    - **OFFSET tests:** Offset mode labels, return to absolute mode
    - **ORG odd-address tests:** `ORG $1001` produces ODD_ADDRESS warning and sets LC to $1002
    - **OPT tests:** Each option toggles correctly
    - **FAIL tests:** FAIL produces error with correct message
    - **LIST/NOLIST tests:** Listing output toggled on/off
    - **PAGE tests:** Page break emitted
    - **SIMHALT tests:** Emits 0xFFFF
    - **REG tests:** Named register list defined and usable with MOVEM
    - **XDEF/XREF tests:** Metadata recorded for S-record output

**Quality Gate 8.5.5:** `directives_test.cc` passes with 50+ test cases. All 16 directive types work correctly. INCLUDE handles nested includes and reports missing files. DCB fills correctly for all sizes. DC.B string handling matches original. ORG odd-address produces ODD_ADDRESS warning and auto-aligns.

---

#### Task 8.5.6 — Conditional Assembly

Port conditional assembly directives from the original ASSEMBLE.CPP. These allow portions of source code to be included or excluded based on conditions evaluated at assembly time.

Source: `EASy68K/Edit68K/ASSEMBLE.CPP` (lines 386-541).

Steps:
1. Port conditional assembly directives:
   - **IFC/IFNC** — string comparison: `IFC 'string1','string2'` / `IFNC 'string1','string2'`
   - **IFEQ/IFNE** — expression comparison: `IFEQ expression` (assemble if == 0) / `IFNE expression` (assemble if != 0)
   - **IFLT/IFLE/IFGT/IFGE** — relational: `IFLT expression` (assemble if < 0), etc.
   - **ELSE** — toggle assembly state within conditional block
   - **ENDC** — end conditional block
2. Implement nesting support:
   - IF within IF (nested conditionals)
   - Each nesting level has its own active/inactive state
   - When an outer level is inactive, inner levels are skipped entirely
3. Implement in `src/asm/directives.cc` with a conditional stack in the assembler state
4. Write `tests/asm/conditional_assembly_test.cc`:
   - **IFC tests:** Equal strings assemble, unequal strings skip; case-sensitive comparison
   - **IFNC tests:** Opposite of IFC
   - **IFEQ/IFNE tests:** Zero/non-zero expression values
   - **IFLT/IFLE/IFGT/IFGE tests:** Negative, zero, positive expression values
   - **ELSE tests:** ELSE toggles assembly state
   - **ENDC tests:** ENDC restores previous state
   - **Nesting tests:** IF within IF, ELSE within ELSE, 3+ levels deep
   - **Error tests:** ENDC without IF, IF without ENDC, ELSE without IF
   - **Integration tests:** Conditional assembly with EQU/SET symbols, forward references

**Quality Gate 8.5.6:** `conditional_assembly_test.cc` passes with 25+ test cases. All 8 conditional types work. Nesting works to arbitrary depth. ELSE toggles correctly. Mismatched delimiters produce appropriate errors.

---

#### Task 8.5.7 — Structured Control Flow

Port EASy68K-specific structured assembly constructs. These are educational extensions that generate conditional branch instructions at assembly time, allowing students to write high-level control flow without manually computing branch displacements.

Source: `EASy68K/Edit68K/STRUCTURED.CPP` (748 lines).

Steps:
1. Port each structured construct:
   - **WHILE expression / ENDW** — while loop: generate `IFxx` + `BRA` at top, `Bcc` at bottom
   - **FOR expr TO expr [STEP expr] / ENDF** — counted for loop: generate `CMPI` + `Bcc` + `BRA` at top, `ADDQ` + `BRA` at bottom
   - **REPEAT / UNTIL expression** — post-test loop: generate `Bcc` at bottom (branch back while condition false)
   - **IF expression / ELSE / ENDI** — structured if: generate `IFxx` + `BRA` skip, ELSE generates `BRA` around else block
   - **DBLOOP Dn,label / UNTIL expression** — DBcc loop: generate `DBcc` + `Bcc` for post-test
   - **UNLESS expression** — inverted if: assemble when expression is false
2. Each construct generates real 68000 instructions; the structured directives are syntactic sugar that the assembler expands
3. Implement in `src/asm/structured.cc` (new file) with a structured-control stack in the assembler state
4. Write `tests/asm/structured_test.cc`:
   - **WHILE tests:** Basic while loop, nested while, condition always false (zero iterations)
   - **FOR tests:** Basic for loop, step value, reverse step, nested for
   - **REPEAT tests:** Basic repeat-until, always-true condition (one iteration)
   - **IF/ELSE/ENDI tests:** If-only, if-else, nested if-else
   - **DBLOOP tests:** Basic DBLOOP, with UNTIL post-condition
   - **UNLESS tests:** Basic unless, unless-else
   - **Error tests:** ENDW without WHILE, ENDF without FOR, ENDI without IF, mismatched nesting
   - **Generated code tests:** Verify the generated branch instructions have correct displacements and opcodes

**Quality Gate 8.5.7:** `structured_test.cc` passes with 30+ test cases. All 6 structured constructs generate correct 68000 branch instructions. Nested structures work. Mismatched delimiters produce errors. Generated branch displacements are correct.

---

#### Task 8.5.8 — Macro Processor

Port the macro system that allows users to define and invoke named code templates with parameter substitution.

Source: `EASy68K/Edit68K/MACRO.CPP` (407 lines).

Steps:
1. Port macro definition and expansion:
   - **MACRO/ENDM** — define a macro: `name MACRO` ... `ENDM`
   - **Parameter substitution:** ``–`\9` positional parameters, `\A`–`\Z` named parameters
   - **Parameter substitution:** `\0` is reserved for the size extension letter (B/W/L) when called with a size suffix; `\1`–`\9` positional parameters; `\A`–`\Z` named parameters
   - **`\@` label numbering** — each macro expansion generates unique local labels by incrementing a counter
   - **NARG** — expands to the number of actual arguments passed to the current macro (MACRO.CPP:329-334)
   - **IFARG** — conditional expansion: `IFARG n` assembles following code only if parameter n was supplied
   - **MEXIT** — early exit from macro expansion (like `return` from a macro)
   - **Nested macro support** — macros can call other macros, depth limit MACRO_NEST_LIMIT=256 (matching original)
   - **Continuation lines** — lines starting with `&` are appended to the previous line
2. Store macro definitions in a macro table (separate from symbol table)
3. During assembly, when a line's opcode matches a macro name, expand the macro with the provided arguments
4. Up to 37 parameters per macro (matching original)
5. Implement in `src/asm/macro_processor.cc`
6. Write `tests/asm/macro_test.cc`:
   - **Definition tests:** Simple macro, macro with parameters, nested macro definition
   - **Expansion tests:** No-arg macro, 1-arg, multi-arg, default parameter handling
   - **Parameter substitution tests:** ``–`\9` correctly replaced, missing parameters produce empty string
   - **`\@` label tests:** Unique labels per expansion, multiple `\@` in same macro
   - **NARG tests:** NARG expands to correct argument count; NARG outside macro is error
   - **`\0` size tests:** Macro called with `.B`/`.W`/`.L` suffix passes correct size letter via `\0`
   - **IFARG tests:** IFARG with supplied/unsupplied parameters
   - **MEXIT tests:** Early exit skips remaining macro body
   - **Nested macro tests:** Macro calling another macro, recursive macro (with depth limit)
   - **Error tests:** Undefined macro call, ENDM without MACRO, MACRO without ENDM, parameter count mismatch

**Quality Gate 8.5.8:** `macro_test.cc` passes with 25+ test cases. Macro definition and expansion work. Parameter substitution is correct including `\0` size parameter. `\@` labels are unique per expansion. NARG returns correct argument count. IFARG and MEXIT work. Nested macros work to depth 256. Error cases are caught.

---

#### Task 8.5.9 — Error Reporter

Port the error reporting system with specific error codes and messages.

Source: `EASy68K/Edit68K/ERROR.CPP` (293 lines) + `asm.h` (error code constants).

Steps:
1. Port all error codes from the original `asm.h`:
   - 43+ specific error codes: `INV_16_BIT_DATA`, `INV_8_BIT_DATA`, `INV_MOVE_QUICK_CONST`, `INV_QUICK_CONST`, `INV_SHIFT_COUNT`, `INV_VECTOR_NUM`, `INV_BRANCH_DISP`, `INV_OFFSET`, `INV_EA`, `INV_SIZE_CODE`, `MISSING_SIZE_CODE`, `ILLEGAL_SIZE_CODE`, `INV_OP`, `INV_REGISTER`, `INV_LABEL`, `INV_SYMBOL`, `MULT_DEFD`, `UNDEF_SYMBOL`, `DIV_BY_ZERO`, etc.
   - Warning codes: `ODD_ADDRESS`, `INV_MEMORY`, etc.
2. Implement error formatting:
   - Line number, column, error code, descriptive message
   - Warning vs. error distinction (warnings don't halt assembly)
   - Error limit: stop assembly after N errors (original: configurable, default 10)
3. Replace generic error strings in `assembler.cc` with specific error codes
4. Implement in `src/asm/error_reporter.cc`
5. Write `tests/asm/error_reporter_test.cc`:
   - **Error code tests:** Each error code produces its specific message
   - **Formatting tests:** Error output contains line number, column, and context
   - **Warning tests:** Warnings are counted but don't halt assembly
   - **Error limit tests:** Assembly stops after N errors
   - **Integration tests:** Assemble invalid source and verify specific error codes (not generic "Unknown instruction")

**Quality Gate 8.5.9:** `error_reporter_test.cc` passes with 20+ test cases. All 43+ error codes have specific messages. Errors include line/column numbers. Warnings don't halt assembly. Error limit works.

---

#### Task 8.5.10 — Listing Generator

Port the listing output generator that produces `.L68` files.

Source: `EASy68K/Edit68K/LISTING.CPP` (297 lines).

Steps:
1. Port listing format generation:
   - `.L68` line format: `HHHHHH MMMM LL SSSSSSSSSSSSSS ;comment`
   - H = hex address (6 chars), M = machine code hex (4 chars per word, up to 5 words), L = line number (2 chars), S = source text
   - Continuation lines for multi-word instructions
   - Lines with no code (directives, comments) show address only
2. Port page formatting:
   - Page headers with filename, page number, date
   - Page length configurable (default 60 lines)
   - PAGE directive forces page break
3. Port LIST/NOLIST integration:
   - Lines between NOLIST and LIST are suppressed from listing output
   - Macro expansion lines can be listed or suppressed
4. Implement in `src/asm/listing_generator.cc`
5. Write `tests/asm/listing_test.cc`:
   - **Line format tests:** Address, machine code, line number, source text in correct columns
   - **Multi-word instruction tests:** Continuation lines for instructions with extension words
   - **Directive listing tests:** ORG, EQU, DS show address only; DC shows data bytes
   - **Page formatting tests:** Page break at correct line, header content
   - **LIST/NOLIST tests:** Lines suppressed between NOLIST/LIST
   - **Macro expansion tests:** Expanded lines shown with `+` prefix

**Quality Gate 8.5.10:** `listing_test.cc` passes with 20+ test cases. Listing output format matches original `.L68` format. Pagination works. LIST/NOLIST controls output. Macro expansion lines are marked.

---

#### Task 8.5.11 — Object/S-Record Output

Port the S-record output generator that produces `.S68`/`.S19` files.

Source: `EASy68K/Edit68K/OBJECT.CPP` (368 lines).

Steps:
1. Port S-record generation:
   - **S0 header record:** Module name, optional metadata
   - **S1 data record:** 16-bit address, up to 32 bytes payload, checksum
   - **S2 data record:** 24-bit address (for addresses > 64K)
   - **S3 data record:** 32-bit address (for addresses > 16M, rare)
   - **S8/S9 termination record:** Start address (entry point from END directive)
2. Port record construction:
   - Byte count field includes address + data + checksum
   - Checksum = one's complement of sum of all bytes in the record
   - Address type selection: S1 for addresses < $10000, S2 for < $1000000, S3 otherwise
   - Maximum 32 data bytes per record (configurable, original default)
3. Port XDEF/XREF metadata:
   - XDEF symbols emitted in S0 header or as special records
   - XREF symbols recorded for linker (informational in S-record context)
4. Port memory map S0 records (OBJECT.CPP:279-341):
   - MEMORY directive data (ROM, READ, PROTECTED, INVALID ranges) written as additional S0 records at end of file
   - `writeMap()` helper formats memory range attributes into S0 record payload
5. Implement in `src/asm/object_output.cc` (new file) or as part of `assembler.cc`
6. Write `tests/asm/object_output_test.cc`:
   - **S0 header tests:** Correct format, module name encoding
   - **S1 data tests:** 16-bit address, payload, checksum calculation
   - **S2 data tests:** 24-bit address for addresses > 64K
   - **S3 data tests:** 32-bit address for addresses > 16M
   - **S8/S9 termination tests:** Start address from END directive
   - **Checksum tests:** Verify one's complement sum for various payloads
   - **Record splitting tests:** Data > 32 bytes splits into multiple records
   - **Address type selection tests:** Correct S1/S2/S3 selection based on address range
   - **XDEF/XREF tests:** Symbol metadata included in output
   - **Memory map S0 tests:** MEMORY directive ranges produce correct S0 records with ROM/READ/PROTECTED/INVALID attributes
   - **Empty program tests:** S0 + S8/S9 with no data records
   - **Integration tests:** Assemble a program and verify complete S-record output matches expected

**Quality Gate 8.5.11:** `object_output_test.cc` passes with 25+ test cases. S-record output format is correct for all record types. Checksums are valid. Address type selection is correct. Record splitting handles large data blocks. XDEF/XREF metadata is included. Memory map S0 records are correct.

---

**Task 8.5 overall quality gate:** All 11 subtask quality gates pass. The assembler can assemble any valid 68000 program with any combination of instructions, directives, macros, and structured control flow. Output matches original EASy68K for golden test inputs.

**Subtask dependency order:**
1. **8.5.1** (instruction table wiring) — no dependencies
2. **8.5.2** (EA encoding + ext words) — depends on 8.5.1
3. **8.5.3** (instruction encoding handlers) — depends on 8.5.2
4. **8.5.4** (MOVEM) — depends on 8.5.3
5. **8.5.5** (remaining directives) — depends on 8.5.1 (can parallel with 8.5.2-8.5.4 for non-codegen directives like INCLUDE)
6. **8.5.6** (conditional assembly) — depends on 8.5.1
7. **8.5.7** (structured control flow) — depends on 8.5.3 (generates branch instructions)
8. **8.5.8** (macro processor) — depends on 8.5.5 (MACRO/ENDM are directive-like)
9. **8.5.9** (error reporter) — can parallel with 8.5.2-8.5.8
10. **8.5.10** (listing generator) — can parallel with 8.5.2-8.5.8
11. **8.5.11** (object output) — depends on 8.5.3 (needs assembled data)

**Estimated test count across all subtasks:** 370+ new tests

**Explicitly excluded from this port (not in original or commented out):**
- MOVES (68010) — commented out in original INSTTABL.CPP and BUILD.CPP
- MOVEC (68010) — commented out in original INSTTABL.CPP and BUILD.CPP
- BINFILE.CPP binary output — Teeside format; dead code in original (not in EDIT68K.bpr build, never called)
- Negated register lists (`/(-D0-D3)`) — not implemented in original MOVEM.CPP

---

### Task 8.6 — Golden Assembly Tests

New file: `tests/asm/golden_assembly_test.cc`

For each golden triplet in `data/golden/asm/`:
1. Read the `.X68` source file
2. Assemble it with `libeasym68k-asm`
3. Compare the output S-Record byte-for-byte against the golden `.S68`
4. Compare the listing output against the golden `.L68`

```cpp
TEST_P(GoldenAssemblyTest, ByteIdenticalOutput) {
  auto [x68_path, s68_path, l68_path] = GetParam();
  std::string source = ReadFile(x68_path);
  auto result = assembler.Assemble(source);
  ASSERT_TRUE(result.success);

  std::string golden_s68 = ReadFile(s68_path);
  EXPECT_EQ(assembler.GenerateSRecord(), golden_s68);

  std::string golden_l68 = ReadFile(l68_path);
  EXPECT_EQ(assembler.GetListing(), golden_l68);
}
```

**Quality Gate 8.6:** All 20+ golden assembly tests pass with byte-for-byte identical output. Any failure blocks all subsequent work.

---

## Phase 9: Simulator Core Library — Full Parity & Headless Readiness

This phase closes all remaining gaps between the ported simulator core and the
original EASy68K simulator (`Sim68K/RUN.CPP`, `hardwareu.cpp`, `SIM68Ku.cpp`).
After this phase, the simulator can execute any valid M68K program headlessly with
identical behavior to the original — including interrupts, cycle counting, SIMHALT
configuration, execution logging, and all exception processing.

The core must remain **GUI-independent**: all features are accessible via the
`M68kSimulator` public API and `Config` struct interfaces alone. No Qt or platform-
specific code is introduced.

**Prerequisite:** Phase 8 (Assembler) complete. Golden assembly tests (8.6) passing.
The assembler is needed to generate `.S68` test programs for Phase 9.

---

### Task 9.1 — Interrupt Injection and Processing

Port the original's interrupt system from `RUN.CPP:1083-1117` (`irqHandler()`)
and `hardwareu.cpp:757-775` (`IRQprocess()`).

**New public API on `M68kSimulator`:**

```cpp
// Inject a hardware interrupt at the given level (1-7).
// Thread-safe; may be called from any thread (e.g., timer, hardware sim).
// The interrupt is processed on the next Step() or Run() iteration.
void Interrupt(int level);

// Returns the current pending IRQ bitmask (for testing).
uint8_t GetPendingIrqs() const;

// Clear all pending interrupts.
void ClearPendingIrqs();
```

**Implementation details:**

1. Add `std::atomic<uint8_t> pending_irqs_{0}` to `M68kSimulator`. Bit 0 = IRQ1,
   bit 6 = IRQ7. `Interrupt(level)` sets the appropriate bit atomically.

2. **IRQ check in `Step()`** — after the existing exception processing `switch`,
   if `pending_irqs_ & int_mask` is non-zero, call `HandleIrq()`:
   - Push exception stack frame (class 2 format: info word + PC + SR)
   - Clear the highest-priority pending IRQ bit
   - Set the SR interrupt mask to the serviced level
   - Load PC from autovector address (`$64 + level * 4`)
   - Add 34 cycle penalty (per original `inc_cyc(34)`)

3. **STOP wake-on-interrupt** — When `state_.stopped == true` and a non-masked
   IRQ is pending, `Interrupt()` must clear `stopped` and `halted` so that the
   next `Step()` or `Run()` call resumes execution. Port from `hardwareu.cpp:763-774`.

4. **Interrupt priority** — IRQ7 (NMI) can never be masked. Levels 1-6 are masked
   when `SR interrupt mask >= IRQ level`. Port from `RUN.CPP:1086-1114`.

**Source references:**
- `RUN.CPP:878-880` — IRQ check after each instruction
- `RUN.CPP:1083-1117` — `irqHandler()` with autovector dispatch
- `hardwareu.cpp:757-775` — `IRQprocess()` with STOP wake
- `hardwareu.cpp:796-903` — Auto-IRQ timer management (GUI side, not ported here)

**New test file:** `tests/sim/interrupt_test.cc`

Minimum test cases (15):
1. `InterruptSetsPendingBit` — Interrupt(5) sets bit 4 in pending bitmask
2. `InterruptLevel1Through7` — all 7 levels set correct bits
3. `IrqBelowMaskNoException` — mask=7, IRQ level 3 → no exception
4. `IrqAboveMaskGeneratesException` — mask=3, IRQ level 5 → exception
5. `Irq7NmiCannotBeMasked` — mask=7, IRQ level 7 → exception (NMI)
6. `AutovectorDispatchCorrectAddress` — IRQ level 3 loads PC from vector $6C
7. `IrqSetsNewInterruptMask` — after servicing IRQ5, SR mask = 5
8. `IrqClearsPendingBit` — after servicing, the serviced IRQ bit is cleared
9. `IrqStackFrameCorrect` — PC and SR pushed in correct order, supervisor set
10. `IrqClearsTraceFlag` — trace mode active, IRQ clears T bit
11. `MultiplePendingIrqsHighestFirst` — IRQ3+IRQ5 pending, services IRQ5 first
12. `StopWakesOnInterrupt` — STOP instruction active, IRQ arrives, execution resumes
13. `StopDoesNotWakeOnMaskedIrq` — STOP active, masked IRQ, stays stopped
14. `ThreadSafeInterrupt` — Interrupt() called from another thread during Run()
15. `ClearPendingIrqs` — clears all pending without processing

**Quality Gate 9.1:** All 15 interrupt tests pass. `Interrupt()` is thread-safe.
STOP instruction wakes on non-masked interrupt.

---

### Task 9.2 — Cycle Counting

Port the original's `inc_cyc()` from `RUN.CPP`. The original increments a cycle
counter in every instruction handler. The ported `CpuState::cycles` field exists
but nothing increments it.

**Implementation details:**

1. Add `void IncrementCycles(uint64_t count)` private helper to `M68kSimulator`
   that adds to `state_.cycles`.

2. **Cycle data table** — Create `src/sim/cycle_data.cc` containing a lookup table
   of base cycle counts per instruction/encoding type. Source: M68000 Programmer's
   Reference Manual, Section 8 (Instruction Execution Times).

   Key cycle counts from the PRM:
   | Instruction | Byte | Word | Long | EA Penalty |
   |-------------|------|------|------|------------|
   | MOVE reg→reg | 4 | 4 | 4 | +EA |
   | MOVE reg→mem | 8 | 8 | 12 | +EA |
   | MOVE mem→reg | 4 | 4 | 4 | +EA |
   | ADD/SUB reg,reg | 4 | 4 | 8 | — |
   | ADD/SUB #imm,reg | 8 | 8 | 16 | — |
   | MULU | 70 | | | +EA |
   | MULS | 70 | | | +EA |
   | DIVU | 140 | | | +EA |
   | DIVS | 158 | | | +EA |
   | Bcc taken | 10 | | | — |
   | Bcc not-taken | 8 (byte), 12 (word) | | | — |
   | BRA/BSR | 10 | | | — |
   | NOP | 4 | | | — |
   | LEA | 4 | | | +EA |
   | PEA | 12 | | | +EA |
   | JSR | 8 | | | +EA |
   | JMP | 4 | | | +EA |
   | RTS | 16 | | | — |
   | RTE | 20 | | | — |
   | TRAP | 34 | | | — |
   | STOP | 4 | | | — |
   | RESET | 132 | | | — |
   | Exception processing | 34-44 | | | — |

3. **EA penalty** — Each addressing mode adds cycle penalties. Add `EaCyclePenalty()`
   function to `cycle_data.cc`:
   | Mode | Penalty |
   |------|---------|
   | Dn | 0 |
   | An | 0 |
   | (An) | 4 |
   | (An)+ | 4 |
   | -(An) | 6 |
   | d(An) | 8 |
   | d(An,Xi) | 10 |
   | (d8,An,Xn) | — |
   | #imm | 4 (word), 8 (long) |
   | d(PC) | 8 |
   | d(PC,Xi) | 10 |

4. **Instrument every opcode handler** — Each `Op*()` method calls
   `IncrementCycles(base + ea_penalty)` at the appropriate point. This is
   mechanical but touches every handler in all 5 opcode files.

**Source references:**
- `RUN.CPP` — `inc_cyc()` calls scattered throughout all instruction handlers
- `extern.h` — `unsigned long cycles` global declaration
- M68000 PRM Section 8 — authoritative cycle count tables

**New test file:** `tests/sim/cycle_test.cc`

Minimum test cases (20):
1. `NopIs4Cycles` — NOP increments cycles by 4
2. `MoveRegRegWord` — MOVE.W D0,D1 = 4 cycles
3. `MoveRegMemWord` — MOVE.W D0,(A0) = 8 cycles + 4 EA = 12
4. `MoveMemRegLong` — MOVE.L (A0),D0 = 4 cycles + 4 EA = 8
5. `AddRegRegByte` — ADD.B D0,D1 = 4 cycles
6. `AddRegRegLong` — ADD.L D0,D1 = 8 cycles
7. `AddImmReg` — ADDI.W #1,D0 = 8 cycles
8. `SubImmReg` — SUBI.W #1,D0 = 8 cycles
9. `MuluIs70Cycles` — MULU D0,D1 = 70 + EA
10. `MulsIs70Cycles` — MULS D0,D1 = 70 + EA
11. `DivuIs140Cycles` — DIVU D0,D1 = 140 + EA
12. `DivsIs158Cycles` — DIVS D0,D1 = 158 + EA
13. `BccTakenIs10` — BRA taken = 10 cycles
14. `BccNotTakenByte` — BRA not-taken byte = 8 cycles
15. `BccNotTakenWord` — BRA not-taken word = 12 cycles
16. `LeaIs4PlusEA` — LEA (A0),A1 = 4 + 4 = 8
17. `RtsIs16Cycles` — RTS = 16
18. `RteIs20Cycles` — RTE = 20
19. `TrapIs34Cycles` — TRAP #15 = 34
20. `ResetIs132Cycles` — RESET = 132
21. `ResetClearsCycles` — Trap #15 task 30 resets counter to 0
22. `GetCyclesReturnsCurrent` — Trap #15 task 31 returns current count
23. `EaPenaltyAnIndirect` — (An) adds 4 cycles
24. `EaPenaltyAnPostInc` — (An)+ adds 4 cycles
25. `EaPenaltyAnPreDec` — -(An) adds 6 cycles
26. `EaPenaltyAnDisplacement` — d(An) adds 8 cycles
27. `EaPenaltyAnIndex` — d(An,Xi) adds 10 cycles
28. `EaPenaltyImmediate` — #imm adds 4 (word) or 8 (long)
29. `MultipleInstructionsAccumulate` — 10 NOPs = 40 cycles

**Quality Gate 9.2:** All 29 cycle tests pass. Trap #15 tasks 30-31 return correct
cycle counts. Cycle counts match original EASy68K within ±1 cycle for all tested
instructions.

---

### Task 9.3 — SIMHALT Configuration and Exception Toggle

Port two runtime configuration options from the original that affect core behavior.

**9.3a — SIMHALT enable/disable**

The original supports `*[sim68k]simhalt_off` in .L68 listing comments to disable
the SIMHALT instruction. When disabled, opcode `0xFFFF` is treated as a Line-F
exception instead of halting the simulator.

Source: `SIM68Ku.cpp:320-328` — `*[sim68k]` command parsing.

**Changes:**

1. Add `bool simhalt_enabled = true` to `M68kSimulator::Config`.

2. Modify `DispatchSimhalt()` in `decode.cc`:
   ```cpp
   // Before (always halts):
   if (opcode == 0xFFFF) return SimResult::kHalted;

   // After (configurable):
   if (opcode == 0xFFFF) {
     if (!config_.simhalt_enabled) return SimResult::kLine1111;
     return SimResult::kHalted;
   }
   ```

**9.3b — Exception processing enable/disable**

The original has `exceptions` flag controlled by `Hardware->setExceptionsEnabled()`.
When disabled, the simulator skips exception processing for illegal instructions,
privilege violations, etc.

Source: `hardwareu.cpp:831-855` — Trap #15 task 32 subtask 5.

**Changes:**

1. Add `bool exceptions_enabled = true` to `M68kSimulator::Config`.

2. In `Step()`, when `exceptions_enabled == false`, skip the exception-handling
   `switch` after `ExecuteInstruction()`. Return `result` directly without calling
   `HandleException()`.

3. Wire to existing `ISimulatorEnv::SetExceptionsEnabled()` — the `SimulatorEnv`
   backend can flip this flag at runtime via Trap #15 task 32 subtask 5.

**New test cases** added to `tests/sim/simulator_test.cc`:

1. `SimhaltEnabledHalts` — default config, SIMHALT returns kHalted
2. `SimhaltDisabledReturnsLineF` — simhalt_enabled=false, SIMHALT returns kLine1111
3. `SimhaltDisabledTriggersException` — SIMHALT when disabled → HandleException(kLine1111)
4. `ExceptionsEnabledProcessesIllegal` — illegal opcode → kIllegalInst exception
5. `ExceptionsDisabledSkipsIllegal` — exceptions_enabled=false, illegal opcode → returns kBadInstruction without exception processing
6. `ExceptionsDisabledSkipPrivilege` — MOVE to SR in user mode → no exception
7. `ConfigDefaultsBothEnabled` — default Config has both flags true
8. `SimhaltToggleAtRuntime` — change simhalt_enabled between Steps

**Quality Gate 9.3:** All 8 config tests pass. SIMHALT can be toggled. Exceptions
can be disabled. Default behavior is unchanged (both enabled).

---

### Task 9.4 — Execution Logging Interface

Port the core execution logging capability from the original's `RUN.CPP:834-845`
and `logU.cpp`. The original writes PC, opcode, register state, and optional memory
dump to a file on each instruction step.

This task adds a **core logging interface** so that any client (GUI, CLI, test
harness) can receive execution trace data without modifying the simulator core.

**New interface file:** `include/easym68k/sim/iexecution_logger.h`

```cpp
class IExecutionLogger {
 public:
  virtual ~IExecutionLogger() = default;

  // Called after each instruction execution in Step()/Trace mode.
  // pc: address of the instruction that just executed
  // opcode: the fetched opcode word
  // state: full CPU state after execution
  virtual void LogInstruction(uint32_t pc, uint16_t opcode,
                               const CpuState& state) = 0;

  // Called to dump a memory range (when "Instructions + Registers + Memory"
  // logging mode is active).
  virtual void LogMemoryDump(uint32_t start, uint32_t length,
                              const Memory& memory) = 0;

  // Called for each Trap #15 text output when output logging is active.
  virtual void LogOutput(const char* text, int length) = 0;

  // Configuration: what to log
  enum class Mode { kOff, kInstructions, kInstructionsAndMemory };
  virtual void SetMode(Mode mode) = 0;
  virtual Mode GetMode() const = 0;

  // Memory dump range configuration
  virtual void SetDumpRange(uint32_t start, uint32_t bytes) = 0;
};
```

**Changes to `M68kSimulator`:**

1. Add `IExecutionLogger* execution_logger = nullptr` to `Config`.

2. In `Step()`, after instruction execution, if `config_.execution_logger` is
   non-null and mode != kOff:
   ```cpp
   if (config_.execution_logger &&
       config_.execution_logger->GetMode() != IExecutionLogger::Mode::kOff) {
     config_.execution_logger->LogInstruction(inst_pc_, opcode, state_);
     if (config_.execution_logger->GetMode() == Mode::kInstructionsAndMemory) {
       config_.execution_logger->LogMemoryDump(dump_start, dump_bytes, memory_);
     }
   }
   ```

3. In `DispatchTrap15()`, for all text output tasks (0-1, 6, 13-14, 17-18, 20),
   call `LogOutput()` if logger is active.

**Source references:**
- `RUN.CPP:834-845` — execution log writing in the instruction loop
- `SIM68Ku.cpp` — log configuration dialog
- `logU.cpp` — log file management (full 296-line source)

**New test file:** `tests/sim/execution_logger_test.cc`

Minimum test cases (12):
1. `NullLoggerIsNoOp` — nullptr execution_logger, Step() works normally
2. `LogInstructionCalledPerStep` — mock logger receives LogInstruction for each Step()
3. `LogInstructionReceivesCorrectPc` — logged PC matches instruction address
4. `LogInstructionReceivesCorrectOpcode` — logged opcode matches fetched word
5. `LogInstructionReceivesPostExecutionState` — logged state reflects instruction result
6. `ModeOffNoLogging` — Mode::kOff → no LogInstruction calls
7. `ModeInstructionsOnly` — Mode::kInstructions → LogInstruction but no LogMemoryDump
8. `ModeInstructionsAndMemory` — Mode::kInstructionsAndMemory → both called
9. `DumpRangeConfigurable` — SetDumpRange sets the range used in LogMemoryDump
10. `LogOutputCapturesText` — Trap #15 task 0 triggers LogOutput
11. `LogOutputCapturesChar` — Trap #15 task 6 triggers LogOutput
12. `MultipleStepsLogMultiple` — 10 Steps → 12 LogInstruction calls

**Quality Gate 9.4:** All 12 execution logger tests pass. Logger is fully optional.
No performance impact when logger is null. Mode switching works at runtime.

---

### Task 9.5 — Event/Observer System

Add an observer pattern to `M68kSimulator` that decouples monitoring and debugging
from the core execution loop. This enables multiple simultaneous consumers of
simulator state changes without modifying core code.

**New interface file:** `include/easym68k/sim/isim_observer.h`

```cpp
enum class SimEventType {
  kInstructionExecuted,
  kBreakpointHit,
  kExceptionRaised,
  kInterruptRequested,
  kHalted,
  kStopped,       // STOP instruction
  kResumed,       // Woke from STOP
};

struct SimEvent {
  SimEventType type;
  uint32_t pc;
  SimResult result;
  int interrupt_level;  // valid for kInterruptRequested
};

class ISimObserver {
 public:
  virtual ~ISimObserver() = default;
  virtual void OnEvent(const SimEvent& event) = 0;
};
```

**Changes to `M68kSimulator`:**

1. Add observer management:
   ```cpp
   void AddObserver(ISimObserver* observer);
   void RemoveObserver(ISimObserver* observer);
   ```

2. Add `std::vector<ISimObserver*> observers_` member.

3. Emit events at key points in `Step()` and `Run()`:
   - `kInstructionExecuted` — after each successful Step()
   - `kBreakpointHit` — when Run() stops at a breakpoint
   - `kExceptionRaised` — when HandleException() is called
   - `kInterruptRequested` — when Interrupt() is called
   - `kHalted` — when state becomes halted
   - `kStopped` — when STOP instruction executes
   - `kResumed` — when STOP wakes from interrupt

4. **Thread safety** — Observer notifications happen on the simulator's execution
   thread (the thread calling Step()/Run()). Observers must be fast or defer work.
   No mutex needed since the call is synchronous.

**New test file:** `tests/sim/observer_test.cc`

Minimum test cases (12):
1. `AddRemoveObserver` — add and remove, no crash
2. `InstructionExecutedEvent` — Step() emits kInstructionExecuted
3. `EventHasCorrectPc` — event.pc matches instruction address
4. `BreakpointHitEvent` — Run() stops at BP, emits kBreakpointHit
5. `ExceptionRaisedEvent` — illegal opcode → kExceptionRaised
6. `HaltedEvent` — SIMHALT → kHalted
7. `StoppedEvent` — STOP instruction → kStopped
8. `ResumedEvent` — STOP + Interrupt() → kResumed
9. `InterruptRequestedEvent` — Interrupt(5) → kInterruptRequested
10. `MultipleObservers` — 3 observers all receive the same event
11. `RemoveObserverDuringRun` — removing observer between Steps is safe
12. `NullObserverIgnored` — nullptr observer is silently ignored

**Quality Gate 9.5:** All 12 observer tests pass. No performance impact when
observer list is empty (check vector empty before iterating). Observer removal
is safe during iteration.

---

### Task 9.6 — Simulator Core Integration Test

A comprehensive integration test that exercises all Phase 9 features together
to prove the simulator can run a non-trivial M68K program headlessly with full
parity.

**New test file:** `tests/sim/core_integration_test.cc`

Test program (assembled by the Phase 8 assembler or hand-coded):

```asm
    ORG     $1000
START:
    MOVE.W  #$1234,D0     ; load test value
    MOVE.W  #$5678,D1     ; load test value
    ADD.W   D0,D1         ; D1 = $68AC
    MOVE.W  D1,$2000      ; store result
    TRAP    #15            ; task 9 = terminate
    DC.W    9
```

Integration test cases (10):
1. `FullProgramExecution` — assemble, load, run, verify D1=$68AC at $2000
2. `CycleCountAccumulated` — above program, verify total cycles > 0
3. `InterruptDuringRun` — run a long loop, inject IRQ3, verify handler called
4. `StopWakeCycle` — STOP instruction, IRQ wakes it, continues execution
5. `LoggerCapturesFullTrace` — run with logger, verify all instructions logged
6. `ObserverReceivesAllEvents` — run with observer, verify event sequence
7. `SimhaltDisabledStillRuns` — program with 0xFFFF data, simhalt_off, no halt
8. `ExceptionsDisabledNoTrap` — illegal opcode, exceptions off, no exception
9. `MemoryProtectionWithInterrupt` — protected memory, IRQ handler writes to it
10. `HeadlessTrap15IO` — program does text output via mock ITextIO, verify output

**Quality Gate 9.6:** All 10 integration tests pass. The simulator runs a complete
M68K program from S-Record load through termination with correct register state,
memory state, cycle count, interrupt handling, and event trace.

---

### Phase 9 Summary

| Task | Description | Tests | Source |
|------|-------------|-------|--------|
| 9.1 | Interrupt Injection & Processing | 15 | RUN.CPP irqHandler(), hardwareu.cpp IRQprocess() |
| 9.2 | Cycle Counting | 29 | RUN.CPP inc_cyc(), M68000 PRM Section 8 |
| 9.3 | SIMHALT Config & Exception Toggle | 8 | SIM68Ku.cpp *[sim68k], hardwareu.cpp setExceptionsEnabled() |
| 9.4 | Execution Logging Interface | 12 | RUN.CPP logging, logU.cpp |
| 9.5 | Event/Observer System | 12 | New (not in original, enables extensibility) |
| 9.6 | Core Integration Test | 10 | Cross-cutting |

**Total new tests:** ~86

**Phase exit criteria:**
- All 86 Phase 9 tests pass
- Simulator can run any valid M68K program headlessly
- Interrupt injection is thread-safe
- Cycle counting matches original ±1 cycle
- SIMHALT and exception toggles work
- Execution logging works without GUI
- Observer system delivers events to all registered listeners
- Zero Qt dependencies in core library

---

## Phase 10: Golden Simulation Traces (Proposal Section 5)

### Task 10.1 — Implement Golden Trace Comparison

New file: `tests/sim/golden_trace_test.cc`

Load a golden execution trace (JSON) and compare step-by-step:

```cpp
TEST_P(GoldenTraceTest, ExecutionTraceMatches) {
  LoadSRecordIntoSimulator(GetParam().s68_path);
  for (const auto& expected : GetParam().trace_steps) {
    SimResult result = sim.Step();
    ASSERT_EQ(result, SimResult::kOk);
    EXPECT_EQ(sim.State().pc, expected.pc);
    EXPECT_EQ(sim.State().d, expected.d);
    EXPECT_EQ(sim.State().a, expected.a);
    EXPECT_EQ(sim.State().sr, expected.sr);
    // Verify memory writes
    for (const auto& write : expected.memory_writes) {
      auto actual = sim.GetMemory().Read(write.address, write.size);
      EXPECT_EQ(actual.value, write.value);
    }
  }
}
```

**Quality Gate 10.1:** All 10+ golden simulation trace tests pass with 100% register and memory parity.

---

## Phase 11: Exception and Interrupt Tests

### Task 11.1 — Exception Tests

New file: `tests/sim/exception_test.cc`

Test cases:
1. Bus error on unmapped memory read
2. Address error on odd-address word read
3. Privilege violation (MOVE to SR in user mode)
4. Illegal instruction exception
5. Divide by zero exception
6. CHK exception (Dn < 0, Dn > bound)
7. TRAPV exception (V flag set)
8. Trace exception (T bit set, single-step)
9. Line 1010 exception
10. Line 1111 exception
11. Exception stack frame: verify PC and SR are pushed in correct order on the supervisor stack
12. RTE restores both SR and PC
13. Double bus fault halts the simulator

**Quality Gate 11.1:** All 13 exception tests pass.

---

### Task 11.2 — Interrupt Tests

New file: (part of `exception_test.cc` or separate)

Test cases:
1. IRQ at level 5 with interrupt mask = 3 → exception generated
2. IRQ at level 5 with interrupt mask = 7 → no exception (masked)
3. Autovector interrupt → reads from vector $64+$68
4. Interrupt clears trace flag
5. Stop instruction wakes on pending interrupt

**Quality Gate 11.2:** All 5 interrupt tests pass.

---

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

## Phase 13: GUI Implementation

This phase implements the three Qt GUI applications. Every task references specific features from the Feature Coverage Matrix above. The simulator GUI is the largest and most complex application and receives the most detailed breakdown.

Prerequisite: All quality gates from Phases 0–12 must pass before any GUI work begins. The core libraries must be proven correct via golden tests before GUI development starts.

### Task 13.1 — Implement Trap #15 Qt Backends

Create the concrete Qt classes implementing the 9 I/O interfaces defined in Phase 7 plus `ILogger`. Each backend is a thin Qt wrapper around the corresponding Qt module.

| File | Interface | Qt Classes Used | Features Covered |
|------|-----------|----------------|------------------|
| `qt_text_io.h/.cc` | `ITextIO` | `QTextEdit`, `QKeyEvent` | #9 (output console) |
| `qt_file_io.h/.cc` | `IFileIO` | `QFile`, `QFileDialog` | #25 (file I/O) |
| `qt_serial_io.h/.cc` | `ISerialIO` | `QSerialPort`, `QSerialPortInfo` | #17 (serial I/O) |
| `qt_network_io.h/.cc` | `INetworkIO` | `QUdpSocket`, `QTcpSocket`, `QTcpServer` | #18 (TCP/UDP) |
| `qt_graphics_io.h/.cc` | `IGraphicsIO` | `QPainter`, `QPixmap`, `QPaintDevice` | #10 (graphics) |
| `qt_sound_io.h/.cc` | `ISoundIO` | `QMediaPlayer`, `QAudioOutput`, `QSoundEffect` | #11 (sound) |
| `qt_peripheral_io.h/.cc` | `IPeripheralIO` | `QMouseEvent`, `QKeyEvent` | #26 (peripherals) |
| `qt_simulator_env.h/.cc` | `ISimulatorEnv` | `QThread`, `QElapsedTimer` | #12 (interrupts), #27 (fullscreen) |
| `qt_print_io.h/.cc` | `IPrintIO` | `QPrinter`, `QPainter` | #24 (printing) |

All implementations must be thread-safe. The simulator runs in a dedicated `QThread` (Proposal Section 7.2). Asynchronous operations (text output, graphics drawing, sound) use `Qt::QueuedConnection`. Synchronous operations (text input, file dialogs) use `QMetaObject::invokeMethod` with `Qt::BlockingQueuedConnection`.

**Quality Gate 13.1:** All 9 Qt backends compile and instantiate. A minimal test harness creates each backend and verifies its interface methods are callable without crash. Thread-safe signal/slot wiring verified for both sync and async paths.

---

### Task 13.2 — Sim68K-Qt Main Window and Execution Controls

Build the main simulator window shell with the threading model and execution controls.

Reference: `EASy68K/Sim68K/SIM68Ku.cpp` (main form), `SIM68Ku.dfm` (layout)

1. **Main window layout** — Create `gui/simulator/mainwindow.h/.cc` with:
   - Menu bar: File, Run, View, Options, Help
   - Toolbar: Open, Run (F9), Step Over, Step Into, Step Out, Pause, Reset, Memory Viewer, Hardware Window, I/O Console
   - Central area placeholder (split between registers panel and source/code view)
   - Bottom message pane (resizable, for simulator log messages)
   - Logging indicator label (visible when logging active)

2. **Threading model** — Implement `QThread`-based simulator execution:
   - `SimulatorThread` class wrapping `M68kSimulator` in a dedicated `QThread`
   - `QMetaObject::invokeMethod` with `Qt::BlockingQueuedConnection` for Step/Trace (synchronous — caller waits for step result)
   - `Qt::QueuedConnection` for Run (asynchronous — caller returns immediately, `stop_requested_` stops the loop)
   - State signals: `executionPaused()`, `executionStopped()`, `breakpointHit(uint32_t address)`, `exceptionRaised(SimResult)`
   - UI thread connects signals to update all views atomically after each instruction (in Step/Trace mode) or on pause/breakpoint

3. **S-Record loading** — Wire `QFileDialog` to `SRecordLoader::LoadFromFile()`, populate Memory, set PC from start address, load .L68 listing file. Feature #16.

4. **Drag-and-drop** — `setAcceptDrops(true)`, accept `.S68` files dropped onto the window. Feature #28.

5. **Execution controls** — Wire toolbar buttons and keyboard shortcuts (F9=Run, F8=Step Over, F7=Step Into, F6=Step Out, Pause, Ctrl+R=Reset) to `M68kSimulator::Step()`/`Run()`/`RequestStop()`. Features #21.

6. **View menu** — Menu items to open/show: Memory Viewer, Hardware Window, Stack Window, I/O Console, Log Window. Each creates or shows the corresponding `QDialog`/`QDockWidget`.

7. **Options menu** — Font selection, Memory Ranges dialog (Task 13.15), Logging setup (Task 12.12).

**Quality Gate 13.2:** Sim68K-Qt launches. User can open an `.S68` file, see the PC set, and use Step/Run/Pause/Reset controls. State signals correctly update a minimal register display. Drag-and-drop works.

---

### Task 13.3 — Register Display Widget

Build `gui/simulator/register_widget.h/.cc` — the dense register panel.

Reference: `EASy68K/Sim68K/SIM68Ku.cpp` register editing controls, `SIM68Ku.dfm` register group box layout.

Features: #1 (register display), #30 (cycle counter)

1. **Layout** — Grid of labeled hex spin boxes:
   - D0–D7 (8 data registers, 32-bit hex display)
   - A0–A7 (8 address registers, 32-bit hex display)
   - PC (32-bit hex display)
   - USP, SSP (32-bit hex display, visible based on supervisor mode)
   - SR bits: T, S, I[2:0], X, N, Z, V, C (individual checkboxes or indicator lights)
   - Cycles counter (64-bit decimal display with "Clear Cycles" button)

2. **Editing** — User can click any register field and type a hex value. On Enter/focus-out, the value is written to `CpuState`. This is how the original works: `loadRegs()` reads the edit controls back into the CPU state before each Step/Run.

3. **Update from simulator** — After each Step/Trace/Pause, the widget reads `CpuState` and updates all fields. The currently-changed fields should be highlighted (e.g., red text for one cycle) to draw attention.

4. **Supervisor mode** — When SR.S bit changes, swap A7 display between USP and SSP, and show/hide the USP/SSP fields accordingly.

**Quality Gate 13.3:** Register widget displays all D0-D7, A0-A7, PC, USP, SSP, SR bits, and Cycles. User can edit values and see them take effect on the next Step. Changed registers highlight after each step.

---

### Task 13.4 — Source Code View

Build `gui/simulator/source_view.h/.cc` — the listing-based source-level debugging view.

Reference: `EASy68K/Sim68K/SIM68Ku.cpp` ListBox1 (listing display), `highlight()`, `breakPMouseDown()`, `isInstruction()`, `lineToLog()`, lines 297-351 (L68 loading), lines 310-337 (*[sim68k] commands).

Features: #2 (source code display), #20 (breakpoints), #22 (run-to-cursor), #23 (*[sim68k] commands)

This is the primary code display in the original simulator. It loads the `.L68` listing file produced by the assembler, which contains lines in the format:
```
001000 4E71 30            NOP          ; comment
001002 303C 1234         MOVE.W #$1234,D0
```
Each line has: 8-char hex address, machine code, line number, source text. The view maps PC values to listing lines for highlighting.

1. **Listing model** — `QAbstractListModel` subclass:
   - Loads `.L68` file into a `QStringList`
   - Provides columns: Breakpoint indicator, Address, Code, Line#, Source
   - `isInstruction()` method: a line is an instruction if address starts with "00" (not "FF"), has data at position 11, and position 9/11 is not '=' (not DS/DC/EQU/SET). Port the exact logic from `SIM68Ku.cpp line 856`.
   - Address-to-row mapping: `QMap<uint32_t, int>` mapping PC addresses to listing rows for O(1) PC lookup

2. **Breakpoint column** — Left column drawn with `QStyleOptionViewItem`:
   - Green dot for instructions without a breakpoint
   - Red dot for instructions with a breakpoint
   - Empty for non-instruction lines
   - Click toggles breakpoint on/off (calls `M68kSimulator::AddBreakpoint`/`RemoveBreakpoint`)

3. **PC highlight** — When PC changes, binary-search the listing for the matching address row (port `highlight()` logic from line 785). Highlight the entire row with blue background. Auto-scroll to keep the highlighted row visible.

4. ***[sim68k] commands** — On `.L68` load, scan for these comment directives:
   - `*[sim68k]break` → set breakpoint on the next instruction line (port from lines 320-328)
   - `*[sim68k]bitfield` → enable bitfield instruction support (set flag on M68kSimulator)
   - `*[sim68k]simhalt_off` → disable SIMHALT instruction (opcode 0xFFFF treated as Line-F exception)

5. **Run-to-cursor** — Double-click or context menu on a listing line sets a temporary breakpoint at that address and starts Run mode. Port logic from `RunToCursorExecute()` at line 468.

6. **Search** — Find dialog that searches listing text (port `find()` at line 818).

7. **Horizontal scroll** — `QScrollBar` for scrolling the listing left/right (port `ScrollBar1Change()` at line 912).

8. **Font selection** — View → Font menu opens `QFontDialog`, applies to listing display. Port `FontSourceExecute()` at line 940.

**Quality Gate 13.4:** Source view loads a `.L68` file, displays it with correct columns. PC highlight works. Breakpoints can be toggled by clicking the margin. Run-to-cursor works. *[sim68k]break and simhalt_off commands are processed on load. Search finds text in the listing.

---

### Task 13.5 — Stack Viewer Window

Build `gui/simulator/stack_window.h/.cc` — a dedicated floating/dockable window showing the 68000 stack contents.

Reference: `EASy68K/Sim68K/Stack1.cpp` — full 188-line source, `Stack1.dfm`.

Features: #3 (stack viewer)

The original stack viewer is a separate window that displays memory around the current stack pointer, with special highlighting:

1. **Layout** — Custom `QWidget` with paint event:
   - Address column: 8-character hex address followed by colon
   - Data columns: 4 bytes per row, displayed as 2-character hex values
   - Rows scroll by 4 bytes (one long word per row)

2. **Address register selector** — `QComboBox` at the top selecting which address register to track (A0–A7). The stack view centers on the selected register's value. Default: A7 (system stack pointer).

3. **Highlighting** — Per-byte background color:
   - **Yellow**: byte at the current A7 (system stack pointer) address — `clYellow` in original
   - **Aqua/Cyan**: byte at the selected address register's value — `clAqua` in original
   - **Red text**: byte in the Invalid memory range — `clRed` font in original
   - **White**: all other bytes

4. **Navigation** — Up/Down buttons or mouse wheel scroll the view by 4 bytes (one row). Spin buttons on the right edge (matching original layout). The view auto-centers on the selected address register so the stack pointer is always near the middle row.

5. **Auto-refresh** — On every Step/Trace/Pause, the stack viewer updates its display. The `midAddr` tracks the center address and re-centers on the selected A register after each step.

6. **Ctrl+Tab switching** — Original uses Ctrl+Tab to cycle between Memory → Stack → Hardware windows. Implement with `QShortcut` and `bringToFront()` logic.

**Quality Gate 13.5:** Stack window opens, displays memory at the stack pointer. A7 row highlighted in yellow, selected A register highlighted in aqua. Address register combo box works. Scroll by 4 bytes. Updates after each step.

---

### Task 13.6 — Memory Viewer Window

Build `gui/simulator/memory_window.h/.cc` — the hex/ASCII memory viewer.

Reference: `EASy68K/Sim68K/Memory1.cpp` — full 664-line source, `Memory1.dfm`.

Features: #4 (memory display)

1. **Layout** — Custom `QWidget` with paint event (or `QAbstractTableModel` + `QTableView` per Proposal Section 7.11):
   - Address column: 8-character hex row address
   - Hex area: 16 bytes per row, 2-char hex values separated by spaces
   - ASCII area: 16 characters per row, non-printable shown as '.'
   - Top controls: Address jump input (`HexSpinBox`), From/To/Bytes inputs, Copy button, Fill button
   - Navigation: Row Up/Down spin buttons on right edge, Page Up/Down

2. **Address jump** — `HexSpinBox` for entering a hex address. On Enter, the view scrolls to display that address at the top row. Port `Addr1Change()` logic.

3. **LivePaint mode** — When enabled, the memory viewer auto-refreshes after each Step/Trace to show current memory contents. Port `LivePaint` checkbox logic.

4. **Data highlighting** — Invalid memory bytes shown in red text. ROM bytes in blue. Protected bytes in olive. Same color scheme as original.

5. **High-performance rendering** — Use `QAbstractTableModel` with `canFetchMore`/`fetchMore` to only render visible rows (Proposal Section 7.11). Memory is 16MB; never render all of it.

**Quality Gate 13.6:** Memory window opens, displays memory contents in hex/ASCII. Address jump scrolls to the target. LivePaint mode refreshes after each step. Invalid/ROM/Protected regions shown in correct colors. Scrolling is smooth for any address range.

---

### Task 13.7 — Memory Editing (Direct Edit, Copy, Fill)

Extend `memory_window.h/.cc` with direct byte editing and block operations.

Reference: `EASy68K/Sim68K/Memory1.cpp` `FormKeyPress()` (lines 397-467), `CopyClick()` (lines 523-570), `FillClick()` (lines 588+).

Features: #5 (direct memory editing), #6 (memory block copy), #7 (memory block fill)

1. **Direct hex editing** — Click on any hex digit in the hex area. Type 0-9 or A-F to change that nybble:
   - Determine the target address from the cursor position (column math from original lines 406-417)
   - If clicking hex area: calculate byte address and whether it's the high or low nybble
   - Read current byte, modify the selected nybble, write back to `Memory`
   - Auto-advance cursor to the next nybble/byte after each keystroke
   - Port `FormKeyPress()` logic exactly

2. **Direct ASCII editing** — Click on any character in the ASCII area. Type a character to replace that byte:
   - Calculate byte address from cursor position
   - Write typed character directly to `Memory` at that address
   - Auto-advance cursor to the next character
   - Port the ASCII editing branch of `FormKeyPress()`

3. **Block copy** — From/To/Bytes input fields + Copy button:
   - Read `fromAddr`, `toAddr`, `bytes` from the input fields
   - Validate: `fromAddr != toAddr`, `bytes != 0`, addresses < MEMSIZE
   - Handle overlap: if `fromAddr < toAddr`, copy from highest address first (reverse order) to handle overlapping regions correctly
   - Port `CopyClick()` overlap-safe logic exactly (lines 539-562)
   - Update display and trigger hardware/stack refresh

4. **Block fill** — From/To/FillValue input fields + Fill button:
   - Read `fromAddr`, `toAddr`, fill value from input fields
   - Fill each byte in the range with the fill value (cycling through the fill value bytes for multi-byte patterns)
   - Port `FillClick()` logic exactly
   - Update display

5. **Cursor navigation** — Arrow keys, Page Up/Down, Tab to switch focus between hex and ASCII areas. Port `FormKeyDown()` navigation logic.

6. **Hardware update** — After any memory write, call `HardwareWindow::updateIfNeeded(address)` and `StackWindow::DisplayStack()` to refresh dependent views. Port `FormKeyPress()` line 464-465.

**Quality Gate 13.7:** Clicking on a hex digit and typing changes the byte in memory. Clicking on an ASCII character and typing replaces the byte. Copy operation correctly handles overlapping regions. Fill operation fills the range with the specified value. Hardware and stack views update after edits.

---

### Task 13.8 — Hardware Window (7-Segment, LEDs, Switches, Interrupts)

Build `gui/simulator/hardware_window.h/.cc` — the simulated hardware peripheral window.

Reference: `EASy68K/Sim68K/hardwareu.cpp` — full source, `hardwareu.dfm`.

Features: #8 (hardware display), #12 (interrupts), #26 (peripheral I/O)

1. **7-segment displays** — 8 custom `QWidget` subclasses rendering 7-segment digits:
   - Red digit segments on black background (original visual style)
   - Each display maps to a memory address shown in an editable address field
   - Reading the mapped address updates the display: high nybble = left digit, low nybble = right digit (2 digits per byte)
   - Supports both 1-byte and 2-byte (word) display widths

2. **LEDs** — 8 circular LED indicators:
   - Red filled circle when ON, dark/gray circle when OFF
   - Each LED maps to a memory address and bit position
   - Bit value at the mapped address determines LED state

3. **Toggle switches** — 8 toggle switch widgets:
   - Visual: up/down position with on/off label
   - Each switch maps to a memory address and bit position
   - Toggling writes the bit to the mapped memory address

4. **Push buttons** — 8 momentary push button widgets:
   - Visual: raised/pressed button appearance
   - Each button maps to a memory address and bit position
   - Pressing sets the bit; releasing clears it

5. **Memory address configuration** — Editable address fields for each peripheral group:
   - 7-segment base address, LED base address, switch base address, push button base address
   - Changing an address field immediately remaps the peripheral
   - Addresses are read from `ISimulatorEnv::GetSeg7Address()`, `GetLEDAddress()`, `GetSwitchAddress()`, `GetPushButtonAddress()`

6. **Auto-IRQ** — Timer-based interrupt generation:
   - `QTimer` fires at configurable intervals
   - On timer tick, generates an interrupt at the configured level via `M68kSimulator` (Trap #15 task 32 sub 6)
   - Enable/disable checkbox with interval configuration
   - Port `autoIRQon()`/`autoIRQoff()` logic

7. **Hardware refresh** — `updateIfNeeded(uint32_t addr)` method: if `addr` falls within any mapped peripheral range, update that peripheral's display. Called after every memory write and every Step.

**Quality Gate 13.8:** Hardware window displays 7-segment digits, LEDs, switches, and push buttons. Each peripheral responds to memory reads/writes at its mapped address. Switches and push buttons write to memory. Auto-IRQ timer generates interrupts at the configured rate.

---

### Task 13.9 — I/O Console Widget

Build `gui/simulator/console_widget.h/.cc` — the Trap #15 text I/O terminal.

Reference: `EASy68K/Sim68K/simIOu.cpp` — full source, `simIOu.dfm`.

Features: #9 (output results)

1. **Terminal display** — `QTextEdit` (read-only) with black background and monospace font:
   - Displays text output from Trap #15 tasks 0-1, 5-6, 11-14, 16-18, 22, 24-25
   - Cursor position tracking for `SetCursorPosition`/`GetCursorPosition`
   - Scroll region support for `ScrollRect`
   - CRLF handling per `SetInputLFDisplay` setting

2. **Input mode** — When Trap #15 task 2 (TextIn) is called:
   - Display a blinking cursor/prompt at the current position
   - Capture keystrokes into a buffer until Enter
   - Use `Qt::BlockingQueuedConnection` so the simulator thread blocks until the user presses Enter
   - Echo handling per `SetKeyboardEcho` setting
   - Prompt character per `SetInputPrompt` setting

3. **Font properties** — `SetFontProperties` (task 21) changes text color and style (bold, italic, underline) for subsequent output. Map to `QTextCharFormat`.

4. **Clear screen** — `ClearScreen()` clears the entire terminal.

5. **Full-screen mode** — Toggle via `QWidget::showFullScreen()` (task 33). Restores with Escape key.

**Quality Gate 13.9:** Console displays text output. User can type input when requested by Trap #15 task 2. Cursor positioning, scrolling, and font property changes work. Full-screen toggle works.

---

### Task 13.10 — Graphics Output Window

Build `gui/simulator/qt_graphics_io.h/.cc` — the `IGraphicsIO` implementation.

Reference: `EASy68K/Sim68K/CODE9.CPP` tasks 80-96, `simIOu.cpp` drawing code.

Features: #10 (graphics), #27 (full-screen)

1. **Drawing surface** — Custom `QWidget` with `QPixmap` back-buffer for double-buffered rendering:
   - `paintEvent()` blits the back-buffer to screen
   - All drawing operations write to the back-buffer, then call `update()`

2. **QPainter mapping** — Implement all `IGraphicsIO` methods:
   - `SetPenColor`/`SetFillColor` → `QPen`/`QBrush` with color conversion (0x00BBGGRR → `QColor`)
   - `DrawPixel` → `QPainter::drawPoint`
   - `GetPixel` → `QImage::pixel` on the back-buffer
   - `DrawLine`/`LineTo`/`MoveTo` → `QPainter::drawLine` with current pen position
   - `DrawRectangle`/`DrawEllipse` → filled shapes with current brush
   - `DrawUnfilledRectangle`/`DrawUnfilledEllipse` → `QPainter::drawRect`/`drawEllipse` with `Qt::NoBrush`
   - `FloodFill` → `QImage::scanLine` based flood fill algorithm
   - `DrawText` → `QPainter::drawText`
   - `SetDrawingMode` → Map 16 Windows raster ops to `QPainter::CompositionMode` (full mapping table)
   - `SetPenWidth` → `QPen::setWidth`
   - `Repaint` → `QWidget::update()`
   - `SetWindowSize`/`GetWindowSize` → `QWidget::resize`/`size`
   - `SetupWindow` → Initialize back-buffer to specified size

3. **Full-screen mode** — `QWidget::showFullScreen()` with `QScreen` selection for multi-monitor. Escape to exit.

**Quality Gate 13.10:** Graphics window renders all primitive shapes (pixel, line, rect, ellipse, text). Pen/fill color changes work. All 16 drawing modes produce correct results. Double-buffered rendering shows no flicker. Full-screen toggle works.

---

### Task 13.11 — Sound System

Build `gui/simulator/qt_sound_io.h/.cc` — the `ISoundIO` implementation.

Reference: `EASy68K/Sim68K/CODE9.CPP` tasks 70-77.

Features: #11 (sound)

1. **Standard player** (single-voice, blocking) — `QMediaPlayer` + `QAudioOutput`:
   - `PlaySound(filename)` → load and play WAV file, block until complete
   - `LoadSound(filename, index)` → cache WAV file by reference number
   - `PlaySoundMem(index)` → play cached sound
   - `ControlSound(control, index)` → pause/resume/rewind control

2. **Multi-voice player** (non-blocking, simultaneous) — Pool of `QSoundEffect` instances:
   - `PlaySoundMulti(filename)` → play without blocking
   - `LoadSoundMulti(filename, index)` → load into `QSoundEffect` by reference number
   - `PlaySoundMemMulti(index)` → play cached multi-voice sound
   - `StopSoundMemMulti(index)` → stop specific voice
   - `ControlSoundMulti(control, index)` → pause/resume
   - Multiple `QSoundEffect` instances can play simultaneously

3. **Sound file loading** — WAV files loaded from host filesystem (not 68000 memory). `QStandardPaths::DocumentsLocation` as default search path.

4. **ResetSounds** — Stop all playback, clear all cached sounds.

**Quality Gate 13.11:** Single-voice playback works (play, pause, resume, stop). Multi-voice playback allows 2+ sounds simultaneously. Sound caching by reference number works. ResetSounds clears all state.

---

### Task 13.12 — Log Window

Build `gui/simulator/log_window.h/.cc` — the logging configuration and execution window.

Reference: `EASy68K/Sim68K/logU.cpp` — full 296-line source, `logU.dfm`, `LogfileDialogu.cpp`.

Features: #13 (execution log), #14 (output log), #15 (memory range dump)

The original has two independent logging systems:
- **Execution Log** (`ElogFile`): Text file recording each instruction executed, with registers and optional memory dump
- **Output Log** (`OlogFile`): Binary file recording all Trap #15 text output

1. **Log configuration dialog** — `QDialog` with:
   - Execution Log section:
     - File name input with browse button (`QFileDialog::getSaveFileName`)
     - Type selector: "Off" / "Instructions and Registers" / "Instructions, Registers and Memory" (radio buttons or combo box)
     - When "Instructions, Registers and Memory" is selected, show Memory Range fields:
       - "From" address (hex input, forced to $10 boundary)
       - "Bytes" count (hex input, forced to $10 increment)
   - Output Log section:
     - File name input with browse button
     - Type selector: "Off" / "On" (radio buttons)
   - OK / Cancel buttons

2. **File exists dialog** — When starting logging and the log file already exists, show a dialog with Replace / Append / Cancel buttons (port `LogfileDialog` from `LogfileDialogu.cpp`). Replace overwrites the file; Append adds to the end.

3. **Execution log output** — On each instruction step (when logging is active):
   - Binary-search the `.L68` listing for the current PC (port `lineToLog()` from SIM68Ku.cpp line 869)
   - Write the matching listing line with "PC="/"Code="/"Line=" labels to the execution log file
   - If "Instructions, Registers and Memory" mode:
     - Write all D0-D7, A0-A7, USP, SSP, PC, SR register values
     - Dump the configured memory range (from `logMemAddr` for `logMemBytes` bytes, formatted as hex)
   - Flush after each write (port `fflush()` behavior)

4. **Output log output** — On each Trap #15 text output (when logging is active):
   - Write the output text to the output log file in binary mode (`\r\n` line endings)
   - The output log captures the raw bytes sent to the console, not the formatted register state

5. **Start/Stop controls** — Toolbar buttons and Run menu items for Log Start / Log Stop:
   - Start: call `prepareLogFile()` which opens files and shows the file-exists dialog
   - Stop: close both log files, update UI state
   - Logging indicator label in the main window (visible when logging active)

6. **Auto-naming** — When an `.S68` file is opened, auto-set log file names to `<basename>_RunLog.txt` and `<basename>_OutLog.txt`. Port `setLogFileNames()` from logU.cpp line 59.

**Quality Gate 13.12:** Log configuration dialog opens and saves settings. Execution log records instructions with registers to a text file. Output log records Trap #15 output to a binary file. Memory range dump works when selected. File-exists dialog offers Replace/Append/Cancel. Start/Stop controls work. Auto-naming derives log names from the S-Record file name.

---

### Task 13.13 — Serial I/O

Build `gui/simulator/qt_serial_io.h/.cc` — the `ISerialIO` implementation.

Reference: `EASy68K/Sim68K/CODE9.CPP` tasks 40-43.

Features: #17 (serial I/O)

1. **Port enumeration** — `QSerialPortInfo::availablePorts()` to list available serial ports.

2. **Port initialization** — `InitPort(cid, port_name, result)`:
   - Create `QSerialPort` instance
   - Map port name ("COM4" on Windows → native name; on macOS → `/dev/tty.*`)

3. **Port configuration** — `SetPortParams(cid, settings, result)`:
   - Decode the original's packed settings word into baud rate, parity, stop bits, data bits
   - Map to `QSerialPort::setBaudRate()`, `setParity()`, `setStopBits()`, `setDataBits()`

4. **Read/Write** — `ReadString`/`SendString`:
   - Read with timeout using `QSerialPort::waitForReadyRead()`
   - Write using `QSerialPort::write()`

5. **CloseAll** — Close and delete all open `QSerialPort` instances.

**Quality Gate 13.13:** Serial port enumeration lists available ports. Port can be opened, configured, read from, and written to. Timeout behavior matches original. CloseAll cleans up all ports.

---

### Task 13.14 — Network I/O (TCP/UDP)

Build `gui/simulator/qt_network_io.h/.cc` — the `INetworkIO` implementation.

Reference: `EASy68K/Sim68K/CODE9.CPP` tasks 100-107.

Features: #18 (TCP/UDP network I/O)

1. **UDP client** — `CreateClient(settings, server, result)`:
   - Create `QUdpSocket` in `UnicastConnectionState`
   - Settings encode port number and local/remote configuration
   - Server parameter is the remote IP/hostname

2. **UDP server** — `CreateServer(settings, result)`:
   - Create `QUdpSocket` and bind to the specified port
   - `QUdpSocket::bind(QHostAddress::Any, port)`

3. **Send/Receive** — `Send`/`Receive`:
   - `QUdpSocket::writeDatagram()` for sending
   - `QUdpSocket::readDatagram()` for receiving
   - Handle the synchronous receive impedance mismatch: use `QEventLoop` + `waitForReadyRead()` with timeout
   - Preserve the original's blocking-receive semantics

4. **TCP support** — SendOnPort/ReceiveOnPort use `QTcpSocket`/`QTcpServer`:
   - Connection management with proper signal/slot wiring
   - `QTcpSocket::waitForConnected()` / `waitForReadyRead()` for synchronous operations

5. **Close/GetLocalIP** — `CloseConnection`, `GetLocalIP`:
   - Close the appropriate socket
   - `QNetworkInterface::allAddresses()` for local IP

**Quality Gate 13.14:** UDP client can send to a server. UDP server can receive from a client. TCP send/receive works. Local IP is correctly reported. Timeout behavior on receive matches original.

---

### Task 13.15 — Memory Range Definition Dialog

Build `gui/simulator/memory_range_dialog.h/.cc` — the Options dialog for defining memory protection regions.

Reference: `EASy68K/Sim68K/hardwareu.cpp` (ProtectedStartEdit, InvalidStartEdit, etc.), `EASy68K/Sim68K/SIMOPS2.CPP` (range parsing from S-Records).

Features: #19 (definable memory ranges)

The original simulator supports four memory region types, each with a start/end address range and an enable checkbox:

1. **ROM region** — Memory in this range is read-only; writes are silently ignored:
   - Enable checkbox (`ROMMap`)
   - Start address field (`ROMStart`)
   - End address field (`ROMEnd`)

2. **Read-Only region** — Memory is read-only unless in supervisor mode:
   - Enable checkbox (`ReadMap`)
   - Start/End address fields

3. **Protected region** — Memory access causes a bus error unless in supervisor mode:
   - Enable checkbox (`ProtectedMap`)
   - Start address field (`ProtectedStart`)
   - End address field (`ProtectedEnd`)

4. **Invalid region** — Memory access causes a bus error unconditionally:
   - Enable checkbox (`InvalidMap`)
   - Start address field (`InvalidStart`)
   - End address field (`InvalidEnd`)

**Dialog layout:**
- Four group boxes, each with: Enable checkbox, Start address hex input, End address hex input
- OK / Cancel / Apply buttons
- On OK/Apply: write the range values and enable flags to the `Memory` class
- The `Memory` class already has `kMemInvalid`, `kMemProtected`, `kMemRom` flags per byte (from EASy68K-qt transfer). The dialog calls `Memory::SetRangeFlags(start, end, MemoryFlags::kMemInvalid)` etc.

**S-Record loading integration:** The original stores memory range definitions in the S-Record file as special header comments (parsed in `SIMOPS2.CPP` lines 270-278). The port must:
- Parse these headers during `SRecordLoader::LoadFromFile()`
- Apply them to the `Memory` object
- Update the Memory Range dialog to reflect the loaded ranges

**Quality Gate 13.15:** Memory range dialog opens. Enabling a range and setting addresses correctly marks those memory bytes. Accessing invalid memory generates a bus error. Accessing protected memory in user mode generates a bus error. ROM memory ignores writes. S-Record files with embedded range definitions are loaded correctly.

---

### Task 13.16 — Printing

Build `gui/simulator/qt_print_io.h/.cc` — the `IPrintIO` implementation.

Reference: `EASy68K/Sim68K/CODE9.CPP` task 10.

Features: #24 (printing)

1. **PrintChar** — Render a character onto a `QPainter` targeting a `QPrinter`:
   - Track current position (X, Y) and line spacing
   - Auto line-wrap at page margins
   - Auto form feed at page bottom

2. **FormFeed** — Eject current page and start a new page.

**Quality Gate 13.16:** PrintChar outputs characters to a printer. FormFeed advances to the next page. Page margins and line wrapping are correct.

---

### Task 13.17 — File I/O

Build `gui/simulator/qt_file_io.h/.cc` — the `IFileIO` implementation.

Reference: `EASy68K/Sim68K/CODE9.CPP` tasks 50-59.

Features: #25 (file I/O)

1. **File handle management** — Map original file IDs to `QFile` instances in a `QMap<int, QFile*>`.

2. **OpenFile/NewFile** — `QFileDialog::getOpenFileName`/`getSaveFileName`, open `QFile` in ReadWrite mode.

3. **ReadFile/WriteFile** — `QFile::read()`/`write()` with size tracking.

4. **PositionFile** — `QFile::seek()` with offset.

5. **CloseFile** — `QFile::close()` and remove from map.

6. **DeleteFile** — `QFile::remove()`.

7. **DisplayFileDialog** — Native file open/save dialog for the 68000 program to request a filename.

**Quality Gate 13.17:** Files can be opened, read, written, seeked, closed, and deleted through the Trap #15 interface. File dialog integration works.

---

### Task 13.18 — Settings Persistence

Implement cross-platform settings storage.

Reference: `EASy68K/Sim68K/SIM68Ku.cpp` `SaveSettings()`/`LoadSettings()`.

Features: #29 (settings persistence)

1. **QSettings with IniFormat** — Store all settings:
   - Window positions and sizes (all simulator windows)
   - Font choices (listing font, console font)
   - Memory range definitions (Protected/Invalid/ROM ranges)
   - Last opened file directory
   - Logging preferences
   - Hardware window peripheral addresses

2. **Save on close** — `QMainWindow::closeEvent()` writes all settings.

3. **Load on start** — `QMainWindow` constructor reads settings and restores window geometry.

**Quality Gate 13.18:** Settings persist across application restarts. Window positions, fonts, and memory ranges are restored correctly.

---

### Task 13.19 — Edit68K-Qt Editor Application

Build the assembler IDE.

Reference: `EASy68K/Edit68K/` (full Borland source), Proposal Section 7.1.1.

1. **Tabbed editor** — `QTabWidget` with `CodeEditor` (custom `QPlainTextEdit` subclass):
   - Syntax highlighting: Comments=Blue, Labels=Purple, Directives=Green, Strings=Teal, Instructions=Black
   - Tab bar with close buttons, New/Open/Save shortcuts
   - Indent/Unindent buttons (original toolbar feature)

2. **Assembler integration** — Wire `libeasym68k-asm` to the editor:
   - Assemble button (green Play icon on toolbar) → calls `Assembler::Assemble(source)`
   - On success: launch Sim68K-Qt with the `.S68` file (QProcess or shared library call)
   - On error: highlight error line, show error message in status bar

3. **Standard menus** — File, Edit, Project, Options, Window, Help:
   - File: New, Open, Save, Save As, Print, Recent Files
   - Edit: Undo, Redo, Cut, Copy, Paste, Find, Replace
   - Project: Assemble, Assemble and Run
   - Options: Font, Tab Size

4. **Status bar** — Cursor position ("ln X col Y"), Insert/Overwrite mode indicator.

**Quality Gate 13.19:** Edit68K-Qt launches, provides syntax highlighting with the correct color scheme, assembles a source file, and can launch Sim68K-Qt with the assembled output.

---

### Task 13.20 — EASyBIN-Qt Binary Utility

Build the binary manipulation utility.

Reference: `EASy68K/EASyBIN/` (full source), `EASyBIN.dfm`, Proposal Section 7.1.1.

1. **Control panel layout** — Three `QGroupBox` sections:
   - "S-Record File": Start/Last/Length fields (read-only), Open button
   - "Binary Output Split": Radio buttons for 1/2/4 files
   - "Binary File(s)": First Address, Length fields, Save button

2. **Color-coded hex viewer** — Large hex/ASCII display:
   - Same underlying widget as MemoryWindow but with color coding for file split visualization
   - Green, blue, black, olive text colors to indicate which binary file each byte belongs to
   - ASCII representation on the right

3. **Memory manipulation toolbar** — Address jump, From/To/Size fields, Copy and Fill ($FF) buttons.

4. **Custom navigation** — Dedicated Row/Page Up/Down spin buttons on the right edge.

5. **S-Record loading** — Uses `SRecordLoader` from `libeasym68k-sim` (shared code, Proposal Section 7.12).

**Quality Gate 13.20:** EASyBIN-Qt correctly loads an `.S68`, displays it with correct color-coding for file splits, and successfully splits it into 1/2/4 binary files. Copy and Fill operations work.

---

### Task 13.21 — Feature Parity Validation

Systematic verification that every feature from the Feature Coverage Matrix works in the GUI.

For each of the 30 features in the matrix:
1. Write a step-by-step test script (manual or automated with `QTest`)
2. Execute the test against the running GUI
3. Record pass/fail
4. Any failure must be fixed before the task is complete

**Priority test scenarios:**
- Load a known `.S68` file, Step through 10 instructions, verify registers at each step
- Run a program that uses Trap #15 text I/O (tasks 0-2), verify console output and input
- Run a program that uses graphics (tasks 80-96), verify drawing output
- Run a program that uses sound (tasks 70-77), verify audio plays
- Open the memory viewer, edit a byte, verify the change persists
- Set a breakpoint in the source view, Run, verify execution stops at the breakpoint
- Configure memory ranges, run a program that accesses protected/invalid memory, verify bus errors
- Start execution logging, run a program, stop logging, verify log file contents
- Run a program that uses serial I/O (requires loopback or virtual COM port)
- Run a program that uses UDP/TCP networking

**Quality Gate 13.21:** All 30 features in the Feature Coverage Matrix pass validation. Zero features are missing or broken.

---

## Summary: Quality Gate Checklist

| Gate | Phase | Verification |
|------|-------|-------------|
| 0.1 | Infrastructure | Directory tree exists |
| 0.2 | Infrastructure | CMake configures successfully |
| 0.3 | Infrastructure | Empty targets compile with -Werror |
| 0.4 | Infrastructure | clang-format passes |
| 0.5 | Infrastructure | CI pipelines exist |
| 0.6 | Infrastructure | 20+ golden test data files committed |
| 1.1 | Types | types.h compiles, SimResult replaces ExecResult |
| 1.2 | Types | EffectiveAddr compiles, constructors tested |
| 1.3 | Memory | Memory class compiles with vector, no raw new/delete |
| 1.4 | Memory | Memory tests pass + ASan clean |
| 2.1 | S-Record | SRecordLoader standalone, checksum validation works |
| 2.2 | S-Record | 14 S-Record tests pass |
| 3.1 | Addressing | addressing_mode.h compiles, no sentinel addresses |
| 3.2 | Addressing | addressing_mode.cc returns EffectiveAddr for all 12 modes |
| 3.3 | Addressing | 20 addressing mode tests pass |
| 4.1 | Simulator | simulator.h compiles, USP/SSP separate, all interfaces injected |
| 4.2 | Simulator | Simulator core tests pass, complete flag computation |
| 4.3 | Simulator | 18 simulator unit tests pass |
| 5.1 | Decoder | Hierarchical dispatch works, O(1) |
| 5.2 | Decoder | 12 decode tests pass, SIMHALT works |
| 6.1 | Move opcodes | 30 move instruction tests pass |
| 6.2 | Arithmetic opcodes | 50 arithmetic tests pass, V/C/X correct |
| 6.3 | Logic opcodes | 30 logic tests pass |
| 6.4 | Branch opcodes | 25 branch tests pass |
| 6.5 | Misc opcodes | 15 misc tests pass, SIMHALT works |
| 6.6 | Shift opcodes | 20 shift tests pass |
| 6.7 | Flags | 60+ flag computation tests pass — **CRITICAL GATE** |
| 7.1 | Trap #15 | All 9 interface headers compile |
| 7.2 | Trap #15 | Dispatch maps all 50+ task numbers |
| 7.3 | Trap #15 | 50+ mock tests pass |
| 8.1–8.4 | Assembler | Lexer/parser/symbol/expression/assembler tests pass |
| 8.5.1 | Assembler | Instruction table tests pass (50+), InstructionSize uses table, HandleInstruction dispatches via InstrEncoding |
| 8.5.2 | Assembler | Code generator EA encoding + ext words tests pass (40+), all 13 addressing modes produce correct bit patterns |
| 8.5.3 | Assembler | Instruction encoding handler tests pass (60+), all 68000 instructions produce correct machine code, peephole optimizations work |
| 8.5.4 | Assembler | MOVEM register list tests pass (25+), pre-decrement mask reversal correct |
| 8.5.5 | Assembler | Remaining directives tests pass (50+), INCLUDE/DCB/DC strings/ORG odd-address all work |
| 8.5.6 | Assembler | Conditional assembly tests pass (25+), IFC/IFNC/IFEQ/IFNE/IFLT/IFLE/IFGT/IFGE/ELSE/ENDC all work |
| 8.5.7 | Assembler | Structured control flow tests pass (30+), WHILE/FOR/REPEAT/IF/DBLOOP/UNLESS generate correct branches |
| 8.5.8 | Assembler | Macro processor tests pass (25+), MACRO/ENDM/parameter substitution/\@ labels/IFARG/MEXIT work |
| 8.5.9 | Assembler | Error reporter tests pass (20+), all 43+ error codes have specific messages |
| 8.5.10 | Assembler | Listing generator tests pass (20+), .L68 format matches original |
| 8.5.11 | Assembler | Object/S-record output tests pass (25+), S0/S1/S2/S3/S8 records correct, checksums valid |
| 8.6 | Assembler | **20+ golden assembly tests pass with byte-identical output** |
| 9.1 | Golden traces | **10+ golden simulation traces pass with 100% parity** |
| 10.1 | Exceptions | 13 exception tests pass |
| 10.2 | Interrupts | 5 interrupt tests pass |
| 11.1 | CI | All 3 platforms green |
| 11.2 | Coverage | ≥90% sim, ≥85% asm |
| 11.3 | clang-tidy | Zero warnings |
| 12.1 | GUI Backends | All 9 Qt Trap #15 backends compile, thread-safe wiring verified |
| 12.2 | GUI Shell | Sim68K-Qt launches, loads S-Record, Step/Run/Pause/Reset work |
| 12.3 | GUI Registers | Register display shows all D/A/PC/SR/Cycles, editing works, changes highlight |
| 12.4 | GUI Source View | .L68 listing loads, PC highlight works, breakpoints toggle, *[sim68k] commands parsed |
| 12.5 | GUI Stack | Stack window shows A7/selected A register, yellow/aqua highlighting, scroll by 4 bytes |
| 12.6 | GUI Memory View | Memory hex/ASCII display, address jump, LivePaint, region colors |
| 12.7 | GUI Memory Edit | Direct hex/ASCII editing, block Copy, block Fill, hardware/stack refresh |
| 12.8 | GUI Hardware | 7-segment/LEDs/switches/buttons respond to memory, Auto-IRQ timer works |
| 12.9 | GUI Console | Text I/O console, input mode, cursor positioning, full-screen |
| 12.10 | GUI Graphics | All primitives, 16 drawing modes, double-buffer, full-screen |
| 12.11 | GUI Sound | Single-voice + multi-voice playback, caching, reset |
| 12.12 | GUI Logging | Execution log + output log to files, memory dump, start/stop, auto-naming |
| 12.13 | GUI Serial | Port enumeration, config, read/write, timeout |
| 12.14 | GUI Network | UDP/TCP client/server, send/receive, local IP |
| 12.15 | GUI Mem Ranges | Memory range dialog, Protected/Invalid/ROM regions, S-Record embedded ranges |
| 12.16 | GUI Printing | PrintChar, FormFeed, page layout |
| 12.17 | GUI File I/O | Open/read/write/seek/close/delete, file dialog |
| 12.18 | GUI Settings | QSettings persistence across restarts |
| 12.19 | GUI Editor | Edit68K-Qt: syntax highlighting, assemble, launch simulator |
| 12.20 | GUI EASyBIN | EASyBIN-Qt: S-Record display, color-coded hex, binary split |
| 12.21 | GUI Parity | **All 30 features in Feature Coverage Matrix pass validation** |

**Progression rule:** A quality gate must pass before proceeding to the next task. If a gate fails, the failure must be fixed before any subsequent work begins. This prevents compounding errors.
