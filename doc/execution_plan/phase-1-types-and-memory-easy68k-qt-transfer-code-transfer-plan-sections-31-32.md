## Phase 1: Types and Memory — EASy68K-qt Transfer (Code Transfer Plan Sections 3.1, 3.2)

### Task 1.1 — Transfer and Adapt `types.h`

Source: `EASy68K-qt/src/core/simulator/types.h` → Target: `include/easym68k/sim/types.h`

Steps:

1. Copy the file to the new location
2. Change include guard to `EASY68K_SIM_TYPES_H_`
3. Change namespace from `sim68k` to `easym68k::sim`
4. Rename `ExecResult` to `SimResult`:
   ```cpp
   enum class SimResult : uint8_t {
     kOk = 0,
     kBadInstruction,
     kPrivilegeViolation,
     kAddressError,
     kBusError,
     kDivideByZero,
     kChkException,
     kTrapVException,
     kTraceException,
     kLine1010,
     kLine1111,
     kStopInstruction,
     kTrapException,
     kHalted
   };
   ```
5. Remove `kBreakpoint` from the enum (breakpoints are a GUI concern)
6. Change `kAddrRegCount = 9` to `kAddrRegCount = 8` and add `constexpr int kSspIndex = 8;`
7. Move addressing mode constants (`kAddrModeDn` through `kAddrModeAll`) out — they go in `addressing_mode.h`
8. Run `clang-format`

**Quality Gate 1.1:** File compiles in isolation. All enum values are present and correctly named. No addressing mode constants remain in this file.

---

### Task 1.2 — Create `effective_addr.h`

New file: `include/easym68k/sim/effective_addr.h`

This is the core type that replaces the sentinel-address pattern:

```cpp
#ifndef EASY68K_SIM_EFFECTIVE_ADDR_H_
#define EASY68K_SIM_EFFECTIVE_ADDR_H_

#include <cstdint>

namespace easym68k::sim {

enum class EaTarget : uint8_t {
  kDataReg,    // Dn
  kAddrReg,    // An
  kMemory,     // (An), (An)+, -(An), d(An), d(An,Xi), xxx.W, xxx.L, d(PC), d(PC,Xi)
  kImmediate   // #imm
};

struct EffectiveAddr {
  EaTarget target;

  // For kDataReg/kAddrReg: register number (0–7)
  // For kMemory: unused (use .address)
  // For kImmediate: unused (use .address for the immediate value)
  uint8_t index;

  // For kMemory: the computed 24-bit memory address
  // For kImmediate: the immediate value itself
  uint32_t address;

  // The original mode/reg fields from the instruction word,
  // needed by some instructions (e.g., MOVEM needs to know
  // predecrement vs postincrement for register ordering)
  uint8_t mode;
  uint8_t reg;
};

// Convenience constructors
inline EffectiveAddr MakeDataRegEa(int reg) {
  return {EaTarget::kDataReg, static_cast<uint8_t>(reg), 0, 0, static_cast<uint8_t>(reg)};
}

inline EffectiveAddr MakeAddrRegEa(int reg) {
  return {EaTarget::kAddrReg, static_cast<uint8_t>(reg), 0, 1, static_cast<uint8_t>(reg)};
}

inline EffectiveAddr MakeMemoryEa(uint32_t address, uint8_t mode, uint8_t reg) {
  return {EaTarget::kMemory, 0, address, mode, reg};
}

inline EffectiveAddr MakeImmediateEa(uint32_t value) {
  return {EaTarget::kImmediate, 0, value, 7, 4};
}

}  // namespace easym68k::sim

#endif  // EASY68K_SIM_EFFECTIVE_ADDR_H_
```

**Quality Gate 1.2:** File compiles. Unit test `effective_addr_test.cc` verifies all four constructors produce correct `EaTarget` values and fields.

---

### Task 1.3 — Transfer and Adapt Memory Class

Source: `EASy68K-qt/src/core/simulator/memory.h` + `memory.cc`
Target: `include/easym68k/sim/memory.h` + `src/sim/memory.cc`

Steps:

1. Copy both files to new locations
2. Change include guard to `EASY68K_SIM_MEMORY_H_`
3. Change namespace to `easym68k::sim`
4. Replace `#include "types.h"` with `#include "easym68k/sim/types.h"`
5. Replace raw pointer members:
   ```cpp
   // Before:
   uint8_t* data_;
   uint8_t* flags_;
   // After:
   std::vector<uint8_t> data_;
   std::vector<uint8_t> flags_;
   ```
6. Remove the destructor (vector cleans up)
7. Remove the move constructor/assignment (vector handles it) — use `= default`
8. Keep copy deleted (16MB is expensive to copy)
9. Change `ReadByte` signature:
   ```cpp
   // Before: uint8_t ReadByte(uint32_t address) const;
   // After:  MemoryAccessResult ReadByte(uint32_t address) const;
   ```
   Update implementation to check `kMemInvalid` flag and return `MemoryAccessResult`
10. Add `error_message` field to `MemoryAccessResult`:
    ```cpp
    struct MemoryAccessResult {
      bool success;
      uint32_t value;
      SimResult error;
      std::string error_message;
    };
    ```
11. Update all `ReadByte` callers in `memory.cc` (none internal — it's the leaf)
12. Update `MaskAddress` to use `data_.size()` bounds check
13. Run `clang-format`

**Quality Gate 1.3:**
- `memory_test.cc` ported from EASy68K-qt and passes (read/write, alignment, endianness, ROM protection)
- `ReadByte` returns `MemoryAccessResult` with proper error on invalid memory
- No raw `new`/`delete` in the file
- `sizeof(Memory)` compiles without warnings
- ASan build passes all memory tests

---

### Task 1.4 — Port Memory Tests

Source: `EASy68K-qt/tests/simulator/memory_test.cc` → Target: `tests/sim/memory_test.cc`

Steps:

1. Copy the file
2. Change namespace to `easym68k::sim`
3. Update `ReadByte` calls to handle `MemoryAccessResult` return type
4. Add new tests:
   - `ReadByteInvalidAddress` — verify `kMemInvalid` returns error
   - `ReadByteBoundary` — read from address 0 and address 0xFFFFFF
   - `WriteThenReadWordEndian` — verify big-endian byte ordering in storage
   - `WriteThenReadLongEndian` — same for long
   - `DoubleAlignmentError` — odd address word+long both fail
   - `ProtectedThenUnprotect` — set then clear protection flags
5. Run tests: `ctest --test-dir build -R memory_test`

**Quality Gate 1.4:** All memory tests pass. No ASan errors.

---
