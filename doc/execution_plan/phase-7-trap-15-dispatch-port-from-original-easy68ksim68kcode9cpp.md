## Phase 7: Trap #15 Dispatch — Port from Original (EASy68K/Sim68K/CODE9.CPP)

### Task 7.1 — Define Trap #15 Interface Headers

Create all 9 interface headers in `include/easym68k/sim/`:

| Header | Interface | Tasks |
|--------|-----------|-------|
| `itext_io.h` | `ITextIO` | 0-2, 5-6, 11-14, 16-18, 22, 24-25 |
| `ifile_io.h` | `IFileIO` | 50-59 |
| `iserial_io.h` | `ISerialIO` | 40-43 |
| `inetwork_io.h` | `INetworkIO` | 100-107 |
| `igraphics_io.h` | `IGraphicsIO` | 80-96 |
| `isound_io.h` | `ISoundIO` | 70-77 |
| `iperipheral_io.h` | `IPeripheralIO` | 60-62 |
| `isimulator_env.h` | `ISimulatorEnv` | 8, 23, 30-33 |
| `iprint_io.h` | `IPrintIO` | 10 |

Copy interface definitions from proposal Section 6.4. Also create:
- `include/easym68k/sim/ilogger.h` — `ILogger` interface

**Quality Gate 7.1:** All 10 interface headers compile. No implementation dependencies.

---

### Task 7.2 — Implement Trap #15 Dispatch

New file: `src/sim/trap15_dispatch.cc`

Port the task-number dispatch logic from `EASy68K/Sim68K/CODE9.CPP`. This is the original `TRAP()` function containing the complete switch statement mapping task numbers (read from D0.B) to interface method calls:

```cpp
SimResult M68kSimulator::DispatchTrap15() {
  uint8_t task = static_cast<uint8_t>(state_.d[0] & 0xFF);

  switch (task) {
    case 0: config_.text_io->TextOut(ReadStringFromEA(...)); break;
    case 1: config_.text_io->TextOutCRLF(ReadStringFromEA(...)); break;
    case 2: config_.text_io->TextIn(...); break;
    // ... all 50+ cases
    case 70: config_.sound_io->PlaySound(...); break;
    case 80: config_.graphics_io->SetPenColor(...); break;
    // ...
    default: /* unknown task — log warning */ break;
  }
  return SimResult::kOk;
}
```

**Reference `EASy68K/Sim68K/CODE9.CPP` for every task number and its D0/D1/A0/A1 register usage pattern.** Each task reads its parameters from specific registers or memory locations — this mapping must be exact. The original is the single authoritative source for all Trap #15 semantics.

**Quality Gate 7.2:**
- `trap15_mock_test.cc` passes with mock implementations of all 9 interfaces
- Every task number from 0–25, 30–33, 40–43, 50–59, 60–62, 70–77, 80–96, 100–107 dispatches to the correct interface method
- Unknown task numbers log a warning and do not crash
- Register parameter mapping verified per original CODE9.CPP

---

### Task 7.3 — Trap #15 Mock Tests

New file: `tests/sim/trap15_mock_test.cc`

Create mock implementations of all 9 interfaces using gtest `MOCK_METHOD`. For each Trap #15 task:

1. Set up register state with expected parameters
2. Execute `TRAP #15`
3. Verify the mock received the expected call with the expected arguments
4. Verify register result values are written back correctly

Minimum test coverage: one test per task number (~50 tests), plus:
- Multiple parameter combinations for complex tasks (graphics modes, sound control values)
- Edge cases: null interface pointer → task is a no-op (log warning)
- SIMHALT verification within Trap context

**Quality Gate 7.3:** All 50+ Trap #15 mock tests pass.

---
