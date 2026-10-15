// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// 68000 instruction set of the assembler — ported from Edit68K/BUILD.CPP,
// Edit68K/CODEGEN.CPP (effAddr, extWords), Edit68K/INSTTABL.CPP (flavor
// lists and instruction table) and Edit68K/ASSEMBLE.CPP (createCode,
// pickMask), by Paul McKee and Charles Kelly. The routines follow the
// original line by line; see instruction_set.h.

#include "easym68k/asm/instruction_set.h"

#include <algorithm>
#include <cstring>
#include <iterator>

#include "easym68k/asm/original_status.h"

namespace easym68k::asm_ {

namespace {

using status::NewError;

// ===========================================================================
// CODEGEN.CPP
// ===========================================================================

void output(BuildContext& ctx, int32_t data, int size) {
  if (ctx.output)
    ctx.output(data, size);
}

int effAddr(const OpDescriptor* operand) {
  switch (operand->mode) {
    case DnDirect:
      return 0x00 | operand->reg;
    case AnDirect:
      return 0x08 | operand->reg;
    case AnInd:
      return 0x10 | operand->reg;
    case AnIndPost:
      return 0x18 | operand->reg;
    case AnIndPre:
      return 0x20 | operand->reg;
    case AnIndDisp:
      return 0x28 | operand->reg;
    case AnIndIndex:
      return 0x30 | operand->reg;
    case AbsShort:
      return 0x38;
    case AbsLong:
      return 0x39;
    case PCDisp:
      return 0x3A;
    case PCIndex:
      return 0x3B;
    case IMMEDIATE:
      return 0x3C;
    default:
      // The original reports "INVALID EFFECTIVE ADDRESSING MODE!" and exits;
      // the flavor lists never pass such a mode here.
      return 0;
  }
}

void extWords(const OpDescriptor* op, int size, BuildContext& ctx) {
  int32_t disp;
  const bool pass2 = ctx.pass2;
  int& status = ctx.status;

  if (op->mode == DnDirect || op->mode == AnDirect || op->mode == AnInd || op->mode == AnIndPost ||
      op->mode == AnIndPre) {
    // no extension word
  } else if (op->mode == AnIndDisp || op->mode == PCDisp) {
    if (pass2) {
      disp = op->data;
      if (op->mode == PCDisp)
        disp -= static_cast<int32_t>(ctx.loc);
      output(ctx, disp & 0xFFFF, WORD_SIZE);
      if (disp < -32768 || disp > 32767)
        NewError(status, status::kInvDisp);
    }
    ctx.loc += 2;
  } else if (op->mode == AnIndIndex || op->mode == PCIndex) {
    if (pass2) {
      disp = op->data;
      if (op->mode == PCIndex)
        disp -= static_cast<int32_t>(ctx.loc);
      output(ctx, ((op->size == LONG_SIZE) ? 0x800 : 0) | (op->index << 12) | (disp & 0xFF),
             WORD_SIZE);
      if (disp < -128 || disp > 127)
        NewError(status, status::kInvDisp);
    }
    ctx.loc += 2;
  } else if (op->mode == AbsShort) {
    if (pass2) {
      output(ctx, op->data & 0xFFFF, WORD_SIZE);
      if (op->data < -32768 || op->data > 32767)
        NewError(status, status::kInvAbsAddress);
    }
    ctx.loc += 2;
  } else if (op->mode == AbsLong) {
    if (pass2)
      output(ctx, op->data, LONG_SIZE);
    ctx.loc += 4;
  } else if (op->mode == IMMEDIATE) {
    if (!size || size == WORD_SIZE) {
      if (pass2) {
        output(ctx, op->data & 0xFFFF, WORD_SIZE);
        if (op->data < -32768 || op->data > 65535)
          NewError(status, status::kInv16BitData);
      }
      ctx.loc += 2;
    } else if (size == BYTE_SIZE) {
      if (pass2) {
        output(ctx, op->data & 0xFF, WORD_SIZE);
        if (op->data < -128 || op->data > 255)
          NewError(status, status::kInv8BitData);
      }
      ctx.loc += 2;
    } else if (size == LONG_SIZE) {
      if (pass2)
        output(ctx, op->data, LONG_SIZE);
      ctx.loc += 4;
    }
  }
  // Other modes: the original shows "INVALID EFFECTIVE ADDRESSING MODE!".
}

// ===========================================================================
// BUILD.CPP
// ===========================================================================

void moveq(int mask, int size, OpDescriptor* source, OpDescriptor* dest, BuildContext& ctx);
void quickMath(int mask, int size, OpDescriptor* source, OpDescriptor* dest, BuildContext& ctx);

// MOVE and MOVEA
void move(int mask, int size, OpDescriptor* source, OpDescriptor* dest, BuildContext& ctx) {
  // Check whether the instruction can be assembled as MOVEQ
  if (source->mode == IMMEDIATE && source->back_ref && source->size == 0 && size == LONG_SIZE &&
      dest->mode == DnDirect && source->data >= -128 && source->data <= 127) {
    moveq(0x7000, size, source, dest, ctx);
    return;
  }
  // Otherwise assemble it as plain MOVE
  int moveMask = mask | effAddr(source);
  int destCode = effAddr(dest) & 0xff;
  moveMask |= ((destCode & 0x38) << 3) | ((destCode & 7) << 9);
  if (ctx.pass2)
    output(ctx, moveMask, WORD_SIZE);
  ctx.loc += 2;
  extWords(source, size, ctx);
  extWords(dest, size, ctx);
}

// ILLEGAL, NOP, RESET, RTE, RTR, RTS, TRAPV
void zeroOp(int mask, int, OpDescriptor*, OpDescriptor*, BuildContext& ctx) {
  if (ctx.pass2)
    output(ctx, mask, WORD_SIZE);
  ctx.loc += 2;
}

// ASd/LSd/ROd/ROXd <ea>, CLR, JMP, JSR, MOVE <ea>,CCR/SR, NBCD, NEG, NEGX,
// NOT, PEA, TAS, TST
void oneOp(int mask, int size, OpDescriptor* source, OpDescriptor*, BuildContext& ctx) {
  if (ctx.pass2)
    output(ctx, mask | effAddr(source), WORD_SIZE);
  ctx.loc += 2;
  extWords(source, size, ctx);
}

// ADD <ea>,Dn, ADDA, AND <ea>,Dn, CHK, CMP, CMPA, DIVS, DIVU, LEA, MULS, MULU,
// OR <ea>,Dn, SUB <ea>,Dn, SUBA
void arithReg(int mask, int size, OpDescriptor* source, OpDescriptor* dest, BuildContext& ctx) {
  // Check whether the instruction is an immediate ADDA or SUBA
  //   that can be assembled as ADDQ or SUBQ
  int type = mask & 0xF000;
  if ((type == 0xD000 || type == 0x9000)  // if ADDA or SUBA
      && source->mode == IMMEDIATE && size != BYTE_SIZE &&
      (source->back_ref && source->data >= 1 && source->data <= 8) &&
      source->size == 0) {  // not forced Long with #n.L
    if (type == 0xD000) {   // if ADDA
      // Assemble as ADDQ
      if (size == LONG_SIZE)
        quickMath(0x5080, size, source, dest, ctx);
      else
        quickMath(0x5040, size, source, dest, ctx);
      return;
    } else {
      // Assemble as SUBQ
      if (size == LONG_SIZE)
        quickMath(0x5180, size, source, dest, ctx);
      else
        quickMath(0x5140, size, source, dest, ctx);
      return;
    }
  }
  // Otherwise assemble as an ordinary instruction
  if (ctx.pass2)
    output(ctx, mask | effAddr(source) | (dest->reg << 9), WORD_SIZE);
  ctx.loc += 2;
  extWords(source, size, ctx);
}

// ADD Dn,<ea>, AND Dn,<ea>, BCHG/BCLR/BSET/BTST Dn,<ea>, EOR, OR Dn,<ea>,
// SUB Dn,<ea>
void arithAddr(int mask, int size, OpDescriptor* source, OpDescriptor* dest, BuildContext& ctx) {
  if (ctx.pass2)
    output(ctx, mask | effAddr(dest) | (source->reg << 9), WORD_SIZE);
  ctx.loc += 2;
  extWords(dest, size, ctx);
}

// BFCHG, BFCLR, BFEXTS, BFEXTU, BFFFO, BFINS, BFSET, BFTST
constexpr int BFEXTS = 0x0300;
constexpr int BFEXTU = 0x0100;
constexpr int BFFFO = 0x0500;
constexpr int BFINS = 0x0700;

void bitField(int mask, int size, OpDescriptor* source, OpDescriptor* dest, BuildContext& ctx) {
  // The original computes `type` only in pass 2 (and reads it uninitialized
  // in pass 1); it is computed in both passes here so the sizes agree.
  int type = mask & 0x0700;
  if (ctx.pass2) {
    if (type == BFINS)
      output(ctx, mask | effAddr(dest), WORD_SIZE);
    else
      output(ctx, mask | effAddr(source), WORD_SIZE);
    ctx.loc += 2;

    if (type == BFEXTS || type == BFEXTU || type == BFFFO)
      source->field |= dest->reg << 12;  // put Dn reg number in field
    if (type == BFINS) {                 // if bfins Dn,<ea>{offset:width}
      dest->field |= source->reg << 12;  // put Dn reg number in field
      output(ctx, dest->field, WORD_SIZE);
    } else {
      output(ctx, source->field, WORD_SIZE);
    }
    ctx.loc += 2;
  } else {
    ctx.loc += 4;
  }
  if (type == BFINS)
    extWords(dest, size, ctx);
  else
    extWords(source, size, ctx);
}

// ADDI, ANDI, CMPI, EORI, ORI, SUBI
void immedInst(int mask, int size, OpDescriptor* source, OpDescriptor* dest, BuildContext& ctx) {
  // Check whether the instruction is an immediate ADD or SUB
  //   that can be assembled as ADDQ or SUBQ
  int type = mask & 0xFF00;
  if ((type == 0x0600 || type == 0x0400) &&
      (source->back_ref && source->data >= 1 && source->data <= 8) && source->size == 0) {
    if (type == 0x0600) {
      // Assemble as ADDQ
      quickMath(0x5000 | (mask & 0x00C0), size, source, dest, ctx);
      return;
    } else {
      // Assemble as SUBQ
      quickMath(0x5100 | (mask & 0x00C0), size, source, dest, ctx);
      return;
    }
  }
  // Otherwise assemble as an ordinary instruction
  if (ctx.pass2)
    output(ctx, mask | effAddr(dest), WORD_SIZE);
  ctx.loc += 2;
  extWords(source, size, ctx);
  extWords(dest, size, ctx);
}

// ADDQ, SUBQ
void quickMath(int mask, int size, OpDescriptor* source, OpDescriptor* dest, BuildContext& ctx) {
  if (ctx.pass2) {
    output(ctx, mask | effAddr(dest) | ((source->data & 7) << 9), WORD_SIZE);
    if (source->data < 1 || source->data > 8)
      NewError(ctx.status, status::kInvQuickConst);
  }
  ctx.loc += 2;
  extWords(dest, size, ctx);
}

// MOVEP
void movep(int mask, int size, OpDescriptor* source, OpDescriptor* dest, BuildContext& ctx) {
  if (ctx.pass2) {
    if (source->mode == DnDirect) {
      // Convert plain address register indirect to address
      //   register indirect with displacement of 0
      if (dest->mode == AnInd) {
        dest->mode = AnIndDisp;
        dest->data = 0;
      }
      output(ctx, mask | (source->reg << 9) | (dest->reg), WORD_SIZE);
      ctx.loc += 2;
      extWords(dest, size, ctx);
    } else {
      // Convert plain address register indirect to address
      //   register indirect with displacement of 0
      if (source->mode == AnInd) {
        source->mode = AnIndDisp;
        source->data = 0;
      }
      output(ctx, mask | (dest->reg << 9) | (source->reg), WORD_SIZE);
      ctx.loc += 2;
      extWords(source, size, ctx);
    }
  } else {
    ctx.loc += 4;
  }
}

// MOVE from SR
void moveReg(int mask, int size, OpDescriptor*, OpDescriptor* dest, BuildContext& ctx) {
  if (ctx.pass2)
    output(ctx, mask | effAddr(dest), WORD_SIZE);
  ctx.loc += 2;
  extWords(dest, size, ctx);
}

// BCHG/BCLR/BSET/BTST #n,<ea>
void staticBit(int mask, int size, OpDescriptor* source, OpDescriptor* dest, BuildContext& ctx) {
  if (ctx.pass2) {
    output(ctx, mask | effAddr(dest), WORD_SIZE);
    ctx.loc += 2;
    output(ctx, source->data & 0xFF, WORD_SIZE);
    ctx.loc += 2;
  } else {
    ctx.loc += 4;
  }
  extWords(dest, size, ctx);
}

// TRAP
void trap(int mask, int, OpDescriptor* source, OpDescriptor*, BuildContext& ctx) {
  if (ctx.pass2) {
    output(ctx, mask | (source->data & 0xF), WORD_SIZE);
    if (source->data < 0 || source->data > 15)
      NewError(ctx.status, status::kInvVectorNum);
  }
  ctx.loc += 2;
}

// Bcc, BRA, BSR
void branch(int mask, int size, OpDescriptor* source, OpDescriptor*, BuildContext& ctx) {
  int32_t disp = source->data - static_cast<int32_t>(ctx.loc) - 2;
  bool shortDisp = false;
  if (((size == SHORT_SIZE) || (size == BYTE_SIZE)) ||
      (size != LONG_SIZE && size != WORD_SIZE && source->back_ref && disp >= -128 && disp <= 127 &&
       disp))
    shortDisp = true;
  if (ctx.pass2) {
    if (shortDisp) {
      output(ctx, mask | (disp & 0xFF), WORD_SIZE);
      ctx.loc += 2;
      if (disp < -128 || disp > 127 || !disp)
        NewError(ctx.status, status::kInvBranchDisp);
    } else {
      output(ctx, mask, WORD_SIZE);
      ctx.loc += 2;
      output(ctx, disp, WORD_SIZE);
      ctx.loc += 2;
      if (disp < -32768 || disp > 32767)
        NewError(ctx.status, status::kInvBranchDisp);
    }
  } else {
    ctx.loc += shortDisp ? 2 : 4;
  }
}

// MOVEQ
void moveq(int mask, int, OpDescriptor* source, OpDescriptor* dest, BuildContext& ctx) {
  if (ctx.pass2) {
    output(ctx, mask | (dest->reg << 9) | (source->data & 0xFF), WORD_SIZE);
    if (source->data < -128 || source->data > 255)
      NewError(ctx.status, status::kInvMoveQuickConst);
  }
  ctx.loc += 2;
}

// ANDI/EORI/ORI to CCR
void immedToCCR(int mask, int, OpDescriptor* source, OpDescriptor*, BuildContext& ctx) {
  if (ctx.pass2) {
    output(ctx, mask, WORD_SIZE);
    ctx.loc += 2;
    output(ctx, source->data & 0xFF, WORD_SIZE);
    ctx.loc += 2;
    if ((source->data & 0xFF) != source->data)
      NewError(ctx.status, status::kInv8BitData);
  } else {
    ctx.loc += 4;
  }
}

// ANDI/EORI/ORI to SR, STOP
void immedWord(int mask, int, OpDescriptor* source, OpDescriptor*, BuildContext& ctx) {
  if (ctx.pass2) {
    output(ctx, mask, WORD_SIZE);
    ctx.loc += 2;
    output(ctx, source->data & 0xFFFF, WORD_SIZE);
    ctx.loc += 2;
    if (source->data < -32768 || source->data > 65535)
      NewError(ctx.status, status::kInv16BitData);
  } else {
    ctx.loc += 4;
  }
}

// DBcc
void dbcc(int mask, int, OpDescriptor* source, OpDescriptor* dest, BuildContext& ctx) {
  int32_t disp = dest->data - static_cast<int32_t>(ctx.loc) - 2;
  if (ctx.pass2) {
    output(ctx, mask | source->reg, WORD_SIZE);
    ctx.loc += 2;
    output(ctx, disp, WORD_SIZE);
    ctx.loc += 2;
    if (disp < -32768 || disp > 32767)
      NewError(ctx.status, status::kInvBranchDisp);
  } else {
    ctx.loc += 4;
  }
}

// Scc
void scc(int mask, int size, OpDescriptor* source, OpDescriptor*, BuildContext& ctx) {
  if (ctx.pass2)
    output(ctx, mask | effAddr(source), WORD_SIZE);
  ctx.loc += 2;
  extWords(source, size, ctx);
}

// ASd/LSd/ROd/ROXd Dx,Dy and #<data>,Dy
void shiftReg(int mask, int, OpDescriptor* source, OpDescriptor* dest, BuildContext& ctx) {
  if (ctx.pass2) {
    mask |= dest->reg;
    if (source->mode == IMMEDIATE) {
      mask |= (source->data & 7) << 9;
      if (source->data < 1 || source->data > 8)
        NewError(ctx.status, status::kInvShiftCount);
    } else {
      mask |= source->reg << 9;
    }
    output(ctx, mask, WORD_SIZE);
  }
  ctx.loc += 2;
}

// EXG
void exg(int mask, int, OpDescriptor* source, OpDescriptor* dest, BuildContext& ctx) {
  if (ctx.pass2) {
    // Are a data register and an address register being exchanged?
    if (source->mode != dest->mode) {
      // If so, the address register goes in bottom three bits
      if (source->mode == AnDirect)
        mask |= source->reg | (dest->reg << 9);
      else
        mask |= dest->reg | (source->reg << 9);
    } else {
      // Otherwise it doesn't matter which way they go
      mask |= dest->reg | (source->reg << 9);
    }
    output(ctx, mask, WORD_SIZE);
  }
  ctx.loc += 2;
}

// ABCD, ADDX, CMPM, SBCD, SUBX
void twoReg(int mask, int, OpDescriptor* source, OpDescriptor* dest, BuildContext& ctx) {
  if (ctx.pass2)
    output(ctx, mask | (dest->reg << 9) | source->reg, WORD_SIZE);
  ctx.loc += 2;
}

// EXT, SWAP, UNLK
void oneReg(int mask, int, OpDescriptor* source, OpDescriptor*, BuildContext& ctx) {
  if (ctx.pass2)
    output(ctx, mask | source->reg, WORD_SIZE);
  ctx.loc += 2;
}

// MOVE USP,An and MOVE An,USP
void moveUSP(int mask, int, OpDescriptor* source, OpDescriptor* dest, BuildContext& ctx) {
  if (ctx.pass2) {
    if (source->mode == AnDirect)
      output(ctx, mask | source->reg, WORD_SIZE);
    else
      output(ctx, mask | dest->reg, WORD_SIZE);
  }
  ctx.loc += 2;
}

// LINK
void link(int mask, int, OpDescriptor* source, OpDescriptor* dest, BuildContext& ctx) {
  if (ctx.pass2) {
    output(ctx, mask | source->reg, WORD_SIZE);
    ctx.loc += 2;
    output(ctx, dest->data, WORD_SIZE);
    ctx.loc += 2;
    if (dest->data < -32768 || dest->data > 32767)
      NewError(ctx.status, status::kInv16BitData);
  } else {
    ctx.loc += 4;
  }
}

// ===========================================================================
// INSTTABL.CPP
// ===========================================================================

// Addressing mode classes
constexpr int Data = DnDirect | AnInd | AnIndPost | AnIndPre | AnIndDisp | AnIndIndex | AbsShort |
                     AbsLong | PCDisp | PCIndex | IMMEDIATE;
constexpr int Memory = AnInd | AnIndPost | AnIndPre | AnIndDisp | AnIndIndex | AbsShort | AbsLong |
                       PCDisp | PCIndex | IMMEDIATE;
constexpr int Control = AnInd | AnIndDisp | AnIndIndex | AbsShort | AbsLong | PCDisp | PCIndex;
constexpr int ControlAlt = AnInd | AnIndDisp | AnIndIndex | AbsShort | AbsLong;
constexpr int Alter =
    DnDirect | AnInd | AnIndPost | AnIndPre | AnIndDisp | AnIndIndex | AbsShort | AbsLong;
constexpr int All = Data | AnDirect;
constexpr int DataAlt = Data & Alter;
constexpr int MemAlt = Memory & Alter;
constexpr int Absolute = AbsLong | AbsShort;
constexpr int GenReg = DnDirect | AnDirect;
constexpr int DnControl = DnDirect | Control;
constexpr int DnControlAlt = DnDirect | ControlAlt;

// Size code masks
constexpr int BW = BYTE_SIZE | WORD_SIZE;
constexpr int WL = WORD_SIZE | LONG_SIZE;
constexpr int BWL = BYTE_SIZE | WORD_SIZE | LONG_SIZE;
constexpr int BL = BYTE_SIZE | LONG_SIZE;

[[maybe_unused]] constexpr int kUnusedClasses = GenReg | BW | BL;

// clang-format off
// The order of the flavors is important.
const Flavor abcdfl[] = {
    {DnDirect, DnDirect, BYTE_SIZE, twoReg, 0xC100, 0xC100, 0},
    {AnIndPre, AnIndPre, BYTE_SIZE, twoReg, 0xC108, 0xC108, 0},
};
const Flavor addfl[] = {
    {All, AnDirect, WL, arithReg, 0, 0xD0C0, 0xD1C0},
    {IMMEDIATE, DataAlt, BWL, immedInst, 0x0600, 0x0640, 0x0680},
    {IMMEDIATE, AnDirect, WL, quickMath, 0, 0x5040, 0x5080},
    {Data, DnDirect, BWL, arithReg, 0xD000, 0xD040, 0xD080},
    {AnDirect, DnDirect, WL, arithReg, 0xD000, 0xD040, 0xD080},
    {DnDirect, MemAlt, BWL, arithAddr, 0xD100, 0xD140, 0xD180},
};
const Flavor addafl[] = {
    {All, AnDirect, WL, arithReg, 0, 0xD0C0, 0xD1C0},
};
const Flavor addifl[] = {
    {IMMEDIATE, DataAlt, BWL, immedInst, 0x0600, 0x0640, 0x0680},
};
const Flavor addqfl[] = {
    {IMMEDIATE, DataAlt, BWL, quickMath, 0x5000, 0x5040, 0x5080},
    {IMMEDIATE, AnDirect, WL, quickMath, 0, 0x5040, 0x5080},
};
const Flavor addxfl[] = {
    {DnDirect, DnDirect, BWL, twoReg, 0xD100, 0xD140, 0xD180},
    {AnIndPre, AnIndPre, BWL, twoReg, 0xD108, 0xD148, 0xD188},
};
const Flavor andfl[] = {
    {Data, DnDirect, BWL, arithReg, 0xC000, 0xC040, 0xC080},
    {DnDirect, MemAlt, BWL, arithAddr, 0xC100, 0xC140, 0xC180},
    {IMMEDIATE, DataAlt, BWL, immedInst, 0x0200, 0x0240, 0x0280},
    {IMMEDIATE, CCRDirect, BYTE_SIZE, immedToCCR, 0x023C, 0x023C, 0},
    {IMMEDIATE, SRDirect, WORD_SIZE, immedWord, 0, 0x027C, 0},
};
const Flavor andifl[] = {
    {IMMEDIATE, DataAlt, BWL, immedInst, 0x0200, 0x0240, 0x0280},
    {IMMEDIATE, CCRDirect, BYTE_SIZE, immedToCCR, 0x023C, 0x023C, 0},
    {IMMEDIATE, SRDirect, WORD_SIZE, immedWord, 0, 0x027C, 0},
};
const Flavor aslfl[] = {
    {MemAlt, 0, WORD_SIZE, oneOp, 0, 0xE1C0, 0},
    {DnDirect, DnDirect, BWL, shiftReg, 0xE120, 0xE160, 0xE1A0},
    {IMMEDIATE, DnDirect, BWL, shiftReg, 0xE100, 0xE140, 0xE180},
};
const Flavor asrfl[] = {
    {MemAlt, 0, WORD_SIZE, oneOp, 0, 0xE0C0, 0},
    {DnDirect, DnDirect, BWL, shiftReg, 0xE020, 0xE060, 0xE0A0},
    {IMMEDIATE, DnDirect, BWL, shiftReg, 0xE000, 0xE040, 0xE080},
};
const Flavor bccfl[] = {
    {Absolute, 0, SHORT_SIZE | LONG_SIZE | BYTE_SIZE | WORD_SIZE, branch, 0x6400, 0x6400, 0x6400},
};
const Flavor bchgfl[] = {
    {DnDirect, MemAlt, BYTE_SIZE, arithAddr, 0x0140, 0x0140, 0},
    {DnDirect, DnDirect, LONG_SIZE, arithAddr, 0, 0x0140, 0x0140},
    {IMMEDIATE, MemAlt, BYTE_SIZE, staticBit, 0x0840, 0x0840, 0},
    {IMMEDIATE, DnDirect, LONG_SIZE, staticBit, 0, 0x0840, 0x0840},
};
const Flavor bclrfl[] = {
    {DnDirect, MemAlt, BYTE_SIZE, arithAddr, 0x0180, 0x0180, 0},
    {DnDirect, DnDirect, LONG_SIZE, arithAddr, 0, 0x0180, 0x0180},
    {IMMEDIATE, MemAlt, BYTE_SIZE, staticBit, 0x0880, 0x0880, 0},
    {IMMEDIATE, DnDirect, LONG_SIZE, staticBit, 0, 0x0880, 0x0880},
};
const Flavor bcsfl[] = {
    {Absolute, 0, SHORT_SIZE | LONG_SIZE | BYTE_SIZE | WORD_SIZE, branch, 0x6500, 0x6500, 0x6500},
};
const Flavor bfchgfl[] = {
    {DnControlAlt, 0, SHORT_SIZE | BWL, bitField, 0xEAC0, 0xEAC0, 0xEAC0},
};
const Flavor bfclrfl[] = {
    {DnControlAlt, 0, SHORT_SIZE | BWL, bitField, 0xECC0, 0xECC0, 0xECC0},
};
const Flavor bfextsfl[] = {
    {DnControl, DnDirect, SHORT_SIZE | BWL, bitField, 0xEBC0, 0xEBC0, 0xEBC0},
};
const Flavor bfextufl[] = {
    {DnControl, DnDirect, SHORT_SIZE | BWL, bitField, 0xE9C0, 0xE9C0, 0xE9C0},
};
const Flavor bfffofl[] = {
    {DnControl, DnDirect, SHORT_SIZE | BWL, bitField, 0xEDC0, 0xEDC0, 0xEDC0},
};
const Flavor bfinsfl[] = {
    {DnDirect, DnControlAlt, SHORT_SIZE | BWL, bitField, 0xEFC0, 0xEFC0, 0xEFC0},
};
const Flavor bfsetfl[] = {
    {DnControlAlt, 0, SHORT_SIZE | BWL, bitField, 0xEEC0, 0xEEC0, 0xEEC0},
};
const Flavor bftstfl[] = {
    {DnControl, 0, SHORT_SIZE | BWL, bitField, 0xE8C0, 0xE8C0, 0xE8C0},
};
const Flavor beqfl[] = {
    {Absolute, 0, SHORT_SIZE | LONG_SIZE | BYTE_SIZE | WORD_SIZE, branch, 0x6700, 0x6700, 0x6700},
};
const Flavor bgefl[] = {
    {Absolute, 0, SHORT_SIZE | LONG_SIZE | BYTE_SIZE | WORD_SIZE, branch, 0x6C00, 0x6C00, 0x6C00},
};
const Flavor bgtfl[] = {
    {Absolute, 0, SHORT_SIZE | LONG_SIZE | BYTE_SIZE | WORD_SIZE, branch, 0x6E00, 0x6E00, 0x6E00},
};
const Flavor bhifl[] = {
    {Absolute, 0, SHORT_SIZE | LONG_SIZE | BYTE_SIZE | WORD_SIZE, branch, 0x6200, 0x6200, 0x6200},
};
[[maybe_unused]] const Flavor bhsfl[] = {  // alias table kept from INSTTABL.CPP
    {Absolute, 0, SHORT_SIZE | LONG_SIZE | BYTE_SIZE | WORD_SIZE, branch, 0x6400, 0x6400, 0x6400},
};
const Flavor blefl[] = {
    {Absolute, 0, SHORT_SIZE | LONG_SIZE | BYTE_SIZE | WORD_SIZE, branch, 0x6f00, 0x6F00, 0x6F00},
};
[[maybe_unused]] const Flavor blofl[] = {  // alias table kept from INSTTABL.CPP
    {Absolute, 0, SHORT_SIZE | LONG_SIZE | BYTE_SIZE | WORD_SIZE, branch, 0x6500, 0x6500, 0x6500},
};
const Flavor blsfl[] = {
    {Absolute, 0, SHORT_SIZE | LONG_SIZE | BYTE_SIZE | WORD_SIZE, branch, 0x6300, 0x6300, 0x6300},
};
const Flavor bltfl[] = {
    {Absolute, 0, SHORT_SIZE | LONG_SIZE | BYTE_SIZE | WORD_SIZE, branch, 0x6d00, 0x6D00, 0x6D00},
};
const Flavor bmifl[] = {
    {Absolute, 0, SHORT_SIZE | LONG_SIZE | BYTE_SIZE | WORD_SIZE, branch, 0x6b00, 0x6B00, 0x6B00},
};
const Flavor bnefl[] = {
    {Absolute, 0, SHORT_SIZE | LONG_SIZE | BYTE_SIZE | WORD_SIZE, branch, 0x6600, 0x6600, 0x6600},
};
const Flavor bplfl[] = {
    {Absolute, 0, SHORT_SIZE | LONG_SIZE | BYTE_SIZE | WORD_SIZE, branch, 0x6a00, 0x6A00, 0x6A00},
};
const Flavor brafl[] = {
    {Absolute, 0, SHORT_SIZE | LONG_SIZE | BYTE_SIZE | WORD_SIZE, branch, 0x6000, 0x6000, 0x6000},
};
const Flavor bsetfl[] = {
    {DnDirect, MemAlt, BYTE_SIZE, arithAddr, 0x01C0, 0x01C0, 0},
    {DnDirect, DnDirect, LONG_SIZE, arithAddr, 0, 0x01C0, 0x01C0},
    {IMMEDIATE, MemAlt, BYTE_SIZE, staticBit, 0x08C0, 0x08C0, 0},
    {IMMEDIATE, DnDirect, LONG_SIZE, staticBit, 0, 0x08C0, 0x08C0},
};
const Flavor bsrfl[] = {
    {Absolute, 0, SHORT_SIZE | LONG_SIZE | BYTE_SIZE | WORD_SIZE, branch, 0x6100, 0x6100, 0x6100},
};
const Flavor btstfl[] = {
    {DnDirect, Memory, BYTE_SIZE, arithAddr, 0x0100, 0x0100, 0},
    {DnDirect, DnDirect, LONG_SIZE, arithAddr, 0, 0x0100, 0x0100},
    {IMMEDIATE, Memory, BYTE_SIZE, staticBit, 0x0800, 0x0800, 0},
    {IMMEDIATE, DnDirect, LONG_SIZE, staticBit, 0, 0x0800, 0x0800},
};
const Flavor bvcfl[] = {
    {Absolute, 0, SHORT_SIZE | LONG_SIZE | BYTE_SIZE | WORD_SIZE, branch, 0x6800, 0x6800, 0x6800},
};
const Flavor bvsfl[] = {
    {Absolute, 0, SHORT_SIZE | LONG_SIZE | BYTE_SIZE | WORD_SIZE, branch, 0x6900, 0x6900, 0x6900},
};
const Flavor chkfl[] = {
    {Data, DnDirect, WORD_SIZE, arithReg, 0, 0x4180, 0},
};
const Flavor clrfl[] = {
    {DataAlt, 0, BWL, oneOp, 0x4200, 0x4240, 0x4280},
};
const Flavor cmpfl[] = {
    {Data, DnDirect, BWL, arithReg, 0xB000, 0xB040, 0xB080},
    {AnDirect, DnDirect, WL, arithReg, 0xB000, 0xB040, 0xB080},
    {All, AnDirect, WL, arithReg, 0, 0xB0C0, 0xB1C0},
    {IMMEDIATE, DataAlt, BWL, immedInst, 0x0C00, 0x0C40, 0x0C80},
    {AnIndPost, AnIndPost, BWL, twoReg, 0xB108, 0xB148, 0xB188},
};
const Flavor cmpafl[] = {
    {All, AnDirect, WL, arithReg, 0, 0xB0C0, 0xB1C0},
};
const Flavor cmpifl[] = {
    {IMMEDIATE, DataAlt, BWL, immedInst, 0x0C00, 0x0C40, 0x0C80},
};
const Flavor cmpmfl[] = {
    {AnIndPost, AnIndPost, BWL, twoReg, 0xB108, 0xB148, 0xB188},
};
const Flavor dbccfl[] = {
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x54C8, 0},
};
const Flavor dbcsfl[] = {
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x55C8, 0},
};
const Flavor dbeqfl[] = {
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x57C8, 0},
};
const Flavor dbffl[] = {
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x51C8, 0},
};
const Flavor dbgefl[] = {
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x5CC8, 0},
};
const Flavor dbgtfl[] = {
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x5EC8, 0},
};
const Flavor dbhifl[] = {
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x52C8, 0},
};
[[maybe_unused]] const Flavor dbhsfl[] = {  // alias table kept from INSTTABL.CPP
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x54C8, 0},
};
const Flavor dblefl[] = {
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x5FC8, 0},
};
[[maybe_unused]] const Flavor dblofl[] = {  // alias table kept from INSTTABL.CPP
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x55C8, 0},
};
const Flavor dblsfl[] = {
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x53C8, 0},
};
const Flavor dbltfl[] = {
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x5DC8, 0},
};
const Flavor dbmifl[] = {
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x5BC8, 0},
};
const Flavor dbnefl[] = {
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x56C8, 0},
};
const Flavor dbplfl[] = {
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x5AC8, 0},
};
const Flavor dbrafl[] = {
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x51C8, 0},
};
const Flavor dbtfl[] = {
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x50C8, 0},
};
const Flavor dbvcfl[] = {
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x58C8, 0},
};
const Flavor dbvsfl[] = {
    {DnDirect, Absolute, WORD_SIZE, dbcc, 0, 0x59C8, 0},
};
const Flavor divsfl[] = {
    {Data, DnDirect, WORD_SIZE, arithReg, 0, 0x81C0, 0},
};
const Flavor divufl[] = {
    {Data, DnDirect, WORD_SIZE, arithReg, 0, 0x80C0, 0},
};
const Flavor eorfl[] = {
    {DnDirect, DataAlt, BWL, arithAddr, 0xB100, 0xB140, 0xB180},
    {IMMEDIATE, DataAlt, BWL, immedInst, 0x0A00, 0x0A40, 0x0A80},
};
const Flavor eorifl[] = {
    {IMMEDIATE, DataAlt, BWL, immedInst, 0x0A00, 0x0A40, 0x0A80},
    {IMMEDIATE, CCRDirect, BYTE_SIZE, immedToCCR, 0x0A3C, 0x0A3C, 0},
    {IMMEDIATE, SRDirect, WORD_SIZE, immedWord, 0, 0x0A7C, 0},
};
const Flavor exgfl[] = {
    {DnDirect, DnDirect, LONG_SIZE, exg, 0, 0xC140, 0xC140},
    {AnDirect, AnDirect, LONG_SIZE, exg, 0, 0xC148, 0xC148},
    {GenReg, GenReg, LONG_SIZE, exg, 0, 0xC188, 0xC188},
};
const Flavor extfl[] = {
    {DnDirect, 0, WL, oneReg, 0, 0x4880, 0x48C0},
};
const Flavor illegalfl[] = {
    {0, 0, 0, zeroOp, 0, 0x4AFC, 0},
};
const Flavor jmpfl[] = {
    {Control, 0, 0, oneOp, 0, 0x4EC0, 0},
};
const Flavor jsrfl[] = {
    {Control, 0, 0, oneOp, 0, 0x4E80, 0},
};
const Flavor leafl[] = {
    {Control, AnDirect, LONG_SIZE, arithReg, 0, 0x41C0, 0x41C0},
};
const Flavor linkfl[] = {
    {AnDirect, IMMEDIATE, 0, link, 0, 0x4E50, 0},
};
const Flavor lslfl[] = {
    {MemAlt, 0, WORD_SIZE, oneOp, 0, 0xE3C0, 0},
    {DnDirect, DnDirect, BWL, shiftReg, 0xE128, 0xE168, 0xE1A8},
    {IMMEDIATE, DnDirect, BWL, shiftReg, 0xE108, 0xE148, 0xE188},
};
const Flavor lsrfl[] = {
    {MemAlt, 0, WORD_SIZE, oneOp, 0, 0xE2C0, 0},
    {DnDirect, DnDirect, BWL, shiftReg, 0xE028, 0xE068, 0xE0A8},
    {IMMEDIATE, DnDirect, BWL, shiftReg, 0xE008, 0xE048, 0xE088},
};
const Flavor movefl[] = {
    {Data, DataAlt, BWL, move, 0x1000, 0x3000, 0x2000},
    {AnDirect, DataAlt, WL, move, 0x1000, 0x3000, 0x2000},
    {All, AnDirect, WL, move, 0, 0x3000, 0x2000},
    {Data, CCRDirect, WORD_SIZE, oneOp, 0, 0x44C0, 0},
    {Data, SRDirect, WORD_SIZE, oneOp, 0, 0x46C0, 0},
    {SRDirect, DataAlt, WORD_SIZE, moveReg, 0, 0x40C0, 0},
    {AnDirect, USPDirect, LONG_SIZE, moveUSP, 0, 0x4E60, 0x4E60},
    {USPDirect, AnDirect, LONG_SIZE, moveUSP, 0, 0x4E68, 0x4E68},
};
const Flavor moveafl[] = {
    {All, AnDirect, WL, move, 0, 0x3000, 0x2000},
};
const Flavor movepfl[] = {
    {DnDirect, AnIndDisp, WL, movep, 0, 0x0188, 0x01C8},
    {AnIndDisp, DnDirect, WL, movep, 0, 0x0108, 0x0148},
    {DnDirect, AnInd, WL, movep, 0, 0x0188, 0x01C8},
    {AnInd, DnDirect, WL, movep, 0, 0x0108, 0x0148},
};
const Flavor moveqfl[] = {
    {IMMEDIATE, DnDirect, LONG_SIZE, moveq, 0, 0x7000, 0x7000},
};
const Flavor mulsfl[] = {
    {Data, DnDirect, WORD_SIZE, arithReg, 0, 0xC1C0, 0},
};
const Flavor mulufl[] = {
    {Data, DnDirect, WORD_SIZE, arithReg, 0, 0xC0C0, 0},
};
const Flavor nbcdfl[] = {
    {DataAlt, 0, BYTE_SIZE, oneOp, 0x4800, 0x4800, 0},
};
const Flavor negfl[] = {
    {DataAlt, 0, BWL, oneOp, 0x4400, 0x4440, 0x4480},
};
const Flavor negxfl[] = {
    {DataAlt, 0, BWL, oneOp, 0x4000, 0x4040, 0x4080},
};
const Flavor nopfl[] = {
    {0, 0, 0, zeroOp, 0, 0x4E71, 0},
};
const Flavor notfl[] = {
    {DataAlt, 0, BWL, oneOp, 0x4600, 0x4640, 0x4680},
};
const Flavor orfl[] = {
    {Data, DnDirect, BWL, arithReg, 0x8000, 0x8040, 0x8080},
    {DnDirect, MemAlt, BWL, arithAddr, 0x8100, 0x8140, 0x8180},
    {IMMEDIATE, DataAlt, BWL, immedInst, 0x0000, 0x0040, 0x0080},
    {IMMEDIATE, CCRDirect, BYTE_SIZE, immedToCCR, 0x003C, 0x003C, 0},
    {IMMEDIATE, SRDirect, WORD_SIZE, immedWord, 0, 0x007C, 0},
};
const Flavor orifl[] = {
    {IMMEDIATE, DataAlt, BWL, immedInst, 0x0000, 0x0040, 0x0080},
    {IMMEDIATE, CCRDirect, BYTE_SIZE, immedToCCR, 0x003C, 0x003C, 0},
    {IMMEDIATE, SRDirect, WORD_SIZE, immedWord, 0, 0x007C, 0},
};
const Flavor peafl[] = {
    {Control, 0, LONG_SIZE, oneOp, 0, 0x4840, 0x4840},
};
const Flavor resetfl[] = {
    {0, 0, 0, zeroOp, 0, 0x4E70, 0},
};
const Flavor rolfl[] = {
    {MemAlt, 0, WORD_SIZE, oneOp, 0, 0xE7C0, 0},
    {DnDirect, DnDirect, BWL, shiftReg, 0xE138, 0xE178, 0xE1B8},
    {IMMEDIATE, DnDirect, BWL, shiftReg, 0xE118, 0xE158, 0xE198},
};
const Flavor rorfl[] = {
    {MemAlt, 0, WORD_SIZE, oneOp, 0, 0xE6C0, 0},
    {DnDirect, DnDirect, BWL, shiftReg, 0xE038, 0xE078, 0xE0B8},
    {IMMEDIATE, DnDirect, BWL, shiftReg, 0xE018, 0xE058, 0xE098},
};
const Flavor roxlfl[] = {
    {MemAlt, 0, WORD_SIZE, oneOp, 0, 0xE5C0, 0},
    {DnDirect, DnDirect, BWL, shiftReg, 0xE130, 0xE170, 0xE1B0},
    {IMMEDIATE, DnDirect, BWL, shiftReg, 0xE110, 0xE150, 0xE190},
};
const Flavor roxrfl[] = {
    {MemAlt, 0, WORD_SIZE, oneOp, 0, 0xE4C0, 0},
    {DnDirect, DnDirect, BWL, shiftReg, 0xE030, 0xE070, 0xE0B0},
    {IMMEDIATE, DnDirect, BWL, shiftReg, 0xE010, 0xE050, 0xE090},
};
const Flavor rtefl[] = {
    {0, 0, 0, zeroOp, 0, 0x4E73, 0},
};
const Flavor rtrfl[] = {
    {0, 0, 0, zeroOp, 0, 0x4E77, 0},
};
const Flavor rtsfl[] = {
    {0, 0, 0, zeroOp, 0, 0x4E75, 0},
};
const Flavor sbcdfl[] = {
    {DnDirect, DnDirect, BYTE_SIZE, twoReg, 0x8100, 0x8100, 0},
    {AnIndPre, AnIndPre, BYTE_SIZE, twoReg, 0x8108, 0x8108, 0},
};
const Flavor sccfl[] = {
    {DataAlt, 0, BYTE_SIZE, scc, 0x54C0, 0x54C0, 0},
};
const Flavor scsfl[] = {
    {DataAlt, 0, BYTE_SIZE, scc, 0x55C0, 0x55C0, 0},
};
const Flavor seqfl[] = {
    {DataAlt, 0, BYTE_SIZE, scc, 0x57C0, 0x57C0, 0},
};
const Flavor sffl[] = {
    {DataAlt, 0, BYTE_SIZE, scc, 0x51C0, 0x51C0, 0},
};
const Flavor sgefl[] = {
    {DataAlt, 0, BYTE_SIZE, scc, 0x5CC0, 0x5CC0, 0},
};
const Flavor sgtfl[] = {
    {DataAlt, 0, BYTE_SIZE, scc, 0x5EC0, 0x5EC0, 0},
};
const Flavor shifl[] = {
    {DataAlt, 0, BYTE_SIZE, scc, 0x52C0, 0x52C0, 0},
};
[[maybe_unused]] const Flavor shsfl[] = {  // alias table kept from INSTTABL.CPP
    {DataAlt, 0, BYTE_SIZE, scc, 0x54C0, 0x54C0, 0},
};
const Flavor slefl[] = {
    {DataAlt, 0, BYTE_SIZE, scc, 0x5FC0, 0x5FC0, 0},
};
[[maybe_unused]] const Flavor slofl[] = {  // alias table kept from INSTTABL.CPP
    {DataAlt, 0, BYTE_SIZE, scc, 0x55C0, 0x55C0, 0},
};
const Flavor slsfl[] = {
    {DataAlt, 0, BYTE_SIZE, scc, 0x53C0, 0x53C0, 0},
};
const Flavor sltfl[] = {
    {DataAlt, 0, BYTE_SIZE, scc, 0x5DC0, 0x5DC0, 0},
};
const Flavor smifl[] = {
    {DataAlt, 0, BYTE_SIZE, scc, 0x5BC0, 0x5BC0, 0},
};
const Flavor snefl[] = {
    {DataAlt, 0, BYTE_SIZE, scc, 0x56C0, 0x56C0, 0},
};
const Flavor splfl[] = {
    {DataAlt, 0, BYTE_SIZE, scc, 0x5AC0, 0x5AC0, 0},
};
const Flavor stfl[] = {
    {DataAlt, 0, BYTE_SIZE, scc, 0x50C0, 0x50C0, 0},
};
const Flavor stopfl[] = {
    {IMMEDIATE, 0, 0, immedWord, 0, 0x4E72, 0},
};
const Flavor subfl[] = {
    {All, AnDirect, WL, arithReg, 0, 0x90C0, 0x91C0},
    {IMMEDIATE, DataAlt, BWL, immedInst, 0x0400, 0x0440, 0x0480},
    {IMMEDIATE, AnDirect, WL, quickMath, 0, 0x5140, 0x5180},
    {Data, DnDirect, BWL, arithReg, 0x9000, 0x9040, 0x9080},
    {AnDirect, DnDirect, WL, arithReg, 0x9000, 0x9040, 0x9080},
    {DnDirect, MemAlt, BWL, arithAddr, 0x9100, 0x9140, 0x9180},
};
const Flavor subafl[] = {
    {All, AnDirect, WL, arithReg, 0, 0x90C0, 0x91C0},
};
const Flavor subifl[] = {
    {IMMEDIATE, DataAlt, BWL, immedInst, 0x0400, 0x0440, 0x0480},
};
const Flavor subqfl[] = {
    {IMMEDIATE, DataAlt, BWL, quickMath, 0x5100, 0x5140, 0x5180},
    {IMMEDIATE, AnDirect, WL, quickMath, 0, 0x5140, 0x5180},
};
const Flavor subxfl[] = {
    {DnDirect, DnDirect, BWL, twoReg, 0x9100, 0x9140, 0x9180},
    {AnIndPre, AnIndPre, BWL, twoReg, 0x9108, 0x9148, 0x9188},
};
const Flavor svcfl[] = {
    {DataAlt, 0, BYTE_SIZE, scc, 0x58C0, 0x58C0, 0},
};
const Flavor svsfl[] = {
    {DataAlt, 0, BYTE_SIZE, scc, 0x59C0, 0x59C0, 0},
};
const Flavor swapfl[] = {
    {DnDirect, 0, WORD_SIZE, oneReg, 0, 0x4840, 0},
};
const Flavor tasfl[] = {
    {DataAlt, 0, BYTE_SIZE, oneOp, 0x4AC0, 0x4AC0, 0},
};
const Flavor trapfl[] = {
    {IMMEDIATE, 0, 0, trap, 0, 0x4E40, 0},
};
const Flavor trapvfl[] = {
    {0, 0, 0, zeroOp, 0, 0x4E76, 0},
};
const Flavor tstfl[] = {
    {DataAlt, 0, BWL, oneOp, 0x4A00, 0x4A40, 0x4A80},
};
const Flavor unlkfl[] = {
    {AnDirect, 0, 0, oneReg, 0, 0x4E58, 0},
};

