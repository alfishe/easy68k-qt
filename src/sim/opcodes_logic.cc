// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Logic instructions: OR, ORI, ORI-to-CCR/SR, AND, ANDI, ANDI-to-CCR/SR,
// EOR, EORI, EORI-to-CCR/SR, NOT, TAS.
// Ported from EASy68K/Sim68K/CODE4.CPP and CODE5.CPP.

#include "easym68k/sim/addressing_mode.h"
#include "easym68k/sim/cycle_data.h"
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

// CODE6.CPP AND/OR timing (excluding EA time).
//   <ea> destination:  8 (B/W) / 12 (L)
//   Dn destination:    4 (B/W); for .L, 8 if source is Dn/An/#imm, else 6.
int AndOrCycles(uint16_t opcode, DataSize size) {
  if (opcode & 0x0100)
    return (size == DataSize::kLong) ? 12 : 8;
  if (size == DataSize::kLong)
    return (!(opcode & 0x0030) || (opcode & 0x003F) == 0x003C) ? 8 : 6;
  return 4;
}

// CODE6.CPP ANDI/ORI/EORI timing (excluding EA time):
//   Dn: 8 (B/W) / 16 (L); memory: 12 (B/W) / 20 (L).
int LogicImmCycles(uint16_t opcode, DataSize size) {
  if (opcode & 0x0038)
    return (size == DataSize::kLong) ? 20 : 12;
  return (size == DataSize::kLong) ? 16 : 8;
}

// CODE6.CPP ANDI/ORI/EORI to CCR/SR.
constexpr int kLogicImmToSrCycles = 20;

}  // namespace

// ---------------------------------------------------------------------------
// OR — dst = dst | src, updates N/Z, clears V/C; X unchanged
// Opcode: 1000 DDD d ss MMMRRR
//   d=0: Dn=dst, <EA>=src   d=1: <EA>=dst, Dn=src
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpOr(uint16_t opcode) {
  int dn = (opcode >> 9) & 0x7;
  bool dst_ea = (opcode & 0x0100) != 0;
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;
  uint32_t mask = SizeMask(size);

  EffectiveAddr ea = CalcEA(mode, reg, size);
  IncrementCycles(EaCyclePenalty(ea, size) + AndOrCycles(opcode, size));
  uint32_t ea_val = ReadEA(ea, size) & mask;
  uint32_t dn_val = state_.d[dn] & mask;

  uint32_t result = (dn_val | ea_val) & mask;
  UpdateFlagsLogic(state_, result, size);

  if (dst_ea) {
    return WriteEA(ea, result, size);
  }
  state_.d[dn] = (state_.d[dn] & ~mask) | result;
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// ORI — EA = EA | imm; special cases: ORI #imm,CCR  ORI #imm,SR
// Opcode: 0000 0000 ss MMMRRR, followed by immediate word(s)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpOri(uint16_t opcode) {
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;
  uint32_t mask = SizeMask(size);

  uint32_t imm = (size == DataSize::kLong) ? FetchLong() : (FetchWord() & mask);

  EffectiveAddr ea = CalcEA(mode, reg, size);
  IncrementCycles(EaCyclePenalty(ea, size) + LogicImmCycles(opcode, size));
  uint32_t dst = ReadEA(ea, size) & mask;
  uint32_t result = (dst | imm) & mask;
  UpdateFlagsLogic(state_, result, size);
  return WriteEA(ea, result, size);
}

SimResult M68kSimulator::OpOriToCcr(uint16_t /*opcode*/) {
  uint16_t imm = FetchWord() & 0xFF;
  state_.sr |= imm;
  IncrementCycles(kLogicImmToSrCycles);
  return SimResult::kOk;
}

