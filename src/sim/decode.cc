// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Two-level instruction dispatcher.
// Top-level routes by opcode bits 15-12 (group); each DispatchGroupN handles
// sub-decoding within that group.  Phase 5 stubs return kBadInstruction for
// instructions not yet implemented; they are filled in during Phase 6.
// Primary source: EASy68K/Sim68K/RUN.CPP exec_inst(), CODE1.CPP–CODE8.CPP.

#include "easym68k/sim/addressing_mode.h"
#include "easym68k/sim/flag_computation.h"
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

SimResult M68kSimulator::DispatchGroup0(uint16_t opcode) {
  // Dynamic bit operations and MOVEP share bit 8 = 1.
  if (opcode & 0x0100) {
    if ((opcode & 0x0038) == 0x0008)
      return OpMovep(opcode);  // MOVEP: mode=001 (An+disp)
    switch ((opcode >> 6) & 0x3) {
      case 0:
        return OpBtst(opcode);
      case 1:
        return OpBchg(opcode);
      case 2:
        return OpBclr(opcode);
      case 3:
        return OpBset(opcode);
    }
  }
  // Static bit ops: bits 11-9 = 100 (0x0800..0x0BFF)
  if ((opcode & 0x0E00) == 0x0800) {
    switch ((opcode >> 6) & 0x3) {
      case 0:
        return OpBtst(opcode);
      case 1:
        return OpBchg(opcode);
      case 2:
        return OpBclr(opcode);
      case 3:
        return OpBset(opcode);
    }
  }
  // Immediate ALU operations: bits 11-9 select operation
  switch ((opcode >> 9) & 0x7) {
    case 0:
      if ((opcode & 0xFF) == 0x3C)
        return OpOriToCcr(opcode);
      if ((opcode & 0xFF) == 0x7C)
        return OpOriToSr(opcode);
      return OpOri(opcode);
    case 1:
      if ((opcode & 0xFF) == 0x3C)
        return OpAndiToCcr(opcode);
      if ((opcode & 0xFF) == 0x7C)
        return OpAndiToSr(opcode);
      return OpAndi(opcode);
    case 2:
      return OpSubi(opcode);
    case 3:
      return OpAddi(opcode);
    case 5:
      if ((opcode & 0xFF) == 0x3C)
        return OpEoriToCcr(opcode);
      if ((opcode & 0xFF) == 0x7C)
        return OpEoriToSr(opcode);
      return OpEori(opcode);
    case 6:
      return OpCmpi(opcode);
  }
  return SimResult::kBadInstruction;
}

// =============================================================================
// Groups 0x1/0x2/0x3 — MOVE and MOVEA
// MOVE has a non-standard size encoding: 01=byte, 11=word, 10=long.
// Destination fields are reversed: bits 11-9 = dst_reg, bits 8-6 = dst_mode.
// =============================================================================

SimResult M68kSimulator::OpMove(uint16_t opcode) {
  // Group 1 — MOVE.B only; MOVEA.B does not exist on 68000.
  int src_mode = (opcode >> 3) & 0x7;
  int src_reg = opcode & 0x7;
  int dst_mode = (opcode >> 6) & 0x7;
  int dst_reg = (opcode >> 9) & 0x7;

  EffectiveAddr src = CalcEA(src_mode, src_reg, DataSize::kByte);
  uint32_t val = ReadEA(src, DataSize::kByte);
  EffectiveAddr dst = CalcEA(dst_mode, dst_reg, DataSize::kByte);
  SimResult r = WriteEA(dst, val, DataSize::kByte);
  if (r != SimResult::kOk)
    return r;
  UpdateFlagsMove(state_, val, DataSize::kByte);
  return SimResult::kOk;
}

SimResult M68kSimulator::DispatchMoveOrMovea(uint16_t opcode) {
  // Groups 2 (long) and 3 (word).
  int dst_mode = (opcode >> 6) & 0x7;
  if (dst_mode == 1)
    return OpMovea(opcode);

  DataSize size = ((opcode >> 12) & 0xF) == 0x2 ? DataSize::kLong : DataSize::kWord;
  int src_mode = (opcode >> 3) & 0x7;
  int src_reg = opcode & 0x7;
  int dst_reg = (opcode >> 9) & 0x7;

  EffectiveAddr src = CalcEA(src_mode, src_reg, size);
  uint32_t val = ReadEA(src, size);
  EffectiveAddr dst = CalcEA(dst_mode, dst_reg, size);
  SimResult r = WriteEA(dst, val, size);
  if (r != SimResult::kOk)
    return r;
  UpdateFlagsMove(state_, val, size);
  return SimResult::kOk;
}