// Instructions assembled through the flavor table (parseFlag true), in
// the original's alphabetical order.
const Instruction kInstructions[] = {
    {"ABCD", abcdfl, std::size(abcdfl)},
    {"ADD", addfl, std::size(addfl)},
    {"ADDA", addafl, std::size(addafl)},
    {"ADDI", addifl, std::size(addifl)},
    {"ADDQ", addqfl, std::size(addqfl)},
    {"ADDX", addxfl, std::size(addxfl)},
    {"AND", andfl, std::size(andfl)},
    {"ANDI", andifl, std::size(andifl)},
    {"ASL", aslfl, std::size(aslfl)},
    {"ASR", asrfl, std::size(asrfl)},
    {"BCC", bccfl, std::size(bccfl)},
    {"BCHG", bchgfl, std::size(bchgfl)},
    {"BCLR", bclrfl, std::size(bclrfl)},
    {"BCS", bcsfl, std::size(bcsfl)},
    {"BEQ", beqfl, std::size(beqfl)},
    {"BFCHG", bfchgfl, std::size(bfchgfl)},
    {"BFCLR", bfclrfl, std::size(bfclrfl)},
    {"BFEXTS", bfextsfl, std::size(bfextsfl)},
    {"BFEXTU", bfextufl, std::size(bfextufl)},
    {"BFFFO", bfffofl, std::size(bfffofl)},
    {"BFINS", bfinsfl, std::size(bfinsfl)},
    {"BFSET", bfsetfl, std::size(bfsetfl)},
    {"BFTST", bftstfl, std::size(bftstfl)},
    {"BGE", bgefl, std::size(bgefl)},
    {"BGT", bgtfl, std::size(bgtfl)},
    {"BHI", bhifl, std::size(bhifl)},
    {"BHS", bccfl, std::size(bccfl)},
    {"BLE", blefl, std::size(blefl)},
    {"BLO", bcsfl, std::size(bcsfl)},
    {"BLS", blsfl, std::size(blsfl)},
    {"BLT", bltfl, std::size(bltfl)},
    {"BMI", bmifl, std::size(bmifl)},
    {"BNE", bnefl, std::size(bnefl)},
    {"BPL", bplfl, std::size(bplfl)},
    {"BRA", brafl, std::size(brafl)},
    {"BSET", bsetfl, std::size(bsetfl)},
    {"BSR", bsrfl, std::size(bsrfl)},
    {"BTST", btstfl, std::size(btstfl)},
    {"BVC", bvcfl, std::size(bvcfl)},
    {"BVS", bvsfl, std::size(bvsfl)},
    {"CHK", chkfl, std::size(chkfl)},
    {"CLR", clrfl, std::size(clrfl)},
    {"CMP", cmpfl, std::size(cmpfl)},
    {"CMPA", cmpafl, std::size(cmpafl)},
    {"CMPI", cmpifl, std::size(cmpifl)},
    {"CMPM", cmpmfl, std::size(cmpmfl)},
    {"DBCC", dbccfl, std::size(dbccfl)},
    {"DBCS", dbcsfl, std::size(dbcsfl)},
    {"DBEQ", dbeqfl, std::size(dbeqfl)},
    {"DBF", dbffl, std::size(dbffl)},
    {"DBGE", dbgefl, std::size(dbgefl)},
    {"DBGT", dbgtfl, std::size(dbgtfl)},
    {"DBHI", dbhifl, std::size(dbhifl)},
    {"DBHS", dbccfl, std::size(dbccfl)},
    {"DBLE", dblefl, std::size(dblefl)},
    {"DBLO", dbcsfl, std::size(dbcsfl)},
    {"DBLS", dblsfl, std::size(dblsfl)},
    {"DBLT", dbltfl, std::size(dbltfl)},
    {"DBMI", dbmifl, std::size(dbmifl)},
    {"DBNE", dbnefl, std::size(dbnefl)},
    {"DBPL", dbplfl, std::size(dbplfl)},
    {"DBRA", dbrafl, std::size(dbrafl)},
    {"DBT", dbtfl, std::size(dbtfl)},
    {"DBVC", dbvcfl, std::size(dbvcfl)},
    {"DBVS", dbvsfl, std::size(dbvsfl)},
    {"DIVS", divsfl, std::size(divsfl)},
    {"DIVU", divufl, std::size(divufl)},
    {"EOR", eorfl, std::size(eorfl)},
    {"EORI", eorifl, std::size(eorifl)},
    {"EXG", exgfl, std::size(exgfl)},
    {"EXT", extfl, std::size(extfl)},
    {"ILLEGAL", illegalfl, std::size(illegalfl)},
    {"JMP", jmpfl, std::size(jmpfl)},
    {"JSR", jsrfl, std::size(jsrfl)},
    {"LEA", leafl, std::size(leafl)},
    {"LINK", linkfl, std::size(linkfl)},
    {"LSL", lslfl, std::size(lslfl)},
    {"LSR", lsrfl, std::size(lsrfl)},
    {"MOVE", movefl, std::size(movefl)},
    {"MOVEA", moveafl, std::size(moveafl)},
    {"MOVEP", movepfl, std::size(movepfl)},
    {"MOVEQ", moveqfl, std::size(moveqfl)},
    {"MULS", mulsfl, std::size(mulsfl)},
    {"MULU", mulufl, std::size(mulufl)},
    {"NBCD", nbcdfl, std::size(nbcdfl)},
    {"NEG", negfl, std::size(negfl)},
    {"NEGX", negxfl, std::size(negxfl)},
    {"NOP", nopfl, std::size(nopfl)},
    {"NOT", notfl, std::size(notfl)},
    {"OR", orfl, std::size(orfl)},
    {"ORI", orifl, std::size(orifl)},
    {"PEA", peafl, std::size(peafl)},
    {"RESET", resetfl, std::size(resetfl)},
    {"ROL", rolfl, std::size(rolfl)},
    {"ROR", rorfl, std::size(rorfl)},
    {"ROXL", roxlfl, std::size(roxlfl)},
    {"ROXR", roxrfl, std::size(roxrfl)},
    {"RTE", rtefl, std::size(rtefl)},
    {"RTR", rtrfl, std::size(rtrfl)},
    {"RTS", rtsfl, std::size(rtsfl)},
    {"SBCD", sbcdfl, std::size(sbcdfl)},
    {"SCC", sccfl, std::size(sccfl)},
    {"SCS", scsfl, std::size(scsfl)},
    {"SEQ", seqfl, std::size(seqfl)},
    {"SF", sffl, std::size(sffl)},
    {"SGE", sgefl, std::size(sgefl)},
    {"SGT", sgtfl, std::size(sgtfl)},
    {"SHI", shifl, std::size(shifl)},
    {"SHS", sccfl, std::size(sccfl)},
    {"SLE", slefl, std::size(slefl)},
    {"SLO", scsfl, std::size(scsfl)},
    {"SLS", slsfl, std::size(slsfl)},
    {"SLT", sltfl, std::size(sltfl)},
    {"SMI", smifl, std::size(smifl)},
    {"SNE", snefl, std::size(snefl)},
    {"SPL", splfl, std::size(splfl)},
    {"ST", stfl, std::size(stfl)},
    {"STOP", stopfl, std::size(stopfl)},
    {"SUB", subfl, std::size(subfl)},
    {"SUBA", subafl, std::size(subafl)},
    {"SUBI", subifl, std::size(subifl)},
    {"SUBQ", subqfl, std::size(subqfl)},
    {"SUBX", subxfl, std::size(subxfl)},
    {"SVC", svcfl, std::size(svcfl)},
    {"SVS", svsfl, std::size(svsfl)},
    {"SWAP", swapfl, std::size(swapfl)},
    {"TAS", tasfl, std::size(tasfl)},
    {"TRAP", trapfl, std::size(trapfl)},
    {"TRAPV", trapvfl, std::size(trapvfl)},
    {"TST", tstfl, std::size(tstfl)},
    {"UNLK", unlkfl, std::size(unlkfl)},
};
// clang-format on

