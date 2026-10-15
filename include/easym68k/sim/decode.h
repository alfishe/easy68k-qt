// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef EASYM68K_SIM_DECODE_H_
#define EASYM68K_SIM_DECODE_H_

#include <cstdint>

namespace easym68k::sim {

// True if `opcode` (first instruction word) is a legal 68000 instruction, or
// a 68020 bit field instruction when `bitfield_enabled`. Line A ($Axxx) and
// Line F ($Fxxx) count as legal here; they raise their own exceptions.
// Privilege is checked when the instruction executes. Table lookup.
bool IsLegalOpcode(uint16_t opcode, bool bitfield_enabled);

// The instruction name Sim68K shows for a traced instruction (OPCODES.H
// inst_arr[] names, padded to 12 characters, e.g. "MOVEQ       ").
const char* InstructionName(uint16_t opcode);

}  // namespace easym68k::sim

#endif  // EASYM68K_SIM_DECODE_H_
