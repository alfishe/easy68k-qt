// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Two-level instruction dispatcher.
// Top-level routes by opcode bits 15-12 (group); each DispatchGroupN handles
// sub-decoding within that group.  Phase 5 stubs return kBadInstruction for
// instructions not yet implemented; they are filled in during Phase 6.
// Primary source: EASy68K/Sim68K/RUN.CPP exec_inst(), CODE1.CPP–CODE8.CPP.

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {

// =============================================================================
// Top-level two-level dispatcher
// =============================================================================

SimResult M68kSimulator::ExecuteInstruction(uint16_t opcode) {
  switch ((opcode >> 12) & 0xF) {
    case 0x0:
      return DispatchGroup0(opcode);
    case 0x1:
      return OpMove(opcode);
    case 0x2:
      return DispatchMoveOrMovea(opcode);
    case 0x3:
      return DispatchMoveOrMovea(opcode);
    case 0x4:
      return DispatchGroup4(opcode);
    case 0x5:
      return DispatchGroup5(opcode);
    case 0x6:
      return DispatchGroup6(opcode);
    case 0x7:
      return OpMoveq(opcode);
    case 0x8:
      return DispatchGroup8(opcode);
    case 0x9:
      return DispatchGroup9(opcode);
    case 0xA:
      return OpLine1010(opcode);
    case 0xB:
      return DispatchGroupB(opcode);
    case 0xC:
      return DispatchGroupC(opcode);
    case 0xD:
      return DispatchGroupD(opcode);
    case 0xE:
      return DispatchGroupE(opcode);
    case 0xF:
      return DispatchSimhalt(opcode);
    default:
      return SimResult::kBadInstruction;
  }
}

// =============================================================================
// Group 0x0 — ORI/ANDI/SUBI/ADDI/EORI/CMPI/BTST/BCHG/BCLR/BSET/MOVEP
// =============================================================================

SimResult M68kSimulator::DispatchGroup0(uint16_t /*opcode*/) {
  return SimResult::kBadInstruction;
}

// =============================================================================
// Groups 0x1/0x2/0x3 — MOVE and MOVEA
// =============================================================================

SimResult M68kSimulator::OpMove(uint16_t /*opcode*/) {
  return SimResult::kBadInstruction;
}

SimResult M68kSimulator::DispatchMoveOrMovea(uint16_t /*opcode*/) {
  return SimResult::kBadInstruction;
}

// =============================================================================
// Group 0x4 — Miscellaneous (CLR/NEG/NOT/TST/EXT/SWAP/PEA/MOVEM/JSR/JMP/LEA
//                              NBCD/TAS/TRAP/LINK/UNLK/MOVE_USP/NOP/STOP/…)
// =============================================================================

SimResult M68kSimulator::DispatchGroup4(uint16_t opcode) {
  if (opcode == 0x4E71)
    return SimResult::kOk;  // NOP
  if (opcode == 0x4E72) {   // STOP #imm (privileged)
    if (!state_.IsSupervisor())
      return SimResult::kPrivilegeViolation;
    return SimResult::kStopInstruction;
  }
  return SimResult::kBadInstruction;
}

// =============================================================================
// Group 0x5 — ADDQ/SUBQ/Scc/DBcc
// =============================================================================

SimResult M68kSimulator::DispatchGroup5(uint16_t /*opcode*/) {
  return SimResult::kBadInstruction;
}

// =============================================================================
// Group 0x6 — BRA/BSR/Bcc
// =============================================================================

SimResult M68kSimulator::DispatchGroup6(uint16_t /*opcode*/) {
  return SimResult::kBadInstruction;
}

// =============================================================================
// Group 0x7 — MOVEQ
// =============================================================================

SimResult M68kSimulator::OpMoveq(uint16_t /*opcode*/) {
  return SimResult::kBadInstruction;
}

// =============================================================================
// Group 0x8 — OR/DIVU/DIVS/SBCD
// =============================================================================

SimResult M68kSimulator::DispatchGroup8(uint16_t /*opcode*/) {
  return SimResult::kBadInstruction;
}

// =============================================================================
// Group 0x9 — SUB/SUBA/SUBX
// =============================================================================

SimResult M68kSimulator::DispatchGroup9(uint16_t /*opcode*/) {
  return SimResult::kBadInstruction;
}

// =============================================================================
// Group 0xA — Line 1010 (always trap)
// =============================================================================

SimResult M68kSimulator::OpLine1010(uint16_t /*opcode*/) {
  return SimResult::kLine1010;
}

// =============================================================================
// Group 0xB — CMP/CMPA/CMPM/EOR
// =============================================================================

SimResult M68kSimulator::DispatchGroupB(uint16_t /*opcode*/) {
  return SimResult::kBadInstruction;
}

// =============================================================================
// Group 0xC — AND/MULU/MULS/ABCD/EXG
// =============================================================================

SimResult M68kSimulator::DispatchGroupC(uint16_t /*opcode*/) {
  return SimResult::kBadInstruction;
}

// =============================================================================
// Group 0xD — ADD/ADDA/ADDX
// =============================================================================

SimResult M68kSimulator::DispatchGroupD(uint16_t /*opcode*/) {
  return SimResult::kBadInstruction;
}

// =============================================================================
// Group 0xE — ASd/LSd/ROXd/ROd
// =============================================================================

SimResult M68kSimulator::DispatchGroupE(uint16_t /*opcode*/) {
  return SimResult::kBadInstruction;
}

// =============================================================================
// Group 0xF — SIMHALT or Line 1111 trap
// SIMHALT (0xFFFF) is a simulator-specific halt opcode, not a 68000 standard.
// Any other 0xFxxx opcode triggers the Line 1111 exception.
// =============================================================================

SimResult M68kSimulator::DispatchSimhalt(uint16_t opcode) {
  if (opcode == 0xFFFF) {
    uint16_t next_word = FetchWord();
    if (next_word == 0xFFFF) {
      return SimResult::kHalted;
    }
    // Rewind PC if it wasn't a valid SIMHALT double-word
    state_.pc = (state_.pc - 2) & kAddressMask;
  }
  return SimResult::kLine1111;
}

}  // namespace easym68k::sim
