# EASy68K Cross-Platform Port Proposal

## 1. Executive Summary

This document outlines the engineering strategy for porting the EASy68K simulator and assembler from its original Borland C++ Builder (VCL) Windows-only implementation to a modern, cross-platform architecture.

The core requirement is a **strict port**, maintaining 100% behavioral parity with the original logic. The existing logic is highly stable and proven; the goal is to decouple it from obsolete UI frameworks, package it as reusable components, and provide a modern, visually identical graphical interface.

All platform-specific technologies (DirectX, Winsock, Win32 serial I/O, VCL Canvas, Windows printing) will be replaced with cross-platform Qt equivalents. No Windows-only APIs will remain in the ported codebase.

## 2. Target Architecture

The new architecture will be split into three distinct layers:

1. **`libeasym68k-asm`**: A static C++ library containing the assembler core.
2. **`libeasym68k-sim`**: A static C++ library containing the simulator core.
3. **`EASy68K-Qt`**: A modern graphical user interface suite built with Qt 6, containing three distinct GUI applications corresponding to the original utilities:
    *   **Edit68K-Qt:** The Editor and Assembler.
    *   **Sim68K-Qt:** The Simulator.
    *   **EASyBIN-Qt:** The Binary/S-Record conversion utility.

### Relationship to Prior Work

An earlier porting attempt exists in the `EASy68K-qt` repository. That project established a clean `sim68k::Cpu` class with proper state encapsulation, a `sim68k::Memory` class with memory mapping, an assembler with lexer/parser/symbol-table, and a CMake build system. However, several files remain stubs (e.g., `code_generator.cc`, `directives.cc`, `listing_generator.cc`, `macro_processor.cc` are minimal), and the Trap #15 system uses only coarse-grained callbacks.

This proposal incorporates the architectural lessons from that attempt. Where the existing code is sound and passes golden tests, it will be adopted. Where it is incomplete or diverges from behavioral parity, it will be rewritten. A detailed file-by-file transfer specification, including what is kept, what is discarded, and the exact adaptation steps, is documented in [Code Transfer Plan](code_transfer_plan.md).

## 3. Coding Standards and Project Structure

To ensure a modern, maintainable codebase, the port will adhere to the following standards:

1. **C++ Standard:** C++17 minimum (required by Qt 6 and modern Google Style Guide conventions). The core libraries will avoid RTTI and exceptions in hot paths; error propagation uses return codes and `absl::Status`-like result types. The GUI layer may use exceptions where Qt conventions expect them.
2. **Google C++ Style Guide:** All core library code (`libeasym68k-asm`, `libeasym68k-sim`) will conform to the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html). The GUI layer (`gui/`) will follow Qt naming conventions (`PascalCase` methods, `camelCase` variables, `m_` member prefix, `Q_SLOTS`/`Q_SIGNALS`) to maintain idiomatic Qt code. This dual-convention approach avoids the jarring inconsistency of forcing Google Style onto Qt subclassing.
3. **File Naming:** Files will be renamed from the legacy Borland conventions (e.g., `ASSEMBLE.CPP`, `SIM68Ku.cpp`) to `snake_case` (e.g., `assemble.cc`, `sim68k.cc`). Headers will use `.h` and source files will use `.cc`.
4. **Directory Structure:** 
   *   `src/asm/`: Core assembler implementation files.
   *   `src/sim/`: Core simulator implementation files.
   *   `include/easym68k/`: Public header files for the static libraries.
   *   `tests/`: Unit test and golden test files using Google Test (`gtest`).
   *   `gui/`: Qt 6 GUI application source code.
   *   `ci/`: Continuous integration scripts and configurations.
   *   `data/golden/`: Golden test reference files (`.S68`, `.L68`, execution traces).

## 4. Deep Discovery Findings

An architectural review of the original Borland C++ codebase reveals several critical constraints and patterns that inform the porting strategy:

1. **Massive Global State (`extern.h`/`var.h`):** The simulator core relies heavily on global variables for the CPU state (`D[D_REGS]`, `A[A_REGS]`, `PC`, `SR`), memory (`char* memory`), and cycle counts. This is fundamentally thread-unsafe and prevents running multiple emulator instances. This state must be encapsulated into a `M68kSimulator` context class.

2. **Pointer-Based Register/Memory Discrimination (`put()`/`value_of()`):** The original `put()` and `value_of()` functions determine whether an operation targets a 68000 register or memory by comparing the target pointer's address against the address ranges of the `D[]`, `A[]`, `PC` arrays and the `memory[]` array (see `var.h` comment: "The following variables must remain together"). This is a fragile pattern that depends on contiguous memory layout of globals. When state is encapsulated into a class, the layout of members is compiler-dependent and this range-checking will break. The port must replace this with an explicit tag-based or type-discriminated approach: the effective address calculation will produce a tagged union (register reference vs. memory reference) rather than a raw `long*` that requires address-range checking.

