// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from EASy68K/Sim68K/RUN.CPP: eff_addr() / eff_addr_noread()

#include "easym68k/sim/addressing_mode.h"

namespace easym68k::sim {

namespace {

// Reads a word from memory at `pc` and advances pc by 2.
uint32_t FetchWord(const Memory& memory, uint32_t& pc) {
  uint32_t val = memory.ReadWord(pc).value;
  pc += 2;
  return val;
}

// Reads a longword from memory at `pc` and advances pc by 4.
uint32_t FetchLong(const Memory& memory, uint32_t& pc) {
  uint32_t val = memory.ReadLong(pc).value;
  pc += 4;
  return val;
}

// Byte increment for SP (A7) is always 2 to preserve even alignment.
uint32_t ByteInc(int reg) {
  return (reg == 7) ? 2u : 1u;
}

// Sign-extend a 16-bit value to 32 bits.
uint32_t SignExtWord(uint32_t val) {
  return static_cast<uint32_t>(static_cast<int32_t>(static_cast<int16_t>(val & 0xFFFF)));
}

// Sign-extend an 8-bit value to 32 bits.
int32_t SignExtByte(uint32_t val) {
  return static_cast<int8_t>(val & 0xFF);
}

}  // namespace

EffectiveAddr CalculateEA(Memory& memory, CpuState& state, uint32_t& pc, int mode, int reg,
                          DataSize size) {
  switch (mode) {
    case 0:  // Dn
      return MakeDataRegEa(reg);

    case 1:  // An  (byte size not permitted — caller validates)
      return MakeAddrRegEa(reg);

    case 2: {  // (An)
      uint32_t addr = state.GetAReg(reg) & kAddressMask;
      return MakeMemoryEa(addr, 2, static_cast<uint8_t>(reg));
    }

    case 3: {  // (An)+
      uint32_t inc = (size == DataSize::kLong) ? 4u : (size == DataSize::kWord) ? 2u : ByteInc(reg);
      uint32_t addr = state.GetAReg(reg) & kAddressMask;
      state.SetAReg(reg, state.GetAReg(reg) + inc);
      return MakeMemoryEa(addr, 3, static_cast<uint8_t>(reg));
    }

    case 4: {  // -(An)
      uint32_t dec = (size == DataSize::kLong) ? 4u : (size == DataSize::kWord) ? 2u : ByteInc(reg);
      state.SetAReg(reg, state.GetAReg(reg) - dec);
      uint32_t addr = state.GetAReg(reg) & kAddressMask;
      return MakeMemoryEa(addr, 4, static_cast<uint8_t>(reg));
    }

    case 5: {  // d(An)
      uint32_t raw = FetchWord(memory, pc);
      int32_t disp = static_cast<int16_t>(raw);
      uint32_t addr =
          static_cast<uint32_t>(static_cast<int32_t>(state.GetAReg(reg)) + disp) & kAddressMask;
      return MakeMemoryEa(addr, 5, static_cast<uint8_t>(reg));
    }

    case 6: {  // d(An,Xi)
      uint32_t ext = FetchWord(memory, pc);
      int32_t disp = SignExtByte(ext);
      int xi_reg = static_cast<int>((ext & 0x7000) >> 12);
      bool xi_is_addr = (ext & 0x8000) != 0;
      bool xi_is_long = (ext & 0x0800) != 0;
      int32_t ind;
      if (xi_is_addr)
        ind = static_cast<int32_t>(state.GetAReg(xi_reg));
      else
        ind = static_cast<int32_t>(state.d[xi_reg]);
      if (!xi_is_long)
        ind = static_cast<int16_t>(ind & 0xFFFF);  // word index — sign extend to 32 bits
      uint32_t addr = static_cast<uint32_t>(static_cast<int32_t>(state.GetAReg(reg)) + disp + ind) &
                      kAddressMask;
      return MakeMemoryEa(addr, 6, static_cast<uint8_t>(reg));
    }

    case 7: {
      switch (reg) {
        case 0: {  // xxx.W — sign-extend 16-bit absolute address
          uint32_t raw = FetchWord(memory, pc);
          uint32_t addr = SignExtWord(raw) & kAddressMask;
          return MakeMemoryEa(addr, 7, 0);
        }
        case 1: {  // xxx.L — 32-bit absolute address
          uint32_t addr = FetchLong(memory, pc) & kAddressMask;
          return MakeMemoryEa(addr, 7, 1);
        }
        case 2: {                // d(PC) — PC-relative word displacement
          uint32_t ext_pc = pc;  // PC points to extension word before fetch
          uint32_t raw = FetchWord(memory, pc);
          int32_t disp = static_cast<int16_t>(raw);
          uint32_t addr = static_cast<uint32_t>(static_cast<int32_t>(ext_pc) + disp) & kAddressMask;
          return MakeMemoryEa(addr, 7, 2);
        }
        case 3: {  // d(PC,Xi) — PC-relative indexed
          uint32_t ext_pc = pc;
          uint32_t ext = FetchWord(memory, pc);
          int32_t disp = SignExtByte(ext);
          int xi_reg = static_cast<int>((ext & 0x7000) >> 12);
          bool xi_is_addr = (ext & 0x8000) != 0;
          bool xi_is_long = (ext & 0x0800) != 0;
          int32_t ind;
          if (xi_is_addr)
            ind = static_cast<int32_t>(state.GetAReg(xi_reg));
          else
            ind = static_cast<int32_t>(state.d[xi_reg]);
          if (!xi_is_long)
            ind = static_cast<int16_t>(ind & 0xFFFF);
          uint32_t addr =
              static_cast<uint32_t>(static_cast<int32_t>(ext_pc) + disp + ind) & kAddressMask;
          return MakeMemoryEa(addr, 7, 3);
        }
        case 4: {  // #imm
          uint32_t val = (size == DataSize::kLong) ? FetchLong(memory, pc) : FetchWord(memory, pc);
          if (size == DataSize::kByte)
            val &= kByteMask;
          return MakeImmediateEa(val);
        }
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
  return MakeDataRegEa(0);  // unreachable for valid instructions
}

uint32_t ReadEA(const Memory& memory, const CpuState& state, const EffectiveAddr& ea,
                DataSize size) {
  switch (ea.target) {
    case EaTarget::kDataReg: {
      uint32_t val = state.d[ea.index];
      switch (size) {
        case DataSize::kByte:
          return val & kByteMask;
        case DataSize::kWord:
          return val & kWordMask;
        case DataSize::kLong:
          return val;
      }
      break;
    }
    case EaTarget::kAddrReg: {
      uint32_t val = state.GetAReg(ea.index);
      switch (size) {
        case DataSize::kByte:
          // Unreachable for valid instructions: the 68000 does not permit byte
          // operations on address registers (eff_addr() enforces size != BYTE_MASK
          // for mode 1). Handled defensively here in case of future misuse.
          return val & kByteMask;
        case DataSize::kWord:
          return val & kWordMask;
        case DataSize::kLong:
          return val;
      }
      break;
    }
    case EaTarget::kMemory:
      return memory.Read(ea.address, size).value;
    case EaTarget::kImmediate:
      return ea.address;  // immediate value stored in address field
  }
  return 0;
}

SimResult WriteEA(Memory& memory, CpuState& state, const EffectiveAddr& ea, uint32_t value,
                  DataSize size) {
  switch (ea.target) {
    case EaTarget::kDataReg: {
      switch (size) {
        case DataSize::kByte:
          state.d[ea.index] = (state.d[ea.index] & ~kByteMask) | (value & kByteMask);
          return SimResult::kOk;
        case DataSize::kWord:
          state.d[ea.index] = (state.d[ea.index] & ~kWordMask) | (value & kWordMask);
          return SimResult::kOk;
        case DataSize::kLong:
          state.d[ea.index] = value;
          return SimResult::kOk;
      }
      break;
    }
    case EaTarget::kAddrReg: {
      switch (size) {
        case DataSize::kByte:
          // Unreachable for valid instructions (same constraint as ReadEA kAddrReg+kByte).
          state.SetAReg(ea.index, static_cast<uint32_t>(static_cast<int8_t>(value)));
          return SimResult::kOk;
        case DataSize::kWord:
          // MOVEA.W sign-extends the word to fill the full 32-bit register
          state.SetAReg(ea.index, SignExtWord(value));
          return SimResult::kOk;
        case DataSize::kLong:
          state.SetAReg(ea.index, value);
          return SimResult::kOk;
      }
      break;
    }
    case EaTarget::kMemory:
      return memory.Write(ea.address, value, size);
    case EaTarget::kImmediate:
      return SimResult::kBadInstruction;
  }
  return SimResult::kBadInstruction;
}

}  // namespace easym68k::sim
