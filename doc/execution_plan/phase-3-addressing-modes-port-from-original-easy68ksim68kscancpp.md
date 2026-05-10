## Phase 3: Addressing Modes — Port from Original (EASy68K/Sim68K/SCAN.CPP)

### Task 3.1 — Create `addressing_mode.h`

New file: `include/easym68k/sim/addressing_mode.h`

Contains:
- Addressing mode constants (moved from `types.h`)
- `EffectiveAddr CalculateEA(Memory& mem, uint32_t& pc, int mode, int reg, DataSize size)`
- Declarations for read/write helper functions that operate on `EffectiveAddr`

```cpp
#ifndef EASY68K_SIM_ADDRESSING_MODE_H_
#define EASY68K_SIM_ADDRESSING_MODE_H_

#include "easym68k/sim/effective_addr.h"
#include "easym68k/sim/types.h"

namespace easym68k::sim {

// Calculate effective address from mode/reg fields.
// May advance PC (for extension words: displacement, index, absolute address, immediate).
// Returns EffectiveAddr with appropriate EaTarget.
// Does NOT read the value — that's a separate operation.
EffectiveAddr CalculateEA(Memory& memory, uint32_t& pc, int mode, int reg, DataSize size);

// Read a value from the given EA.
uint32_t ReadEA(Memory& memory, const CpuState& state, const EffectiveAddr& ea, DataSize size);

// Write a value to the given EA.
SimResult WriteEA(Memory& memory, CpuState& state, const EffectiveAddr& ea, uint32_t value, DataSize size);

}  // namespace easym68k::sim

#endif
```

**Quality Gate 3.1:** Header compiles. No sentinel addresses. Clean separation: CalculateEA produces address, ReadEA/WriteEA perform data movement.

---

### Task 3.2 — Implement `addressing_mode.cc`

New file: `src/sim/addressing_mode.cc`

Primary source: `EASy68K/Sim68K/SCAN.CPP` — the original `eff_addr()` function that computes effective addresses for all 12 modes.
Secondary reference: `EASy68K-qt/src/core/simulator/addressing_modes.cc` — for the mode/register switch structure only. Do NOT copy sentinel address patterns.

Steps:

1. Implement `CalculateEA` — switch on mode 0–7:
   - Mode 0 (Dn): return `MakeDataRegEa(reg)`
   - Mode 1 (An): return `MakeAddrRegEa(reg)` — reject byte size
   - Mode 2 ((An)): return `MakeMemoryEa(state.a[reg], mode, reg)`
   - Mode 3 ((An)+): compute EA from An, then advance An by size increment (SP alignment rule for byte), return `MakeMemoryEa`
   - Mode 4 (-(An)): decrement An by size, then return `MakeMemoryEa`
   - Mode 5 (d(An)): fetch displacement word from PC, compute address, return `MakeMemoryEa`
   - Mode 6 (d(An,Xi)): fetch extension word from PC, decode index register + size + displacement, compute address, return `MakeMemoryEa`
   - Mode 7 sub-cases (7.0 absolute.W, 7.1 absolute.L, 7.2 d(PC), 7.3 d(PC,Xi), 7.4 #imm): return appropriate `MakeMemoryEa` or `MakeImmediateEa`
2. Implement `ReadEA` — switch on `ea.target`:
   - `kDataReg`: read from `state.d[ea.index]` with size masking
   - `kAddrReg`: read from `state.a[ea.index]` with size masking (word = sign-extend)
   - `kMemory`: read from `memory.Read(ea.address, size)`
   - `kImmediate`: return `ea.address` (already the value)
3. Implement `WriteEA` — switch on `ea.target`:
   - `kDataReg`: write to `state.d[ea.index]` preserving upper bits for byte/word
   - `kAddrReg`: write to `state.a[ea.index]` (full 32-bit for long, sign-extend for word)
   - `kMemory`: write via `memory.Write(ea.address, value, size)`
   - `kImmediate`: return `SimResult::kBadInstruction` (can't write to immediate)

**Critical difference from EASy68K-qt:** No sentinel addresses. No `0xFF000000` constants. Every branch of every switch uses `EffectiveAddr.target` as the discriminator.

**Quality Gate 3.2:**
- `addressing_mode_test.cc` passes for all 12 addressing modes
- Every `EffectiveAddr` returned has the correct `EaTarget`
- No sentinel address constants anywhere in the file
- ASan build passes

---

### Task 3.3 — Addressing Mode Tests

New file: `tests/sim/addressing_mode_test.cc`

Test cases for every addressing mode:

1. **Dn** (mode 0): EA target is kDataReg, index matches reg
2. **An** (mode 1): EA target is kAddrReg; byte size returns error
3. **(An)** (mode 2): EA target is kMemory, address matches An value
4. **(An)+** (mode 3): EA address is pre-increment An; An advanced by 1/2/4; SP byte increments by 2
5. **-(An)** (mode 4): An decremented first; EA address is post-decrement An
6. **d(An)** (mode 5): Extension word fetched from PC; EA address = An + d
7. **d(An,Xi)** (mode 6): Extension word decoded; index register value read; EA address = An + d + Xi
8. **xxx.W** (mode 7.0): 16-bit address sign-extended; PC advanced by 2
9. **xxx.L** (mode 7.1): 32-bit address; PC advanced by 4
10. **d(PC)** (mode 7.2): PC-relative displacement; EA address = PC + d (where PC points past extension word)
11. **d(PC,Xi)** (mode 7.3): PC-relative indexed
12. **#imm** (mode 7.4): EA target is kImmediate; value is the fetched immediate; PC advanced by 2 or 4

Plus ReadEA/WriteEA tests:
13. ReadEA from data register (byte/word/long masking)
14. ReadEA from address register (word sign-extends)
15. ReadEA from memory (big-endian read)
16. ReadEA from immediate (returns stored value)
17. WriteEA to data register (preserves upper bits for byte/word)
18. WriteEA to address register (word sign-extends, long stores full)
19. WriteEA to memory (big-endian write)
20. WriteEA to immediate returns error

**Quality Gate 3.3:** All 20 addressing mode tests pass.

---
