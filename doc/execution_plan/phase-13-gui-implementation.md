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