3. **Pervasive VCL Types:** Borland's `AnsiString` and UI constructs (`Form1->Message->Lines`) bleed deeply into the core instruction execution files (e.g., `RUN.CPP`, `CODE9.CPP`). These must be replaced with `std::string` and abstract interfaces respectively.

4. **Trap #15 Complexity (`CODE9.CPP`):** Trap #15 is the backbone of EASy68K's simulated hardware. It spans approximately 50 case labels (tasks 0-25, 30-33, 40-43, 50-59, 60-62, 70-77, 80-96, 100-107) with sub-operations on some tasks. It handles Text I/O, File I/O, Serial I/O, Network I/O, Keyboard/Mouse, hardware timers, graphical drawing, sound playback, and printing. The original uses DirectX (DirectMusic) for multi-voice sound, the VCL `TMediaPlayer` for single-voice sound, Winsock for networking, Win32 serial APIs for COM ports, and `TPrinter` for printing. All of these must be replaced with cross-platform Qt equivalents. A single monolithic `ITrap15Handler` interface with ~50 virtual methods would violate the Interface Segregation Principle and be untestable. Instead, Trap #15 will be decomposed into focused interfaces (see Section 6.4).

5. **Cooperative Multitasking Model (`Application->ProcessMessages()`):** The original uses a single-threaded cooperative execution model. `runprog()` calls `Application->ProcessMessages()` to pump the VCL message loop during simulation. Trap #15 task 23 (delay) does the same. Input mode uses `Sleep(10)` + `ProcessMessages` to avoid blocking. Moving to a dedicated `QThread` is not a simple threading addition — the original code calls back into the GUI synchronously from the execution loop (e.g., `simIO->textOut()`, `Form1->Message->Lines->Add()`). The port must carefully address synchronous vs. asynchronous trap operations (see Section 7.2).

6. **Endianness and Memory Layout:** The m68k is a big-endian architecture. The original code runs on little-endian x86 and stores 68000 memory in **host byte order** (little-endian), relying on direct pointer casts like `EA1 = (long *) &memory[A[reg] & ADDRMASK]` to read/write multi-byte values. This is a strict aliasing violation and will produce incorrect results on ARM64 (Apple Silicon). The port must choose an explicit memory byte-order policy (see Section 6.6).

7. **Custom `SIMHALT` Instruction:** The original implements a non-standard `SIMHALT` instruction (opcode `0xFFFFFFFF`) in `LINE1111()`. This is critical for classroom use and must be preserved as a first-class feature, not treated as a generic Line-F emulator exception.

8. **Assembler Decoupling (`Edit68K/asm.h`):** The assembler uses a jump-table dispatch pattern (`flavor` and `instruction` structs). The assembler logic is significantly less entangled with the UI than the simulator and should port cleanly once `AnsiString` (including `TColor` in `FontStyle`) and `#include <vcl.h>` are removed.

## 5. Feature and Quality Parity Guarantee

A central requirement of this port is that no functionality is lost. The ported application must be a 1:1 functional equivalent of the original Windows application.

*   **Feature Mapping Matrix:** A comprehensive feature matrix will be created, listing every capability of the original `Edit68K`, `Sim68K`, and `EASyBIN` (e.g., breakpoints, memory searching, S-record exporting, all Trap 15 tasks). Each feature will be individually validated in the Qt port.
*   **Behavioral Quality Parity:** The application's runtime behavior, performance, and stability must match or exceed the original. This includes ensuring that legacy quirks or specific emulator behaviors (e.g., how condition codes are handled during specific instructions) are identical to the Borland C++ release.
*   **Golden Assembly Tests:** Compile a comprehensive suite of `.X68` source files using the original Windows `Edit68K` executable to generate "Golden" `.S68` and `.L68` (listing) files. The ported `libeasym68k-asm` must produce byte-for-byte identical output for the same input.
*   **Golden Simulation Traces:** Create headless simulation runs (using the original engine if possible, or heavily logging it) to generate instruction execution traces (PC, Registers, Memory mutations). The ported `libeasym68k-sim` must match these traces exactly.
*   **Code Coverage:** Set up `gcov`/`lcov` to ensure the Golden tests exercise at least 90%+ of the core simulation and assembly logic.
*   **Sanitizers:** The new build system will run tests with AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) to catch latent memory bugs present in the original C++ code that Borland ignored.

### Golden Test Scenarios & Coverage Outline
To guarantee 100% parity, the following specific test suites will be compiled and executed using both the original binaries and the ported libraries:

1. **Instruction Set Architecture (ISA) Suite:** 
   *   Tests for every 68000 opcode combined with every valid addressing mode.
   *   Validation of Condition Code Register (CCR) mutations for arithmetic, logical, and bit-manipulation instructions.
   *   Edge cases for boundary conditions (e.g., overflow, carry, zero).
2. **Assembler Feature Suite:**
   *   **Directives:** Validation of `ORG`, `EQU`, `DS`, `DC`, `SECTION`, etc.
   *   **Macros & Conditional Assembly:** Testing deeply nested macros, arguments, and `IF`/`ENDC` blocks.
   *   **Symbol Table Validation:** Ensuring correct forward referencing and address resolution.
