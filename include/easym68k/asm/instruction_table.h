// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// 68000 Instruction Table — encoding metadata for all standard instructions.

#ifndef EASYM68K_ASM_INSTRUCTION_TABLE_H_
#define EASYM68K_ASM_INSTRUCTION_TABLE_H_

#include <cstdint>
#include <string>
#include <unordered_map>

#include "easym68k/asm/lexer.h"
#include "easym68k/asm/parser.h"

namespace easym68k::asm_ {

// ---------------------------------------------------------------------------
// Size validity flags (OR together for multi-size instructions)
// ---------------------------------------------------------------------------
inline constexpr uint8_t kSzByte = 0x01;
inline constexpr uint8_t kSzWord = 0x02;
inline constexpr uint8_t kSzLong = 0x04;
inline constexpr uint8_t kSzBW = kSzByte | kSzWord;
inline constexpr uint8_t kSzWL = kSzWord | kSzLong;
inline constexpr uint8_t kSzBWL = kSzByte | kSzWord | kSzLong;
inline constexpr uint8_t kSzNone = 0x08;  // Instruction takes no size suffix

// ---------------------------------------------------------------------------
// Addressing mode masks (values match AddressMode enum from parser.h)
// ---------------------------------------------------------------------------
inline constexpr uint32_t kModeD = 0x001;       // Dn
inline constexpr uint32_t kModeA = 0x002;       // An
inline constexpr uint32_t kModeInd = 0x004;     // (An)
inline constexpr uint32_t kModePost = 0x008;    // (An)+
inline constexpr uint32_t kModePre = 0x010;     // -(An)
inline constexpr uint32_t kModeDisp = 0x020;    // d(An)
inline constexpr uint32_t kModeIdx = 0x040;     // d(An,Xi)
inline constexpr uint32_t kModeAbsW = 0x080;    // xxx.W
inline constexpr uint32_t kModeAbsL = 0x100;    // xxx.L
inline constexpr uint32_t kModePCDisp = 0x200;  // d(PC)
inline constexpr uint32_t kModePCIdx = 0x400;   // d(PC,Xi)
inline constexpr uint32_t kModeImm = 0x800;     // #imm
inline constexpr uint32_t kModeSR = 0x1000;     // SR
inline constexpr uint32_t kModeCCR = 0x2000;    // CCR
inline constexpr uint32_t kModeUSP = 0x4000;    // USP

// Compound mode groups
inline constexpr uint32_t kModeAbs = kModeAbsW | kModeAbsL;
inline constexpr uint32_t kModePCRel = kModePCDisp | kModePCIdx;
inline constexpr uint32_t kModeMem =
    kModeInd | kModePost | kModePre | kModeDisp | kModeIdx | kModeAbs;
inline constexpr uint32_t kModeDataAlt = kModeD | kModeMem;
inline constexpr uint32_t kModeData = kModeDataAlt | kModePCRel | kModeImm;
inline constexpr uint32_t kModeAll = kModeData | kModeA;
inline constexpr uint32_t kModeMemAlt = kModeMem;
inline constexpr uint32_t kModeAlt = kModeD | kModeA | kModeMem;
inline constexpr uint32_t kModeControl = kModeInd | kModeDisp | kModeIdx | kModeAbs | kModePCRel;

// ---------------------------------------------------------------------------
// Encoding type — controls which CodeGenerator handler processes the entry.
// ---------------------------------------------------------------------------
enum class InstrEncoding : uint8_t {
  kFixed,       // Fixed 1-word opcode (NOP, RTS, etc.)
  kFixedWord,   // Fixed opcode + 1 extension word (STOP)
  kRegOp,       // Single Dn operand (SWAP, EXT)
  kAnOp,        // Single An operand (UNLK)
  kEaOnly,      // Single EA operand (CLR, NEG, NOT, TST, TAS, NBCD, PEA)
  kEaBidirect,  // <ea>,Dn OR Dn,<ea> (ADD, SUB, AND, OR)
  kEaToReg,     // <ea>,Dn only (CMP)
  kRegToEa,     // Dn,<ea> only (EOR)
  kAddrEa,      // <ea>,An (ADDA, SUBA, CMPA)
  kMove,        // MOVE <ea>,<ea>
  kMoveA,       // MOVEA <ea>,An
  kMoveM,       // MOVEM <list>,<ea> / MOVEM <ea>,<list>
  kMoveP,       // MOVEP Dn,d(An) / MOVEP d(An),Dn
  kImmedToEa,   // #imm,<ea> (ADDI, SUBI, ANDI, ORI, EORI, CMPI)
  kImmedToCcr,  // #imm,CCR
  kImmedToSr,   // #imm,SR
  kQuick,       // #q(1-8),<ea> (ADDQ, SUBQ)
  kMoveq,       // MOVEQ #imm,Dn
  kBranch,      // Bcc (including BRA, BSR)
  kDBcc,        // DBcc Dn,<label>
  kScc,         // Scc <ea>
  kShift,       // Shift/rotate (Dn,Dn or #imm,Dn or <ea>)
  kBitReg,      // Bit ops with Dn count (BTST/BCHG/BCLR/BSET Dn,<ea>)
  kBitImm,      // Bit ops with immediate count
  kMulDiv,      // MULS/MULU/DIVS/DIVU <ea>,Dn
  kLea,         // LEA <ea>,An
  kJmpJsr,      // JMP/JSR <ea>
  kChk,         // CHK <ea>,Dn
  kMoveSr,      // MOVE SR,<ea> or MOVE <ea>,SR or MOVE <ea>,CCR
  kMoveUsp,     // MOVE USP,An or MOVE An,USP
  kLink,        // LINK An,#disp
  kTrap,        // TRAP #v
  kAddxSubx,    // ADDX/SUBX Dy,Dx or -(Ay),-(Ax)
  kAbcdSbcd,    // ABCD/SBCD Dy,Dx or -(Ay),-(Ax)
  kCmpm,        // CMPM (An)+,(An)+
  kExg,         // EXG Dx,Dy / Dx,Ay / Ax,Ay
};

// ---------------------------------------------------------------------------
// Instruction table entry
// ---------------------------------------------------------------------------
struct InstrEntry {
  const char* name;  // Uppercase mnemonic
  InstrEncoding encoding;
  uint8_t cond;        // Condition code (0-15) for Bcc/DBcc/Scc; 0 otherwise
  uint8_t sizes;       // OR of kSz* flags (kSzNone for unsized)
  uint8_t nops;        // Number of operands (0, 1, or 2)
  uint32_t src_modes;  // Valid source addressing modes (0 = not applicable)
  uint32_t dst_modes;  // Valid destination addressing modes (0 = not applicable)
  uint16_t base;       // Fixed portion of opcode (variable fields zeroed)
};

// ---------------------------------------------------------------------------
// InstructionTable class
// ---------------------------------------------------------------------------
class InstructionTable {
 public:
  InstructionTable();

  // Returns nullptr if not found.
  const InstrEntry* Lookup(const std::string& name) const;

  // Returns the number of 16-bit extension words required by an addressing
  // mode.  0 for Dn/An/(An)/(An)+/-(An); 1 for d(An)/d(An,Xi)/Abs.W/PC-rel;
  // 2 for Abs.L; 1 or 2 for #imm depending on size.
  static int ExtWordCount(AddressMode mode, SizeSpec size);

  // Maps SizeSpec to the 2-bit size field used in most 68000 opcodes.
  // B=0, W=1, L=2.  Returns -1 for kNone/kShort.
  static int SizeField(SizeSpec size);

 private:
  std::unordered_map<std::string, const InstrEntry*> index_;
};

}  // namespace easym68k::asm_

#endif  // EASYM68K_ASM_INSTRUCTION_TABLE_H_
