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
