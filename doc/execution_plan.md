# EASy68K Port Execution Plan

This document is the step-by-step execution plan for the EASy68K cross-platform port. It is ordered for correctness: each task produces a compilable, testable state with explicit quality gates.

Companion documents:
- [Porting Proposal](porting_proposal.md) — architecture, interfaces, and design decisions
- [Code Transfer Plan](code_transfer_plan.md) — file-by-file transfer specifications from EASy68K-qt (Phases 1–2 only)

### Source Transition Point

This plan has a deliberate **source transition** at the boundary between Phase 2 and Phase 3:

- **Phases 0–2** transfer salvageable code from `EASy68K-qt` per the [Code Transfer Plan](code_transfer_plan.md). Only types.h, the Memory class, and the S-Record loader are transferred — these are the components assessed as sound.

- **Phases 3–8** port directly from the **original EASy68K Borland sources** (`EASy68K/Sim68K/` and `EASy68K/Edit68K/`). EASy68K-qt is used only as a secondary cross-reference where noted. The original sources are the authoritative reference for behavioral parity — every opcode, every flag, every Trap #15 task must match what the original `CODE1.CPP`–`CODE9.CPP`, `RUN.CPP`, `SCAN.CPP`, and assembler files do.

**Rationale:** EASy68K-qt has fatal architectural flaws (sentinel EA addresses, linear instruction decoder, incomplete flag computation, toy-level I/O callbacks, Cpu-owns-Memory, stub assembler components). Continuing to use it as the primary source beyond the salvageable skeleton would propagate these bugs. The original Borland code, despite its VCL coupling and global state, contains proven-correct instruction logic that has been tested by thousands of students over two decades.

---

## Feature Coverage Matrix

This matrix maps every feature of the original EASy68K simulator to the execution plan tasks that implement it. **Every feature must have at least one task.** Gaps identified during analysis are marked with NEW.

