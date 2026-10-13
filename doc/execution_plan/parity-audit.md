# EASy68K Functional Parity Audit

**Goal:** full functional parity with the original Windows-only EASy68K
(Sim68K, Edit68K, EASyBIN, Help) as a cross-platform Qt 6 application set.

**Method:** every user-visible element was extracted from the original Borland
forms (`*.dfm`: menus, actions, shortcuts, dialogs, options) and cross-checked
against the original sources and the port. Each item has a parity ID, the
original source, the current port status, and the plan task that delivers it.
Items without a task in the original execution plan are marked **NEW** and are
added to the plan below.

Status legend: **Done** (implemented and tested in the port), **Partial**,
**Gap** (planned, not implemented), **NEW** (missing from the plan; added here).

Snapshot: core libraries ~85% (Phase 9 in progress), assembler done (21 golden
tests), all 27 GUI source files are still 1-line stubs.

---

## 1. Simulator core (headless, `easym68k-sim`)

| ID | Feature | Original | Port status | Task |
|----|---------|----------|-------------|------|
| S1 | All 68000 instructions, addressing modes, N/Z/V/C/X | RUN.CPP, CODE1-9.CPP | Done | 3-6 |
| S2 | Exceptions: illegal, privilege, div0, CHK, TRAPV, TRAP #n, trace, Line A/F | RUN.CPP | Done | 4.2, 6.5 |
| S3 | Address/bus error (class 0 frame, 50 cycles) | RUN.CPP exceptionHandler | **Partial**: port halts instead of building the class 0 frame | **NEW 9.7** |
| S4 | Interrupts: autovector IRQ 1-7, NMI, STOP wake | RUN.CPP irqHandler, hardwareu.cpp | Done | 9.1 |
| S5 | Cycle counting, Trap #15 tasks 30/31 | inc_cyc(), CODE9.CPP | Done | 9.2 |
| S6 | SIMHALT on/off (`*[sim68k]simhalt_off`) | SIM68Ku.cpp | Gap | 9.3 |
| S7 | Enable Exceptions toggle (Options menu, task 32/5) | SIM68Ku.cpp, hardwareu.cpp | Gap | 9.3 (core-owned flag, see note A) |
| S8 | **68020 bit-field instructions** BFTST/BFEXTU/BFCHG/BFEXTS/BFCLR/BFFFO/BFSET/BFINS (Options → Enable Bit Field Instructions, `*[sim68k]bitfield`) | CODE7.CPP BIT_FIELD() | **Gap**: not in simulator, not in plan | **NEW 9.8** |
| S9 | Trap #15 tasks 0-25, 30-33, 40-43, 50-62, 70-77, 80-96, 100-107 | CODE9.CPP TRAP() | Done (dispatch + interfaces; back ends in Phase 13) | 7.x |
| S10 | Memory map: ROM / Read / Protected / Invalid ranges | hardwareu.cpp memoryMapCheck | Partial (Memory flags exist; verify behaviour) | 9.6 + 13.15 |
| S11 | Execution logging (instruction / +registers / +memory range) | RUN.CPP, logU.cpp | Gap | 9.4 |
| S12 | Output (Trap #15 text) logging | logU.cpp OlogFile | Gap | 9.4 |
| S13 | PC breakpoints | BPoint.cpp | Done (core) | 4.1 |
| S14 | **Advanced breakpoints**: register conditions (D0-D7/A0-A7/PC, ==, !=, >, >=, <, <=, size B/W/L), memory conditions (address, value, R/W/Read/Write, size), expression builder (AND/OR/parentheses, pass count, ON/OFF) | BREAKPOINTSu.cpp, BPoint*.cpp | **Gap**: not in plan | **NEW 9.9** (core engine) + **NEW 13.22** (dialog) |
| S15 | Step Over (skip into BSR/JSR), Trace Into, Run To Cursor, Auto Trace | SIM68Ku.cpp, CODE8.CPP stepToAddr | Partial: Step/Run only | **NEW 9.10** (step-over / run-to core API) |
| S16 | Event/observer hooks for GUI | — (new) | Gap | 9.5 |

## 2. Assembler (`easym68k-asm`)

| ID | Feature | Original | Port status | Task |
|----|---------|----------|-------------|------|
| A1 | Full instruction set, directives, macros, conditional + structured assembly | Edit68K/*.CPP | Done | 8.x |
| A2 | Listing (.L68) and S-Record (.S68) output | LISTING.CPP, BUILD.CPP | Done (golden byte-identical) | 8.5.10-11, 8.6 |
| A3 | Error/warning reporting, max errors | ERROR.CPP | Done | 8.5.9 |
| A4 | **Bit-field instruction encoding** (Assembler Options → Assemble Bit Field Instructions) | CODEGEN.CPP, INSTTABL.CPP | **Gap**: lexer knows mnemonics, no encoding | **NEW 8.7** |
| A5 | OPT CRE: cross-reference / symbol table appended to listing | LISTING.CPP optCRE() | **Gap** | **NEW 8.8** |
| A6 | OPT MEX / SEX / CEX listing expansion (macros, structured, constants) | LISTING.CPP | Partial: only CEX wired (`cex_flag`) | **NEW 8.8** |
| A7 | Show-warnings option; auto-save before assemble; generate list / S-record toggles | optionsS.cpp | Gap (option plumbing) | **NEW 8.9** (AssemblerOptions struct) |
| A8 | Source template for new files | optionsS.cpp Template, template.NEW | Gap | 13.19 |

## 3. Sim68K GUI

| ID | Feature | Original | Task |
|----|---------|----------|------|
| G1 | Main window: registers D0-D7, A0-A7, PC, SS, US, SR with T S INT XNZVC, editable | SIM68Ku.dfm | 13.3 |
| G2 | Cycles display + Clear Cycles | SIM68Ku.dfm | 13.3 |
| G3 | Toolbar + Run menu: Open, Run (F9), Run To Cursor (Ctrl+F9), Auto Trace (F10), Step Over (F8), Trace Into (F7), Pause (F6), Rewind (Ctrl+F2), Reload (Ctrl+F3), Log Start (Ctrl+F4), Log Stop (Ctrl+F5) | SIM68Ku.dfm ActionList | 13.2 |
| G4 | File: Open (Ctrl+O), Open Data (load extra S-record), Close, Printer Setup, Exit | SIM68Ku.dfm | 13.2 |
| G5 | Source view (.L68 mapped to PC), breakpoint gutter, Clear All PC Breakpoints popup, message pane | SIM68Ku.dfm ListBox1/breakP | 13.4 |
| G6 | `*[sim68k]break`, `bitfield`, `simhalt_off` listing commands | SIM68Ku.cpp | 13.4 |
| G7 | Search: Find (Ctrl+F), Find Next (F3), Go to PC (Ctrl+G) | findDialogS, SIM68Ku.dfm | **NEW 13.23** |
| G8 | View menu: Output Window, Memory, Stack, Hardware, Break Points, EASyBIN | SIM68Ku.dfm | 13.2 |
| G9 | Options: Source / Output / Printer fonts | SIM68Ku.dfm | 13.18 |
| G10 | Options: Log Output dialog (exec log type, output log type, file names, memory range) + Replace/Append prompt | logU.dfm, LogfileDialogu.dfm | 13.12 |
| G11 | Options: Enable Exceptions, Enable Bit Field Instructions | SIM68Ku.dfm | 13.2 (wires S7, S8) |
| G12 | Options: Auto Trace Options (interval ms, disable display update) | Optionsu.dfm | **NEW 13.24** |
| G13 | Options: Fullscreen Options (screen number) | FullscreenOptions.dfm | 13.10 |
| G14 | Options: Output Window Size (640x480 … 1920x1200, 9 presets), Text Wrap | SIM68Ku.dfm | 13.9/13.10 |
| G15 | Memory window: hex/ASCII, Row/Page spin, Live update, address jump, edit bytes, Copy, Fill, Save range to file | Memory1.dfm/.cpp | 13.6, 13.7 (+ Save: **NEW** in 13.7) |
| G16 | Stack window: view A0-A6/US/SS, scroll | Stack1.dfm | 13.5 |
| G17 | Hardware window: 7-seg ×8, LEDs, 8 toggle switches, 8 push buttons, editable addresses, IRQ 1-7 buttons, auto IRQ (level + interval ms), Reset button, memory map (ROM/Read/Protected/Invalid start/end) | hardwareu.dfm | 13.8, 13.15 |
| G18 | Output window: text I/O, graphics, sound, mouse/keyboard, fullscreen | simIOu.cpp, CODE9.CPP | 13.9-13.11 |
| G19 | Advanced Break Points dialog (S14) | BREAKPOINTSu.dfm | **NEW 13.22** |
| G20 | Drag-and-drop open, settings persistence, Help, About | SIM68Ku.cpp | 13.2, 13.18 |
| G21 | Launch EASyBIN from View menu | SIM68Ku.dfm | 13.2 |

## 4. Edit68K GUI

| ID | Feature | Original | Task |
|----|---------|----------|------|
| E1 | MDI multi-document editor; Window menu: Arrange, Cascade, Tile H/V, Minimize All | mainS.dfm | 13.19 (QMdiArea) |
| E2 | File: New X68 (from template), Open (Ctrl+O), Save (Ctrl+S), Save As, Print (Ctrl+P), Printer Setup, Close, Exit; save-changes prompt | mainS.dfm, chksaveS | 13.19 |
| E3 | Edit: Undo (Ctrl+Z) / Redo (Ctrl+Shift+Z), Cut/Copy/Paste/Select All, Comment / Uncomment Selection (Ctrl+Shift+C / U), context menu, Reload File | mainS.dfm, textS.dfm, undo.cpp | 13.19 |
| E4 | Find (Ctrl+F, whole word, match case), Find Next (F3), Find and Replace (Ctrl+R, Replace / Replace All) | findDialogS, replaceDialogS | 13.19 |
| E5 | Syntax highlighting: Code, Comment, Directive, Label, Macro/Other, Structured, Structure Error, Text, Background; presets Color1/Color2/No Color/Custom/Disabled; bold/italic/underline | editorOptions.dfm | 13.19 |
| E6 | Editor options: new-file font + size, Assembly tabs vs fixed tabs (size), Auto Indent, Real Tabs, Print in black | editorOptions.dfm | 13.19 |
| E7 | Adjust Font dialog for current file | fontForm1.dfm | 13.19 |
| E8 | Assemble (F9): status dialog with warnings/errors count, Execute (launch Sim68K), Load L68 | assembleS.dfm | 13.19 |
| E9 | Error list pane: double-click jumps to source line, Clear Error Messages | textS.dfm Messages | 13.19 |
| E10 | Listing viewer window | listS.dfm | 13.19 |
| E11 | Assembler Options dialog (A4-A8) + Template tab | optionsS.dfm | 13.19 |
| E12 | Print preview | preview.dfm | 13.19 (QPrintPreviewDialog) |
| E13 | Status bar (line/column), progress bar during assembly | textS.dfm | 13.19 |
| E14 | Help, About | mainS.dfm | 13.19, H1 |

## 5. EASyBIN

| ID | Feature | Original | Task |
|----|---------|----------|------|
| B1 | Open S-record and binary files, drag-and-drop | Form1u.cpp OpenSRecordFile/OpenBinFile/WmDropFiles | 13.20 |
| B2 | Save as S-record or binary; output start address, first address, length, split output | Form1u.cpp SaveSRecFile/SaveBinFile/Output* | 13.20 |
| B3 | Hex/ASCII memory editor: row/page spin, mouse wheel, cursor navigation, byte editing | Form1u.cpp FormPaint/FormKeyPress | 13.20 |
| B4 | Copy, Fill (from/to/bytes/fill byte), New, Help, About | Form1u.cpp | 13.20 |

## 6. Help

| ID | Feature | Original | Task |
|----|---------|----------|------|
| H1 | 40 HTML help pages + quick reference PDF, opened from all three apps | Help/ (Windows HTML Help) | **NEW 13.25**: bundle pages, show with QTextBrowser (or QtHelp); PDF via QDesktopServices |

## 7. Windows-specific behaviour and cross-platform equivalents

| Original (Windows) | Cross-platform replacement |
|--------------------|----------------------------|
| COM ports (Win32 serial) | QtSerialPort, optional at build time (`ENABLE_SERIAL_PORT`); tasks 40-43 report failure when unavailable |
| Winsock TCP/UDP (tasks 100-107) | QtNetwork (QTcpSocket/QTcpServer/QUdpSocket) |
| Win32 sound (tasks 70-77) | QtMultimedia (QSoundEffect / QMediaPlayer / QAudioSink) |
| GDI raster ops, double buffering (tasks 80-96) | QPainter composition modes on QImage |
| Win32 timers (tasks 8, 23; auto IRQ; auto trace) | QElapsedTimer / QTimer |
| Fullscreen on a chosen monitor (task 33) | QScreen + showFullScreen |
| Win32 printing (task 10, Print menus) | QtPrintSupport |
| HTML Help (.chm) | QTextBrowser / QtHelp |
| MDI (Borland) | QMdiArea |
| Borland TRichEdit syntax colouring | QSyntaxHighlighter on QPlainTextEdit |
| Drag-and-drop (WM_DROPFILES) | Qt drag-and-drop events |
| Registry/INI settings | QSettings |
| CRLF files, case-insensitive file names | Accept CRLF and LF; case-insensitive extension matching (.X68/.x68, .L68, .S68) |

## 8. Plan changes (new tasks)

| New task | Content | Depends on |
|----------|---------|------------|
| 8.7 | Bit-field instruction encoding (A4) | 8.5 |
| 8.8 | OPT CRE symbol table; OPT MEX/SEX listing expansion (A5, A6) — golden tests | 8.5.10 |
| 8.9 | `AssemblerOptions` struct: list/S-record/warnings/bitfield/expansion flags (A7) | 8.8 |
| 9.7 | Address/bus error class 0 exception frame, 50 cycles (S3) | 9.3 |
| 9.8 | Bit-field instructions in simulator, gated by flag (S8) | 9.3 |
| 9.9 | Advanced breakpoint engine: register/memory/expression conditions, pass counts (S14) | 9.5 |
| 9.10 | Step Over / Run To address core API (S15) | 9.5 |
| 13.22 | Advanced Break Points dialog (G19) | 9.9 |
| 13.23 | Sim68K Find / Find Next / Go to PC (G7) | 13.4 |
| 13.24 | Auto Trace + options dialog (G12) | 9.10 |
| 13.25 | Help system (H1) | — |

**Note A — core-owned toggles.** Trap #15 task 32 subtask 5 (exceptions) and the
`*[sim68k]` listing commands change simulator behaviour, so the flags live in
`M68kSimulator` (setters callable at runtime), not in `ISimulatorEnv`. This
follows the Task 9.2 decision for the cycle counter.

## 9. Parity acceptance

Parity is reached when every row in sections 1-6 is **Done**, verified by:
1. Unit tests per core item (S*, A*).
2. Golden assembly tests for A4-A6 and golden simulation traces (Phase 10).
3. A manual UI checklist generated from sections 3-6, run on Linux, macOS and
   Windows (Phase 13.21), plus CI on all three platforms (Phase 12).
