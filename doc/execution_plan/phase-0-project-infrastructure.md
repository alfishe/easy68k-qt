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