3. **Exception & Interrupt Suite:**
   *   Generation of simulated Bus Errors (unmapped memory) and Address Errors (unaligned word/long access).
   *   Privilege violations (executing privileged instructions in user mode).
   *   `TRAP` instructions, `CHK` bounds exceptions, and Divide-by-Zero (`DIVS`/`DIVU`).
4. **Hardware Emulation (Trap 15) Suite:**
   *   Automated mock tests for all Trap 15 tasks, organized by subsystem interface.
   *   Text I/O tasks (0-2, 5-6, 11-14, 16-18, 22, 24-25): verify string output, input buffering, cursor positioning, scrolling.
   *   Numeric I/O tasks (3-4, 15, 17-18, 20): verify decimal and arbitrary-base conversion.
   *   File I/O tasks (50-59): verify open, read, write, seek, delete, file dialogs.
   *   Serial I/O tasks (40-43): verify port initialization, parameter configuration, read/write with timeout.
   *   Network I/O tasks (100-107): verify UDP client/server creation, send, receive, close.
   *   Graphics tasks (80-96): verify all 16 raster operations (drawing modes), double buffering, pixel/line/shape drawing, text rendering.
   *   Sound tasks (70-77): verify standard playback (single voice), multi-voice playback, load/play/control/stop with reference numbers.
   *   Peripheral tasks (60-62): verify mouse/keyboard IRQ enable/disable and state reads.
   *   Simulator environment tasks (8, 23, 30-33): verify time, delay, cycle counter, hardware window, version, output window size/fullscreen.
   *   Printing task (10): verify character output to printer with form feed/line feed.
   *   Validation that the core sends the correct command payloads to each interface.
5. **EASyBIN / S-Record Suite:**
   *   Parsing of standard and malformed Motorola S-Records (`.S19`, `.S3`, `.S68`).
   *   Verification of binary output padding and alignment.
   *   Validation that `libeasym68k-sim` exports S-Record parsing for EASyBIN reuse (eliminating the code duplication in the original `EASyBIN/fileIO.cpp`).

## 6. Phase 1: Decoupling Core from UI

The current Borland implementation tightly couples UI code with core logic. For example, `RUN.CPP` in `Sim68K` directly manipulates `Form1` elements and processes UI events.

### 6.1 State Encapsulation

Move all global state from `extern.h`/`var.h` (Registers, Memory pointer, Flags, Breakpoints, Mouse state, etc.) into an opaque `M68kSimulator` context object. Every function that currently reads/writes global state will take a `M68kSimulator&` parameter (or be a method on the class).

### 6.2 Replace Pointer-Based Register Discrimination

The original `put()` and `value_of()` functions use pointer-range comparisons against the `D[]`/`A[]`/`PC` arrays to distinguish register operations from memory operations. This will break when state is encapsulated. Replace this with a tagged effective address type:

```cpp
enum class EaTarget : uint8_t { kDataReg, kAddrReg, kMemory, kImmediate };
struct EffectiveAddr {
  EaTarget target;
  uint32_t index;  // register number for reg targets, unused for memory
  uint32_t address; // memory address for memory target, immediate value for immediate
};
```

All instruction handlers will switch on `EaTarget` rather than performing pointer arithmetic.

### 6.3 Remove VCL and Windows Dependencies

*   Replace `AnsiString` with `std::string` throughout.
*   Replace `TColor` constants (`clBlack`, `clWhite`, etc.) with `uint32_t` RGB values matching the original `0x00BBGGRR` encoding used by Trap #15 tasks 21, 80, 81.
*   Replace `#include <vcl.h>`, `#include <vcl\Clipbrd.hpp>`, and all VCL headers.
*   Replace `Sleep()` with `std::this_thread::sleep_for`.
*   Replace `Application->ProcessMessages()` with an abstract `IEventPump` interface (see Section 7.2).
*   Replace `gettime()` (Borland DOS extension in task 8) with `std::chrono`.
*   Replace `__int64` with `uint64_t`, `CALLBACK` typedefs with standard function pointers.

### 6.4 Trap #15 Interface Decomposition

The original single `TRAP()` function in `CODE9.CPP` will be decomposed into focused interfaces. The core simulator will hold references to all interfaces and dispatch Trap #15 tasks to the appropriate one:

