// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Addressing mode constants and effective address calculation.

#ifndef EASY68K_SIM_ADDRESSING_MODE_H_
#define EASY68K_SIM_ADDRESSING_MODE_H_

#include <cstdint>

#include "easym68k/sim/cpu_state.h"
#include "easym68k/sim/effective_addr.h"
#include "easym68k/sim/memory.h"
#include "easym68k/sim/types.h"

namespace easym68k::sim {

// =============================================================================
// Addressing Mode Masks (for instruction validation and decoding)
// =============================================================================

constexpr int kAddrModeDn = 0x0001;       // Dn
constexpr int kAddrModeAn = 0x0002;       // An
constexpr int kAddrModeAnInd = 0x0004;    // (An)
constexpr int kAddrModeAnPost = 0x0008;   // (An)+
constexpr int kAddrModeAnPre = 0x0010;    // -(An)
constexpr int kAddrModeAnDisp = 0x0020;   // d(An)
constexpr int kAddrModeAnIndex = 0x0040;  // d(An,Xi)
constexpr int kAddrModeAbsW = 0x0080;     // xxx.W
constexpr int kAddrModeAbsL = 0x0100;     // xxx.L
constexpr int kAddrModePcDisp = 0x0200;   // d(PC)
constexpr int kAddrModePcIndex = 0x0400;  // d(PC,Xi)
constexpr int kAddrModeImm = 0x0800;      // #xxx

// Combined addressing mode masks
constexpr int kAddrModeData = 0x0FFD;
constexpr int kAddrModeMemory = 0x0FFC;
constexpr int kAddrModeControl = 0x07E4;
constexpr int kAddrModeAlter = 0x01FF;
constexpr int kAddrModeAll = 0x0FFF;
constexpr int kAddrModeDataAlt = kAddrModeData & kAddrModeAlter;
constexpr int kAddrModeMemAlt = kAddrModeMemory & kAddrModeAlter;
constexpr int kAddrModeCtrlAlt = kAddrModeControl & kAddrModeAlter;

// Instruction word field extraction masks
constexpr int kModeMask = 0x0038;
constexpr int kRegMask = 0x0007;

// =============================================================================
// Effective Address API
// =============================================================================

// Calculate effective address from mode/reg fields.
// May advance pc (extension words: displacement, index, absolute, immediate).
// For modes 3/(An)+ and 4/-(An), advances or decrements the address register in state.
// Does NOT read or write the value at the EA — that is ReadEA / WriteEA.
EffectiveAddr CalculateEA(Memory& memory, CpuState& state, uint32_t& pc, int mode, int reg,
                          DataSize size);

// Read a value from ea according to size.
// For kDataReg/kAddrReg: reads from state registers.
// For kMemory: reads from memory (big-endian).
// For kImmediate: returns ea.address (the pre-fetched immediate value).
uint32_t ReadEA(const Memory& memory, const CpuState& state, const EffectiveAddr& ea,
                DataSize size);

// Write value to ea according to size.
// For kDataReg: preserves upper bits for byte/word operations.
// For kAddrReg: word writes sign-extend to 32 bits.
// For kMemory: writes to memory (big-endian).
// For kImmediate: returns SimResult::kBadInstruction.
SimResult WriteEA(Memory& memory, CpuState& state, const EffectiveAddr& ea, uint32_t value,
                  DataSize size);

}  // namespace easym68k::sim

#endif  // EASY68K_SIM_ADDRESSING_MODE_H_
