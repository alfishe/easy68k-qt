# Future Backlog — Post-Phase-13 Enhancements

Features not present in the original EASy68K that could be added as extensions
after the port achieves full behavioral parity, plus deployment targets that
build on top of the headless core library.

Core parity gaps (interrupt injection, cycle counting, SIMHALT config, execution
logging, observer system) are now covered in **Phase 9** of the execution plan.

These items are **out of scope** for the current execution plan. They are
documented here so they are not forgotten and can be prioritized later.

---

## 1. Negated Register Lists for MOVEM

**Affected instructions:** MOVEM, REG directive

**Description:**

Allow register lists to be specified with negation syntax, excluding registers
from the full set. This is a convenience extension not present in the original
EASy68K MOVEM.CPP.

**Proposed syntax:**

```asm
    MOVEM.L D0-D7/A0-A7-D3-A1,-(SP)   ; all except D3 and A1
```

or with explicit negation:

```asm
    MOVEM.L /(-D3/A1),-(SP)             ; same meaning
```

**Implementation notes:**

- Start with a 16-bit mask of all registers (0xFFFF for D0-D7/A0-A7)
- Remove bits corresponding to the negated register sub-list
- Requires extending `evalList()` in the register list parser
- Must interact correctly with pre-decrement bit reversal (negation applies
  before reversal)
- Original EASy68K only supports positive ranges and individual registers

**Estimated effort:** Small (parser extension + ~10 tests)

---

## 2. 68010+ Instruction Support

**Affected instructions:** MOVES, MOVEC, RTD

**Description:**

The original EASy68K source files (INSTTABL.CPP, BUILD.CPP) contain commented-out
handlers for 68010+ instructions. These could be enabled and ported.

**Implementation notes:**

- MOVES (BUILD.CPP:405-434) — Move to/from address space
- MOVEC (BUILD.CPP:492-527) — Move control register
- RTD — Return and Deallocate (already handled via `immedWord` handler)
- Would require instruction table entries and encoding handlers
- Consider gating behind an assembler option (e.g., `OPT 68010`)

**Estimated effort:** Medium (3 handlers + table entries + ~20 tests)

---

## 3. 68020 Bit-Field Instructions

**Affected instructions:** BFCHG, BFCLR, BFEXTS, BFEXTU, BFFFO, BFINS, BFSET, BFTST

**Description:**

The original BUILD.CPP (lines 253-284) contains a `bitField` handler guarded by
a config flag. The `{offset:width}` parsing exists in ASSEMBLE.CPP
(`fieldParse()`, lines 716-789). The instruction table entries exist but are
commented out.

**Implementation notes:**

