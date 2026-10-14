// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// 68020 bit field instructions (BFTST, BFEXTU, BFCHG, BFEXTS, BFCLR, BFFFO,
// BFSET, BFINS) — ported from Sim68K CODE7.CPP BIT_FIELD() (itself patterned
// after UAE). Enabled by Options > Enable Bit Field Instructions or the
// `*[sim68k]bitfield` listing command; otherwise the opcodes are illegal.

#include "easym68k/sim/cycle_data.h"
#include "easym68k/sim/simulator.h"

namespace easym68k::sim {

SimResult M68kSimulator::OpBitField(uint16_t opcode) {
  constexpr int kBftst = 0, kBfextu = 1, kBfchg = 2, kBfexts = 3, kBfclr = 4, kBfffo = 5;
  constexpr int kBfset = 6, kBfins = 7;

  if (!bitfield_enabled_.load(std::memory_order_relaxed))  // not enabled
    return SimResult::kBadInstruction;

  const uint32_t extra = FetchWord();
  uint32_t offset;
  if (extra & 0x800)
    offset = state_.d[(extra >> 6) & 0x07];
  else
    offset = (extra >> 6) & 0x1f;

  int width;
  if (extra & 0x20)
    width = static_cast<int>(((state_.d[extra & 7] - 1) & 0x1f) + 1);
  else
    width = static_cast<int>(((extra - 1) & 0x1f) + 1);

  const int mode = (opcode >> 3) & 0x7;
  const int reg = opcode & 0x7;
  uint32_t tmp;
  uint32_t bf0 = 0, bf1 = 0, bf4 = 0;
  uint32_t dest_addr = 0;
  int cycles;
  if (mode == 0) {  // Dn
    tmp = state_.d[reg] << (offset & 0x1f);
    cycles = 7;
  } else {
    // eff_addr(BYTE_MASK, CONTROL_ADDR, true): control modes only, with the
    // EA time added.
    const bool control = mode == 2 || mode == 5 || mode == 6 || (mode == 7 && reg <= 3);
    if (!control)
      return SimResult::kBadInstruction;
    EffectiveAddr ea = CalcEA(mode, reg, DataSize::kByte);
    IncrementCycles(EaCyclePenalty(ea, DataSize::kByte));
    // add the (signed) byte offset of the field
    dest_addr = ea.address + static_cast<uint32_t>(static_cast<int32_t>(offset) >> 3);
    uint32_t b[5];
    for (int i = 0; i < 5; ++i) {  // get 5 bytes from memory
      MemoryAccessResult r = memory_.ReadByte(dest_addr + static_cast<uint32_t>(i));
      if (memory_.HasFault())
        return SimResult::kOk;  // Step() raises the bus/address error
      b[i] = r.value & 0xFF;
    }
    bf1 = b[1];
    bf4 = b[4];
    bf0 = b[0] << 24 | b[1] << 16 | b[2] << 8 | b[3];
    // 32 bits of potential data (the original shifts the byte right by 8
    // when the offset is byte aligned, which yields 0)
    tmp = (bf0 << (offset & 7)) | ((offset & 7) ? (bf4 >> (8 - (offset & 7))) : 0);
    cycles = ((offset & 7) + static_cast<uint32_t>(width) > 32) ? 16 : 12;
  }
  if (width < 32)
    tmp >>= (32 - width);  // width determines how many bits to use
  const bool x_bit = state_.GetFlag(kSrExtend);
  bool n_bit = (tmp & (1u << (width - 1))) != 0;
  bool z_bit = tmp == 0;

  const int op = (opcode >> 8) & 0x7;
  auto pick = [&cycles](int c7, int c12, int c16) {
    cycles = cycles == 7 ? c7 : cycles == 12 ? c12 : c16;
  };
  switch (op) {
    case kBftst:
      break;
    case kBfextu:
      state_.d[(extra >> 12) & 7] = tmp;
      pick(8, 13, 18);
      break;
    case kBfchg:
      tmp = ~tmp;
      pick(12, 16, 24);
      break;
    case kBfexts:
      if (n_bit)
        tmp |= width == 32 ? 0 : (0xFFFFFFFFu << width);
      state_.d[(extra >> 12) & 7] = tmp;
      pick(8, 13, 18);
      break;
    case kBfclr:
      tmp = 0;
      pick(12, 16, 24);
      break;
    case kBfffo: {
      uint32_t mask = 1u << (width - 1);
      while (mask) {
        if (tmp & mask)
          break;
        mask >>= 1;
        offset++;
      }
      state_.d[(extra >> 12) & 7] = offset;
      pick(18, 24, 32);
      break;
    }
    case kBfset:
      tmp = 0xFFFFFFFFu;
      pick(12, 16, 24);
      break;
    case kBfins:
      tmp = state_.d[(extra >> 12) & 7];
      n_bit = (tmp & (1u << (width - 1))) != 0;
      z_bit = tmp == 0;
      pick(10, 15, 21);
      break;
  }
  IncrementCycles(cycles);

  // set SR according to results (X unchanged, V = C = 0)
  state_.SetFlag(kSrExtend, x_bit);
  state_.SetFlag(kSrNegative, n_bit);
  state_.SetFlag(kSrZero, z_bit);
  state_.SetFlag(kSrOverflow, false);
  state_.SetFlag(kSrCarry, false);

  if (op == kBfchg || op == kBfclr || op == kBfset || op == kBfins) {
    tmp = width == 32 ? tmp : tmp << (32 - width);
    const uint32_t off5 = offset & 0x1f;
    if (mode == 0) {  // ea is data register
      uint32_t d = state_.d[reg];
      state_.d[reg] = (d & (off5 == 0 ? 0 : (0xFFFFFFFFu << (32 - off5)))) | (tmp >> off5) |
                      ((off5 + static_cast<uint32_t>(width)) >= 32
                           ? 0
                           : (d & (0xFFFFFFFFu >> (off5 + static_cast<uint32_t>(width)))));
    } else {
      const uint32_t off3 = offset & 7;
      bf0 = (bf0 & (off3 == 0 ? 0 : (0xFF000000u << (8 - off3)))) | (tmp >> off3) |
            ((off3 + static_cast<uint32_t>(width)) >= 32
                 ? 0
                 : (bf0 & (0xFFFFFFFFu >> (off3 + static_cast<uint32_t>(width)))));
      memory_.WriteByte(dest_addr, static_cast<uint8_t>(bf0 >> 24));
      memory_.WriteByte(dest_addr + 1, static_cast<uint8_t>(bf0 >> 16));
      memory_.WriteByte(dest_addr + 2, static_cast<uint8_t>(bf0 >> 8));
      memory_.WriteByte(dest_addr + 3, static_cast<uint8_t>(bf0));
      if ((off3 + static_cast<uint32_t>(width)) > 32) {
        // The original computes the new fifth byte (bf4) but writes bf1, the
        // old second byte, to it (CODE7.CPP); kept for parity.
        (void)bf4;
        memory_.WriteByte(dest_addr + 4, static_cast<uint8_t>(bf1));
      }
    }
  }
  return SimResult::kOk;
}

}  // namespace easym68k::sim
