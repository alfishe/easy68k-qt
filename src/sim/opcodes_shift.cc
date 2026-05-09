// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Shift/rotate instructions: ASL/ASR, LSL/LSR, ROL/ROR, ROXL/ROXR (register
// and memory forms), and bit operations: BTST, BCHG, BCLR, BSET.
// Ported from EASy68K/Sim68K/CODE7.CPP.

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

// Shift/rotate one step left; type: 0=AS, 1=LS, 2=ROX, 3=RO.
// x_in is the current extend flag (used only by ROXL).
// Returns {result, carry_out, new_x}.
struct ShiftOut {
  uint32_t result;
  bool c;
  bool x;
};

ShiftOut ShiftLeft1(uint32_t val, DataSize size, int type, bool x_in) {
  uint32_t mask = SizeMask(size);
  uint32_t msb = SignBit(size);
  bool c = (val & msb) != 0;
  uint32_t result = (val << 1) & mask;
  bool x = x_in;
  if (type == 2) {
    result |= (x_in ? 1u : 0u);  // ROXL: shift in X
    x = c;
  } else if (type == 3) {
    result |= (c ? 1u : 0u);  // ROL: rotate C into LSB
  }
  return {result, c, x};
}

ShiftOut ShiftRight1(uint32_t val, DataSize size, int type, bool x_in) {
  uint32_t mask = SizeMask(size);
  uint32_t msb = SignBit(size);
  bool c = (val & 1u) != 0;
  uint32_t result;
  bool x = x_in;
  switch (type) {
    case 0:  // ASR: fill with sign bit
      result = ((val >> 1) | ((val & msb) ? msb : 0u)) & mask;
      break;
    case 2:  // ROXR: fill with X
      result = ((val >> 1) | (x_in ? msb : 0u)) & mask;
      x = c;
      break;
    case 3:  // ROR: rotate C (= old LSB) into MSB
      result = ((val >> 1) | (c ? msb : 0u)) & mask;
      break;
    default:  // LSR: fill with 0
      result = (val >> 1) & mask;
      break;
  }
  return {result, c, x};
}

}  // namespace

