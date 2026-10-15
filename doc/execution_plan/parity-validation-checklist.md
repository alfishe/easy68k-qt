# Task 13.21 — Feature Parity Validation Checklist

This document records the automated and interactive validation of all **30 Feature Coverage Matrix items** (`doc/execution_plan/feature-coverage-matrix.md`) and all **69 Parity Audit rows** (`doc/execution_plan/parity-audit.md`) across `easy68k-simulator` (Sim68K-Qt), `easy68k-editor` (Edit68K-Qt), `easybin` (EASyBIN-Qt), and `HelpViewer`.

## 1. Automated Verification Summary

All test suites run headlessly under `QT_QPA_PLATFORM=offscreen` in both standard and AddressSanitizer + UndefinedBehaviorSanitizer (`build-gui-asan`) configurations:

- `easym68k-sim-tests`: **812 / 812** pass (incl. 28 oracle-verified golden simulation traces)
- `easym68k-asm-tests`: **406 / 406** pass (incl. 98,583-case instruction matrix, 21 golden programs, 33 corpus programs × 3 option variants)
- `easym68k-integration-tests`: **10 / 10** pass
- `easym68k-gui-tests`: **166 / 166** pass (incl. `ParityValidationTest` end-to-end IDE → Simulator → EASyBIN → Help validation)

---

## 2. Feature Coverage Matrix (30 / 30 Validated)

