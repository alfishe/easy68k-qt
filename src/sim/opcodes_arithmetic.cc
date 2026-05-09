// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Arithmetic instructions: ADD, ADDA, ADDI, ADDQ, ADDX, SUB, SUBA, SUBI,
// SUBQ, SUBX, MULU, MULS, DIVU, DIVS, NEG, NEGX, CLR, CMP, CMPA, CMPI,
// CMPM, TST, EXT, ABCD, SBCD, NBCD, CHK.
// Ported from EASy68K/Sim68K/CODE2.CPP and CODE3.CPP.

#include "easym68k/sim/addressing_mode.h"
#include "easym68k/sim/flag_computation.h"
#include "easym68k/sim/simulator.h"

namespace easym68k::sim {

namespace {

DataSize SizeFromSS(int ss) {
  switch (ss) {
    case 0:
      return DataSize::kByte;
    case 1:
      return DataSize::kWord;
    default:
      return DataSize::kLong;
  }
}

}  // namespace

// ---------------------------------------------------------------------------
// ADD — dst = dst + src, updates N/Z/V/C/X
// Opcode: 1101 DDD d ssEEEEEE
//   d=0: Dn=dst, <EA>=src   d=1: <EA>=dst, Dn=src
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpAdd(uint16_t opcode) {
  int dn = (opcode >> 9) & 0x7;
  bool dst_ea = (opcode & 0x0100) != 0;
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  EffectiveAddr ea = CalcEA(mode, reg, size);
  uint32_t ea_val = ReadEA(ea, size);
  uint32_t dn_val = state_.d[dn] & SizeMask(size);

  uint32_t src, dst;
  if (dst_ea) {
    src = dn_val;
    dst = ea_val;
  } else {
    src = ea_val;
    dst = dn_val;
  }

  uint32_t result = (src + dst) & SizeMask(size);
  UpdateFlagsAdd(state_, src, dst, result, size);

  if (dst_ea) {
    return WriteEA(ea, result, size);
  } else {
    uint32_t mask = SizeMask(size);
    state_.d[dn] = (state_.d[dn] & ~mask) | (result & mask);
    return SimResult::kOk;
  }
}

// ---------------------------------------------------------------------------
// ADDA — An = An + src (sign-extended); no flag update
// Opcode: 1101 AAA S 11EEEEEE   S=0→.W S=1→.L
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpAdda(uint16_t opcode) {
  int an = (opcode >> 9) & 0x7;
  bool is_long = (opcode & 0x0100) != 0;
  DataSize src_size = is_long ? DataSize::kLong : DataSize::kWord;
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  EffectiveAddr ea = CalcEA(mode, reg, src_size);
  uint32_t val = ReadEA(ea, src_size);
  if (!is_long)
    val = static_cast<uint32_t>(static_cast<int16_t>(val & 0xFFFF));

  state_.SetAReg(an, state_.GetAReg(an) + val);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// ADDI — dst = dst + imm, updates N/Z/V/C/X
// Opcode: 0000 0110 ssEEEEEE, followed by immediate word(s)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpAddi(uint16_t opcode) {
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  uint32_t imm;
  if (size == DataSize::kLong) {
    imm = FetchLong();
  } else {
    imm = FetchWord() & SizeMask(size);
  }

  EffectiveAddr ea = CalcEA(mode, reg, size);
  uint32_t dst = ReadEA(ea, size) & SizeMask(size);
  uint32_t result = (imm + dst) & SizeMask(size);
  UpdateFlagsAdd(state_, imm, dst, result, size);
  return WriteEA(ea, result, size);
}

// ---------------------------------------------------------------------------
// ADDQ — dst = dst + imm3, updates N/Z/V/C/X (An: no flags, always .L)
// Opcode: 0101 DDD0 ssEEEEEE   DDD=immediate (0→8)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpAddq(uint16_t opcode) {
  int imm3 = (opcode >> 9) & 0x7;
  uint32_t imm = (imm3 == 0) ? 8u : static_cast<uint32_t>(imm3);
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  bool dst_is_an = (mode == 1);
  if (dst_is_an) {
    // Address register: always 32-bit add, no flags
    uint32_t an_val = state_.GetAReg(reg);
    state_.SetAReg(reg, an_val + imm);
    return SimResult::kOk;
  }

