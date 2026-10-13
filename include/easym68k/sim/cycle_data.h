// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Instruction cycle timing data.
// Ported from EASy68K/Sim68K/RUN.CPP (eff_addr() bwinc/linc timing),
// CODE1.CPP (move_bw_times/move_l_times/movem tables) and UTILS.CPP
// (eff_addr_code()). Values follow the original simulator, which follows the
// M68000 PRM Section 8 tables.

#ifndef EASY68K_SIM_CYCLE_DATA_H_
#define EASY68K_SIM_CYCLE_DATA_H_

#include <cstdint>

#include "easym68k/sim/effective_addr.h"
#include "easym68k/sim/types.h"

namespace easym68k::sim {

// =============================================================================
// Exception processing cycle counts (RUN.CPP runprog() exception switch)
// =============================================================================

constexpr int kCyclesIllegalInst = 34;   // BAD_INST
constexpr int kCyclesPrivilege = 34;     // NO_PRIVILEGE
constexpr int kCyclesTrace = 34;         // trace exception
constexpr int kCyclesTrap = 38;          // TRAP #0-#14
constexpr int kCyclesDivideByZero = 42;  // DIV_BY_ZERO
constexpr int kCyclesChk = 44;           // CHK_EXCEPTION
constexpr int kCyclesTrapV = 34;         // TRAPV_TRAP
constexpr int kCyclesLineA = 34;         // LINE_1010
constexpr int kCyclesLineF = 34;         // LINE_1111
constexpr int kCyclesIrq = 34;           // irqHandler()

// =============================================================================
// Effective address timing
// =============================================================================

// Effective address calculation time, matching the bwinc/linc values that the
// original eff_addr()/eff_addr_noread() add when called with add_times=true.
//
//   Mode        Byte/Word  Long
//   Dn, An          0        0
//   (An)            4        8
//   (An)+           4        8
//   -(An)           6       10
//   d(An)           8       12
//   d(An,Xi)       10       14
//   xxx.W           8       12
//   xxx.L          12       16
//   d(PC)           8       12
//   d(PC,Xi)       10       14
//   #imm            4        8
int EaCyclePenalty(int mode, int reg, DataSize size);

inline int EaCyclePenalty(const EffectiveAddr& ea, DataSize size) {
  return EaCyclePenalty(ea.mode, ea.reg, size);
}

// Addressing-mode code used to index the MOVE/MOVEM timing tables.
// Port of UTILS.CPP eff_addr_code(): modes 0-6 map to 0-6; mode 7 maps
// reg 0..4 (xxx.W, xxx.L, d(PC), d(PC,Xi), #imm) to 7..11; anything else is 12.
int EaCode(int mode, int reg);

// MOVE execution time (CODE1.CPP move_bw_times / move_l_times).
// src_code: EaCode() of the source (0-11); dst_code: EaCode() of the
// destination (0-8). Returns 0 for out-of-range codes.
int MoveCycles(int src_code, int dst_code, DataSize size);

// MOVEM base execution time (CODE1.CPP movem_t_r_times / mover_t_m_times),
// indexed by EaCode() of the memory operand. Per-register cost is added by
// the caller. Returns 0 for out-of-range codes.
int MovemCycles(int ea_code, bool memory_to_register);

// MULU execution time excluding EA: 38 + 2n, n = number of 1 bits in source
// (CODE4.CPP MULU()).
int MuluCycles(uint16_t source);

// MULS execution time excluding EA: 38 + 2n, n = number of 0->1 / 1->0
// transitions in the 17-bit value <source:0> (CODE4.CPP MULS()).
int MulsCycles(uint16_t source);

// Exact DIVU / DIVS execution time excluding EA, for a register operand.
// Port of getDivu68kCycles() / getDivs68kCycles() (Jorge Cwik, 2005) in
// CODE4.CPP. Return 0 when divisor is zero.
int DivuCycles(uint32_t dividend, uint16_t divisor);
int DivsCycles(int32_t dividend, int16_t divisor);

}  // namespace easym68k::sim

#endif  // EASY68K_SIM_CYCLE_DATA_H_
