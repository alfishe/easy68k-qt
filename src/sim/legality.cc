// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Opcode legality for the 68000 (plus the optional 68020 bit field group).
//
// The original decides legality in two places: the decode table (OPCODES.H
// inst_arr, first match wins) and the addressing-mode masks each instruction
// passes to eff_addr() (RUN.CPP; e.g. DATA_ALT_ADDR, An never with byte
// size). Doing that per executed instruction is replaced by a table built
// once: one lookup per instruction, and an illegal encoding is rejected
// before any operand is fetched, as on the 68000 (the original may advance
// (An)+ / the PC and add EA cycles before noticing).

#include <array>

#include "easym68k/sim/decode.h"

namespace easym68k::sim {
namespace {

// Effective address classes, as the bit numbers of the original's masks
// (def.h bit_1..bit_12): Dn, An, (An), (An)+, -(An), d16(An), d8(An,Xn),
// abs.W, abs.L, d16(PC), d8(PC,Xn), #imm.
enum : uint16_t {
  kEaDn = 1 << 0,
  kEaAn = 1 << 1,
  kEaInd = 1 << 2,
  kEaPostInc = 1 << 3,
  kEaPreDec = 1 << 4,
  kEaDisp = 1 << 5,
  kEaIndex = 1 << 6,
  kEaAbsW = 1 << 7,
  kEaAbsL = 1 << 8,
  kEaPcDisp = 1 << 9,
  kEaPcIndex = 1 << 10,
  kEaImm = 1 << 11,
};
constexpr uint16_t kAll = 0x0FFF;                                      // ALL_ADDR
constexpr uint16_t kData = kAll & ~kEaAn;                              // DATA_ADDR
constexpr uint16_t kMemory = kData & ~kEaDn;                           // MEMORY_ADDR
constexpr uint16_t kControl = kEaInd | kEaDisp | kEaIndex | kEaAbsW |  // CONTROL_ADDR
                              kEaAbsL | kEaPcDisp | kEaPcIndex;
constexpr uint16_t kAlterable = 0x01FF;  // ALTERABLE_ADDR
constexpr uint16_t kDataAlt = kData & kAlterable;
constexpr uint16_t kMemAlt = kMemory & kAlterable;
constexpr uint16_t kControlAlt = kControl & kAlterable;

enum class Size { kByte, kWord, kLong, kBad };

// EA class bit of a mode/register pair; 0 for the unused mode 7 encodings.
uint16_t EaBit(int mode, int reg) {
  if (mode < 7)
    return static_cast<uint16_t>(1u << mode);
  return reg <= 4 ? static_cast<uint16_t>(1u << (7 + reg)) : 0;
}

// eff_addr(): the mode must be in `mask`; An is never legal with byte size.
bool EaOk(int mode, int reg, uint16_t mask, Size size = Size::kWord) {
  uint16_t bit = EaBit(mode, reg);
  if (bit == 0 || !(mask & bit))
    return false;
  return bit != kEaAn || size != Size::kByte;
}

bool SrcEaOk(uint16_t op, uint16_t mask, Size size = Size::kWord) {
  return EaOk((op >> 3) & 7, op & 7, mask, size);
}

// decode_size(): bits 7-6, 11 is invalid.
Size StdSize(uint16_t op) {
  switch ((op >> 6) & 3) {
    case 0:
      return Size::kByte;
    case 1:
      return Size::kWord;
    case 2:
      return Size::kLong;
    default:
      return Size::kBad;
  }
}

bool Group0(uint16_t op) {
  switch (op) {
    case 0x003C:  // ORI to CCR
    case 0x007C:  // ORI to SR
    case 0x023C:  // ANDI to CCR
    case 0x027C:  // ANDI to SR
    case 0x0A3C:  // EORI to CCR
    case 0x0A7C:  // EORI to SR
      return true;
    default:
      break;
  }
  int mode = (op >> 3) & 7;
  int kind = (op >> 6) & 3;  // BTST, BCHG, BCLR, BSET
  if (op & 0x0100) {
    if (mode == 1)
      return true;  // MOVEP
    // Dynamic bit operation: BTST Dn,<ea> may read any data operand,
    // including #imm; the others need a data alterable destination.
    return SrcEaOk(op, kind == 0 ? kData : kDataAlt);
  }
  switch ((op >> 9) & 7) {
    case 4:  // static bit operation #n,<ea> (BTST #n,#imm accepted, as in
             // the original assembler and simulator)
      return SrcEaOk(op, kind == 0 ? kData : kDataAlt);
    case 0:  // ORI
    case 1:  // ANDI
    case 2:  // SUBI
    case 3:  // ADDI
    case 5:  // EORI
    case 6:  // CMPI (data alterable on the 68000)
    {
      Size size = StdSize(op);
      return size != Size::kBad && SrcEaOk(op, kDataAlt, size);
    }
    default:  // 0x0E00: MOVES (68010+)
      return false;
  }
}

bool Move(uint16_t op) {
  Size size = Size::kLong;
  switch ((op >> 12) & 3) {
    case 1:
      size = Size::kByte;
      break;
    case 3:
      size = Size::kWord;
      break;
    default:
      size = Size::kLong;
      break;
  }
  int dst_mode = (op >> 6) & 7;
  int dst_reg = (op >> 9) & 7;
  if (!SrcEaOk(op, kAll, size))
    return false;
  if (dst_mode == 1)
    return size != Size::kByte;  // MOVEA.W/.L
  return EaOk(dst_mode, dst_reg, kDataAlt, size);
}

bool Group4(uint16_t op) {
  int mode = (op >> 3) & 7;
  if (op & 0x0100) {
    switch ((op >> 6) & 3) {
      case 2:  // CHK.W
        return SrcEaOk(op, kData);
      case 3:  // LEA
        return SrcEaOk(op, kControl);
      default:  // CHK.L (68020)
        return false;
    }
  }
  switch ((op >> 8) & 0xF) {
    case 0x0:  // NEGX / MOVE from SR
    case 0x2:  // CLR / (68010 MOVE from CCR)
    case 0x4:  // NEG / MOVE to CCR
    case 0x6:  // NOT / MOVE to SR
    {
      Size size = StdSize(op);
      if (size == Size::kBad) {
        switch ((op >> 8) & 0xF) {
          case 0x0:
            return SrcEaOk(op, kDataAlt);  // MOVE from SR
          case 0x4:
          case 0x6:
            return SrcEaOk(op, kData);  // MOVE to CCR / SR
          default:
            return false;
        }
      }
      return SrcEaOk(op, kDataAlt, size);
    }
    case 0x8:
      switch ((op >> 6) & 3) {
        case 0:  // NBCD
          return SrcEaOk(op, kDataAlt, Size::kByte);
        case 1:  // SWAP / PEA (mode 1: BKPT, 68020)
          if (mode == 0)
            return true;
          return SrcEaOk(op, kControl);
        default:  // EXT / MOVEM registers to memory
          if (mode == 0)
            return true;
          return SrcEaOk(op, kControlAlt | kEaPreDec);
      }
    case 0xA:
      if (((op >> 6) & 3) == 3)
        return SrcEaOk(op, kDataAlt, Size::kByte);  // TAS (TAS #imm is ILLEGAL $4AFC)
      {
        Size size = StdSize(op);
        return SrcEaOk(op, kDataAlt, size);  // TST
      }
    case 0xC:  // MOVEM memory to registers (bits 7-6 = 1x); MULL/DIVL 68020
      if (!(op & 0x0080))
        return false;
      return SrcEaOk(op, kControl | kEaPostInc);
    case 0xE:
      if (op >= 0x4E80)
        return SrcEaOk(op, kControl);  // JSR / JMP
      if (op >= 0x4E40 && op <= 0x4E73)
        return true;                        // TRAP, LINK, UNLK, MOVE USP, RESET, NOP, STOP, RTE
      return op >= 0x4E75 && op <= 0x4E77;  // RTS, TRAPV, RTR (RTD/MOVEC: 68010)
    default:
      return false;
  }
}

bool Group5(uint16_t op) {
  int mode = (op >> 3) & 7;
  if (((op >> 6) & 3) == 3) {
    if (mode == 1)
      return true;                              // DBcc
    return SrcEaOk(op, kDataAlt, Size::kByte);  // Scc
  }
  Size size = StdSize(op);
  return SrcEaOk(op, kAlterable, size);  // ADDQ / SUBQ
}

// OR / SUB / AND / ADD style groups: bits 8-6 opmode.
bool ArithGroup(uint16_t op, bool is_and_or) {
  int opmode = (op >> 6) & 7;
  Size size = StdSize(op);
  if (opmode == 3 || opmode == 7) {
    return false;  // handled by the caller (DIV/MUL/ADDA/SUBA)
  }
  if (opmode < 3)  // <ea>,Dn
    return SrcEaOk(op, is_and_or ? kData : kAll, size);
  // Dn,<ea>: memory alterable (the Dn/An modes are ADDX/SUBX/ABCD/SBCD/EXG,
  // decided by the caller).
  return SrcEaOk(op, kMemAlt, size);
}

bool Group8(uint16_t op) {  // OR, DIVU, DIVS, SBCD
  int mode = (op >> 3) & 7;
  int opmode = (op >> 6) & 7;
  if (opmode == 3 || opmode == 7)
    return SrcEaOk(op, kData);  // DIVU / DIVS
  if (opmode == 4 && mode < 2)
    return true;                // SBCD Dy,Dx / -(Ay),-(Ax)
  return ArithGroup(op, true);  // PACK/UNPK (68020) fall out as illegal
}

bool AddSub(uint16_t op) {  // SUB/SUBA/SUBX, ADD/ADDA/ADDX
  int mode = (op >> 3) & 7;
  int opmode = (op >> 6) & 7;
  if (opmode == 3 || opmode == 7)
    return SrcEaOk(op, kAll);  // ADDA / SUBA (.W / .L)
  if (opmode >= 4 && mode < 2)
    return true;  // ADDX / SUBX
  return ArithGroup(op, false);
}

bool GroupB(uint16_t op) {  // CMP, CMPA, CMPM, EOR
  int mode = (op >> 3) & 7;
  int opmode = (op >> 6) & 7;
  if (opmode == 3 || opmode == 7)
    return SrcEaOk(op, kAll);  // CMPA
  Size size = StdSize(op);
  if (opmode < 3)
    return SrcEaOk(op, kAll, size);  // CMP
  if (mode == 1)
    return true;                       // CMPM
  return SrcEaOk(op, kDataAlt, size);  // EOR
}

bool GroupC(uint16_t op) {  // AND, MULU, MULS, ABCD, EXG
  int mode = (op >> 3) & 7;
  int opmode = (op >> 6) & 7;
  if (opmode == 3 || opmode == 7)
    return SrcEaOk(op, kData);  // MULU / MULS
  if (opmode == 4 && mode < 2)
    return true;  // ABCD
  if (opmode == 5 && mode < 2)
    return true;  // EXG Dx,Dy / Ax,Ay
  if (opmode == 6 && mode == 1)
    return true;  // EXG Dx,Ay
  if (opmode >= 4 && mode < 2)
    return false;
  return ArithGroup(op, true);
}

bool GroupE(uint16_t op, bool bitfield) {
  if (((op >> 6) & 3) != 3)
    return true;  // register shifts and rotates
  if (!(op & 0x0800))
    return SrcEaOk(op, kMemAlt);  // memory shift/rotate, one bit
  if (!bitfield)
    return false;
  // 68020 bit field: Dn or a control (alterable for the modifying ones) EA.
  int mode = (op >> 3) & 7;
  if (mode == 0)
    return true;
  switch ((op >> 8) & 7) {
    case 0:  // BFTST
    case 1:  // BFEXTU
    case 3:  // BFEXTS
    case 5:  // BFFFO
      return SrcEaOk(op, kControl);
    default:  // BFCHG, BFCLR, BFSET, BFINS
      return SrcEaOk(op, kControlAlt);
  }
}

bool ComputeLegal(uint16_t op, bool bitfield) {
  switch (op >> 12) {
    case 0x0:
      return Group0(op);
    case 0x1:
    case 0x2:
    case 0x3:
      return Move(op);
    case 0x4:
      return Group4(op);
    case 0x5:
      return Group5(op);
    case 0x6:
      return true;  // Bcc / BRA / BSR
    case 0x7:
      return !(op & 0x0100);  // MOVEQ
    case 0x8:
      return Group8(op);
    case 0x9:
    case 0xD:
      return AddSub(op);
    case 0xB:
      return GroupB(op);
    case 0xC:
      return GroupC(op);
    case 0xE:
      return GroupE(op, bitfield);
    default:
      return true;  // Line A / Line F: their own exceptions
  }
}

struct LegalityTables {
  std::array<uint8_t, 65536> legal[2]{};
  LegalityTables() {
    for (int bf = 0; bf < 2; bf++)
      for (uint32_t op = 0; op < 65536; op++)
        legal[bf][op] = ComputeLegal(static_cast<uint16_t>(op), bf != 0) ? 1 : 0;
  }
};

}  // namespace

bool IsLegalOpcode(uint16_t opcode, bool bitfield_enabled) {
  static const LegalityTables tables;
  return tables.legal[bitfield_enabled ? 1 : 0][opcode] != 0;
}

}  // namespace easym68k::sim
