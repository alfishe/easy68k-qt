// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Effective address type — replaces the sentinel-pointer EA pattern from EASy68K-qt.

#ifndef EASY68K_SIM_EFFECTIVE_ADDR_H_
#define EASY68K_SIM_EFFECTIVE_ADDR_H_

#include <cstdint>

namespace easym68k::sim {

enum class EaTarget : uint8_t {
  kDataReg,   // Dn
  kAddrReg,   // An
  kMemory,    // (An), (An)+, -(An), d(An), d(An,Xi), xxx.W, xxx.L, d(PC), d(PC,Xi)
  kImmediate  // #imm
};

struct EffectiveAddr {
  EaTarget target;

  // For kDataReg/kAddrReg: register number (0-7).
  // Unused for kMemory and kImmediate.
  uint8_t index;

  // For kMemory: computed 24-bit address.
  // For kImmediate: the immediate value itself.
  uint32_t address;

  // Original mode/reg fields from the instruction word; needed by some
  // instructions (e.g. MOVEM distinguishes predecrement vs postincrement).
  uint8_t mode;
  uint8_t reg;
};

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