```cpp
// Text output and input (tasks 0-2, 5-6, 11-14, 16-18, 22, 24-25)
class ITextIO {
 public:
  virtual ~ITextIO() = default;
  virtual void TextOut(std::string_view text) = 0;
  virtual void TextOutCRLF(std::string_view text) = 0;
  virtual void TextIn(char* buf, long* size, long* result) = 0;
  virtual void CharIn(char* ch) = 0;
  virtual void CharOut(char ch) = 0;
  virtual void SetCursorPosition(int col, int row) = 0;
  virtual void GetCursorPosition(int* col, int* row) = 0;
  virtual void ClearScreen() = 0;
  virtual void SetKeyboardEcho(bool enable) = 0;
  virtual void SetInputPrompt(bool enable) = 0;
  virtual void SetInputLFDisplay(bool enable) = 0;
  virtual void GetCharAt(int row, int col, char* ch) = 0;
  virtual void ScrollRect(int row, int col, int height, int width, int direction) = 0;
  virtual void SetFontProperties(uint32_t color, uint32_t style) = 0;
  virtual void GetKeyState(long* state) = 0;
  virtual void SetKeyCommandsEnabled(bool enabled) = 0;
};

// File I/O (tasks 50-59)
class IFileIO {
 public:
  virtual ~IFileIO() = default;
  virtual void CloseAll(short* result) = 0;
  virtual void OpenFile(long* fid, const char* name, short* result) = 0;
  virtual void NewFile(long* fid, const char* name, short* result) = 0;
  virtual void ReadFile(long fid, char* buf, unsigned int* size, short* result) = 0;
  virtual void WriteFile(long fid, const char* buf, unsigned int size, short* result) = 0;
  virtual void PositionFile(long fid, int offset, short* result) = 0;
  virtual void CloseFile(long fid, short* result) = 0;
  virtual void DeleteFile(const char* name, short* result) = 0;
  virtual void DisplayFileDialog(long* mode, long a1, long a2, long a3, short* result) = 0;
  virtual void FileOp(long* mode, const char* name, short* result) = 0;
};

// Serial I/O (tasks 40-43) — replaces Win32 HANDLE/COMMTIMEOUTS
class ISerialIO {
 public:
  virtual ~ISerialIO() = default;
  virtual void InitPort(int cid, const char* port_name, short* result) = 0;
  virtual void SetPortParams(int cid, int settings, short* result) = 0;
  virtual void ReadString(int cid, unsigned char* count, char* buf, short* result) = 0;
  virtual void SendString(int cid, unsigned char* count, const char* buf, short* result) = 0;
  virtual void CloseAll() = 0;
};

// Network I/O (tasks 100-107) — replaces Winsock
class INetworkIO {
 public:
  virtual ~INetworkIO() = default;
  virtual void CreateClient(int settings, const char* server, int* result) = 0;
  virtual void CreateServer(int settings, int* result) = 0;
  virtual void Send(int settings, const char* data, const char* remote_ip, int* count, int* result) = 0;
  virtual void Receive(int settings, char* buf, int* count, char* sender_ip, int* result) = 0;
  virtual void CloseConnection(int close_ip, int* result) = 0;
  virtual void GetLocalIP(char* local_ip, int* result) = 0;
  virtual void SendOnPort(long* d0, long* d1, const char* data, const char* remote_ip) = 0;
  virtual void ReceiveOnPort(long* d0, long* d1, char* buf, char* sender_ip) = 0;
};

// Graphics (tasks 80-96) — replaces VCL TCanvas/QPainter
class IGraphicsIO {
 public:
  virtual ~IGraphicsIO() = default;
  virtual void SetPenColor(uint32_t color) = 0;
  virtual void SetFillColor(uint32_t color) = 0;
  virtual void DrawPixel(int x, int y) = 0;
  virtual int GetPixel(int x, int y) = 0;
  virtual void DrawLine(int x1, int y1, int x2, int y2) = 0;
  virtual void LineTo(int x, int y) = 0;
  virtual void MoveTo(int x, int y) = 0;
  virtual void GetXY(int* x, int* y) = 0;
  virtual void DrawRectangle(int x1, int y1, int x2, int y2) = 0;
  virtual void DrawEllipse(int x1, int y1, int x2, int y2) = 0;
  virtual void FloodFill(int x, int y) = 0;
  virtual void DrawUnfilledRectangle(int x1, int y1, int x2, int y2) = 0;
  virtual void DrawUnfilledEllipse(int x1, int y1, int x2, int y2) = 0;
  virtual void SetDrawingMode(int mode) = 0;
  virtual void SetPenWidth(int width) = 0;
  virtual void Repaint() = 0;
  virtual void DrawText(const char* str, int x, int y) = 0;
  virtual void SetWindowSize(unsigned short width, unsigned short height) = 0;
  virtual void GetWindowSize(unsigned short* width, unsigned short* height) = 0;
  virtual void SetupWindow() = 0;
};

// Sound (tasks 70-77) — replaces DirectX DirectMusic and TMediaPlayer
class ISoundIO {
 public:
  virtual ~ISoundIO() = default;
  // Standard single-voice player (replaces TMediaPlayer)
  virtual void PlaySound(const char* filename, short* result) = 0;
  virtual void LoadSound(const char* filename, int wave_index) = 0;
  virtual void PlaySoundMem(int wave_index, short* result) = 0;
  virtual void ControlSound(int control, int wave_index, short* result) = 0;
  // Multi-voice player (replaces DirectX DirectMusic)
  virtual void PlaySoundMulti(const char* filename, short* result) = 0;
  virtual void LoadSoundMulti(const char* filename, int wave_index, short* result) = 0;
  virtual void PlaySoundMemMulti(int wave_index, short* result) = 0;
  virtual void StopSoundMemMulti(int wave_index, short* result) = 0;
  virtual void ControlSoundMulti(int control, int wave_index, short* result) = 0;
  // Lifecycle
  virtual void ResetSounds() = 0;
};

// Peripheral I/O — Mouse/Keyboard (tasks 60-62)
class IPeripheralIO {
 public:
  virtual ~IPeripheralIO() = default;
  virtual void SetMouseIRQ(uint8_t irq_level, uint8_t events) = 0;
  virtual void ReadMouse(int mode, long* button_state, long* position) = 0;
  virtual void SetKeyIRQ(uint8_t irq_level, uint8_t events) = 0;
};

// Simulator environment (tasks 8, 23, 30-33)
class ISimulatorEnv {
 public:
  virtual ~ISimulatorEnv() = default;
  virtual uint32_t GetTimeHundredths() = 0;      // task 8
  virtual void DelayHundredths(unsigned int n) = 0; // task 23
  virtual void ShowHardware() = 0;                // task 32 sub 0
  virtual uint32_t GetSeg7Address() = 0;           // task 32 sub 1
  virtual uint32_t GetLEDAddress() = 0;            // task 32 sub 2
  virtual uint32_t GetSwitchAddress() = 0;         // task 32 sub 3
  virtual uint32_t GetPushButtonAddress() = 0;     // task 32 sub 7
  virtual void SetAutoIRQ(uint8_t irq_config, uint32_t interval) = 0; // task 32 sub 6
};

// Printing (task 10) — replaces VCL TPrinter
class IPrintIO {
 public:
  virtual ~IPrintIO() = default;
  virtual void PrintChar(char ch) = 0;
  virtual void FormFeed() = 0;
};
```