  EffectiveAddr ea = CalcEA(mode, reg, size);
  uint32_t dst = ReadEA(ea, size) & SizeMask(size);
  uint32_t result = (imm + dst) & SizeMask(size);
  UpdateFlagsAdd(state_, imm, dst, result, size);
  return WriteEA(ea, result, size);
}

// ---------------------------------------------------------------------------
// ADDX — dst = dst + src + X, Z cleared only if result != 0
// Opcode: 1101 DDD1 ss 0 R 0RRR   R=0: Dn/Dn  R=1: -(An)/-(An)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpAddx(uint16_t opcode) {
  int dst_reg = (opcode >> 9) & 0x7;
  int src_reg = opcode & 0x7;
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  bool mem_mode = (opcode & 0x0008) != 0;
  uint32_t mask = SizeMask(size);
  uint32_t x = state_.GetFlag(kSrExtend) ? 1u : 0u;

  uint32_t src, dst;
  if (mem_mode) {
    EffectiveAddr src_ea = CalcEA(4, src_reg, size);  // -(An)
    EffectiveAddr dst_ea = CalcEA(4, dst_reg, size);
    src = ReadEA(src_ea, size) & mask;
    dst = ReadEA(dst_ea, size) & mask;
    uint32_t result = (dst + src + x) & mask;
    UpdateFlagsAddX(state_, src, dst, result, size);
    return WriteEA(dst_ea, result, size);
  } else {
    src = state_.d[src_reg] & mask;
    dst = state_.d[dst_reg] & mask;
    uint32_t result = (dst + src + x) & mask;
    UpdateFlagsAddX(state_, src, dst, result, size);
    state_.d[dst_reg] = (state_.d[dst_reg] & ~mask) | result;
    return SimResult::kOk;
  }
}

// ---------------------------------------------------------------------------
// SUB — dst = dst - src, updates N/Z/V/C/X
// Opcode: 1001 DDD d ssEEEEEE
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpSub(uint16_t opcode) {
  int dn = (opcode >> 9) & 0x7;
  bool dst_ea = (opcode & 0x0100) != 0;
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  EffectiveAddr ea = CalcEA(mode, reg, size);
  uint32_t ea_val = ReadEA(ea, size);
  uint32_t dn_val = state_.d[dn] & SizeMask(size);

  uint32_t src, dst;
  if (dst_ea) {
    src = dn_val;
    dst = ea_val;
  } else {
    src = ea_val;
    dst = dn_val;
  }

  uint32_t result = (dst - src) & SizeMask(size);
  UpdateFlagsSub(state_, src, dst, result, size);

  if (dst_ea) {
    return WriteEA(ea, result, size);
  } else {
    uint32_t mask = SizeMask(size);
    state_.d[dn] = (state_.d[dn] & ~mask) | (result & mask);
    return SimResult::kOk;
  }
}