| # | Feature | Original Source | Automated Verification Suite(s) | Result |
|---|---------|-----------------|---------------------------------|--------|
| 1 | 68000 register display (`D0-D7`, `A0-A7`, `PC`, `USP`, `SSP`, `SR`, `Cycles`) | `SIM68Ku.cpp` | `RegisterWidgetTest.*`, `ParityValidationTest.EndToEndEditAssembleStep10InstructionsAndRunTextAndGraphics` | **PASS** |
| 2 | Source code display (`.L68` listing mapped to `PC`) | `SIM68Ku.cpp` `ListBox1`, `highlight()` | `SourceViewTest.*`, `ParityValidationTest.*` | **PASS** |
| 3 | Stack viewer (`A7` highlight, address register selection, scroll) | `Stack1.cpp` | `StackWindowTest.*` | **PASS** |
| 4 | Memory display (hex/ASCII viewer, address jump) | `Memory1.cpp` `FormPaint` | `MemoryWindowTest.*` | **PASS** |
| 5 | Direct memory editing (click hex/ASCII byte, type new value) | `Memory1.cpp` `FormKeyPress` | `MemoryWindowTest.EditHexAndAsciiBytes`, `ParityValidationTest.*` | **PASS** |
| 6 | Memory block copy (`From`/`To`/`Bytes` fields, overlap-safe) | `Memory1.cpp` `CopyClick` | `MemoryWindowTest.CopyFillAndSave` | **PASS** |
| 7 | Memory block fill (`From`/`To`/`FillValue`) | `Memory1.cpp` `FillClick` | `MemoryWindowTest.CopyFillAndSave` | **PASS** |
| 8 | Hardware display (7-segment, LEDs, switches, push buttons) | `hardwareu.cpp` | `HardwareWindowTest.*` | **PASS** |
| 9 | Output results (Trap #15 text I/O console) | `simIOu.cpp` | `SimIoScreenTest.*`, `IoWindowTest.*`, `ParityValidationTest.*` | **PASS** |
| 10 | Graphics output (`QPainter`, 16 raster ops, double buffer) | `CODE9.CPP` tasks 80-96 | `SimIoScreenTest.*`, `ParityValidationTest.*` | **PASS** |
| 11 | Sound output (single-voice + multi-voice) | `CODE9.CPP` tasks 70-77 | `QtSoundIoTest.*` | **PASS** |
| 12 | Interrupts (auto-IRQ, hardware IRQ, autovector) | `hardwareu.cpp` `autoIRQ` | `InterruptTest.*`, `HardwareWindowTest.AutomaticInterrupts` | **PASS** |
| 13 | Log execution output (instruction + registers to text file) | `logU.cpp` `ElogFile` | `SimLogTest.*`, `ParityValidationTest.BreakpointsMemoryEditHardwareMemoryMapAndLoggingValidation` | **PASS** |
| 14 | Log Trap #15 output (text file) | `logU.cpp` `OlogFile` | `SimLogTest.ExecutionAndOutputLogs`, `SimLogTest.OutputLogFollowsTheWindow` | **PASS** |
| 15 | Log memory range dump (configurable `From`/`Bytes`) | `logU.cpp` `MemFrom`/`MemBytes` | `SimLogTest.ExecutionAndOutputLogs`, `ParityValidationTest.*` | **PASS** |
| 16 | Load S-Record files (`.S68`, `.S19`, `.S3`) | `SIM68Ku.cpp` `loadSrec` | `SRecordLoaderTest.*`, `SimSessionTest.*` | **PASS** |
| 17 | Serial I/O (COM port init, read, send) | `CODE9.CPP` tasks 40-43 | `QtSerialIoTest.*` | **PASS** |
| 18 | TCP/UDP network I/O (client/server, send/receive) | `CODE9.CPP` tasks 100-107 | `StdNetworkIoTest.*` | **PASS** |
| 19 | Definable memory ranges (`ROM`/`Read`/`Protected`/`Invalid` with start/end) | `SIMOPS2.CPP` + `hardwareu.cpp` | `HardwareWindowTest.MemoryMapFieldsAndS0Records`, `AddressBusErrorTest.*` | **PASS** |
| 20 | Breakpoints (PC breakpoints with toggle, clear all; advanced breakpoints `R1..R50`, `A1..A50`, `E1..E50`) | `SIM68Ku.cpp`, `BREAKPOINTSu.cpp` | `SourceViewTest.*`, `BreakpointsWindowTest.*`, `BreakConditionTest.*` | **PASS** |
| 21 | Run / Step Over / Trace Into / Auto Trace / Pause / Rewind / Reload controls | `SIM68Ku.cpp` | `SimSessionTest.*`, `MainWindowTest.*`, `ParityValidationTest.*` | **PASS** |
| 22 | Run-to-cursor (`Ctrl+F9`) | `SIM68Ku.cpp` `RunToCursorExecute` | `StepOverTest.*`, `SimSessionTest.*`, `MainWindowTest.*` | **PASS** |
| 23 | `*[sim68k]` listing commands (`break`, `bitfield`, `simhalt_off`) | `SIM68Ku.cpp` | `SimSessionTest.ListingDirectivesBreakBitfieldAndSimhaltOff` | **PASS** |
| 24 | Printing (character output, form feed) | `CODE9.CPP` task 10 | `QtDevicesTest.PrintLayoutAndPageBreaks` | **PASS** |
| 25 | File I/O (open, read, write, seek, close, delete, dialog) | `CODE9.CPP` tasks 50-59 | `StdFileIoTest.*`, `QtIoBackendsTest.*` | **PASS** |
| 26 | Peripheral I/O (mouse/keyboard IRQ enable, state read) | `CODE9.CPP` tasks 60-62 | `QtDevicesTest.*`, `SimIoScreenTest.KeyStateAndHook` | **PASS** |
| 27 | Full-screen output window (`Alt+Enter`, `/f`, task 33, `FullscreenOptionsDialog`) | `CODE9.CPP` task 33, `simIOu.cpp` | `IoWindowTest.FullScreenScalesTheCanvas`, `FullscreenOptionsTest.*`, `IoWindowMainTest.*` | **PASS** |
| 28 | Drag-and-drop file open | `SIM68Ku.cpp` `WmDropFiles` | `MainWindowTest.DragAndDropAndCommandLine`, `EasyBinTest.DropFileSRecordAndBinaryFallback` | **PASS** |
| 29 | Settings persistence (`QSettings` INI for window positions, fonts, options) | `SIM68Ku.cpp` `SaveSettings`, `Edit68K` `settings.dat` | `MainWindowTest.SettingsPersistenceRoundTrip`, `EditorTest.SettingsRoundTripPersistsPreferences` | **PASS** |
| 30 | Cycle counter with Clear button | `SIM68Ku.cpp` | `CycleTest.*`, `RegisterWidgetTest.*` | **PASS** |

---

## 3. Cross-Platform Manual Smoke Checklist (Sim68K, Edit68K, EASyBIN, Help)

1. **Edit68K (`easy68k-editor`, E1–E14, A7, A8):**
   - Launch `easy68k-editor`; verify `UNTITLED1.X68` opens from `data/template.NEW` with syntax highlighting (`Color 1`).
   - Open `Options > Assembler Options...` and `Options > Editor Options...`; toggle presets (`Color 1`, `Color 2`, `Mono`, `Disabled`) and tab modes (`Assembly` vs `Fixed`).
   - Save as `test.X68` and press `F9` (`Project > Assemble Source...`); test `Load L68` (opens read-only `.L68` MDI child) and `Execute` (launches `easy68k-simulator test.S68`).
   - Press `F1` on an instruction (`MOVE`); verify `HelpViewer` opens `Reference/68ks4d.htm`.
2. **Sim68K (`easy68k-simulator`, G1–G24):**
   - Step (`F8`), Trace (`F7`), Auto Trace (`F10`), Run (`F9`), Pause (`F6`), Rewind (`Ctrl+F2`), Reload (`Ctrl+F3`).
   - Open `View > Output Window`, `View > Memory`, `View > Stack`, `View > Hardware`, `View > Break Points`, and cycle windows with `Ctrl+Tab`.
   - Start/stop logging (`Ctrl+F4` / `Ctrl+F5`) and verify `<prog>_RunLog.txt` and `<prog>_OutLog.txt`.
3. **EASyBIN (`easybin`, B1–B4):**
   - Launch from `View > EASyBIN` in Sim68K or standalone `easybin`; open `.S68`, inspect 1/2/4-way split color coding, edit hex/ASCII bytes, and save split binary / S-record output.
4. **Help (`HelpViewer`, H1):**
   - Browse `Contents` tree and filter `Index` list; click `Quick Reference (PDF)`.
