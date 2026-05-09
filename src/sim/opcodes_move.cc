// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Move instructions: MOVEA, MOVEM, MOVEP, MOVE_FR_SR, MOVE_TO_CCR, MOVE_TO_SR,
// MOVE_USP, EXG, LEA, PEA, LINK, UNLK, SWAP.
// Ported from EASy68K/Sim68K/CODE1.CPP and CODE2.CPP.

#include "easym68k/sim/addressing_mode.h"
#include "easym68k/sim/flag_computation.h"
#include "easym68k/sim/simulator.h"

namespace easym68k::sim {

// ---------------------------------------------------------------------------
// MOVEA.W / MOVEA.L — move to address register; no flag update
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpMovea(uint16_t opcode) {
  DataSize size = (opcode & 0x1000) ? DataSize::kWord : DataSize::kLong;
  int src_mode = (opcode >> 3) & 0x7;
  int src_reg = opcode & 0x7;
  int dst_reg = (opcode >> 9) & 0x7;

  EffectiveAddr src = CalcEA(src_mode, src_reg, size);
  uint32_t val = ReadEA(src, size);

  if (size == DataSize::kWord)
    val = static_cast<uint32_t>(static_cast<int16_t>(val & 0xFFFF));
  state_.SetAReg(dst_reg, val);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// MOVEM.W / MOVEM.L — move multiple registers
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpMovem(uint16_t opcode) {
  bool to_regs = (opcode & 0x0400) != 0;
  DataSize size = (opcode & 0x0040) ? DataSize::kLong : DataSize::kWord;
  int sz = (size == DataSize::kLong) ? 4 : 2;
  int mode = (opcode >> 3) & 0x7;
  int areg = opcode & 0x7;
  bool predecrement = (mode == 4);
  bool postincrement = (mode == 3);

  uint16_t mask = static_cast<uint16_t>(FetchWord());

  // Save An before any side effects for potential self-reference (predecrement case).
  uint32_t save_an = state_.GetAReg(areg);

  uint32_t addr;
  if (predecrement || postincrement) {
    addr = save_an & kAddressMask;
  } else {
    // Control addressing: use CalcEA to compute the base address (no register side effects).
    EffectiveAddr ea = CalcEA(mode, areg, size);
    addr = ea.address;
  }

  for (int counter = 0; counter < 16; counter++) {
    if (!(mask & (1u << counter)))
      continue;

    if (predecrement) {
      // Reversed register mapping: counter 0=A7, 7=A0, 8=D7, 15=D0
      addr = (addr - sz) & kAddressMask;
      uint32_t val;
      if (counter < 8) {
        int an_idx = 7 - counter;
        // If writing the An being used as base, write the saved (pre-decrement) value.
        val = (an_idx == areg) ? save_an : state_.GetAReg(an_idx);
      } else {
        val = state_.d[15 - counter];
      }
      if (size == DataSize::kLong)
        memory_.WriteLong(addr, val);
      else
        memory_.WriteWord(addr, static_cast<uint16_t>(val));
    } else if (to_regs) {
      // Normal register mapping: counter 0=D0, 7=D7, 8=A0, 15=A7
      uint32_t temp;
      if (size == DataSize::kLong) {
        temp = memory_.ReadLong(addr).value;
      } else {
        temp = static_cast<uint32_t>(static_cast<int16_t>(memory_.ReadWord(addr).value));
      }
      if (counter < 8)
        state_.d[counter] = temp;
      else
        state_.SetAReg(counter - 8, temp);
      addr = (addr + sz) & kAddressMask;
    } else {
      // Normal register mapping, registers → memory
      uint32_t val;
      if (counter < 8)
        val = state_.d[counter];
      else
        val = state_.GetAReg(counter - 8);
      if (size == DataSize::kLong)
        memory_.WriteLong(addr, val);
      else
        memory_.WriteWord(addr, static_cast<uint16_t>(val));
      addr = (addr + sz) & kAddressMask;
    }
  }

  // Update An for (An)+ and -(An); for other modes An is unchanged.
  // The final SetAReg overrides any loaded value when An is in the register list.
  if (predecrement || postincrement)
    state_.SetAReg(areg, addr);

  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// MOVEP.W / MOVEP.L — move peripheral data (alternating bytes)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpMovep(uint16_t opcode) {
  int dreg = (opcode >> 9) & 0x7;
  int areg = opcode & 0x7;
  bool dn_to_mem = (opcode & 0x0080) != 0;
  bool is_long = (opcode & 0x0040) != 0;
  int count = is_long ? 4 : 2;

  int16_t disp = static_cast<int16_t>(FetchWord());
  uint32_t address = (state_.GetAReg(areg) + static_cast<uint32_t>(disp)) & kAddressMask;

  if (dn_to_mem) {
    uint32_t dx = state_.d[dreg];
    for (int i = count; i > 0; i--) {
      memory_.WriteByte(address, static_cast<uint8_t>((dx >> (8 * (i - 1))) & 0xFF));
      address = (address + 2) & kAddressMask;
    }
  } else {
    uint32_t result = 0;
    for (int i = count; i > 0; i--) {
      result |= (memory_.ReadByte(address).value & 0xFF) << (8 * (i - 1));
      address = (address + 2) & kAddressMask;
    }
    if (is_long)
      state_.d[dreg] = result;
    else
      state_.d[dreg] = (state_.d[dreg] & 0xFFFF0000) | (result & 0xFFFF);
  }
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// MOVE from SR — writes SR to data-alterable EA
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpMoveFrSr(uint16_t opcode) {
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;
  EffectiveAddr ea = CalcEA(mode, reg, DataSize::kWord);
  return WriteEA(ea, state_.sr, DataSize::kWord);
}

// ---------------------------------------------------------------------------
// MOVE to CCR — writes lower byte of data source to CCR
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpMoveToCcr(uint16_t opcode) {
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;
  EffectiveAddr ea = CalcEA(mode, reg, DataSize::kWord);
  uint32_t val = ReadEA(ea, DataSize::kWord);
  state_.sr = static_cast<uint16_t>((state_.sr & 0xFF00) | (val & 0x00FF));
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// MOVE to SR — writes word to full SR (privileged)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpMoveToSr(uint16_t opcode) {
  if (!state_.IsSupervisor())
    return SimResult::kPrivilegeViolation;
  int mode = (opcode >> 3) & 0x7;
  int reg = opcode & 0x7;
  EffectiveAddr ea = CalcEA(mode, reg, DataSize::kWord);
  uint32_t val = ReadEA(ea, DataSize::kWord);
  state_.sr = static_cast<uint16_t>(val);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// MOVE USP — exchange between USP (a[7]) and An (privileged)
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpMoveUsp(uint16_t opcode) {
  if (!state_.IsSupervisor())
    return SimResult::kPrivilegeViolation;
  int reg = opcode & 0x7;
  if (opcode & 0x8) {
    // An ← USP: load user SP (a[7]) into address register
    state_.SetAReg(reg, state_.a[7]);
  } else {
    // USP ← An: store address register into user SP (a[7])
    state_.a[7] = state_.GetAReg(reg);
  }
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// EXG — exchange registers
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpExg(uint16_t opcode) {
  int rx = (opcode >> 9) & 0x7;
  int ry = opcode & 0x7;
  switch ((opcode >> 3) & 0x1F) {
    case 0x08: {
      uint32_t t = state_.d[rx];
      state_.d[rx] = state_.d[ry];
      state_.d[ry] = t;
      break;
    }
    case 0x09: {
      uint32_t t = state_.GetAReg(rx);
      state_.SetAReg(rx, state_.GetAReg(ry));
      state_.SetAReg(ry, t);
      break;
    }
    case 0x11: {
      uint32_t t = state_.d[rx];
      state_.d[rx] = state_.GetAReg(ry);
      state_.SetAReg(ry, t);
      break;
    }
    default:
      return SimResult::kBadInstruction;
  }
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// LEA — load effective address into An
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpLea(uint16_t opcode) {
  int dst_reg = (opcode >> 9) & 0x7;
  int src_mode = (opcode >> 3) & 0x7;
  int src_reg = opcode & 0x7;
  // Use kByte to avoid even-address requirement on the EA calculation itself.
  EffectiveAddr ea = CalcEA(src_mode, src_reg, DataSize::kByte);
  state_.SetAReg(dst_reg, ea.address);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// PEA — push effective address
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpPea(uint16_t opcode) {
  int src_mode = (opcode >> 3) & 0x7;
  int src_reg = opcode & 0x7;
  EffectiveAddr ea = CalcEA(src_mode, src_reg, DataSize::kByte);

  uint32_t sp = state_.GetSP() - 4;
  state_.SetSP(sp);
  memory_.WriteLong(sp & kAddressMask, ea.address);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// LINK — allocate stack frame
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpLink(uint16_t opcode) {
  int reg = opcode & 0x7;
  int32_t disp = static_cast<int16_t>(FetchWord());

  uint32_t sp = state_.GetSP() - 4;
  state_.SetSP(sp);
  memory_.WriteLong(sp & kAddressMask, state_.a[reg]);  // push raw a[reg]
  state_.a[reg] = sp;                                   // set raw a[reg] = SP
  state_.SetSP(sp + static_cast<uint32_t>(disp));
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// UNLK — deallocate stack frame
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpUnlk(uint16_t opcode) {
  int reg = opcode & 0x7;
  state_.SetSP(state_.a[reg]);
  state_.a[reg] = memory_.ReadLong(state_.GetSP() & kAddressMask).value;
  state_.SetSP(state_.GetSP() + 4);
  return SimResult::kOk;
}

// ---------------------------------------------------------------------------
// SWAP — swap 16-bit halves of a data register
// ---------------------------------------------------------------------------

SimResult M68kSimulator::OpSwap(uint16_t opcode) {
  int reg = opcode & 0x7;
  uint32_t val = state_.d[reg];
  val = ((val & 0x0000FFFF) << 16) | ((val & 0xFFFF0000) >> 16);
  state_.d[reg] = val;
  UpdateFlagsMove(state_, val, DataSize::kLong);
  return SimResult::kOk;
}

}  // namespace easym68k::sim
