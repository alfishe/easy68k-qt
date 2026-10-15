## Feature Coverage Matrix

This matrix maps every feature of the original EASy68K simulator to the execution plan tasks that implement it. **Every feature must have at least one task.** Gaps identified during analysis are marked with NEW.

| # | Original Feature | Original Source | Execution Plan Task(s) | Status |
|---|-----------------|-----------------|----------------------|--------|
| 1 | 68000 register display (D0-D7, A0-A7, PC, USP, SSP, SR, Cycles) | SIM68Ku.cpp | Task 13.3 RegisterWidget | **Done & Validated (13.21)** |
| 2 | Source code display (.L68 listing mapped to PC) | SIM68Ku.cpp ListBox1, highlight() | Task 13.4 SourceView | **Done & Validated (13.21)** |
| 3 | Stack viewer (A7 highlight, address register selection, scroll) | Stack1.cpp | Task 13.5 StackWindow | **Done & Validated (13.21)** |
| 4 | Memory display (hex/ASCII viewer, address jump) | Memory1.cpp FormPaint | Task 13.6 MemoryWindow | **Done & Validated (13.21)** |
| 5 | Direct memory editing (click hex/ASCII byte, type new value) | Memory1.cpp FormKeyPress | Task 13.7 Memory Editing | **Done & Validated (13.21)** |
| 6 | Memory block copy (From/To/Bytes fields, overlap-safe) | Memory1.cpp CopyClick | Task 13.7 Memory Editing | **Done & Validated (13.21)** |
| 7 | Memory block fill (From/To/FillValue) | Memory1.cpp FillClick | Task 13.7 Memory Editing | **Done & Validated (13.21)** |
| 8 | Hardware display (7-segment, LEDs, switches, push buttons) | hardwareu.cpp | Task 13.8 HardwareWindow | **Done & Validated (13.21)** |
| 9 | Output results (Trap #15 text I/O console) | simIOu.cpp | Task 13.9 IoWindow | **Done & Validated (13.21)** |
| 10 | Graphics output (QPainter, 16 raster ops, double buffer) | CODE9.CPP tasks 80-96 | Task 13.10 GraphicsIO | **Done & Validated (13.21)** |
| 11 | Sound output (single-voice + multi-voice) | CODE9.CPP tasks 70-77 | Task 13.11 SoundIO | **Done & Validated (13.21)** |
| 12 | Interrupts (auto-IRQ, hardware IRQ, autovector) | hardwareu.cpp autoIRQ | Task 13.8 HardwareWindow + Task 11.2 | **Done & Validated (13.21)** |
| 13 | Log execution output (instruction + registers to text file) | logU.cpp ElogFile | Task 13.12 SimLog | **Done & Validated (13.21)** |
| 14 | Log Trap #15 output (text file) | logU.cpp OlogFile | Task 13.12 SimLog | **Done & Validated (13.21)** |
| 15 | Log memory range dump (configurable from/bytes) | logU.cpp MemFrom/MemBytes | Task 13.12 SimLog | **Done & Validated (13.21)** |
| 16 | Load S-Record files (.S68, .S19, .S3) | SIM68Ku.cpp loadSrec | Task 2.1 SRecordLoader + Task 13.8b | **Done & Validated (13.21)** |
| 17 | Serial I/O (COM port init, read, send) | CODE9.CPP tasks 40-43 | Task 13.13 SerialIO | **Done & Validated (13.21)** |
| 18 | TCP/UDP network I/O (client/server, send/receive) | CODE9.CPP tasks 100-107 | Task 13.14 NetworkIO | **Done & Validated (13.21)** |
| 19 | Definable memory ranges (Protected/Invalid/ROM with start/end) | SIMOPS2.CPP + hardwareu.cpp | Task 13.8 / 13.15 HardwareWindow | **Done & Validated (13.21)** |
| 20 | Breakpoints (PC breakpoints with toggle, clear all; advanced breakpoints) | SIM68Ku.cpp, BREAKPOINTSu.cpp | Task 4.1 + 9.9 + 13.4 + 13.22 | **Done & Validated (13.21)** |
| 21 | Run/Step/Trace/Auto Trace/Pause/Reset controls | SIM68Ku.cpp Run/Step/Trace | Task 13.2 + 13.24 MainWindow | **Done & Validated (13.21)** |
| 22 | Run-to-cursor | SIM68Ku.cpp RunToCursorExecute | Task 13.2 + 13.4 SourceView | **Done & Validated (13.21)** |
| 23 | *[sim68k] listing commands (break, bitfield, SIMHALT_OFF) | SIM68Ku.cpp lines 310-337 | Task 13.2a SimSession | **Done & Validated (13.21)** |
| 24 | Printing (character output, form feed) | CODE9.CPP task 10 | Task 13.1b / 13.16 QtPrintIO | **Done & Validated (13.21)** |
| 25 | File I/O (open, read, write, seek, close, delete, dialog) | CODE9.CPP tasks 50-59 | Task 10.4 + 13.1b / 13.17 QtFileIO | **Done & Validated (13.21)** |
| 26 | Peripheral I/O (mouse/keyboard IRQ enable, state read) | CODE9.CPP tasks 60-62 | Task 13.1b + 13.8 HardwareWindow | **Done & Validated (13.21)** |
| 27 | Full-screen output window | CODE9.CPP task 33 | Task 13.10 IoWindow | **Done & Validated (13.21)** |
| 28 | Drag-and-drop file open | SIM68Ku.cpp WmDropFiles | Task 13.2b MainWindow | **Done & Validated (13.21)** |
| 29 | Settings persistence (QSettings for window positions, fonts, options) | SIM68Ku.cpp SaveSettings | Task 13.18 SettingsPersistence | **Done & Validated (13.21)** |
| 30 | Cycle counter with clear button | SIM68Ku.cpp | Task 13.3 RegisterWidget | **Done & Validated (13.21)** |

---