The Qt GUI will implement all of these interfaces. The headless test harness will provide mock implementations. This decomposition allows each subsystem to be tested independently.

### 6.5 Logging & Error Reporting

Define an abstract `ILogger` interface. `RUN.CPP` will call `logger->Log(...)` instead of `Form1->Message->Lines->Add(...)`. The core libraries will use a `SimResult` type for error propagation:

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

No exceptions will be thrown from the core libraries. The GUI layer translates `SimResult` codes into user-visible messages.

### 6.6 Memory Byte-Order Policy

**Decision: Memory will be stored in big-endian (68000 native) byte order.**

Rationale:
* The original code stores memory in host (little-endian) byte order on x86, relying on direct pointer casts like `(long*)&memory[addr]`. This is a strict aliasing violation and breaks on ARM64.
* Storing in big-endian order is faithful to the 68000 architecture and makes the memory dump display correct without byte-swapping.
* All `mem_put()`, `mem_req()`, `put()`, and `value_of()` functions will perform byte-swapping on little-endian hosts using `bswap16`/`bswap32` intrinsics or `std::endian` checks.
* S-Record loading already reads hex digits byte-by-byte, so it naturally writes big-endian.
* The `eff_addr()` functions that cast `(long*)&memory[addr]` will be replaced with proper `ReadLong()`/`WriteLong()` calls through the `Memory` class.

This is a foundational decision that affects every memory access in the simulator. It must be made before any code is ported.

### 6.7 Build System

Create a standard `CMakeLists.txt` to compile the core `.cc` files into static libraries. CMake 3.16+ minimum. Qt 6 will be an optional dependency (required only for GUI builds). The core libraries must compile with zero Qt dependencies.

## 7. Phase 2: The Qt GUI Suite

Once the core is compiling cleanly as static libraries with zero VCL/Windows dependencies, GUI development begins.

### 7.1 UI Design Constraints

*   **Visual Parity:** Original GUI layouts must be strictly preserved. All font sizes, colors, styling, button placements, and general aesthetics must perfectly match the original Borland application. The original `.dfm` form definition files will be reverse-engineered to produce equivalent Qt `.ui` files or programmatic layouts. Each `.dfm` file defines pixel-precise widget positions, sizes, fonts, and colors that must be faithfully reproduced.
*   **Editor Interface:** The original MDI (Multiple Document Interface) in Edit68K will transition to a modern tabbed editor interface (`QTabWidget` with `QPlainTextEdit` or `QScintilla` for syntax highlighting).
*   **Framework Features:** Transitioning from Borland-specific UI controls to their closest Qt equivalents is expected, but the overall visual result must remain identical.

#### 7.1.1 Required Layouts and UI Features
Based on a deep analysis of the original UI screenshots, the following specific layouts, controls, and visual details must be explicitly recreated:

**Edit68K (Editor/Assembler) UI Requirements:**
*   **Menu & Toolbar:** Must include standard menus (File, Edit, Project, Options, Window, Help) and a toolbar with specific icons: New, Open, Save, Print, Find, Cut, Copy, Paste, Undo, Indent, Unindent, and a prominent 'Play' icon for "Assemble".
*   **MDI/Tabbed Layout:** The original uses an MDI (Multiple Document Interface) layout where each `.X68` file is a sub-window. The port will transition this to a modern tabbed interface (`QTabWidget`), but the internal text editor must maintain the same visual width and spacing.
*   **Syntax Highlighting:** The custom `QSyntaxHighlighter` must strictly implement the original color scheme:
    *   Comments: Blue
    *   Labels (e.g., `START`, `MESSAGE`): Purple
    *   Directives (e.g., `ORG`, `DC.B`, `END`): Green
    *   Strings: Teal/Cyan
    *   Instructions: Black
*   **Status Bar:** Must display current cursor position (e.g., "ln 1 col 1") and insert/overwrite mode.

**Sim68K (Simulator) UI Requirements:**
*   **Main Simulator Window:**
    *   **Menus:** File, Run, View, Options, Help.
    *   **Toolbar Icons:** Open, Run (F9), Step Over, Step Into, Step Out, Pause, Reset, and specific icons for opening sub-windows (Memory Viewer, Hardware Window, I/O Console, Stack Window, Log Window).
    *   **Registers Panel:** A dense grid of individual text boxes for D0-D7 and A0-A7 (all hex), plus PC, USP, SSP, SR bits (T, S, INT, XNZVC), and a Cycle counter with a "Clear Cycles" button. All register fields are user-editable; values typed by the user are loaded into the CPU state before each Step/Run.
    *   **Source Code View (Listing):** A multi-column view displaying the `.L68` listing file produced by the assembler. Columns: Breakpoint Margin (with green/red dots for active/inactive breakpoints), Address (hex), Code (hex opcode/operands), Line Number, and Source text. The current PC is located via binary search and highlighted with a solid blue background spanning the entire row. The listing file is loaded automatically when an `.S68` file is opened (if a matching `.L68` file exists). The view processes `*[sim68k]` embedded commands in listing comments: `*[sim68k]break` sets a breakpoint on the next line, `*[sim68k]bitfield` enables bitfield instructions, and `*[sim68k]simhalt_off` disables the SIMHALT instruction.
    *   **Run-to-Cursor:** Double-click or context menu on a listing line sets a temporary breakpoint and starts execution.
    *   **Message Pane:** A resizable text area at the very bottom for simulator logs (e.g., ".S68 file read successful").
    *   **Logging Indicator:** A visible label (e.g., "Logging") shown when execution or output logging is active.
*   **Stack Viewer Window:** A dedicated floating/dockable window displaying the 68000 stack contents:
    *   Address column (8-char hex) followed by 4 bytes per row in hex.
    *   Address register selector (`QComboBox` for A0-A7) to choose which register to track; default is A7 (system stack pointer).
    *   Per-byte background highlighting: **Yellow** for bytes at the A7 (system SP) address, **Aqua** for bytes at the selected address register value, **Red text** for bytes in the Invalid memory range, **White** for all other bytes.
    *   Scroll by 4 bytes (one long word per row) via spin buttons or mouse wheel.
    *   Auto-centers on the selected address register so the stack pointer is always near the middle row.
    *   Auto-refreshes after each Step/Trace/Pause.
    *   Ctrl+Tab cycles between Memory → Stack → Hardware windows.
*   **Memory Viewer Window:** Hex/ASCII memory viewer with full editing capabilities:
    *   Standard hex dump format: address column, 16 hex bytes per row, 16-character ASCII representation.
    *   Address jump input for navigating to any address.
    *   **Direct hex editing:** Click on any hex digit (0-9, A-F) to modify that nybble. The byte is read, the target nybble is modified, and the result is written back. Cursor auto-advances to the next nybble/byte.
    *   **Direct ASCII editing:** Click on any character in the ASCII area and type a replacement. Cursor auto-advances to the next character.
    *   **Block Copy:** From/To/Bytes input fields with a Copy button. Handles overlapping regions correctly (copies from highest address first when destination > source).
    *   **Block Fill:** From/To/FillValue input fields with a Fill button. Fills each byte in the range with the specified value.
    *   LivePaint mode (auto-refresh on each Step/Trace).
    *   Data highlighting by memory type: Invalid=red, ROM=blue, Protected=olive.
    *   Row/Page Up/Down navigation buttons.
    *   After any memory edit, the Hardware window and Stack window are refreshed.
