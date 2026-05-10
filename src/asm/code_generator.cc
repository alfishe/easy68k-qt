// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Code Generator — Tasks 8.5.1 through 8.5.3.
// Task 8.5.1: kFixed and kMoveq fully encoded; all others stub.
// Task 8.5.2: EffAddr() and ExtWords() ported from CODEGEN.CPP.
// Task 8.5.3: Full instruction encoding handlers ported from BUILD.CPP.

#include "easym68k/asm/code_generator.h"

#include "easym68k/asm/instruction_table.h"

namespace easym68k::asm_ {

// ---------------------------------------------------------------------------
// EffAddr — CODEGEN.CPP effAddr()
// ---------------------------------------------------------------------------

int CodeGenerator::EffAddr(const Operand& op) {
  switch (op.mode) {
    case AddressMode::kDnDirect:
      return 0x00 | (op.reg & 7);
    case AddressMode::kAnDirect:
      return 0x08 | (op.reg & 7);
    case AddressMode::kAnIndirect:
      return 0x10 | (op.reg & 7);
    case AddressMode::kAnIndirectPost:
      return 0x18 | (op.reg & 7);
    case AddressMode::kAnIndirectPre:
      return 0x20 | (op.reg & 7);
    case AddressMode::kAnIndirectDisp:
      return 0x28 | (op.reg & 7);
    case AddressMode::kAnIndirectIdx:
      return 0x30 | (op.reg & 7);
    case AddressMode::kAbsShort:
      return 0x38;
    case AddressMode::kAbsLong:
      return 0x39;
    case AddressMode::kPCDisp:
      return 0x3A;
    case AddressMode::kPCIndex:
      return 0x3B;
    case AddressMode::kImmediate:
      return 0x3C;
    default:
      return -1;  // SR, CCR, USP, etc. — handled by their own instruction encoders
  }
}

// ---------------------------------------------------------------------------
// ExtWords — CODEGEN.CPP extWords()
// Emits extension word(s) for op into the output stream and advances loc.
// PC-relative displacement uses disp = value - (loc + 2) per M68000 PRM §2.2.
// ---------------------------------------------------------------------------

bool CodeGenerator::ExtWords(const Operand& op, SizeSpec size, uint32_t& loc,
                             const EmitWordFn& emit_fn, std::string* error_msg) {
  switch (op.mode) {
    case AddressMode::kDnDirect:
    case AddressMode::kAnDirect:
    case AddressMode::kAnIndirect:
    case AddressMode::kAnIndirectPost:
    case AddressMode::kAnIndirectPre:
      // Register-direct and simple indirect — no extension words.
      break;

    case AddressMode::kAnIndirectDisp: {
      int32_t disp = op.data;
      emit_fn(static_cast<uint16_t>(disp & 0xFFFF));
      loc += 2;
      if (disp < -32768 || disp > 32767) {
        *error_msg = "INV_DISP: displacement out of 16-bit range";
        return false;
      }
      break;
    }

    case AddressMode::kAnIndirectIdx:
    case AddressMode::kPCIndex: {
      int32_t disp =
          (op.mode == AddressMode::kPCIndex) ? op.data - static_cast<int32_t>(loc + 2) : op.data;
      uint16_t ext = static_cast<uint16_t>(((op.index_reg & 0xF) << 12) |
                                           ((op.index_size == SizeSpec::kLong ? 1 : 0) << 11) |
                                           (static_cast<uint8_t>(disp & 0xFF)));
      emit_fn(ext);
      loc += 2;
      if (disp < -128 || disp > 127) {
        *error_msg = "INV_DISP: indexed displacement out of 8-bit range";
        return false;
      }
      break;
    }

    case AddressMode::kPCDisp: {
      // Displacement relative to PC, which points to the word after the ext word.
      int32_t disp = op.data - static_cast<int32_t>(loc + 2);
      emit_fn(static_cast<uint16_t>(disp & 0xFFFF));
      loc += 2;
      if (disp < -32768 || disp > 32767) {
        *error_msg = "INV_DISP: PC-relative displacement out of 16-bit range";
        return false;
      }
      break;
    }

    case AddressMode::kAbsShort: {
      emit_fn(static_cast<uint16_t>(op.data & 0xFFFF));
      loc += 2;
      if (op.data < -32768 || op.data > 32767) {
        *error_msg = "INV_ABS_ADDRESS: absolute short address out of 16-bit range";
        return false;
      }
      break;
    }

    case AddressMode::kAbsLong:
      emit_fn(static_cast<uint16_t>((static_cast<uint32_t>(op.data) >> 16) & 0xFFFF));
      emit_fn(static_cast<uint16_t>(static_cast<uint32_t>(op.data) & 0xFFFF));
      loc += 4;
      break;

    case AddressMode::kImmediate: {
      if (size == SizeSpec::kByte) {
        emit_fn(static_cast<uint16_t>(static_cast<uint8_t>(op.data)));
        loc += 2;
        if (op.data < -128 || op.data > 255) {
          *error_msg = "INV_8_BIT_DATA: immediate byte value out of range";
          return false;
        }
      } else if (size == SizeSpec::kLong) {
        emit_fn(static_cast<uint16_t>((static_cast<uint32_t>(op.data) >> 16) & 0xFFFF));
        emit_fn(static_cast<uint16_t>(static_cast<uint32_t>(op.data) & 0xFFFF));
        loc += 4;
      } else {
        // kWord or kNone — treat as 16-bit.
        emit_fn(static_cast<uint16_t>(op.data & 0xFFFF));
        loc += 2;
        if (op.data < -32768 || op.data > 65535) {
          *error_msg = "INV_16_BIT_DATA: immediate value out of 16-bit range";
          return false;
        }
      }
      break;
    }

    default:
      *error_msg = "invalid addressing mode in extension word";
      return false;
  }
  return true;
}

// ---------------------------------------------------------------------------
// Encode — dispatch to per-encoding handler (BUILD.CPP)
// ---------------------------------------------------------------------------

namespace {

// 2-bit size field used by most 68000 instructions (bits 7-6): B=0, W=1, L=2.
inline uint16_t SizeBits(SizeSpec sz) {
  int s = InstructionTable::SizeField(sz);
  return static_cast<uint16_t>(s >= 0 ? (s << 6) : 0);
}

// For instructions where multi-size means the 2-bit size field applies.
// Single-size instructions (e.g. PEA, TAS, NBCD) do not carry a size field.
inline bool HasSizeField(uint8_t sizes) {
  uint8_t bwl = sizes & 0x07;
  return bwl != 0 && (bwl & (bwl - 1)) != 0;  // more than one bit set
}

// Emit ADDQ/SUBQ from a quick-data immediate (data 1–8, size W or L).
// Returns the encoded opcode word; does NOT call emit_word.
inline uint16_t QuickOpcode(uint16_t base_add_sub, SizeSpec sz, int32_t data, int dst_ea) {
  // base_add_sub: 0x5000 = ADDQ base, 0x5100 = SUBQ base (byte form).
  uint16_t qbase = base_add_sub;
  if (sz == SizeSpec::kLong)
    qbase |= 0x0080;
  else if (sz == SizeSpec::kWord)
    qbase |= 0x0040;
  return static_cast<uint16_t>(qbase | ((data & 7) << 9) | dst_ea);
}

// The immediate-form opcode base for an ADD/SUB/AND/OR instruction.
// ADD(0xD000)→ADDI(0x0600), SUB→SUBI, AND→ANDI, OR→ORI.
inline uint16_t ImmFormBase(uint16_t base) {
  switch (base & 0xF000) {
    case 0xD000:
      return 0x0600;
    case 0x9000:
      return 0x0400;
    case 0xC000:
      return 0x0200;
    case 0x8000:
      return 0x0000;
    default:
      return base;
  }
}

// True if the ADDQ/SUBQ peephole should fire for a given source immediate.
inline bool CanUseQuick(const Operand& src, SizeSpec sz) {
  return src.mode == AddressMode::kImmediate && src.is_back_ref && src.data >= 1 && src.data <= 8 &&
         src.forced_size == SizeSpec::kNone && sz != SizeSpec::kByte;
}

}  // namespace

bool CodeGenerator::Encode(const InstrEntry& entry, const ParsedLine& line,
                           uint32_t location_counter, const SymbolTable& /*symbols*/,
                           const EmitWordFn& emit_word, std::string* error_msg) {
  using AM = AddressMode;
  const SizeSpec size = (line.size != SizeSpec::kNone) ? line.size : SizeSpec::kWord;

  // Convenience accessors — bounds-checked by the individual cases.
  auto src = [&]() -> const Operand& { return line.operands[0]; };
  auto dst = [&]() -> const Operand& { return line.operands[1]; };

  // Helper: emit opcode word + ExtWords for one operand.
  auto emit_ea1 = [&](uint16_t mask, const Operand& op) -> bool {
    emit_word(mask);
    uint32_t ext_loc = location_counter + 2;
    return ExtWords(op, size, ext_loc, emit_word, error_msg);
  };

  // Helper: emit opcode + ExtWords for two operands.
  auto emit_ea2 = [&](uint16_t mask, const Operand& op0, const Operand& op1) -> bool {
    emit_word(mask);
    uint32_t ext_loc = location_counter + 2;
    if (!ExtWords(op0, size, ext_loc, emit_word, error_msg))
      return false;
    return ExtWords(op1, size, ext_loc, emit_word, error_msg);
  };

  switch (entry.encoding) {
    // -----------------------------------------------------------------------
    // kFixed — zeroOp(): emit the base opcode, no operands (NOP, RTE, etc.)
    // -----------------------------------------------------------------------
    case InstrEncoding::kFixed:
      emit_word(entry.base);
      return true;

    // -----------------------------------------------------------------------
    // kFixedWord — immedWord() variant for STOP: emit base + immediate word.
    // -----------------------------------------------------------------------
    case InstrEncoding::kFixedWord: {
      emit_word(entry.base);
      int32_t val = src().data;
      if (val < -32768 || val > 65535) {
        *error_msg = "INV_16_BIT_DATA: immediate out of 16-bit range";
        return false;
      }
      emit_word(static_cast<uint16_t>(val & 0xFFFF));
      return true;
    }

    // -----------------------------------------------------------------------
    // kMoveq — MOVEQ #imm,Dn: 0111 DDD0 iiiiiiii
    // -----------------------------------------------------------------------
    case InstrEncoding::kMoveq: {
      if (line.operands.size() < 2) {
        *error_msg = "MOVEQ requires two operands";
        return false;
      }
      if (src().mode != AM::kImmediate) {
        *error_msg = "MOVEQ source must be immediate";
        return false;
      }
      if (dst().mode != AM::kDnDirect) {
        *error_msg = "MOVEQ destination must be data register";
        return false;
      }
      emit_word(static_cast<uint16_t>(0x7000 | ((dst().reg & 7) << 9) |
                                      static_cast<uint8_t>(src().data & 0xFF)));
      return true;
    }

    // -----------------------------------------------------------------------
    // kRegOp — oneReg(): emit base | Dn  (SWAP, EXT)
    // EXT has a 1-bit size field at bit 6: W=0, L=0x40.
    // -----------------------------------------------------------------------
    case InstrEncoding::kRegOp: {
      uint16_t mask = static_cast<uint16_t>(entry.base | (src().reg & 7));
      if (size == SizeSpec::kLong)
        mask |= 0x0040;
      emit_word(mask);
      return true;
    }

    // -----------------------------------------------------------------------
    // kAnOp — oneReg() for An operand (UNLK): emit base | An
    // -----------------------------------------------------------------------
    case InstrEncoding::kAnOp:
      emit_word(static_cast<uint16_t>(entry.base | (src().reg & 7)));
      return true;

    // -----------------------------------------------------------------------
    // kEaOnly — oneOp(): emit base | [size] | EA + ExtWords
    // Used by CLR, NEG, NEGX, NOT, TST, TAS, NBCD, PEA.
    // -----------------------------------------------------------------------
    case InstrEncoding::kEaOnly: {
      uint16_t mask = entry.base;
      if (HasSizeField(entry.sizes))
        mask |= SizeBits(size);
      return emit_ea1(static_cast<uint16_t>(mask | EffAddr(src())), src());
    }

    // -----------------------------------------------------------------------
    // kJmpJsr — oneOp() without size (JMP, JSR)
    // -----------------------------------------------------------------------
    case InstrEncoding::kJmpJsr:
      return emit_ea1(static_cast<uint16_t>(entry.base | EffAddr(src())), src());

    // -----------------------------------------------------------------------
    // kEaToReg — arithReg() form: <ea>,Dn  (CMP)
    // -----------------------------------------------------------------------
    case InstrEncoding::kEaToReg:
      return emit_ea1(
          static_cast<uint16_t>(entry.base | SizeBits(size) | (dst().reg << 9) | EffAddr(src())),
          src());

    // -----------------------------------------------------------------------
    // kRegToEa — arithAddr() form: Dn,<ea>  (EOR)
    // -----------------------------------------------------------------------
    case InstrEncoding::kRegToEa:
      return emit_ea1(
          static_cast<uint16_t>(entry.base | SizeBits(size) | (src().reg << 9) | EffAddr(dst())),
          dst());

    // -----------------------------------------------------------------------
    // kAddrEa — arithReg() for ADDA/SUBA/CMPA: <ea>,An
    // 1-bit size at bit 8 (W=0, L=0x100).  Peephole: ADDA/SUBA #1-8 → ADDQ/SUBQ.
    // -----------------------------------------------------------------------
    case InstrEncoding::kAddrEa: {
      uint16_t top = entry.base & 0xF000;
      if ((top == 0xD000 || top == 0x9000) && CanUseQuick(src(), size)) {
        uint16_t qbase = (top == 0xD000) ? 0x5000 : 0x5100;
        return emit_ea1(QuickOpcode(qbase, size, src().data, EffAddr(dst())), dst());
      }
      uint16_t size_bit = (size == SizeSpec::kLong) ? 0x0100u : 0x0000u;
      return emit_ea1(
          static_cast<uint16_t>(entry.base | size_bit | (dst().reg << 9) | EffAddr(src())), src());
    }

    // -----------------------------------------------------------------------
    // kMulDiv — arithReg() for MULS/MULU/DIVS/DIVU: <ea>,Dn (always word)
    // -----------------------------------------------------------------------
    case InstrEncoding::kMulDiv:
      return emit_ea1(static_cast<uint16_t>(entry.base | (dst().reg << 9) | EffAddr(src())), src());

    // -----------------------------------------------------------------------
    // kLea — LEA <ea>,An
    // -----------------------------------------------------------------------
    case InstrEncoding::kLea:
      return emit_ea1(static_cast<uint16_t>(entry.base | (dst().reg << 9) | EffAddr(src())), src());

    // -----------------------------------------------------------------------
    // kChk — CHK <ea>,Dn (word only)
    // -----------------------------------------------------------------------
    case InstrEncoding::kChk:
      return emit_ea1(static_cast<uint16_t>(entry.base | (dst().reg << 9) | EffAddr(src())), src());

    // -----------------------------------------------------------------------
    // kMoveSr — MOVE SR,<ea> or MOVE <ea>,SR (no table entries yet; no-op)
    // -----------------------------------------------------------------------
    case InstrEncoding::kMoveSr:
      return emit_ea1(static_cast<uint16_t>(entry.base | EffAddr(src())), src());

    // -----------------------------------------------------------------------
    // kMoveUsp — MOVE USP,An / MOVE An,USP (no table entries yet; no-op)
    // -----------------------------------------------------------------------
    case InstrEncoding::kMoveUsp:
      emit_word(entry.base);
      return true;

    // -----------------------------------------------------------------------
    // kEaBidirect — ADD/SUB/AND/OR: <ea>,Dn  or  Dn,<ea>
    // Detects direction from operand modes; handles immediate sub-forms
    // (immedToCCR, immedToSR, immedInst) and the ADDQ/SUBQ peephole.
    // -----------------------------------------------------------------------
    case InstrEncoding::kEaBidirect: {
      if (src().mode == AM::kImmediate) {
        if (dst().mode == AM::kCCRDirect) {
          // AND/OR #n,CCR → ANDI/ORI to CCR
          uint16_t ccr_base = static_cast<uint16_t>((ImmFormBase(entry.base) & 0xFF00) | 0x003C);
          emit_word(ccr_base);
          emit_word(static_cast<uint16_t>(src().data & 0xFF));
          return true;
        }
        if (dst().mode == AM::kSRDirect) {
          uint16_t sr_base = static_cast<uint16_t>((ImmFormBase(entry.base) & 0xFF00) | 0x007C);
          emit_word(sr_base);
          emit_word(static_cast<uint16_t>(src().data & 0xFFFF));
          return true;
        }
        // ADDQ/SUBQ peephole for ADD/SUB with immediate
        uint16_t top = entry.base & 0xF000;
        if ((top == 0xD000 || top == 0x9000) && CanUseQuick(src(), size)) {
          uint16_t qbase = (top == 0xD000) ? 0x5000 : 0x5100;
          return emit_ea1(QuickOpcode(qbase, size, src().data, EffAddr(dst())), dst());
        }
        // Immediate to EA: use ADDI/SUBI/ANDI/ORI encoding
        uint16_t imm_base = ImmFormBase(entry.base);
        return emit_ea2(static_cast<uint16_t>(imm_base | SizeBits(size) | EffAddr(dst())), src(),
                        dst());
      }
      if (dst().mode == AM::kDnDirect) {
        // <ea>,Dn form — direction bit = 0 (already clear in base)
        return emit_ea1(
            static_cast<uint16_t>(entry.base | SizeBits(size) | (dst().reg << 9) | EffAddr(src())),
            src());
      }
      // Dn,<ea> form — set direction bit (bit 8)
      return emit_ea1(static_cast<uint16_t>((entry.base | 0x0100) | SizeBits(size) |
                                            (src().reg << 9) | EffAddr(dst())),
                      dst());
    }

    // -----------------------------------------------------------------------
    // kImmedToEa — ADDI/SUBI/ANDI/ORI/EORI/CMPI: #imm,<ea>
    // Handles CCR/SR sub-forms and ADDQ/SUBQ peephole.
    // -----------------------------------------------------------------------
    case InstrEncoding::kImmedToEa: {
      if (dst().mode == AM::kCCRDirect) {
        uint16_t ccr_base = static_cast<uint16_t>((entry.base & 0xFF00) | 0x003C);
        emit_word(ccr_base);
        emit_word(static_cast<uint16_t>(src().data & 0xFF));
        return true;
      }
      if (dst().mode == AM::kSRDirect) {
        uint16_t sr_base = static_cast<uint16_t>((entry.base & 0xFF00) | 0x007C);
        emit_word(sr_base);
        emit_word(static_cast<uint16_t>(src().data & 0xFFFF));
        return true;
      }
      // ADDQ/SUBQ peephole: ADDI/SUBI #1-8 → ADDQ/SUBQ
      uint16_t itype = entry.base & 0xFF00;
      if ((itype == 0x0600 || itype == 0x0400) && CanUseQuick(src(), size)) {
        uint16_t qbase = (itype == 0x0600) ? 0x5000 : 0x5100;
        return emit_ea1(QuickOpcode(qbase, size, src().data, EffAddr(dst())), dst());
      }
      return emit_ea2(static_cast<uint16_t>(entry.base | SizeBits(size) | EffAddr(dst())), src(),
                      dst());
    }

    // -----------------------------------------------------------------------
    // kImmedToCcr — ANDI/EORI/ORI to CCR: emit base + byte word
    // -----------------------------------------------------------------------
    case InstrEncoding::kImmedToCcr:
      emit_word(entry.base);
      emit_word(static_cast<uint16_t>(src().data & 0xFF));
      return true;

    // -----------------------------------------------------------------------
    // kImmedToSr — ANDI/EORI/ORI to SR / RTD / STOP: emit base + word
    // -----------------------------------------------------------------------
    case InstrEncoding::kImmedToSr:
      emit_word(entry.base);
      emit_word(static_cast<uint16_t>(src().data & 0xFFFF));
      return true;

    // -----------------------------------------------------------------------
    // kQuick — ADDQ/SUBQ: quickMath()
    // -----------------------------------------------------------------------
    case InstrEncoding::kQuick: {
      int32_t data = src().data;
      if (data < 1 || data > 8) {
        *error_msg = "INV_QUICK_CONST: quick constant must be 1-8";
        return false;
      }
      return emit_ea1(QuickOpcode(entry.base, size, data, EffAddr(dst())), dst());
    }

    // -----------------------------------------------------------------------
    // kBranch — Bcc/BRA/BSR: short (±127 byte) or word (±32767) displacement.
    // Displacement formula: disp = target - (loc + 2), per M68000 PRM.
    // -----------------------------------------------------------------------
    case InstrEncoding::kBranch: {
      int32_t disp = src().data - static_cast<int32_t>(location_counter) - 2;
      bool short_disp = (size == SizeSpec::kShort || size == SizeSpec::kByte) ||
                        (size != SizeSpec::kLong && size != SizeSpec::kWord && src().is_back_ref &&
                         disp >= -128 && disp <= 127 && disp != 0);
      if (short_disp) {
        emit_word(static_cast<uint16_t>(entry.base | (disp & 0xFF)));
        if (disp < -128 || disp > 127 || disp == 0) {
          *error_msg = "INV_BRANCH_DISP: short branch displacement out of range";
          return false;
        }
      } else {
        emit_word(entry.base);
        emit_word(static_cast<uint16_t>(disp & 0xFFFF));
        if (disp < -32768 || disp > 32767) {
          *error_msg = "INV_BRANCH_DISP: word branch displacement out of range";
          return false;
        }
      }
      return true;
    }

    // -----------------------------------------------------------------------
    // kDBcc — DBcc Dn,<label>: emit base | Dn + displacement word
    // -----------------------------------------------------------------------
    case InstrEncoding::kDBcc: {
      int32_t disp = dst().data - static_cast<int32_t>(location_counter) - 2;
      emit_word(static_cast<uint16_t>(entry.base | (src().reg & 7)));
      emit_word(static_cast<uint16_t>(disp & 0xFFFF));
      if (disp < -32768 || disp > 32767) {
        *error_msg = "INV_BRANCH_DISP: DBcc displacement out of range";
        return false;
      }
      return true;
    }

    // -----------------------------------------------------------------------
    // kScc — Scc <ea>: scc() — same as oneOp without size
    // -----------------------------------------------------------------------
    case InstrEncoding::kScc:
      return emit_ea1(static_cast<uint16_t>(entry.base | EffAddr(src())), src());

    // -----------------------------------------------------------------------
    // kShift — shiftReg() or memory oneOp.
    // Register form (2 operands): count is src (imm or Dn), dest is dst (Dn).
    // Memory form (1 operand): shift <ea> — always word, special mask.
    // -----------------------------------------------------------------------
    case InstrEncoding::kShift: {
      if (line.operands.size() == 1) {
        // Memory form: rebuild opcode from direction and shift-type bits in base.
        int dir = (entry.base >> 8) & 1;
        int type = (entry.base >> 3) & 3;
        uint16_t mask = static_cast<uint16_t>(0xE0C0 | (dir << 8) | (type << 9) | EffAddr(src()));
        return emit_ea1(mask, src());
      }
      // Register form: reconstruct with correct size bits and i/r flag.
      int dir = (entry.base >> 8) & 1;
      int type = (entry.base >> 3) & 3;
      int sz2 = InstructionTable::SizeField(size);
      uint16_t mask;
      if (src().mode == AM::kImmediate) {
        int32_t cnt = src().data;
        if (cnt < 1 || cnt > 8) {
          *error_msg = "INV_SHIFT_COUNT: shift count must be 1-8";
          return false;
        }
        mask = static_cast<uint16_t>(0xE000 | ((cnt & 7) << 9) | (dir << 8) | (sz2 << 6) |
                                     (type << 3) | (dst().reg & 7));
      } else {
        // Register count form (bit 5 = 1)
        mask = static_cast<uint16_t>(0xE000 | (src().reg << 9) | (dir << 8) | (sz2 << 6) | 0x20 |
                                     (type << 3) | (dst().reg & 7));
      }
      emit_word(mask);
      return true;
    }

    // -----------------------------------------------------------------------
    // kBitReg — BTST/BCHG/BCLR/BSET: dynamic (Dn,<ea>) or static (#n,<ea>).
    // Dynamic: arithAddr() style.  Static: staticBit() style.
    // -----------------------------------------------------------------------
    case InstrEncoding::kBitReg: {
      if (src().mode == AM::kDnDirect) {
        return emit_ea1(static_cast<uint16_t>(entry.base | (src().reg << 9) | EffAddr(dst())),
                        dst());
      }
      // Static immediate bit number — static form: clear bit 8, set bit 11.
      uint16_t static_base = static_cast<uint16_t>((entry.base & ~0x0100u) | 0x0800u);
      emit_word(static_cast<uint16_t>(static_base | EffAddr(dst())));
      emit_word(static_cast<uint16_t>(src().data & 0xFF));
      uint32_t ext_loc = location_counter + 4;
      return ExtWords(dst(), size, ext_loc, emit_word, error_msg);
    }

    // -----------------------------------------------------------------------
    // kBitImm — static bit ops with immediate count (separate encoding entry)
    // -----------------------------------------------------------------------
    case InstrEncoding::kBitImm: {
      emit_word(static_cast<uint16_t>(entry.base | EffAddr(dst())));
      emit_word(static_cast<uint16_t>(src().data & 0xFF));
      uint32_t ext_loc = location_counter + 4;
      return ExtWords(dst(), size, ext_loc, emit_word, error_msg);
    }

    // -----------------------------------------------------------------------
    // kMove — MOVE <ea>,<ea>: special destination EA encoding + peephole.
    // Dest EA bits 11-9 = dest reg, bits 8-6 = dest mode (re-mapped).
    // Peephole: MOVE.L #imm,Dn → MOVEQ when imm fits signed 8 bits.
    // Also handles MOVE <ea>,CCR/SR and MOVE An,USP.
    // -----------------------------------------------------------------------
    case InstrEncoding::kMove: {
      if (line.operands.size() < 2) {
        *error_msg = "MOVE requires two operands";
        return false;
      }
      // Special destination forms
      if (dst().mode == AM::kCCRDirect) {
        return emit_ea1(static_cast<uint16_t>(0x44C0 | EffAddr(src())), src());
      }
      if (dst().mode == AM::kSRDirect) {
        return emit_ea1(static_cast<uint16_t>(0x46C0 | EffAddr(src())), src());
      }
      if (dst().mode == AM::kUSPDirect) {
        // MOVE An,USP
        emit_word(static_cast<uint16_t>(0x4E60 | (src().reg & 7)));
        return true;
      }
      // MOVEQ peephole: MOVE.L #imm,Dn where imm fits signed 8 bits
      if (src().mode == AM::kImmediate && src().is_back_ref && src().data >= -128 &&
          src().data <= 127 && dst().mode == AM::kDnDirect && size == SizeSpec::kLong &&
          src().forced_size == SizeSpec::kNone) {
        emit_word(static_cast<uint16_t>(0x7000 | ((dst().reg & 7) << 9) |
                                        static_cast<uint8_t>(src().data & 0xFF)));
        return true;
      }
      // Normal MOVE: size selects the base opcode word.
      uint16_t move_base;
      if (size == SizeSpec::kLong)
        move_base = 0x2000;
      else if (size == SizeSpec::kByte)
        move_base = 0x1000;
      else
        move_base = 0x3000;  // word (default)
      int dst_ea = EffAddr(dst());
      uint16_t mask = static_cast<uint16_t>(move_base | ((dst_ea & 0x38) << 3) |
                                            ((dst_ea & 7) << 9) | EffAddr(src()));
      return emit_ea2(mask, src(), dst());
    }

    // -----------------------------------------------------------------------
    // kMoveA — MOVEA <ea>,An: like MOVE but dest is always An
    // -----------------------------------------------------------------------
    case InstrEncoding::kMoveA: {
      uint16_t move_base = (size == SizeSpec::kLong) ? 0x2000u : 0x3000u;
      int dst_ea = EffAddr(dst());
      uint16_t mask = static_cast<uint16_t>(move_base | ((dst_ea & 0x38) << 3) |
                                            ((dst_ea & 7) << 9) | EffAddr(src()));
      return emit_ea1(mask, src());
    }

    // -----------------------------------------------------------------------
    // kMoveM — MOVEM: stubbed until Task 8.5.4
    // -----------------------------------------------------------------------
    case InstrEncoding::kMoveM:
      emit_word(entry.base);
      return true;

    // -----------------------------------------------------------------------
    // kMoveP — MOVEP Dn,d(An) / MOVEP d(An),Dn: four sub-forms.
    // Converts AnIndirect → AnIndirectDisp(0) as the original does.
    // -----------------------------------------------------------------------
    case InstrEncoding::kMoveP: {
      if (src().mode == AM::kDnDirect) {
        // Dn → d(An): direction=1
        Operand mem = dst();
        if (mem.mode == AM::kAnIndirect) {
          mem.mode = AM::kAnIndirectDisp;
          mem.data = 0;
        }
        uint16_t movep_base = (size == SizeSpec::kLong) ? 0x01C8u : 0x0188u;
        emit_word(static_cast<uint16_t>(movep_base | (src().reg << 9) | (mem.reg & 7)));
        uint32_t ext_loc = location_counter + 2;
        return ExtWords(mem, size, ext_loc, emit_word, error_msg);
      } else {
        // d(An) → Dn: direction=0
        Operand mem = src();
        if (mem.mode == AM::kAnIndirect) {
          mem.mode = AM::kAnIndirectDisp;
          mem.data = 0;
        }
        uint16_t movep_base = (size == SizeSpec::kLong) ? 0x0148u : 0x0108u;
        emit_word(static_cast<uint16_t>(movep_base | (dst().reg << 9) | (mem.reg & 7)));
        uint32_t ext_loc = location_counter + 2;
        return ExtWords(mem, size, ext_loc, emit_word, error_msg);
      }
    }

    // -----------------------------------------------------------------------
    // kTrap — TRAP #v: emit base | (vector & 0xF), validate 0-15
    // -----------------------------------------------------------------------
    case InstrEncoding::kTrap: {
      int32_t vec = src().data;
      emit_word(static_cast<uint16_t>(entry.base | (vec & 0xF)));
      if (vec < 0 || vec > 15) {
        *error_msg = "INV_VECTOR_NUM: TRAP vector must be 0-15";
        return false;
      }
      return true;
    }

    // -----------------------------------------------------------------------
    // kLink — LINK An,#disp: emit base | An + displacement word
    // -----------------------------------------------------------------------
    case InstrEncoding::kLink: {
      emit_word(static_cast<uint16_t>(entry.base | (src().reg & 7)));
      int32_t disp = dst().data;
      emit_word(static_cast<uint16_t>(disp & 0xFFFF));
      if (disp < -32768 || disp > 32767) {
        *error_msg = "INV_16_BIT_DATA: LINK displacement out of range";
        return false;
      }
      return true;
    }

    // -----------------------------------------------------------------------
    // kAddxSubx — ADDX/SUBX: twoReg() with size + predecrement detection
    // -----------------------------------------------------------------------
    case InstrEncoding::kAddxSubx: {
      uint16_t mask =
          static_cast<uint16_t>(entry.base | SizeBits(size) | (dst().reg << 9) | (src().reg & 7));
      if (src().mode == AM::kAnIndirectPre)
        mask |= 0x0008;
      emit_word(mask);
      return true;
    }

    // -----------------------------------------------------------------------
    // kAbcdSbcd — ABCD/SBCD: twoReg() (byte only, no size field)
    // -----------------------------------------------------------------------
    case InstrEncoding::kAbcdSbcd: {
      uint16_t mask = static_cast<uint16_t>(entry.base | (dst().reg << 9) | (src().reg & 7));
      if (src().mode == AM::kAnIndirectPre)
        mask |= 0x0008;
      emit_word(mask);
      return true;
    }

    // -----------------------------------------------------------------------
    // kCmpm — CMPM (An)+,(An)+: twoReg() with size
    // -----------------------------------------------------------------------
    case InstrEncoding::kCmpm:
      emit_word(
          static_cast<uint16_t>(entry.base | SizeBits(size) | (dst().reg << 9) | (src().reg & 7)));
      return true;

    // -----------------------------------------------------------------------
    // kExg — EXG Dx,Dy / Dx,Ay / Ax,Ay: mode bits select form
    // -----------------------------------------------------------------------
    case InstrEncoding::kExg: {
      uint16_t mask;
      if (src().mode == dst().mode) {
        // Both same type: data-data (mode=01000) or addr-addr (mode=01001)
        uint16_t mode_bits = (src().mode == AM::kAnDirect) ? 0x0008u : 0x0000u;
        mask = static_cast<uint16_t>(entry.base | mode_bits | (src().reg << 9) | dst().reg);
      } else {
        // Mixed data/address: data-addr form (mode=10001)
        int dn_reg = (src().mode == AM::kDnDirect) ? src().reg : dst().reg;
        int an_reg = (src().mode == AM::kAnDirect) ? src().reg : dst().reg;
        mask = static_cast<uint16_t>((entry.base & ~0x0040u) | 0x0088u | (dn_reg << 9) | an_reg);
      }
      emit_word(mask);
      return true;
    }

    default:
      emit_word(entry.base);
      return true;
  }
}

}  // namespace easym68k::asm_
