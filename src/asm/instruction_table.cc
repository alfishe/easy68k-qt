// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Ported from Edit68K/INSTTABL.CPP and Edit68K/INSTLOOK.CPP (Paul McKee /
// Charles Kelly).  Each row in kInstrTable corresponds to one InstrEntry;
// multi-flavor instructions (ADD, AND, etc.) are collapsed into a single row
// whose code-generator handler (CodeGenerator::Encode) performs internal
// flavor selection based on the operand addressing modes at assembly time.
//
// base field convention: byte-size base opcode with all variable fields
// (register, EA, size) zeroed, matching the original bytemask.  For
// instructions with no byte form (e.g. ADDA, EXT) the wordmask is used.
// Shift/rotate instructions use the word-size register-count form so that
// the direction and shift-type bits are visible to the code generator.

#include "easym68k/asm/instruction_table.h"

#include "easym68k/asm/lexer.h"
#include "easym68k/asm/parser.h"

namespace easym68k::asm_ {

using E = InstrEncoding;

// ---------------------------------------------------------------------------
// Convenience aliases matching original asm.h macro definitions
// ---------------------------------------------------------------------------
static constexpr uint32_t Data = kModeD | kModeInd | kModePost | kModePre | kModeDisp | kModeIdx |
                                 kModeAbs | kModePCRel | kModeImm;
static constexpr uint32_t Memory =
    kModeInd | kModePost | kModePre | kModeDisp | kModeIdx | kModeAbs | kModePCRel | kModeImm;
static constexpr uint32_t Control = kModeInd | kModeDisp | kModeIdx | kModeAbs | kModePCRel;
static constexpr uint32_t Alter =
    kModeD | kModeInd | kModePost | kModePre | kModeDisp | kModeIdx | kModeAbs;
static constexpr uint32_t All = Data | kModeA;
static constexpr uint32_t DataAlt = Data & Alter;
static constexpr uint32_t MemAlt = Memory & Alter;

// ---------------------------------------------------------------------------
// Instruction table — alphabetical order (matches instTable[] in INSTTABL.CPP)
// ---------------------------------------------------------------------------
// clang-format off
static const InstrEntry kInstrTable[] = {
  // name       encoding         cond  sizes      nops src_modes            dst_modes           base
  {"ABCD",   E::kAbcdSbcd,    0,  kSzByte,  2, kModeD|kModePre,      kModeD|kModePre,       0xC100},
  {"ADD",    E::kEaBidirect,  0,  kSzBWL,   2, All,                  DataAlt|kModeA,         0xD000},
  {"ADDA",   E::kAddrEa,      0,  kSzWL,    2, All,                  kModeA,                 0xD0C0},
  {"ADDI",   E::kImmedToEa,   0,  kSzBWL,   2, kModeImm,             DataAlt,                0x0600},
  {"ADDQ",   E::kQuick,       0,  kSzBWL,   2, kModeImm,             DataAlt|kModeA,         0x5000},
  {"ADDX",   E::kAddxSubx,    0,  kSzBWL,   2, kModeD|kModePre,      kModeD|kModePre,        0xD100},
  // AND: covers Data,Dn / Dn,MemAlt / #imm,DataAlt / #imm,CCR / #imm,SR
  {"AND",    E::kEaBidirect,  0,  kSzBWL,   2, Data|kModeImm,        DataAlt|kModeCCR|kModeSR, 0xC000},
  {"ANDI",   E::kImmedToEa,   0,  kSzBWL,   2, kModeImm,             DataAlt|kModeCCR|kModeSR, 0x0200},
  {"ASL",    E::kShift,       0,  kSzBWL,   2, MemAlt|kModeD|kModeImm, kModeD,               0xE160},
  {"ASR",    E::kShift,       0,  kSzBWL,   2, MemAlt|kModeD|kModeImm, kModeD,               0xE060},
  // Bcc — cond encodes the condition code (bits 11-8 of the opcode)
  {"BCC",    E::kBranch,      4,  kSzByte|kSzWord, 1, kModeAbs,      0,                      0x6400},
  {"BCHG",   E::kBitReg,      0,  kSzByte|kSzLong, 2, kModeD|kModeImm, Memory|kModeD,       0x0140},
  {"BCLR",   E::kBitReg,      0,  kSzByte|kSzLong, 2, kModeD|kModeImm, Memory|kModeD,       0x0180},
  {"BCS",    E::kBranch,      5,  kSzByte|kSzWord, 1, kModeAbs,      0,                      0x6500},
  {"BEQ",    E::kBranch,      7,  kSzByte|kSzWord, 1, kModeAbs,      0,                      0x6700},
  // 68020 bit-field instructions (BITflag gated in original; omitted here)
  {"BGE",    E::kBranch,     12,  kSzByte|kSzWord, 1, kModeAbs,      0,                      0x6C00},
  {"BGT",    E::kBranch,     14,  kSzByte|kSzWord, 1, kModeAbs,      0,                      0x6E00},
  {"BHI",    E::kBranch,      2,  kSzByte|kSzWord, 1, kModeAbs,      0,                      0x6200},
  {"BHS",    E::kBranch,      4,  kSzByte|kSzWord, 1, kModeAbs,      0,                      0x6400},
  {"BLE",    E::kBranch,     15,  kSzByte|kSzWord, 1, kModeAbs,      0,                      0x6F00},
  {"BLO",    E::kBranch,      5,  kSzByte|kSzWord, 1, kModeAbs,      0,                      0x6500},
  {"BLS",    E::kBranch,      3,  kSzByte|kSzWord, 1, kModeAbs,      0,                      0x6300},
  {"BLT",    E::kBranch,     13,  kSzByte|kSzWord, 1, kModeAbs,      0,                      0x6D00},
  {"BMI",    E::kBranch,     11,  kSzByte|kSzWord, 1, kModeAbs,      0,                      0x6B00},
  {"BNE",    E::kBranch,      6,  kSzByte|kSzWord, 1, kModeAbs,      0,                      0x6600},
  {"BPL",    E::kBranch,     10,  kSzByte|kSzWord, 1, kModeAbs,      0,                      0x6A00},
  {"BRA",    E::kBranch,      0,  kSzByte|kSzWord, 1, kModeAbs,      0,                      0x6000},
  {"BSET",   E::kBitReg,      0,  kSzByte|kSzLong, 2, kModeD|kModeImm, Memory|kModeD,       0x01C0},
  {"BSR",    E::kBranch,      0,  kSzByte|kSzWord, 1, kModeAbs,      0,                      0x6100},
  {"BTST",   E::kBitReg,      0,  kSzByte|kSzLong, 2, kModeD|kModeImm, Memory|kModeD,       0x0100},
  {"BVC",    E::kBranch,      8,  kSzByte|kSzWord, 1, kModeAbs,      0,                      0x6800},
  {"BVS",    E::kBranch,      9,  kSzByte|kSzWord, 1, kModeAbs,      0,                      0x6900},
  {"CHK",    E::kChk,         0,  kSzWord,  2, Data,                 kModeD,                 0x4180},
  {"CLR",    E::kEaOnly,      0,  kSzBWL,   1, DataAlt,              0,                      0x4200},
  {"CMP",    E::kEaToReg,     0,  kSzBWL,   2, All,                  kModeD,                 0xB000},
  {"CMPA",   E::kAddrEa,      0,  kSzWL,    2, All,                  kModeA,                 0xB0C0},
  {"CMPI",   E::kImmedToEa,   0,  kSzBWL,   2, kModeImm,             DataAlt,                0x0C00},
  {"CMPM",   E::kCmpm,        0,  kSzBWL,   2, kModePost,            kModePost,              0xB108},
  // DBcc — cond encodes the condition code
  {"DBCC",   E::kDBcc,        4,  kSzWord,  2, kModeD,               kModeAbs,               0x54C8},
  {"DBCS",   E::kDBcc,        5,  kSzWord,  2, kModeD,               kModeAbs,               0x55C8},
  {"DBEQ",   E::kDBcc,        7,  kSzWord,  2, kModeD,               kModeAbs,               0x57C8},
  {"DBF",    E::kDBcc,        1,  kSzWord,  2, kModeD,               kModeAbs,               0x51C8},
  {"DBGE",   E::kDBcc,       12,  kSzWord,  2, kModeD,               kModeAbs,               0x5CC8},
  {"DBGT",   E::kDBcc,       14,  kSzWord,  2, kModeD,               kModeAbs,               0x5EC8},
  {"DBHI",   E::kDBcc,        2,  kSzWord,  2, kModeD,               kModeAbs,               0x52C8},
  {"DBHS",   E::kDBcc,        4,  kSzWord,  2, kModeD,               kModeAbs,               0x54C8},
  {"DBLE",   E::kDBcc,       15,  kSzWord,  2, kModeD,               kModeAbs,               0x5FC8},
  {"DBLO",   E::kDBcc,        5,  kSzWord,  2, kModeD,               kModeAbs,               0x55C8},
  {"DBLS",   E::kDBcc,        3,  kSzWord,  2, kModeD,               kModeAbs,               0x53C8},
  {"DBLT",   E::kDBcc,       13,  kSzWord,  2, kModeD,               kModeAbs,               0x5DC8},
  {"DBMI",   E::kDBcc,       11,  kSzWord,  2, kModeD,               kModeAbs,               0x5BC8},
  {"DBNE",   E::kDBcc,        6,  kSzWord,  2, kModeD,               kModeAbs,               0x56C8},
  {"DBPL",   E::kDBcc,       10,  kSzWord,  2, kModeD,               kModeAbs,               0x5AC8},
  {"DBRA",   E::kDBcc,        1,  kSzWord,  2, kModeD,               kModeAbs,               0x51C8},
  {"DBT",    E::kDBcc,        0,  kSzWord,  2, kModeD,               kModeAbs,               0x50C8},
  {"DBVC",   E::kDBcc,        8,  kSzWord,  2, kModeD,               kModeAbs,               0x58C8},
  {"DBVS",   E::kDBcc,        9,  kSzWord,  2, kModeD,               kModeAbs,               0x59C8},
  {"DIVS",   E::kMulDiv,      0,  kSzWord,  2, Data,                 kModeD,                 0x81C0},
  {"DIVU",   E::kMulDiv,      0,  kSzWord,  2, Data,                 kModeD,                 0x80C0},
  // EOR: only Dn,DataAlt (no reversed form); EORI handles immediate
  {"EOR",    E::kRegToEa,     0,  kSzBWL,   2, kModeD,               DataAlt,                0xB100},
  {"EORI",   E::kImmedToEa,   0,  kSzBWL,   2, kModeImm,             DataAlt|kModeCCR|kModeSR, 0x0A00},
  // EXG: three sub-forms (Dx,Dy / Ax,Ay / Dx,Ay) selected by code gen
  {"EXG",    E::kExg,         0,  kSzLong,  2, kModeD|kModeA,        kModeD|kModeA,          0xC140},
  // EXT: word (0x4880) or long (0x48C0); size bit at bit 6
  {"EXT",    E::kRegOp,       0,  kSzWL,    1, kModeD,               0,                      0x4880},
  {"ILLEGAL",E::kFixed,       0,  kSzNone,  0, 0,                    0,                      0x4AFC},
  {"JMP",    E::kJmpJsr,      0,  kSzNone,  1, Control,              0,                      0x4EC0},
  {"JSR",    E::kJmpJsr,      0,  kSzNone,  1, Control,              0,                      0x4E80},
  {"LEA",    E::kLea,         0,  kSzLong,  2, Control,              kModeA,                 0x41C0},
  {"LINK",   E::kLink,        0,  kSzNone,  2, kModeA,               kModeImm,               0x4E50},
  {"LSL",    E::kShift,       0,  kSzBWL,   2, MemAlt|kModeD|kModeImm, kModeD,               0xE168},
  {"LSR",    E::kShift,       0,  kSzBWL,   2, MemAlt|kModeD|kModeImm, kModeD,               0xE068},
  // MOVE: covers Data/An→DataAlt, All→An (MOVEA), Data→CCR, Data→SR, SR→DataAlt,
  //       An↔USP; code gen dispatches on dst mode
  {"MOVE",   E::kMove,        0,  kSzBWL,   2, All,
             DataAlt|kModeA|kModeCCR|kModeSR|kModeUSP,               0x1000},
  {"MOVEA",  E::kMoveA,       0,  kSzWL,    2, All,                  kModeA,                 0x3000},
  // MOVEM: register list + EA; code gen handles parseFlag=false logic
  {"MOVEM",  E::kMoveM,       0,  kSzWL,    2, 0,                    0,                      0x4880},
  // MOVEP: Dn↔d(An), four sub-forms; base = Dn→memory word form
  {"MOVEP",  E::kMoveP,       0,  kSzWL,    2, kModeD|kModeDisp,     kModeD|kModeDisp,       0x0188},
  {"MOVEQ",  E::kMoveq,       0,  kSzLong,  2, kModeImm,             kModeD,                 0x7000},
  {"MULS",   E::kMulDiv,      0,  kSzWord,  2, Data,                 kModeD,                 0xC1C0},
  {"MULU",   E::kMulDiv,      0,  kSzWord,  2, Data,                 kModeD,                 0xC0C0},
  {"NBCD",   E::kEaOnly,      0,  kSzByte,  1, DataAlt,              0,                      0x4800},
  {"NEG",    E::kEaOnly,      0,  kSzBWL,   1, DataAlt,              0,                      0x4400},
  {"NEGX",   E::kEaOnly,      0,  kSzBWL,   1, DataAlt,              0,                      0x4000},
  {"NOP",    E::kFixed,       0,  kSzNone,  0, 0,                    0,                      0x4E71},
  {"NOT",    E::kEaOnly,      0,  kSzBWL,   1, DataAlt,              0,                      0x4600},
  // OR: covers Data,Dn / Dn,MemAlt / #imm,DataAlt / #imm,CCR / #imm,SR
  {"OR",     E::kEaBidirect,  0,  kSzBWL,   2, Data|kModeImm,        DataAlt|kModeCCR|kModeSR, 0x8000},
  {"ORI",    E::kImmedToEa,   0,  kSzBWL,   2, kModeImm,             DataAlt|kModeCCR|kModeSR, 0x0000},
  {"PEA",    E::kEaOnly,      0,  kSzLong,  1, Control,              0,                      0x4840},
  {"RESET",  E::kFixed,       0,  kSzNone,  0, 0,                    0,                      0x4E70},
  {"ROL",    E::kShift,       0,  kSzBWL,   2, MemAlt|kModeD|kModeImm, kModeD,               0xE178},
  {"ROR",    E::kShift,       0,  kSzBWL,   2, MemAlt|kModeD|kModeImm, kModeD,               0xE078},
  {"ROXL",   E::kShift,       0,  kSzBWL,   2, MemAlt|kModeD|kModeImm, kModeD,               0xE170},
  {"ROXR",   E::kShift,       0,  kSzBWL,   2, MemAlt|kModeD|kModeImm, kModeD,               0xE070},
  {"RTE",    E::kFixed,       0,  kSzNone,  0, 0,                    0,                      0x4E73},
  {"RTR",    E::kFixed,       0,  kSzNone,  0, 0,                    0,                      0x4E77},
  {"RTS",    E::kFixed,       0,  kSzNone,  0, 0,                    0,                      0x4E75},
  {"SBCD",   E::kAbcdSbcd,    0,  kSzByte,  2, kModeD|kModePre,      kModeD|kModePre,        0x8100},
  // Scc — cond encodes the condition code
  {"SCC",    E::kScc,         4,  kSzByte,  1, DataAlt,              0,                      0x54C0},
  {"SCS",    E::kScc,         5,  kSzByte,  1, DataAlt,              0,                      0x55C0},
  {"SEQ",    E::kScc,         7,  kSzByte,  1, DataAlt,              0,                      0x57C0},
  {"SF",     E::kScc,         1,  kSzByte,  1, DataAlt,              0,                      0x51C0},
  {"SGE",    E::kScc,        12,  kSzByte,  1, DataAlt,              0,                      0x5CC0},
  {"SGT",    E::kScc,        14,  kSzByte,  1, DataAlt,              0,                      0x5EC0},
  {"SHI",    E::kScc,         2,  kSzByte,  1, DataAlt,              0,                      0x52C0},
  {"SHS",    E::kScc,         4,  kSzByte,  1, DataAlt,              0,                      0x54C0},
  {"SLE",    E::kScc,        15,  kSzByte,  1, DataAlt,              0,                      0x5FC0},
  {"SLO",    E::kScc,         5,  kSzByte,  1, DataAlt,              0,                      0x55C0},
  {"SLS",    E::kScc,         3,  kSzByte,  1, DataAlt,              0,                      0x53C0},
  {"SLT",    E::kScc,        13,  kSzByte,  1, DataAlt,              0,                      0x5DC0},
  {"SMI",    E::kScc,        11,  kSzByte,  1, DataAlt,              0,                      0x5BC0},
  {"SNE",    E::kScc,         6,  kSzByte,  1, DataAlt,              0,                      0x56C0},
  {"SPL",    E::kScc,        10,  kSzByte,  1, DataAlt,              0,                      0x5AC0},
  {"ST",     E::kScc,         0,  kSzByte,  1, DataAlt,              0,                      0x50C0},
  {"STOP",   E::kFixedWord,   0,  kSzNone,  1, kModeImm,             0,                      0x4E72},
  // SUB: mirrors ADD structure
  {"SUB",    E::kEaBidirect,  0,  kSzBWL,   2, All,                  DataAlt|kModeA,         0x9000},
  {"SUBA",   E::kAddrEa,      0,  kSzWL,    2, All,                  kModeA,                 0x90C0},
  {"SUBI",   E::kImmedToEa,   0,  kSzBWL,   2, kModeImm,             DataAlt,                0x0400},
  {"SUBQ",   E::kQuick,       0,  kSzBWL,   2, kModeImm,             DataAlt|kModeA,         0x5100},
  {"SUBX",   E::kAddxSubx,    0,  kSzBWL,   2, kModeD|kModePre,      kModeD|kModePre,        0x9100},
  {"SVC",    E::kScc,         8,  kSzByte,  1, DataAlt,              0,                      0x58C0},
  {"SVS",    E::kScc,         9,  kSzByte,  1, DataAlt,              0,                      0x59C0},
  // SWAP: oneReg, word only; base = 0x4840 (same bits as PEA but different mode)
  {"SWAP",   E::kRegOp,       0,  kSzWord,  1, kModeD,               0,                      0x4840},
  {"TAS",    E::kEaOnly,      0,  kSzByte,  1, DataAlt,              0,                      0x4AC0},
  {"TRAP",   E::kTrap,        0,  kSzNone,  1, kModeImm,             0,                      0x4E40},
  {"TRAPV",  E::kFixed,       0,  kSzNone,  0, 0,                    0,                      0x4E76},
  {"TST",    E::kEaOnly,      0,  kSzBWL,   1, DataAlt,              0,                      0x4A00},
  {"UNLK",   E::kAnOp,        0,  kSzNone,  1, kModeA,               0,                      0x4E58},
};
// clang-format on

static constexpr int kInstrTableSize =
    static_cast<int>(sizeof(kInstrTable) / sizeof(kInstrTable[0]));

// ---------------------------------------------------------------------------
// InstructionTable constructor — build hash index from the static table
// ---------------------------------------------------------------------------
InstructionTable::InstructionTable() {
  for (int i = 0; i < kInstrTableSize; ++i)
    index_.emplace(kInstrTable[i].name, &kInstrTable[i]);
}

// ---------------------------------------------------------------------------
// Lookup — O(1) hash map search (original used binary search on sorted table)
// ---------------------------------------------------------------------------
const InstrEntry* InstructionTable::Lookup(const std::string& name) const {
  auto it = index_.find(name);
  return (it != index_.end()) ? it->second : nullptr;
}

// ---------------------------------------------------------------------------
// ExtWordCount — number of 16-bit extension words for an addressing mode
// Mirrors extWords() size accounting in CODEGEN.CPP.
// ---------------------------------------------------------------------------
int InstructionTable::ExtWordCount(AddressMode mode, SizeSpec size) {
  switch (mode) {
    case AddressMode::kDnDirect:
    case AddressMode::kAnDirect:
    case AddressMode::kAnIndirect:
    case AddressMode::kAnIndirectPost:
    case AddressMode::kAnIndirectPre:
      return 0;
    case AddressMode::kAnIndirectDisp:
    case AddressMode::kAnIndirectIdx:
    case AddressMode::kAbsShort:
    case AddressMode::kPCDisp:
    case AddressMode::kPCIndex:
      return 1;
    case AddressMode::kAbsLong:
      return 2;
    case AddressMode::kImmediate:
      if (size == SizeSpec::kLong)
        return 2;
      return 1;  // byte and word both occupy one extension word
    default:
      return 0;
  }
}

// ---------------------------------------------------------------------------
// SizeField — maps SizeSpec to the 2-bit size field used in most opcodes
// B=0, W=1, L=2.  Returns -1 for kNone/kShort.
// ---------------------------------------------------------------------------
int InstructionTable::SizeField(SizeSpec size) {
  switch (size) {
    case SizeSpec::kByte:
      return 0;
    case SizeSpec::kWord:
      return 1;
    case SizeSpec::kLong:
      return 2;
    default:
      return -1;
  }
}

}  // namespace easym68k::asm_