// =============================================================================
// Group 0x4 — Miscellaneous (CLR/NEG/NOT/TST/EXT/SWAP/PEA/MOVEM/JSR/JMP/LEA
//                              NBCD/TAS/TRAP/LINK/UNLK/MOVE_USP/NOP/STOP/…)
// =============================================================================

SimResult M68kSimulator::DispatchGroup4(uint16_t opcode) {
  // Sub-decode group 4 (0x4xxx) by bits 11-8, then by bits 7-6 (size/subtype).
  // CHK/LEA span all of bits 11-8 via bit 8 = 1; check them first.
  if ((opcode & 0x0100) && (opcode & 0xFF00) < 0x4800) {
    // CHK: bit 8=1, bits 7-6=10; LEA: bit 8=1, bits 7-6=11
    uint16_t sz = opcode & 0x00C0;
    if (sz == 0x0080)
      return OpChk(opcode);
    if (sz == 0x00C0)
      return OpLea(opcode);
    return SimResult::kBadInstruction;
  }

  switch ((opcode >> 8) & 0xF) {
    case 0x0: {
      // NEGX (ss=00/01/10) or MOVE_FR_SR (ss=11)
      uint16_t ss = opcode & 0x00C0;
      return (ss == 0x00C0) ? OpMoveFrSr(opcode) : OpNegx(opcode);
    }
    case 0x2: {
      // CLR (ss=00/01/10); ss=11 is invalid
      return ((opcode & 0x00C0) == 0x00C0) ? SimResult::kBadInstruction : OpClr(opcode);
    }
    case 0x4: {
      // NEG (ss=00/01/10) or MOVE_TO_CCR (ss=11)
      uint16_t ss = opcode & 0x00C0;
      return (ss == 0x00C0) ? OpMoveToCcr(opcode) : OpNeg(opcode);
    }
    case 0x6: {
      // NOT (ss=00/01/10) or MOVE_TO_SR (ss=11)
      uint16_t ss = opcode & 0x00C0;
      return (ss == 0x00C0) ? OpMoveToSr(opcode) : OpNot(opcode);
    }
    case 0x8: {
      // NBCD/SWAP/PEA/EXT.W/MOVEM.W/EXT.L/MOVEM.L
      // Distinguished by bits 7-6 and whether mode==Dn (bits 5-3==000)
      uint16_t b76 = (opcode >> 6) & 0x3;
      int mode = (opcode >> 3) & 0x7;
      if (b76 == 0)
        return OpNbcd(opcode);
      if (b76 == 1)
        return (mode == 0) ? OpSwap(opcode) : OpPea(opcode);
      // b76 == 2: EXT.W (mode==Dn) or MOVEM.W regs→mem
      // b76 == 3: EXT.L (mode==Dn) or MOVEM.L regs→mem
      return (mode == 0) ? OpExt(opcode) : OpMovem(opcode);
    }
    case 0xA: {
      // ILLEGAL (0x4AFC), TAS (bits 7-6=11), TST (bits 7-6=00/01/10)
      if (opcode == 0x4AFC)
        return OpIllegal(opcode);
      return ((opcode & 0x00C0) == 0x00C0) ? OpTas(opcode) : OpTst(opcode);
    }
    case 0xC: {
      // MOVEM mem→regs: bit 7 must be 1 (0x4C80..0x4CFF)
      return ((opcode & 0x0080) == 0x0080) ? OpMovem(opcode) : SimResult::kBadInstruction;
    }
    case 0xE: {
      // TRAP / LINK / UNLK / MOVE_USP / NOP / STOP / RTE / RTS / RTR / TRAPV / JSR / JMP
      if ((opcode & 0xFFF0) == 0x4E40)
        return OpTrap(opcode);
      if ((opcode & 0xFFF8) == 0x4E50)
        return OpLink(opcode);
      if ((opcode & 0xFFF8) == 0x4E58)
        return OpUnlk(opcode);
      if ((opcode & 0xFFF0) == 0x4E60)
        return OpMoveUsp(opcode);
      switch (opcode) {
        case 0x4E70:
          return OpReset(opcode);
        case 0x4E71:
          return SimResult::kOk;  // NOP
        case 0x4E72:
          if (!state_.IsSupervisor())
            return SimResult::kPrivilegeViolation;
          return SimResult::kStopInstruction;
        case 0x4E73:
          return OpRte(opcode);
        case 0x4E75:
          return OpRts(opcode);
        case 0x4E76:
          return OpTrapV(opcode);
        case 0x4E77:
          return OpRtr(opcode);
      }
      if ((opcode & 0xFFC0) == 0x4E80)
        return OpJsr(opcode);
      if ((opcode & 0xFFC0) == 0x4EC0)
        return OpJmp(opcode);
      return SimResult::kBadInstruction;
    }
    default:
      return SimResult::kBadInstruction;
  }
}

