// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from EASy68K/Sim68K/UTILS.CPP: cc_update()

#include "easym68k/sim/flag_computation.h"

namespace easym68k::sim {

namespace {

// Returns the sign-bit mask for the given size.
uint32_t SignBit(DataSize size) {
  switch (size) {
    case DataSize::kByte:
      return 0x80u;
    case DataSize::kWord:
      return 0x8000u;
    case DataSize::kLong:
      return 0x80000000u;
  }
  return 0;
}

uint32_t SizeMask(DataSize size) {
  switch (size) {
    case DataSize::kByte:
      return kByteMask;
    case DataSize::kWord:
      return kWordMask;
    case DataSize::kLong:
      return kLongMask;
  }
  return 0;
}

}  // namespace

void UpdateFlagsAdd(CpuState& state, uint32_t src, uint32_t dst, uint32_t result, DataSize size) {
  uint32_t mask = SizeMask(size);
  uint32_t msb = SignBit(size);

  src &= mask;
  dst &= mask;
  result &= mask;

  bool sm = (src & msb) != 0;
  bool dm = (dst & msb) != 0;
  bool rm = (result & msb) != 0;

  bool n = rm;
  bool z = (result == 0);
  bool v = (sm && dm && !rm) || (!sm && !dm && rm);
  bool c = (sm && dm) || (!rm && dm) || (sm && !rm);  // carry out of MSB

  state.SetFlag(kSrNegative, n);
  state.SetFlag(kSrZero, z);
  state.SetFlag(kSrOverflow, v);
  state.SetFlag(kSrCarry, c);
  state.SetFlag(kSrExtend, c);
}

void UpdateFlagsSub(CpuState& state, uint32_t src, uint32_t dst, uint32_t result, DataSize size) {
  uint32_t mask = SizeMask(size);
  uint32_t msb = SignBit(size);

  src &= mask;
  dst &= mask;
  result &= mask;

  bool sm = (src & msb) != 0;
  bool dm = (dst & msb) != 0;
  bool rm = (result & msb) != 0;

  bool n = rm;
  bool z = (result == 0);
  bool v = (!sm && dm && !rm) || (sm && !dm && rm);
  bool c = (sm && !dm) || (rm && !dm) || (sm && rm);  // borrow

  state.SetFlag(kSrNegative, n);
  state.SetFlag(kSrZero, z);
  state.SetFlag(kSrOverflow, v);
  state.SetFlag(kSrCarry, c);
  state.SetFlag(kSrExtend, c);
}

// TODO: identical to UpdateFlagsMove — candidates for merging into a shared helper.
void UpdateFlagsLogic(CpuState& state, uint32_t result, DataSize size) {
  uint32_t mask = SizeMask(size);
  uint32_t msb = SignBit(size);
  result &= mask;

  state.SetFlag(kSrNegative, (result & msb) != 0);
  state.SetFlag(kSrZero, result == 0);
  state.SetFlag(kSrOverflow, false);
  state.SetFlag(kSrCarry, false);
  // X is not affected by logic operations
}

// TODO: identical to UpdateFlagsLogic — candidates for merging into a shared helper.
void UpdateFlagsMove(CpuState& state, uint32_t result, DataSize size) {
  uint32_t mask = SizeMask(size);
  uint32_t msb = SignBit(size);
  result &= mask;

  state.SetFlag(kSrNegative, (result & msb) != 0);
  state.SetFlag(kSrZero, result == 0);
  state.SetFlag(kSrOverflow, false);
  state.SetFlag(kSrCarry, false);
  // X is not affected by MOVE
}

void UpdateFlagsShift(CpuState& state, uint32_t result, DataSize size, bool carry, int count) {
  uint32_t mask = SizeMask(size);
  uint32_t msb = SignBit(size);
  result &= mask;

  state.SetFlag(kSrNegative, (result & msb) != 0);
  state.SetFlag(kSrZero, result == 0);
  state.SetFlag(kSrOverflow, false);
  state.SetFlag(kSrCarry, carry);
  if (count != 0)
    state.SetFlag(kSrExtend, carry);
}

}  // namespace easym68k::sim
