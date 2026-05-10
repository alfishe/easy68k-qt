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