SimResult M68kSimulator::OpOriToSr(uint16_t /*opcode*/) {
  if (!state_.IsSupervisor())
    return SimResult::kPrivilegeViolation;
  uint16_t imm = FetchWord();
  state_.sr |= imm;
  IncrementCycles(kLogicImmToSrCycles);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// AND — dst = dst & src, updates N/Z, clears V/C; X unchanged
// Opcode: 1100 DDD d ss MMMRRR
//   d=0: Dn=dst, <EA>=src   d=1: <EA>=dst, Dn=src
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpAnd(uint16_t opcode) {
  int dn = (opcode >> 9) & 0x7;
  bool dst_ea = (opcode & 0x0100) != 0;
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;
  uint32_t mask = SizeMask(size);

  EffectiveAddr ea = CalcEA(mode, reg, size);
  IncrementCycles(EaCyclePenalty(ea, size) + AndOrCycles(opcode, size));
  uint32_t ea_val = ReadEA(ea, size) & mask;
  uint32_t dn_val = state_.d[dn] & mask;

  uint32_t result = (dn_val & ea_val) & mask;
  UpdateFlagsLogic(state_, result, size);

  if (dst_ea) {
    return WriteEA(ea, result, size);
  }
  state_.d[dn] = (state_.d[dn] & ~mask) | result;
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// ANDI — EA = EA & imm; special cases: ANDI #imm,CCR  ANDI #imm,SR
// Opcode: 0000 0010 ss MMMRRR, followed by immediate word(s)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpAndi(uint16_t opcode) {
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;
  uint32_t mask = SizeMask(size);

  uint32_t imm = (size == DataSize::kLong) ? FetchLong() : (FetchWord() & mask);

  EffectiveAddr ea = CalcEA(mode, reg, size);
  IncrementCycles(EaCyclePenalty(ea, size) + LogicImmCycles(opcode, size));
  uint32_t dst = ReadEA(ea, size) & mask;
  uint32_t result = (dst & imm) & mask;
  UpdateFlagsLogic(state_, result, size);
  return WriteEA(ea, result, size);
}

SimResult M68kSimulator::OpAndiToCcr(uint16_t /*opcode*/) {
  uint16_t imm = FetchWord() & 0xFF;
  state_.sr = static_cast<uint16_t>((state_.sr & 0xFF00u) | (state_.sr & imm & 0xFFu));
  IncrementCycles(kLogicImmToSrCycles);
  return SimResult::kOk;
}

SimResult M68kSimulator::OpAndiToSr(uint16_t /*opcode*/) {
  if (!state_.IsSupervisor())
    return SimResult::kPrivilegeViolation;
  uint16_t imm = FetchWord();
  state_.sr &= imm;
  IncrementCycles(kLogicImmToSrCycles);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// EOR — EA = Dn ^ EA, updates N/Z, clears V/C; X unchanged
// Opcode: 1011 DDD 1 ss MMMRRR  (always <EA>=dst, Dn=src)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpEor(uint16_t opcode) {
  int dn = (opcode >> 9) & 0x7;
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;
  uint32_t mask = SizeMask(size);

  EffectiveAddr ea = CalcEA(mode, reg, size);
  // CODE6.CPP EOR: EA time + 4/8 (Dn) or 8/12 (memory) for B,W/L.
  IncrementCycles(EaCyclePenalty(ea, size));
  if (opcode & 0x0038)
    IncrementCycles(size == DataSize::kLong ? 12 : 8);
  else
    IncrementCycles(size == DataSize::kLong ? 8 : 4);
  uint32_t ea_val = ReadEA(ea, size) & mask;
  uint32_t dn_val = state_.d[dn] & mask;

  uint32_t result = (dn_val ^ ea_val) & mask;
  UpdateFlagsLogic(state_, result, size);
  return WriteEA(ea, result, size);
}

// ---------------------------------------------------------------------------
// EORI — EA = EA ^ imm; special cases: EORI #imm,CCR  EORI #imm,SR
// Opcode: 0000 1010 ss MMMRRR, followed by immediate word(s)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpEori(uint16_t opcode) {
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;
  uint32_t mask = SizeMask(size);

  uint32_t imm = (size == DataSize::kLong) ? FetchLong() : (FetchWord() & mask);

  EffectiveAddr ea = CalcEA(mode, reg, size);
  IncrementCycles(EaCyclePenalty(ea, size) + LogicImmCycles(opcode, size));
  uint32_t dst = ReadEA(ea, size) & mask;
  uint32_t result = (dst ^ imm) & mask;
  UpdateFlagsLogic(state_, result, size);
  return WriteEA(ea, result, size);
}

SimResult M68kSimulator::OpEoriToCcr(uint16_t /*opcode*/) {
  uint16_t imm = FetchWord() & 0xFF;
  state_.sr ^= imm;
  IncrementCycles(kLogicImmToSrCycles);
  return SimResult::kOk;
}

SimResult M68kSimulator::OpEoriToSr(uint16_t /*opcode*/) {
  if (!state_.IsSupervisor())
    return SimResult::kPrivilegeViolation;
  uint16_t imm = FetchWord();
  state_.sr ^= imm;
  IncrementCycles(kLogicImmToSrCycles);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// NOT — EA = ~EA, updates N/Z, clears V/C; X unchanged
// Opcode: 0100 0110 ss MMMRRR
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpNot(uint16_t opcode) {
  DataSize size = SizeFromSS((opcode >> 6) & 0x3);
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;
  uint32_t mask = SizeMask(size);

  EffectiveAddr ea = CalcEA(mode, reg, size);
  // CODE6.CPP NOT: EA time + 4/6 (Dn) or 8/12 (memory) for B,W/L.
  IncrementCycles(EaCyclePenalty(ea, size));
  if (opcode & 0x0030)
    IncrementCycles(size == DataSize::kLong ? 12 : 8);
  else
    IncrementCycles(size == DataSize::kLong ? 6 : 4);
  uint32_t val = ReadEA(ea, size) & mask;
  uint32_t result = (~val) & mask;
  UpdateFlagsLogic(state_, result, size);
  return WriteEA(ea, result, size);
}

// ---------------------------------------------------------------------------
// TAS — test byte EA and set bit 7; updates N/Z, clears V/C; X unchanged
// Opcode: 0100 1010 11 MMMRRR
// On real 68000 the read-modify-write is indivisible; we emulate the semantics.
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpTas(uint16_t opcode) {
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;

  EffectiveAddr ea = CalcEA(mode, reg, DataSize::kByte);
  // CODE7.CPP TAS: EA time + 4 (Dn) / 10 (memory).
  IncrementCycles(EaCyclePenalty(ea, DataSize::kByte) + ((opcode & 0x30) ? 10 : 4));
  uint32_t val = ReadEA(ea, DataSize::kByte) & kByteMask;
  UpdateFlagsLogic(state_, val, DataSize::kByte);
  return WriteEA(ea, val | 0x80u, DataSize::kByte);
}

}  // namespace easym68k::sim