// =============================================================================
// Group 0x5 — ADDQ/SUBQ/Scc/DBcc
// =============================================================================

SimResult M68kSimulator::DispatchGroup5(uint16_t opcode) {
  // bit 8 = 0: ADDQ/SUBQ; bit 7-6 = 11: Scc/DBcc
  if ((opcode & 0x00C0) == 0x00C0) {
    // DBcc (mode=001=An with post-inc semantics) or Scc
    return ((opcode & 0x0038) == 0x0008) ? OpDbcc(opcode) : OpScc(opcode);
  }
  return (opcode & 0x0100) ? OpSubq(opcode) : OpAddq(opcode);
}

// =============================================================================
// Group 0x6 — BRA/BSR/Bcc
// =============================================================================

SimResult M68kSimulator::DispatchGroup6(uint16_t opcode) {
  uint8_t cond = (opcode >> 8) & 0xF;
  if (cond == 0)
    return OpBra(opcode);
  if (cond == 1)
    return OpBsr(opcode);
  return OpBcc(opcode);
}

// =============================================================================
// Group 0x7 — MOVEQ
// =============================================================================

SimResult M68kSimulator::OpMoveq(uint16_t opcode) {
  if (opcode & 0x0100)
    return SimResult::kBadInstruction;  // bit 8 must be clear
  int reg = (opcode >> 9) & 0x7;
  int8_t imm = static_cast<int8_t>(opcode & 0xFF);
  state_.d[reg] = static_cast<uint32_t>(static_cast<int32_t>(imm));
  UpdateFlagsMove(state_, state_.d[reg], DataSize::kLong);
  return SimResult::kOk;
}

// =============================================================================
// Group 0x8 — OR/DIVU/DIVS/SBCD
// =============================================================================

SimResult M68kSimulator::DispatchGroup8(uint16_t opcode) {
  uint16_t b76 = (opcode >> 6) & 0x3;
  if (b76 == 0x3) {
    // DIVU (bits 8=0) or DIVS (bits 8=1)
    return (opcode & 0x0100) ? OpDivs(opcode) : OpDivu(opcode);
  }
  if ((opcode & 0x01F0) == 0x0100)
    return OpSbcd(opcode);
  return OpOr(opcode);
}

// =============================================================================
// Group 0x9 — SUB/SUBA/SUBX
// =============================================================================

SimResult M68kSimulator::DispatchGroup9(uint16_t opcode) {
  uint16_t b76 = (opcode >> 6) & 0x3;
  if (b76 == 0x3)
    return OpSuba(opcode);
  if ((opcode & 0x0130) == 0x0100)
    return OpSubx(opcode);
  return OpSub(opcode);
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

SimResult M68kSimulator::DispatchGroupB(uint16_t opcode) {
  uint16_t b76 = (opcode >> 6) & 0x3;
  if (b76 == 0x3)
    return OpCmpa(opcode);
  if (opcode & 0x0100) {
    // EOR or CMPM (mode=001)
    return ((opcode & 0x0038) == 0x0008) ? OpCmpm(opcode) : OpEor(opcode);
  }
  return OpCmp(opcode);
}

// =============================================================================
// Group 0xC — AND/MULU/MULS/ABCD/EXG
// =============================================================================

SimResult M68kSimulator::DispatchGroupC(uint16_t opcode) {
  uint16_t b76 = (opcode >> 6) & 0x3;
  if (b76 == 0x3) {
    return (opcode & 0x0100) ? OpMuls(opcode) : OpMulu(opcode);
  }
  if ((opcode & 0x01F0) == 0x0100)
    return OpAbcd(opcode);
  if ((opcode & 0x01F8) == 0x0140 || (opcode & 0x01F8) == 0x0148 || (opcode & 0x01F8) == 0x0188)
    return OpExg(opcode);
  return OpAnd(opcode);
}

// =============================================================================
// Group 0xD — ADD/ADDA/ADDX
// =============================================================================

SimResult M68kSimulator::DispatchGroupD(uint16_t opcode) {
  uint16_t b76 = (opcode >> 6) & 0x3;
  if (b76 == 0x3)
    return OpAdda(opcode);
  if ((opcode & 0x0130) == 0x0100)
    return OpAddx(opcode);
  return OpAdd(opcode);
}

// =============================================================================
// Group 0xE — ASd/LSd/ROXd/ROd (all shift/rotate variants)
// =============================================================================

SimResult M68kSimulator::DispatchGroupE(uint16_t opcode) {
  return OpShiftRotate(opcode);
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
