// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Condition code update free functions.
// Ported from EASy68K/Sim68K/UTILS.CPP: cc_update().

#ifndef EASY68K_SIM_FLAG_COMPUTATION_H_
#define EASY68K_SIM_FLAG_COMPUTATION_H_

#include <cstdint>

#include "easym68k/sim/cpu_state.h"
#include "easym68k/sim/types.h"

namespace easym68k::sim {

// ADD / ADDI / ADDQ / ADDX — updates N, Z, V, C, X
// src and dst are the pre-addition operands; result is their sum, all masked to size.
void UpdateFlagsAdd(CpuState& state, uint32_t src, uint32_t dst, uint32_t result, DataSize size);

// SUB / SUBI / SUBQ / SUBX — updates N, Z, V, C, X
// Semantics: result = dst - src.
void UpdateFlagsSub(CpuState& state, uint32_t src, uint32_t dst, uint32_t result, DataSize size);

// AND / OR / EOR / NOT — clears V and C, updates N and Z; X unchanged.
void UpdateFlagsLogic(CpuState& state, uint32_t result, DataSize size);

// MOVE / MOVEQ — clears V and C, updates N and Z; X unchanged.
void UpdateFlagsMove(CpuState& state, uint32_t result, DataSize size);

// Shift/rotate — updates N, Z, V, C; X = C when count != 0.
// carry is the bit shifted out; count is the shift amount.
void UpdateFlagsShift(CpuState& state, uint32_t result, DataSize size, bool carry, int count);

// NEG — V=(Dm&&Rm), C=(result!=0); updates N, Z, V, C, X.
// dst is the pre-negation operand; result = 0 - dst, masked to size.
void UpdateFlagsNeg(CpuState& state, uint32_t dst, uint32_t result, DataSize size);

// CMP / CMPA / CMPI / CMPM — like SUB but X is not affected.
void UpdateFlagsCmp(CpuState& state, uint32_t src, uint32_t dst, uint32_t result, DataSize size);

// ADDX — like ADD but Z is cleared only if result != 0 (preserved if already 0).
void UpdateFlagsAddX(CpuState& state, uint32_t src, uint32_t dst, uint32_t result, DataSize size);

// SUBX — like SUB but Z is cleared only if result != 0.
void UpdateFlagsSubX(CpuState& state, uint32_t src, uint32_t dst, uint32_t result, DataSize size);

// NEGX — like NEG but Z is cleared only if result != 0.
void UpdateFlagsNegX(CpuState& state, uint32_t dst, uint32_t result, DataSize size);

}  // namespace easym68k::sim

#endif  // EASY68K_SIM_FLAG_COMPUTATION_H_