// ---------------------------------------------------------------------------
// SUBA — An = An - src (sign-extended); no flag update
// Opcode: 1001 AAA S 11EEEEEE
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpSuba(uint16_t opcode) {
  int an = (opcode >> 9) & 0x7;
  bool is_long = (opcode & 0x0100) != 0;
  DataSize src_size = is_long ? DataSize::kLong : DataSize::kWord;
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  EffectiveAddr ea = CalcEA(mode, reg, src_size);
  uint32_t val = ReadEA(ea, src_size);
  if (!is_long)
    val = static_cast<uint32_t>(static_cast<int16_t>(val & 0xFFFF));

  state_.SetAReg(an, state_.GetAReg(an) - val);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// SUBI — dst = dst - imm, updates N/Z/V/C/X
// Opcode: 0000 0100 ssEEEEEE, followed by immediate word(s)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpSubi(uint16_t opcode) {
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  uint32_t imm;
  if (size == DataSize::kLong) {
    imm = FetchLong();
  } else {
    imm = FetchWord() & SizeMask(size);
  }

  EffectiveAddr ea = CalcEA(mode, reg, size);
  uint32_t dst = ReadEA(ea, size) & SizeMask(size);
  uint32_t result = (dst - imm) & SizeMask(size);
  UpdateFlagsSub(state_, imm, dst, result, size);
  return WriteEA(ea, result, size);
}

// ---------------------------------------------------------------------------
// SUBQ — dst = dst - imm3; An: no flags, always .L
// Opcode: 0101 DDD1 ssEEEEEE
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpSubq(uint16_t opcode) {
  int imm3 = (opcode >> 9) & 0x7;
  uint32_t imm = (imm3 == 0) ? 8u : static_cast<uint32_t>(imm3);
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  bool dst_is_an = (mode == 1);
  if (dst_is_an) {
    state_.SetAReg(reg, state_.GetAReg(reg) - imm);
    return SimResult::kOk;
  }

  EffectiveAddr ea = CalcEA(mode, reg, size);
  uint32_t dst = ReadEA(ea, size) & SizeMask(size);
  uint32_t result = (dst - imm) & SizeMask(size);
  UpdateFlagsSub(state_, imm, dst, result, size);
  return WriteEA(ea, result, size);
}

// ---------------------------------------------------------------------------
// SUBX — dst = dst - src - X, Z cleared only if result != 0
// Opcode: 1001 DDD1 ss 0 R 0RRR
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpSubx(uint16_t opcode) {
  int dst_reg = (opcode >> 9) & 0x7;
  int src_reg = opcode & 0x7;
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  bool mem_mode = (opcode & 0x0008) != 0;
  uint32_t mask = SizeMask(size);
  uint32_t x = state_.GetFlag(kSrExtend) ? 1u : 0u;

  uint32_t src, dst;
  if (mem_mode) {
    EffectiveAddr src_ea = CalcEA(4, src_reg, size);
    EffectiveAddr dst_ea = CalcEA(4, dst_reg, size);
    src = ReadEA(src_ea, size) & mask;
    dst = ReadEA(dst_ea, size) & mask;
    uint32_t result = (dst - src - x) & mask;
    UpdateFlagsSubX(state_, src, dst, result, size);
    return WriteEA(dst_ea, result, size);
  } else {
    src = state_.d[src_reg] & mask;
    dst = state_.d[dst_reg] & mask;
    uint32_t result = (dst - src - x) & mask;
    UpdateFlagsSubX(state_, src, dst, result, size);
    state_.d[dst_reg] = (state_.d[dst_reg] & ~mask) | result;
    return SimResult::kOk;
  }
}