- Requires `fieldParse()` port from ASSEMBLE.CPP
- Extension word encoding is complex (dynamic register/offset fields)
- Consider gating behind `OPT BIT` (matching original's flag name)
- Original implementation was present but likely undertested

**Estimated effort:** Medium-Large (1 handler + field parser + ~25 tests)

---

## 4. Binary File Output (BINFILE.CPP)

**Description:**

The original source tree contains BINFILE.CPP (209 lines) implementing Teeside-
compatible binary output. However, **this file was never compiled** — it is absent
from the project build file (EDIT68K.bpr), `initBin()` is never called, and
`outputBin()` is only mentioned in a CODEGEN.CPP comment, never invoked.

This is dead code that could be revived if Teeside binary output is needed.

**Implementation notes:**

- Source: `EASy68K/Edit68K/BINFILE.CPP` (209 lines)
- Functions: `initBin()`, `outputBin()`, `finishBin()`, `writeBigEndian()`
- Multi-section support with address tracking
- Odd-address and even-file-size padding
- Not required for EASy68K simulator compatibility — S-record output is sufficient

**Estimated effort:** Small-Medium (209 lines + ~15 tests)

---

## 5. Additional Assembler Quality-of-Life Improvements

Potential enhancements that go beyond the original EASy68K:

- **Better error messages** — include source context, caret positioning, and
  suggestion-based hints (original only provides error code + line number)
- **Structured error output** — machine-readable error format (JSON) for IDE
  integration
- **Relaxation pass** — automatically widen branch displacements when short
  branches are out of range (original requires manual `.W` hint)
- **Expression type checking** — warn when absolute values are used where
  PC-relative is expected, or vice versa
- **Multiple output formats** — ELF, raw binary with configurable base address,
  Intel HEX in addition to Motorola S-record

---

## 6. Headless Simulator C API (libeasym68k-sim as standalone library)

**Category:** Future extensibility — API wrapper

**Description:**

After the simulator core is complete and parity-tested, package it as a clean
C API or C++ shared library that can be consumed by any client without Qt
dependencies.

**Proposed interface:**

```cpp
// C API for maximum language compatibility
extern "C" {
  typedef void* Easy68kSim;

  Easy68kSim* easy68k_sim_create(void);
  void easy68k_sim_destroy(Easy68kSim* sim);

  // Execution
  int easy68k_sim_reset(Easy68kSim* sim);
  int easy68k_sim_step(Easy68kSim* sim);
  int easy68k_sim_run(Easy68kSim* sim, int max_instructions);
  void easy68k_sim_request_stop(Easy68kSim* sim);

  // State access
  uint32_t easy68k_get_register(Easy68kSim* sim, int reg);
  void easy68k_set_register(Easy68kSim* sim, int reg, uint32_t value);
  uint64_t easy68k_get_cycles(Easy68kSim* sim);

  // Memory access
  uint8_t easy68k_mem_read_byte(Easy68kSim* sim, uint32_t addr);
  uint16_t easy68k_mem_read_word(Easy68kSim* sim, uint32_t addr);
  uint32_t easy68k_mem_read_long(Easy68kSim* sim, uint32_t addr);
  void easy68k_mem_write_byte(Easy68kSim* sim, uint32_t addr, uint8_t val);
  void easy68k_mem_write_word(Easy68kSim* sim, uint32_t addr, uint16_t val);
  void easy68k_mem_write_long(Easy68kSim* sim, uint32_t addr, uint32_t val);

  // Loading
  int easy68k_load_srecord_file(Easy68kSim* sim, const char* path);
  int easy68k_load_srecord_data(Easy68kSim* sim, const char* data, int len);

  // Breakpoints
  void easy68k_add_breakpoint(Easy68kSim* sim, uint32_t addr);
  void easy68k_remove_breakpoint(Easy68kSim* sim, uint32_t addr);
  void easy68k_clear_breakpoints(Easy68kSim* sim);

  // Interrupts
  void easy68k_interrupt(Easy68kSim* sim, int level);

  // I/O callbacks (Trap #15 backends)
  void easy68k_set_text_io(Easy68kSim* sim, void* text_io);
  void easy68k_set_file_io(Easy68kSim* sim, void* file_io);
}
```

**Key design requirements:**

- Zero Qt dependency in the library (pure C++ standard library)
- Stable ABI via extern "C" wrapper
- All I/O via injected interfaces (already done — Trap #15 design)
- Thread-safe: the C API must be callable from any thread
- Opaque handle pattern to hide C++ implementation details

**Estimated effort:** Medium-Large (C wrapper layer + CMake shared library target + docs)

---

## 7. Console/CLI Simulator Client

**Category:** Future extensibility — console client

**Description:**

A command-line simulator client that can load and execute M68K programs without
any GUI. This proves the headless API works and is immediately useful for
automated testing, CI, and scripting.

**Proposed features:**

```
$ easy68k-sim --help
Usage: easy68k-sim [options] <file.s68>

Options:
  --run              Run until halt (default)
  --step <N>         Execute N steps
  --trace            Enable trace mode (print state after each step)
  --break <addr>     Set breakpoint at hex address
  --regs             Print register state after execution
  --mem <addr> <N>   Dump N bytes of memory at hex address
  --load-addr <hex>  Override start address
  --timeout <ms>     Maximum execution time in milliseconds
  --trap15-log       Log all Trap #15 calls to stderr
  --expect-exit <N>  Assert program halts via Trap #15 task 9 (exit code N)
```

**Example usage:**

```bash
# Run a program and dump registers
$ easy68k-sim --run --regs program.s68

# Trace first 100 instructions
$ easy68k-sim --trace --step 100 program.s68

# Automated test: expect exit code 0
$ easy68k-sim --expect-exit 0 test_program.s68 && echo PASS || echo FAIL
```

**Implementation:**

- Links against `libeasym68k-sim` (no Qt dependency)
- Provides concrete `ITextIO` that reads from stdin and writes to stdout
- Provides `IFileIO` using standard C++ file I/O
- Provides `ILogger` that writes to stderr
- Other interfaces (graphics, sound, network, serial) are no-ops
- Exit code reflects Trap #15 task 9 result or error

**Estimated effort:** Medium (CLI parsing + concrete I/O backends + ~20 integration tests)

---

## 8. REST/gRPC Simulator API

**Category:** Future extensibility — remote API

**Description:**

A network-accessible API that allows remote clients to control the simulator.
This enables web-based UIs, remote debugging, and multi-user educational
environments.

**Proposed REST API:**

```
POST /sim/create          → Create simulator instance → { sim_id }
POST /sim/{id}/reset      → Reset simulator
POST /sim/{id}/step       → Execute one step → { result, pc, sr }
POST /sim/{id}/run        → Run until halt/breakpoint → { result }
POST /sim/{id}/stop       → Request stop
GET  /sim/{id}/state      → Get full register + SR state
GET  /sim/{id}/mem/{addr} → Read memory
PUT  /sim/{id}/mem/{addr} → Write memory
POST /sim/{id}/load       → Load S-Record data
POST /sim/{id}/bp         → Add/remove breakpoint
POST /sim/{id}/irq/{lvl}  → Inject interrupt
DELETE /sim/{id}          → Destroy simulator
```

**Key design requirements:**

- Built on top of the C API from item 6 (no Qt dependency)
- WebSocket support for real-time state updates on Step/Trace
- Session management (multiple concurrent simulator instances)
- Optional authentication for multi-user deployments
- Docker-friendly deployment (single binary + config)

**Estimated effort:** Large (HTTP framework + WebSocket + session management + docs)

---

## 9. WebAssembly Simulator

**Category:** Future extensibility — browser-based

**Description:**

Compile the simulator core to WebAssembly (Wasm) for in-browser execution.
This enables web-based M68K education without server-side simulation.

**Key components:**

- `libeasym68k-sim` compiled to `.wasm` via Emscripten
- JavaScript shim providing Trap #15 I/O backends:
  - `ITextIO` → DOM terminal emulator (xterm.js)
  - `IGraphicsIO` → HTML5 Canvas
  - `ISoundIO` → Web Audio API
  - `IFileIO` → browser File API / IndexedDB
- JavaScript API wrapping the Wasm module for web framework integration

**Feasibility:** High. The simulator core has no OS-specific dependencies.
All I/O is abstracted via interfaces. The only challenge is the 16MB memory
model (address space), which fits easily in browser memory.

**Estimated effort:** Large (Emscripten build config + JS shim layer + demo page)