// ===========================================================================
// ASSEMBLE.CPP
// ===========================================================================

// pickMask(): note the original's dangling else — an unspecified size picks
// the word mask whatever the flavor's sizes; an illegal size reports
// INV_SIZE_CODE and also picks the word mask.
uint16_t pickMask(int size, const Flavor* flavorPtr, int& status) {
  if (!size || size & flavorPtr->sizes) {
    if (size & (BYTE_SIZE | SHORT_SIZE))
      return flavorPtr->bytemask;
    else if (!size || size == WORD_SIZE)
      return flavorPtr->wordmask;
    else
      return flavorPtr->longmask;
  }
  NewError(status, status::kInvSizeCode);
  return flavorPtr->wordmask;
}

}  // namespace

const Instruction* LookupInstruction(const std::string& mnemonic) {
  const Instruction* begin = std::begin(kInstructions);
  const Instruction* end = std::end(kInstructions);
  const Instruction* it =
      std::lower_bound(begin, end, mnemonic, [](const Instruction& i, const std::string& m) {
        return std::strcmp(i.mnemonic, m.c_str()) < 0;
      });
  if (it != end && mnemonic == it->mnemonic)
    return it;
  return nullptr;
}

bool IsBitFieldFlavor(const Flavor& flavor) {
  return flavor.exec == bitField;
}

const std::vector<Instruction>& AllInstructions() {
  static const std::vector<Instruction> all(std::begin(kInstructions), std::end(kInstructions));
  return all;
}

uint16_t PickMask(int size, const Flavor& flavor, int& status) {
  return pickMask(size, &flavor, status);
}

bool IsBitFieldInstruction(const Instruction& instruction) {
  return instruction.flavor_count > 0 && instruction.flavors[0].exec == bitField;
}

}  // namespace easym68k::asm_