*   **Peripheral Windows:** The simulator utilizes independent floating tool windows that must be recreated (using `QDialog` or `QDockWidget`):
    *   **Hardware Window:** Visual representations of 68k hardware: 8x 7-segment displays (red digits on black), 8x LEDs (red circles), 8x push buttons, and 8x toggle switches, along with their mapped memory address inputs. Auto-IRQ timer for generating periodic interrupts. The `updateIfNeeded(address)` method refreshes only the peripherals affected by a memory write at the given address.
    *   **Sim68k I/O Console:** A dedicated black-background console window for Trap 15 text input/output.
    *   **Log Window:** Configuration dialog for execution logging and output logging:
        *   Execution Log: Records each instruction with registers and optional memory dump to a text file. Modes: Off / Instructions & Registers / Instructions, Registers & Memory (with configurable From address and Bytes count).
        *   Output Log: Records all Trap #15 text output to a binary file. Modes: Off / On.
        *   File-exists dialog on start: Replace / Append / Cancel.
        *   Log file names auto-derived from the S-Record filename (`<basename>_RunLog.txt`, `<basename>_OutLog.txt`).
        *   Start/Stop toolbar buttons and menu items.
    *   **Memory Range Dialog:** Options dialog for defining memory protection regions:
        *   ROM region (reads allowed, writes silently ignored): enable checkbox + start/end address.
        *   Protected region (bus error in user mode): enable checkbox + start/end address.
        *   Invalid region (bus error unconditionally): enable checkbox + start/end address.
        *   Read-Only region (read-only unless supervisor): enable checkbox + start/end address.
        *   Ranges are also loaded from S-Record file headers (parsed from EASy68K-specific S-Record comments).

**EASyBIN UI Requirements:**
*   **Control Panel Layout:** The top section must contain three distinct group boxes (`QGroupBox`): "S-Record File" (with Start/Last/Length fields and an Open button), "Binary Output Split" (radio buttons for 1, 2, or 4 files), and "Binary File(s)" (First Address, Length, Save button).
*   **Memory Manipulation Tools:** A toolbar below the group boxes containing "Address" jump input, "From/To/Size" block definitions, and "Copy" / "Fill ($FF)" buttons.
*   **Color-Coded Hex Viewer:** A large hex viewer that strictly implements the original color-coding scheme (e.g., green, blue, black, olive text) to visually indicate how bytes are split across multiple binary files, accompanied by an ASCII representation on the right.
*   **Custom Navigation:** Dedicated "Row" and "Page" Up/Down spin buttons flanking the hex viewer on the right edge.

### 7.2 Threading Model

The original uses a single-threaded cooperative model with `Application->ProcessMessages()`. The port must decide between two approaches:

**Option A — Dedicated QThread (Recommended):**
The simulator runs in a dedicated `QThread`. The GUI implements the Trap #15 interfaces with thread-safe wrappers:
*   **Asynchronous operations** (text output, graphics drawing, sound): The interface implementation emits Qt signals, which are connected across threads via `Qt::QueuedConnection`. The simulator thread does not block.
*   **Synchronous operations** (text input, file dialogs): The interface implementation uses `QMetaObject::invokeMethod` with `Qt::BlockingQueuedConnection` to marshal the call to the GUI thread and block the simulator until the user responds. This preserves the original's synchronous input model.
*   **Delay/task 23:** Implemented with `QThread::msleep()` or a `QWaitCondition` instead of `Sleep()` + `ProcessMessages()`.
*   **Halt/breakpoint:** The simulator thread checks an `std::atomic<bool> stop_requested` flag on each instruction boundary.

**Option B — Single Thread with `QCoreApplication::processEvents()`:**
Closer to the original model. Simpler to implement but provides less responsive UI during long-running simulations. This is a viable fallback if Option A proves too complex for synchronous input.

The project will start with Option A.

### 7.3 Sound System

The original has two sound backends:
1.  **Standard player** (`TMediaPlayer`): Single-voice, blocking. Maps to `QMediaPlayer` + `QAudioOutput`.
2.  **DirectX player** (DirectMusic): Multi-voice, non-blocking. Maps to `QSoundEffect` for low-latency multi-voice playback, or a `QMediaPlayer` pool.

The `ISoundIO` interface unifies both. The Qt implementation will:
*   Use `QMediaPlayer` for the standard single-voice player (tasks 70-72, 76).
*   Use `QSoundEffect` instances keyed by reference number for the multi-voice player (tasks 73-75, 77). `QSoundEffect` supports simultaneous playback of multiple sounds with low latency.
*   Sound files (WAV) are loaded from the host filesystem, not from 68000 memory.
*   The `ResetSounds()` method stops all playback and clears the sound cache.

### 7.4 Graphics System

The original uses VCL `TCanvas` for drawing with a back-buffer (`Graphics::TBitmap* BackBuffer`). The Qt implementation will:
*   Use `QPainter` on a `QPixmap` back-buffer for double-buffered drawing (tasks 92 modes 16-17).
*   Implement the 16 raster operations (drawing modes, task 92) using `QPainter::CompositionMode`. The mapping from original mode numbers to Qt composition modes:
    *   0 (R2_BLACK) → `QPainter::CompositionMode_Source` with black brush
    *   4 (R2_COPYPEN) → `QPainter::CompositionMode_SourceOver`
    *   3 (R2_NOT) → `QPainter::CompositionMode_Source` with inverted pen
    *   etc. (full mapping table to be produced during implementation)