| # | Original Feature | Original Source | Execution Plan Task(s) | Status |
|---|-----------------|-----------------|----------------------|--------|
| 1 | 68000 register display (D0-D7, A0-A7, PC, USP, SSP, SR, Cycles) | SIM68Ku.cpp | Task 12.3 RegisterWidget | Covered |
| 2 | Source code display (.L68 listing mapped to PC) | SIM68Ku.cpp ListBox1, highlight() | Task 12.4 SourceView | **NEW** |
| 3 | Stack viewer (A7 highlight, address register selection, scroll) | Stack1.cpp | Task 12.5 StackWindow | **NEW** |
| 4 | Memory display (hex/ASCII viewer, address jump) | Memory1.cpp FormPaint | Task 12.6 MemoryWindow | Covered |
| 5 | Direct memory editing (click hex/ASCII byte, type new value) | Memory1.cpp FormKeyPress | Task 12.7 Memory Editing | **NEW** |
| 6 | Memory block copy (From/To/Bytes fields, overlap-safe) | Memory1.cpp CopyClick | Task 12.7 Memory Editing | **NEW** |
| 7 | Memory block fill (From/To/FillValue) | Memory1.cpp FillClick | Task 12.7 Memory Editing | **NEW** |
| 8 | Hardware display (7-segment, LEDs, switches, push buttons) | hardwareu.cpp | Task 12.8 HardwareWindow | Covered |
| 9 | Output results (Trap #15 text I/O console) | simIOu.cpp | Task 12.9 ConsoleWidget | Covered |
| 10 | Graphics output (QPainter, 16 raster ops, double buffer) | CODE9.CPP tasks 80-96 | Task 12.10 GraphicsIO | Covered |
| 11 | Sound output (single-voice + multi-voice) | CODE9.CPP tasks 70-77 | Task 12.11 SoundIO | Covered |
| 12 | Interrupts (auto-IRQ, hardware IRQ, autovector) | hardwareu.cpp autoIRQ | Task 12.8 HardwareWindow + Task 10.2 | Covered |
| 13 | Log execution output (instruction + registers to text file) | logU.cpp ElogFile | Task 12.12 LogWindow | **NEW** |
| 14 | Log Trap #15 output (binary file) | logU.cpp OlogFile | Task 12.12 LogWindow | **NEW** |
| 15 | Log memory range dump (configurable from/bytes) | logU.cpp MemFrom/MemBytes | Task 12.12 LogWindow | **NEW** |
| 16 | Load S-Record files (.S68, .S19, .S3) | SIM68Ku.cpp loadSrec | Task 2.1 SRecordLoader + Task 12.2 | Covered |
| 17 | Serial I/O (COM port init, read, send) | CODE9.CPP tasks 40-43 | Task 12.13 SerialIO | Covered |
| 18 | TCP/UDP network I/O (client/server, send/receive) | CODE9.CPP tasks 100-107 | Task 12.14 NetworkIO | Covered |
| 19 | Definable memory ranges (Protected/Invalid/ROM with start/end) | SIMOPS2.CPP + hardwareu.cpp | Task 12.15 MemoryRangeDialog | **NEW** |
| 20 | Breakpoints (PC breakpoints with toggle, clear all) | SIM68Ku.cpp BreaksFrm | Task 4.1 M68kSimulator + Task 12.4 SourceView | Covered |
| 21 | Run/Step/Trace/Pause/Reset controls | SIM68Ku.cpp Run/Step/Trace | Task 12.2 MainWindow | Covered |
| 22 | Run-to-cursor | SIM68Ku.cpp RunToCursorExecute | Task 12.4 SourceView | Covered |
| 23 | *[sim68k] listing commands (break, bitfield, SIMHALT_OFF) | SIM68Ku.cpp lines 310-337 | Task 12.4 SourceView | **NEW** |
| 24 | Printing (character output, form feed) | CODE9.CPP task 10 | Task 12.16 PrintIO | Covered |
| 25 | File I/O (open, read, write, seek, close, delete, dialog) | CODE9.CPP tasks 50-59 | Task 12.17 FileIO | Covered |
| 26 | Peripheral I/O (mouse/keyboard IRQ enable, state read) | CODE9.CPP tasks 60-62 | Task 12.8 HardwareWindow | Covered |
| 27 | Full-screen output window | CODE9.CPP task 33 | Task 12.10 GraphicsIO | Covered |
| 28 | Drag-and-drop file open | SIM68Ku.cpp WmDropFiles | Task 12.2 MainWindow | Covered |
| 29 | Settings persistence (QSettings for window positions, fonts, options) | SIM68Ku.cpp SaveSettings | Task 12.18 SettingsPersistence | Covered |
| 30 | Cycle counter with clear button | SIM68Ku.cpp | Task 12.3 RegisterWidget | Covered |

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

Port each missing component from the **original Borland source**, not from EASy68K-qt stubs:

| Component | Original Source | Target File |
|-----------|----------------|-------------|
| Code generator | `EASy68K/Edit68K/CODEGEN.CPP` | `src/asm/code_generator.cc` |
| All directives | `EASy68K/Edit68K/DIRECTIV.CPP` | `src/asm/directives.cc` |
| Instruction table | `EASy68K/Edit68K/INSTTABL.CPP` + `INSTLOOK.CPP` | `src/asm/instruction_table.cc` |
| Listing generator | `EASy68K/Edit68K/LISTING.CPP` | `src/asm/listing_generator.cc` |
| Macro processor | `EASy68K/Edit68K/MACRO.CPP` | `src/asm/macro_processor.cc` |
| MOVEM encoding | `EASy68K/Edit68K/MOVEM.CPP` | part of `code_generator.cc` |
| Structured control flow | `EASy68K/Edit68K/STRUCTURED.CPP` | part of `directives.cc` |
| Binary output | `EASy68K/Edit68K/BINFILE.CPP` | part of `assembler.cc` |

For each:
1. Read the original `.CPP` file
2. Replace `AnsiString` with `std::string`
3. Replace `TColor`/`FontStyle` with `uint32_t` RGB
4. Remove `#include <vcl.h>`
5. Implement in the target file following the class structure
6. Write tests

**Quality Gate 8.5:** Each component has its own test file and passes. Partial order:
- `instruction_table.cc` first (no dependencies)
- `code_generator.cc` depends on instruction_table
- `directives.cc` depends on code_generator and symbol_table
- `macro_processor.cc` depends on directives
- `listing_generator.cc` can be done in parallel
- `golden_assembly_test.cc` runs last

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

## Phase 9: Golden Simulation Traces (Proposal Section 5)

### Task 9.1 — Implement Golden Trace Comparison

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

**Quality Gate 9.1:** All 10+ golden simulation trace tests pass with 100% register and memory parity.

---

## Phase 10: Exception and Interrupt Tests

### Task 10.1 — Exception Tests

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

**Quality Gate 10.1:** All 13 exception tests pass.

---

### Task 10.2 — Interrupt Tests

New file: (part of `exception_test.cc` or separate)

Test cases:
1. IRQ at level 5 with interrupt mask = 3 → exception generated
2. IRQ at level 5 with interrupt mask = 7 → no exception (masked)
3. Autovector interrupt → reads from vector $64+$68
4. Interrupt clears trace flag
5. Stop instruction wakes on pending interrupt

**Quality Gate 10.2:** All 5 interrupt tests pass.

---

## Phase 11: CI and Code Quality

### Task 11.1 — Enable CI

Push CI configs from Task 0.5. Verify:
- Linux (gcc + clang) builds and tests pass
- macOS (clang) builds and tests pass
- Windows (MSVC) builds and tests pass
- ASan/UBSan builds pass on all platforms

**Quality Gate 11.1:** All CI pipelines green.

---

### Task 11.2 — Code Coverage

Add `gcov`/`lcov` to CI. Target:
- `libeasym68k-sim`: ≥90% line coverage
- `libeasym68k-asm`: ≥85% line coverage

**Quality Gate 11.2:** Coverage meets targets on all platforms.

---

### Task 11.3 — clang-tidy

Add `clang-tidy` checks to CI using the config in `ci/clang-tidy.cfg`.

**Quality Gate 11.3:** Zero `clang-tidy` warnings on core library code.

## Phase 12: GUI Implementation

This phase implements the three Qt GUI applications. Every task references specific features from the Feature Coverage Matrix above. The simulator GUI is the largest and most complex application and receives the most detailed breakdown.

Prerequisite: All quality gates from Phases 0–11 must pass before any GUI work begins. The core libraries must be proven correct via golden tests before GUI development starts.

### Task 12.1 — Implement Trap #15 Qt Backends

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

**Quality Gate 12.1:** All 9 Qt backends compile and instantiate. A minimal test harness creates each backend and verifies its interface methods are callable without crash. Thread-safe signal/slot wiring verified for both sync and async paths.

---

### Task 12.2 — Sim68K-Qt Main Window and Execution Controls

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

7. **Options menu** — Font selection, Memory Ranges dialog (Task 12.15), Logging setup (Task 12.12).

**Quality Gate 12.2:** Sim68K-Qt launches. User can open an `.S68` file, see the PC set, and use Step/Run/Pause/Reset controls. State signals correctly update a minimal register display. Drag-and-drop works.

---

### Task 12.3 — Register Display Widget

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

**Quality Gate 12.3:** Register widget displays all D0-D7, A0-A7, PC, USP, SSP, SR bits, and Cycles. User can edit values and see them take effect on the next Step. Changed registers highlight after each step.

---

### Task 12.4 — Source Code View

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

**Quality Gate 12.4:** Source view loads a `.L68` file, displays it with correct columns. PC highlight works. Breakpoints can be toggled by clicking the margin. Run-to-cursor works. *[sim68k]break and simhalt_off commands are processed on load. Search finds text in the listing.

---

### Task 12.5 — Stack Viewer Window

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

**Quality Gate 12.5:** Stack window opens, displays memory at the stack pointer. A7 row highlighted in yellow, selected A register highlighted in aqua. Address register combo box works. Scroll by 4 bytes. Updates after each step.

---

### Task 12.6 — Memory Viewer Window

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

**Quality Gate 12.6:** Memory window opens, displays memory contents in hex/ASCII. Address jump scrolls to the target. LivePaint mode refreshes after each step. Invalid/ROM/Protected regions shown in correct colors. Scrolling is smooth for any address range.

---

### Task 12.7 — Memory Editing (Direct Edit, Copy, Fill)

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

**Quality Gate 12.7:** Clicking on a hex digit and typing changes the byte in memory. Clicking on an ASCII character and typing replaces the byte. Copy operation correctly handles overlapping regions. Fill operation fills the range with the specified value. Hardware and stack views update after edits.

---

### Task 12.8 — Hardware Window (7-Segment, LEDs, Switches, Interrupts)

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

**Quality Gate 12.8:** Hardware window displays 7-segment digits, LEDs, switches, and push buttons. Each peripheral responds to memory reads/writes at its mapped address. Switches and push buttons write to memory. Auto-IRQ timer generates interrupts at the configured rate.

---

### Task 12.9 — I/O Console Widget

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

**Quality Gate 12.9:** Console displays text output. User can type input when requested by Trap #15 task 2. Cursor positioning, scrolling, and font property changes work. Full-screen toggle works.

---

### Task 12.10 — Graphics Output Window

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

**Quality Gate 12.10:** Graphics window renders all primitive shapes (pixel, line, rect, ellipse, text). Pen/fill color changes work. All 16 drawing modes produce correct results. Double-buffered rendering shows no flicker. Full-screen toggle works.

---

### Task 12.11 — Sound System

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

**Quality Gate 12.11:** Single-voice playback works (play, pause, resume, stop). Multi-voice playback allows 2+ sounds simultaneously. Sound caching by reference number works. ResetSounds clears all state.

---

### Task 12.12 — Log Window

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

**Quality Gate 12.12:** Log configuration dialog opens and saves settings. Execution log records instructions with registers to a text file. Output log records Trap #15 output to a binary file. Memory range dump works when selected. File-exists dialog offers Replace/Append/Cancel. Start/Stop controls work. Auto-naming derives log names from the S-Record file name.

---

### Task 12.13 — Serial I/O

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

**Quality Gate 12.13:** Serial port enumeration lists available ports. Port can be opened, configured, read from, and written to. Timeout behavior matches original. CloseAll cleans up all ports.

---

### Task 12.14 — Network I/O (TCP/UDP)

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

**Quality Gate 12.14:** UDP client can send to a server. UDP server can receive from a client. TCP send/receive works. Local IP is correctly reported. Timeout behavior on receive matches original.

---

### Task 12.15 — Memory Range Definition Dialog

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

**Quality Gate 12.15:** Memory range dialog opens. Enabling a range and setting addresses correctly marks those memory bytes. Accessing invalid memory generates a bus error. Accessing protected memory in user mode generates a bus error. ROM memory ignores writes. S-Record files with embedded range definitions are loaded correctly.

---

### Task 12.16 — Printing

Build `gui/simulator/qt_print_io.h/.cc` — the `IPrintIO` implementation.

Reference: `EASy68K/Sim68K/CODE9.CPP` task 10.

Features: #24 (printing)

1. **PrintChar** — Render a character onto a `QPainter` targeting a `QPrinter`:
   - Track current position (X, Y) and line spacing
   - Auto line-wrap at page margins
   - Auto form feed at page bottom

2. **FormFeed** — Eject current page and start a new page.

**Quality Gate 12.16:** PrintChar outputs characters to a printer. FormFeed advances to the next page. Page margins and line wrapping are correct.

---

### Task 12.17 — File I/O

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

**Quality Gate 12.17:** Files can be opened, read, written, seeked, closed, and deleted through the Trap #15 interface. File dialog integration works.

---

### Task 12.18 — Settings Persistence

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

**Quality Gate 12.18:** Settings persist across application restarts. Window positions, fonts, and memory ranges are restored correctly.

---

### Task 12.19 — Edit68K-Qt Editor Application

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

**Quality Gate 12.19:** Edit68K-Qt launches, provides syntax highlighting with the correct color scheme, assembles a source file, and can launch Sim68K-Qt with the assembled output.

---

### Task 12.20 — EASyBIN-Qt Binary Utility

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

**Quality Gate 12.20:** EASyBIN-Qt correctly loads an `.S68`, displays it with correct color-coding for file splits, and successfully splits it into 1/2/4 binary files. Copy and Fill operations work.

---

### Task 12.21 — Feature Parity Validation

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

**Quality Gate 12.21:** All 30 features in the Feature Coverage Matrix pass validation. Zero features are missing or broken.

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
| 8.5 | Assembler | All directives, macros, structured flow implemented |
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