// ---------------------------------------------------------------------------
// ShiftRotate — unified handler for all 8 shift/rotate types
// Register form: 1110 ccc d ss i tt rrr
//   bits 11-9: count (if i=1, 1-8 with 0→8) or Dn index (if i=0)
//   bit    8:  direction (1=left, 0=right)
//   bits 7-6:  size (00=byte, 01=word, 10=long); 11=memory form
//   bit    5:  immediate (1) or register count (0)
//   bits 4-3:  type (00=AS, 01=LS, 10=ROX, 11=RO)
//   bits 2-0:  destination data register
// Memory form: bits 7-6 = 11; always word, count=1; bits 11-9 = type+dir.
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpShiftRotate(uint16_t opcode) {
  int sz = (opcode >> 6) & 0x3;

  if (sz == 3) {
    // ---- Memory form ----
    // bits 10-9 = type (00=AS,01=LS,10=ROX,11=RO), bit 8 = direction
    int type = (opcode >> 9) & 0x3;
    bool left = (opcode >> 8) & 1;
    int mode = (opcode >> 3) & 0x7;
    int reg = opcode & 0x7;

    EffectiveAddr ea = CalcEA(mode, reg, DataSize::kWord);
    uint32_t val = ReadEA(ea, DataSize::kWord) & kWordMask;
    bool x = state_.GetFlag(kSrExtend);

    ShiftOut s = left ? ShiftLeft1(val, DataSize::kWord, type, x)
                      : ShiftRight1(val, DataSize::kWord, type, x);

    UpdateFlagsShift(state_, s.result, DataSize::kWord, s.c, 1);
    if (type == 3)  // ROL/ROR: X not affected
      state_.SetFlag(kSrExtend, x);
    if (type == 0 && left)  // ASL: V set if sign bit changed
      state_.SetFlag(kSrOverflow, (val & kWordSignBit) != (s.result & kWordSignBit));

    return WriteEA(ea, s.result, DataSize::kWord);
  }

  // ---- Register form ----
  DataSize size = SizeFromSS(sz);
  bool left = (opcode >> 8) & 1;
  bool imm = (opcode >> 5) & 1;
  int type = (opcode >> 3) & 0x3;
  int dst = opcode & 0x7;

  uint32_t count;
  if (imm) {
    count = (opcode >> 9) & 0x7;
    if (count == 0)
      count = 8;
  } else {
    count = state_.d[(opcode >> 9) & 0x7] % 64;
  }

  uint32_t mask = SizeMask(size);
  uint32_t msb = SignBit(size);
  uint32_t val = state_.d[dst] & mask;
  bool x = state_.GetFlag(kSrExtend);
  // Default carry for count=0: ROXL/ROXR copies X, others clear C.
  bool c = (type == 2) ? x : false;
  bool v = false;
  uint32_t result = val;

  for (uint32_t i = 0; i < count; ++i) {
    uint32_t prev = result;
    ShiftOut s = left ? ShiftLeft1(result, size, type, x) : ShiftRight1(result, size, type, x);
    result = s.result;
    c = s.c;
    x = s.x;
    if (type == 0 && left && (result & msb) != (prev & msb))
      v = true;  // ASL: sign changed
  }

  bool orig_x = state_.GetFlag(kSrExtend);
  UpdateFlagsShift(state_, result, size, c, (int)count);
  if (type == 3)  // ROL/ROR: X not affected
    state_.SetFlag(kSrExtend, orig_x);
  if (type == 0 && left)  // ASL: override V
    state_.SetFlag(kSrOverflow, v);

  state_.d[dst] = (state_.d[dst] & ~mask) | (result & mask);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// Bit operations — BTST/BCHG/BCLR/BSET
// Dynamic form: bit 8 = 1; bit number from Dn (bits 11-9).
// Static form:  bit 8 = 0; bit number from next immediate word (low byte).
// Dn destination: 32-bit, bit# mod 32.
// Memory destination: 8-bit, bit# mod 8.
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpBtst(uint16_t opcode) {
  uint32_t bit_num = (opcode & 0x0100) ? state_.d[(opcode >> 9) & 0x7] : (FetchWord() & 0xFF);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;
  uint32_t val;
  if (mode == 0) {
    bit_num &= 31;
    val = state_.d[reg];
  } else {
    bit_num &= 7;
    EffectiveAddr ea = CalcEA(mode, reg, DataSize::kByte);
    val = ReadEA(ea, DataSize::kByte);
  }
  state_.SetFlag(kSrZero, !((val >> bit_num) & 1u));
  return SimResult::kOk;
}

SimResult M68kSimulator::OpBchg(uint16_t opcode) {
  uint32_t bit_num = (opcode & 0x0100) ? state_.d[(opcode >> 9) & 0x7] : (FetchWord() & 0xFF);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;
  if (mode == 0) {
    bit_num &= 31;
    uint32_t val = state_.d[reg];
    state_.SetFlag(kSrZero, !((val >> bit_num) & 1u));
    state_.d[reg] = val ^ (1u << bit_num);
  } else {
    bit_num &= 7;
    EffectiveAddr ea = CalcEA(mode, reg, DataSize::kByte);
    uint32_t val = ReadEA(ea, DataSize::kByte) & kByteMask;
    state_.SetFlag(kSrZero, !((val >> bit_num) & 1u));
    return WriteEA(ea, val ^ (1u << bit_num), DataSize::kByte);
  }
  return SimResult::kOk;
}

SimResult M68kSimulator::OpBclr(uint16_t opcode) {
  uint32_t bit_num = (opcode & 0x0100) ? state_.d[(opcode >> 9) & 0x7] : (FetchWord() & 0xFF);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;
  if (mode == 0) {
    bit_num &= 31;
    uint32_t val = state_.d[reg];
    state_.SetFlag(kSrZero, !((val >> bit_num) & 1u));
    state_.d[reg] = val & ~(1u << bit_num);
  } else {
    bit_num &= 7;
    EffectiveAddr ea = CalcEA(mode, reg, DataSize::kByte);
    uint32_t val = ReadEA(ea, DataSize::kByte) & kByteMask;
    state_.SetFlag(kSrZero, !((val >> bit_num) & 1u));
    return WriteEA(ea, val & ~(1u << bit_num), DataSize::kByte);
  }
  return SimResult::kOk;
}

SimResult M68kSimulator::OpBset(uint16_t opcode) {
  uint32_t bit_num = (opcode & 0x0100) ? state_.d[(opcode >> 9) & 0x7] : (FetchWord() & 0xFF);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;
  if (mode == 0) {
    bit_num &= 31;
    uint32_t val = state_.d[reg];
    state_.SetFlag(kSrZero, !((val >> bit_num) & 1u));
    state_.d[reg] = val | (1u << bit_num);
  } else {
    bit_num &= 7;
    EffectiveAddr ea = CalcEA(mode, reg, DataSize::kByte);
    uint32_t val = ReadEA(ea, DataSize::kByte) & kByteMask;
    state_.SetFlag(kSrZero, !((val >> bit_num) & 1u));
    return WriteEA(ea, val | (1u << bit_num), DataSize::kByte);
  }
  return SimResult::kOk;
}

}  // namespace easym68k::sim