*   The output window (`SimIO` form) becomes a custom `QWidget` with `paintEvent` that blits the back-buffer.
*   Full-screen mode (task 33) uses `QWidget::showFullScreen()` with `QScreen` selection for multi-monitor support.

### 7.5 Network System

The original uses Winsock (UDP/TCP). The Qt implementation will:
*   Use `QUdpSocket` for UDP and `QTcpServer`/`QTcpSocket` for TCP.
*   The `INetworkIO` implementation wraps Qt network classes, preserving the original's task number protocol (100-107).
*   Network operations are inherently asynchronous in Qt, so the implementation must handle the impedance mismatch with the original's synchronous receive model using `QEventLoop` or `QwaitForReadyRead()`.

### 7.6 Serial I/O System

The original uses Win32 `HANDLE`/`COMMTIMEOUTS`. The Qt implementation will:
*   Use `QSerialPort` (from the `QtSerialPort` module) for cross-platform serial communication.
*   Map the original's port speed/parity/stop-bit encoding to `QSerialPort::BaudRate`, `QSerialPort::Parity`, `QSerialPort::StopBits`.
*   Port names like "COM4" on Windows map to `QSerialPortInfo::availablePorts()`; on macOS/Linux they become `/dev/tty.*` or `/dev/ttyS*`.

### 7.7 Printing

The original uses VCL `TPrinter`. The Qt implementation will:
*   Use `QPrinter` + `QPainter` for cross-platform printing.
*   The `IPrintIO::PrintChar()` implementation renders characters onto a `QPainter` targeting a `QPrinter`, with equivalent page layout logic.

### 7.8 Help System

The original uses Windows HTML Help (`.chm`) with context-sensitive help IDs defined in `ez68k.h` (142 IDs). **Porting the help system is out of scope for this port.** The Qt applications will launch the existing HTML help files in the system's default browser using `QDesktopServices::openUrl()`. Context-sensitive help IDs from `ez68k.h` will be mapped to corresponding HTML file URLs. No conversion to Qt Help format (`.qhp`/`.qch`) or in-app help viewer will be implemented.

### 7.9 Settings Persistence

The original stores settings in the Windows Registry and `.dat` files. The Qt implementation will:
*   Use `QSettings` with `IniFormat` for cross-platform settings storage.
*   Window positions, font choices, output window size, and simulator options will be persisted automatically.

### 7.10 OS Integration

*   **Drag-and-drop:** The original supports drag-and-drop from Explorer (`WM_DROPFILES`). The Qt implementation uses `setAcceptDrops(true)` + `dropEvent()` on the main window.
*   **File associations:** Optional; handled via platform-specific installer configuration rather than registry manipulation.

### 7.11 High-Performance Memory View

The memory viewer must efficiently display large blocks of memory without UI lag. It will utilize the Model/View architecture (`QAbstractTableModel`) to only fetch memory blocks that are currently visible on-screen.

### 7.12 EASyBIN Code Sharing

The original `EASyBIN/fileIO.cpp` duplicates S-Record parsing logic from the simulator. In the ported architecture, `libeasym68k-sim` will export its S-Record parsing as a public API, and `EASyBIN-Qt` will use it directly, eliminating duplication.

## 8. Summary of Execution Plan

1.  **Setup & Baseline:** Create CMake structure, port original files to `snake_case`, and establish Golden test data using the original binaries. Audit and decide on memory byte-order policy (Section 6.6). See [Code Transfer Plan](code_transfer_plan.md) for the file-by-file transfer execution order.
2.  **Core Refactor:** Iteratively strip VCL/Windows includes. Replace pointer-based register discrimination with tagged effective addresses (Section 6.2). Introduce decomposed Trap #15 interfaces (Section 6.4), `ILogger`, and `SimResult` error types (Section 6.5).
3.  **Headless Verification:** Run `gtest` suites against `libeasym68k-asm` and `libeasym68k-sim` to guarantee byte-for-byte and execution-trace parity with the Golden data. Fix any endian or sanitizer issues.
4.  **Continuous Integration:** Establish GitHub Actions to run the tests automatically on Linux, macOS, and Windows, including ASan/UBSan builds.
5.  **GUI Implementation:** Build the three Qt applications (Edit68K-Qt, Sim68K-Qt, EASyBIN-Qt), implementing all Trap #15 interfaces with Qt backends (QPainter, QMediaPlayer, QSoundEffect, QSerialPort, QUdpSocket/QTcpSocket, QPrinter, QSettings). Strictly adhere to visual parity constraints and the threading model (Section 7.2).
6.  **Feature Parity Validation:** Run the full Feature Mapping Matrix against all three Qt applications, verifying every Trap #15 task, every menu option, every keyboard shortcut, and every visual element against the original Windows application. The execution plan's Feature Coverage Matrix tracks all 30 original simulator features to their implementing tasks, ensuring zero gaps.