// ---------------------------------------------------------------------------
// MULU.W — Dn.L = Dn.W * <EA>.W (unsigned)
// Opcode: 1100 DDD0 11MMMRRR
// Flags: N/Z from 32-bit result, V=0, C=0
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpMulu(uint16_t opcode) {
  int dn = (opcode >> 9) & 0x7;
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  EffectiveAddr ea = CalcEA(mode, reg, DataSize::kWord);
  uint32_t src = ReadEA(ea, DataSize::kWord) & kWordMask;
  uint32_t dst = state_.d[dn] & kWordMask;
  uint32_t result = src * dst;

  state_.d[dn] = result;
  UpdateFlagsLogic(state_, result, DataSize::kLong);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// MULS.W — Dn.L = Dn.W * <EA>.W (signed)
// Opcode: 1100 DDD1 11MMMRRR
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpMuls(uint16_t opcode) {
  int dn = (opcode >> 9) & 0x7;
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  EffectiveAddr ea = CalcEA(mode, reg, DataSize::kWord);
  int32_t src = static_cast<int16_t>(ReadEA(ea, DataSize::kWord) & kWordMask);
  int32_t dst = static_cast<int16_t>(state_.d[dn] & kWordMask);
  uint32_t result = static_cast<uint32_t>(src * dst);

  state_.d[dn] = result;
  UpdateFlagsLogic(state_, result, DataSize::kLong);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// DIVU.W — Dn.L / <EA>.W (unsigned)
// Result: high word = remainder, low word = quotient
// Divide-by-zero → kDivideByZero; overflow → V=1 (Dn unchanged)
// Opcode: 1000 DDD0 11MMMRRR
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpDivu(uint16_t opcode) {
  int dn = (opcode >> 9) & 0x7;
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  EffectiveAddr ea = CalcEA(mode, reg, DataSize::kWord);
  uint32_t divisor = ReadEA(ea, DataSize::kWord) & kWordMask;

  if (divisor == 0)
    return SimResult::kDivideByZero;

  uint32_t dividend = state_.d[dn];
  uint32_t quotient = dividend / divisor;
  uint32_t remainder = dividend % divisor;

  if (quotient > 0xFFFF) {
    // Overflow: V=1, C=0, N/Z undefined, Dn unchanged
    state_.SetFlag(kSrOverflow, true);
    state_.SetFlag(kSrCarry, false);
    return SimResult::kOk;
  }

  state_.d[dn] = ((remainder & kWordMask) << 16) | (quotient & kWordMask);
  // Flags from quotient (16-bit)
  state_.SetFlag(kSrNegative, (quotient & 0x8000) != 0);
  state_.SetFlag(kSrZero, quotient == 0);
  state_.SetFlag(kSrOverflow, false);
  state_.SetFlag(kSrCarry, false);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// DIVS.W — Dn.L / <EA>.W (signed)
// Opcode: 1000 DDD1 11MMMRRR
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpDivs(uint16_t opcode) {
  int dn = (opcode >> 9) & 0x7;
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  EffectiveAddr ea = CalcEA(mode, reg, DataSize::kWord);
  int32_t divisor = static_cast<int16_t>(ReadEA(ea, DataSize::kWord) & kWordMask);

  if (divisor == 0)
    return SimResult::kDivideByZero;

  int32_t dividend = static_cast<int32_t>(state_.d[dn]);

  if (dividend == INT32_MIN && divisor == -1) {
    state_.SetFlag(kSrOverflow, true);
    state_.SetFlag(kSrCarry, false);
    return SimResult::kOk;
  }

  int32_t quotient = dividend / divisor;
  int32_t remainder = dividend % divisor;

  // Overflow if quotient doesn't fit in a signed 16-bit value
  if (quotient < -32768 || quotient > 32767) {
    state_.SetFlag(kSrOverflow, true);
    state_.SetFlag(kSrCarry, false);
    return SimResult::kOk;
  }

  state_.d[dn] = (static_cast<uint32_t>(remainder & 0xFFFF) << 16) |
                 (static_cast<uint32_t>(quotient) & kWordMask);
  state_.SetFlag(kSrNegative, (quotient & 0x8000) != 0);
  state_.SetFlag(kSrZero, quotient == 0);
  state_.SetFlag(kSrOverflow, false);
  state_.SetFlag(kSrCarry, false);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// NEG — dst = 0 - dst, updates N/Z/V/C/X
// Opcode: 0100 0100 ssEEEEEE
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpNeg(uint16_t opcode) {
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  EffectiveAddr ea = CalcEA(mode, reg, size);
  uint32_t dst = ReadEA(ea, size) & SizeMask(size);
  uint32_t result = (0u - dst) & SizeMask(size);
  UpdateFlagsNeg(state_, dst, result, size);
  return WriteEA(ea, result, size);
}

// ---------------------------------------------------------------------------
// NEGX — dst = 0 - dst - X, Z cleared only if result != 0
// Opcode: 0100 0000 ssEEEEEE
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpNegx(uint16_t opcode) {
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;
  uint32_t x = state_.GetFlag(kSrExtend) ? 1u : 0u;

  EffectiveAddr ea = CalcEA(mode, reg, size);
  uint32_t dst = ReadEA(ea, size) & SizeMask(size);
  uint32_t result = (0u - dst - x) & SizeMask(size);
  UpdateFlagsNegX(state_, dst, result, size);
  return WriteEA(ea, result, size);
}

// ---------------------------------------------------------------------------
// CLR — dst = 0; N=0, Z=1, V=0, C=0; X unchanged
// Opcode: 0100 0010 ssEEEEEE
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpClr(uint16_t opcode) {
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  EffectiveAddr ea = CalcEA(mode, reg, size);
  SimResult r = WriteEA(ea, 0, size);
  if (r != SimResult::kOk)
    return r;
  state_.SetFlag(kSrNegative, false);
  state_.SetFlag(kSrZero, true);
  state_.SetFlag(kSrOverflow, false);
  state_.SetFlag(kSrCarry, false);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// CMP — Dn - <EA>; flags only, result discarded
// Opcode: 1011 DDD0 ssEEEEEE
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpCmp(uint16_t opcode) {
  int dn = (opcode >> 9) & 0x7;
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  EffectiveAddr ea = CalcEA(mode, reg, size);
  uint32_t src = ReadEA(ea, size) & SizeMask(size);
  uint32_t dst = state_.d[dn] & SizeMask(size);
  uint32_t result = (dst - src) & SizeMask(size);
  UpdateFlagsCmp(state_, src, dst, result, size);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// CMPA — An - <EA> (sign-extended to 32); always .L comparison
// Opcode: 1011 AAA S 11EEEEEE   S=0→.W  S=1→.L
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpCmpa(uint16_t opcode) {
  int an = (opcode >> 9) & 0x7;
  bool is_long = (opcode & 0x0100) != 0;
  DataSize src_size = is_long ? DataSize::kLong : DataSize::kWord;
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  EffectiveAddr ea = CalcEA(mode, reg, src_size);
  uint32_t val = ReadEA(ea, src_size);
  if (!is_long)
    val = static_cast<uint32_t>(static_cast<int16_t>(val & 0xFFFF));

  uint32_t dst = state_.GetAReg(an);
  uint32_t result = dst - val;
  UpdateFlagsCmp(state_, val, dst, result, DataSize::kLong);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// CMPI — <EA> - imm; flags only
// Opcode: 0000 1100 ssEEEEEE, followed by immediate word(s)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpCmpi(uint16_t opcode) {
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  uint32_t imm;
  if (size == DataSize::kLong) {
    imm = FetchLong();
  } else {
    imm = FetchWord() & SizeMask(size);
  }

  EffectiveAddr ea = CalcEA(mode, reg, size);
  uint32_t dst = ReadEA(ea, size) & SizeMask(size);
  uint32_t result = (dst - imm) & SizeMask(size);
  UpdateFlagsCmp(state_, imm, dst, result, size);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// CMPM — (Ay)+ - (Ax)+; flags only
// Opcode: 1011 XXX1 ss 001YYY   XXX=Ax(dst), YYY=Ay(src)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpCmpm(uint16_t opcode) {
  int ax = (opcode >> 9) & 0x7;
  int ay = opcode & 0x7;
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);

  EffectiveAddr src_ea = CalcEA(3, ay, size);  // (Ay)+
  EffectiveAddr dst_ea = CalcEA(3, ax, size);  // (Ax)+
  uint32_t src = ReadEA(src_ea, size) & SizeMask(size);
  uint32_t dst = ReadEA(dst_ea, size) & SizeMask(size);
  uint32_t result = (dst - src) & SizeMask(size);
  UpdateFlagsCmp(state_, src, dst, result, size);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// TST — test <EA>; N/Z from operand, V=0, C=0; X unchanged
// Opcode: 0100 1010 ssEEEEEE
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpTst(uint16_t opcode) {
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  EffectiveAddr ea = CalcEA(mode, reg, size);
  uint32_t val = ReadEA(ea, size) & SizeMask(size);
  UpdateFlagsMove(state_, val, size);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// EXT — sign extend Dn
// Opcode: 0100 1000 10 000DDD (byte→word)
//         0100 1000 11 000DDD (word→long)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpExt(uint16_t opcode) {
  int dn = opcode & 0x7;
  bool to_long = (opcode & 0x0040) != 0;  // bit 6: 0=byte→word, 1=word→long

  uint32_t result;
  if (to_long) {
    result = static_cast<uint32_t>(static_cast<int16_t>(state_.d[dn] & 0xFFFF));
    state_.d[dn] = result;
    UpdateFlagsMove(state_, result, DataSize::kLong);
  } else {
    result = static_cast<uint32_t>(static_cast<int8_t>(state_.d[dn] & 0xFF)) & kWordMask;
    state_.d[dn] = (state_.d[dn] & 0xFFFF0000u) | result;
    UpdateFlagsMove(state_, result, DataSize::kWord);
  }
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// ABCD — dst_bcd = dst_bcd + src_bcd + X (BCD)
// Opcode: 1100 DDD1 0000 0RRR (Dn/Dn)
//         1100 DDD1 0000 1RRR (-(An)/-(An))
// Flags: X=C=carry out, Z cleared if result != 0; N/V undefined
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpAbcd(uint16_t opcode) {
  int dst_reg = (opcode >> 9) & 0x7;
  int src_reg = opcode & 0x7;
  bool mem_mode = (opcode & 0x0008) != 0;
  uint32_t x = state_.GetFlag(kSrExtend) ? 1u : 0u;

  uint32_t src, dst;
  EffectiveAddr src_ea, dst_ea;
  if (mem_mode) {
    src_ea = CalcEA(4, src_reg, DataSize::kByte);
    dst_ea = CalcEA(4, dst_reg, DataSize::kByte);
    src = ReadEA(src_ea, DataSize::kByte) & kByteMask;
    dst = ReadEA(dst_ea, DataSize::kByte) & kByteMask;
  } else {
    src = state_.d[src_reg] & kByteMask;
    dst = state_.d[dst_reg] & kByteMask;
  }

  uint32_t low = (src & 0xF) + (dst & 0xF) + x;
  if (low > 9)
    low += 6;
  uint32_t high = (src >> 4) + (dst >> 4) + (low >> 4);
  low &= 0xF;
  if (high > 9)
    high += 6;
  uint32_t result = ((high & 0xF) << 4) | low;
  bool carry = (high > 0xF);

  if (result != 0)
    state_.SetFlag(kSrZero, false);
  state_.SetFlag(kSrCarry, carry);
  state_.SetFlag(kSrExtend, carry);
  state_.SetFlag(kSrNegative, (result & 0x80) != 0);

  if (mem_mode) {
    return WriteEA(dst_ea, result, DataSize::kByte);
  } else {
    state_.d[dst_reg] = (state_.d[dst_reg] & ~kByteMask) | result;
    return SimResult::kOk;
  }
}

// ---------------------------------------------------------------------------
// SBCD — dst_bcd = dst_bcd - src_bcd - X (BCD)
// Opcode: 1000 DDD1 0000 0RRR (Dn/Dn)
//         1000 DDD1 0000 1RRR (-(An)/-(An))
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpSbcd(uint16_t opcode) {
  int dst_reg = (opcode >> 9) & 0x7;
  int src_reg = opcode & 0x7;
  bool mem_mode = (opcode & 0x0008) != 0;
  uint32_t x = state_.GetFlag(kSrExtend) ? 1u : 0u;

  uint32_t src, dst;
  EffectiveAddr src_ea, dst_ea;
  if (mem_mode) {
    src_ea = CalcEA(4, src_reg, DataSize::kByte);
    dst_ea = CalcEA(4, dst_reg, DataSize::kByte);
    src = ReadEA(src_ea, DataSize::kByte) & kByteMask;
    dst = ReadEA(dst_ea, DataSize::kByte) & kByteMask;
  } else {
    src = state_.d[src_reg] & kByteMask;
    dst = state_.d[dst_reg] & kByteMask;
  }

  int low = static_cast<int>((dst & 0xF)) - static_cast<int>((src & 0xF)) - static_cast<int>(x);
  int borrow_low = 0;
  if (low < 0) {
    low += 10;
    borrow_low = 1;
  }
  int high = static_cast<int>((dst >> 4)) - static_cast<int>((src >> 4)) - borrow_low;
  int borrow_high = 0;
  if (high < 0) {
    high += 10;
    borrow_high = 1;
  }
  uint32_t result = (static_cast<uint32_t>(high & 0xF) << 4) | static_cast<uint32_t>(low & 0xF);
  bool carry = (borrow_high != 0);

  if (result != 0)
    state_.SetFlag(kSrZero, false);
  state_.SetFlag(kSrCarry, carry);
  state_.SetFlag(kSrExtend, carry);
  state_.SetFlag(kSrNegative, (result & 0x80) != 0);

  if (mem_mode) {
    return WriteEA(dst_ea, result, DataSize::kByte);
  } else {
    state_.d[dst_reg] = (state_.d[dst_reg] & ~kByteMask) | result;
    return SimResult::kOk;
  }
}

// ---------------------------------------------------------------------------
// NBCD — dst_bcd = 0 - dst_bcd - X (BCD negate)
// Opcode: 0100 1000 00MMMRRR
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpNbcd(uint16_t opcode) {
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;
  uint32_t x = state_.GetFlag(kSrExtend) ? 1u : 0u;

  EffectiveAddr ea = CalcEA(mode, reg, DataSize::kByte);
  uint32_t dst = ReadEA(ea, DataSize::kByte) & kByteMask;

  int low = 0 - static_cast<int>(dst & 0xF) - static_cast<int>(x);
  int borrow_low = 0;
  if (low < 0) {
    low += 10;
    borrow_low = 1;
  }
  int high = 0 - static_cast<int>(dst >> 4) - borrow_low;
  int borrow_high = 0;
  if (high < 0) {
    high += 10;
    borrow_high = 1;
  }
  uint32_t result = (static_cast<uint32_t>(high & 0xF) << 4) | static_cast<uint32_t>(low & 0xF);
  bool carry = (borrow_high != 0);

  if (result != 0)
    state_.SetFlag(kSrZero, false);
  state_.SetFlag(kSrCarry, carry);
  state_.SetFlag(kSrExtend, carry);
  state_.SetFlag(kSrNegative, (result & 0x80) != 0);
  return WriteEA(ea, result, DataSize::kByte);
}

// ---------------------------------------------------------------------------
// CHK — if Dn < 0 or Dn > <EA>.W, raise CHK exception
// Opcode: 0100 DDD1 10MMMRRR
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpChk(uint16_t opcode) {
  int dn = (opcode >> 9) & 0x7;
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  EffectiveAddr ea = CalcEA(mode, reg, DataSize::kWord);
  int16_t upper = static_cast<int16_t>(ReadEA(ea, DataSize::kWord) & kWordMask);
  int16_t val = static_cast<int16_t>(state_.d[dn] & kWordMask);

  if (val < 0 || val > upper)
    return SimResult::kChkException;
  return SimResult::kOk;
}

}  // namespace easym68k::sim
