# Track S — Systems & Devices

Trap #15 I/O, graphics, sound, the hardware window, exceptions, interrupts, supervisor mode and memory protection.

Curriculum and rationale: [`EDUCATION_STRATEGY.md`](../../doc/examples/EDUCATION_STRATEGY.md).

Delivery: E4.

| ID | Example | Concepts | Status |
|----|---------|----------|--------|
| S-01 | text-io-advanced | Cursor, colours, keyboard state, input modes (tasks 11, 12, 16, 19, 21) | done |
| S-02 | file-io | Open/read/write/seek/close, file dialogs (tasks 50–59) | done |
| S-03 | graphics-basics | Pixels, lines, rectangles, ellipses, fill, pen (tasks 80–96) | done |
| S-04 | double-buffering | Animation without flicker (task 92 modes, task 94 repaint) | done |
| S-05 | sound | Play/stop and multi-voice (tasks 70–77) | done |
| S-06 | timing | Task 8 time, task 23 delay, frame pacing | done |
| S-07 | hardware-window | 7-segment display, LEDs, switches, push buttons (task 32) | done |
| S-08 | exceptions | Vector table, TRAP #n handlers, CHK, TRAPV, divide by zero, illegal | done |
| S-09 | address-bus-errors | Odd addresses, class 0 frames, recovery | done |
| S-10 | interrupts | Autovectors, IRQ buttons and auto-IRQ, interrupt masks, STOP | done |
| S-11 | supervisor-user | Privilege, USP/SSP, RTE, privilege violation | done |
| S-12 | memory-map | ROM/Read/Protected/Invalid ranges, MEMORY directive | done |
| S-13 | mouse-keyboard-irq | Peripheral IRQs (tasks 60–62) | done |
| S-14 | serial | COM ports (tasks 40–43), loopback | done |
| S-15 | network | TCP/UDP client/server (tasks 100–107) | done |
| S-16 | logging-debugging | Execution log, advanced breakpoints, run-to-cursor | done |

Directory names follow `<ID>-<name>`, e.g. `L2-06-stack-frames`.
