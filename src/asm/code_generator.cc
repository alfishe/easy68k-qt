// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Code Generator — Tasks 8.5.1 through 8.5.3.
// Task 8.5.1: kFixed and kMoveq fully encoded; all others stub.
// Task 8.5.2: EffAddr() and ExtWords() ported from CODEGEN.CPP.
// Task 8.5.3: Full instruction encoding handlers (TODO).

#include "easym68k/asm/code_generator.h"

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
// Encode — dispatch to per-encoding handler
// ---------------------------------------------------------------------------

// TODO(8.5.3): location_counter and symbols are suppressed until the EA-based
// encoding handlers need them for PC-relative displacement and symbol resolution.
bool CodeGenerator::Encode(const InstrEntry& entry, const ParsedLine& line,
                           uint32_t /*location_counter*/, const SymbolTable& /*symbols*/,
                           const EmitWordFn& emit_word, std::string* error_msg) {
  switch (entry.encoding) {
    // Fixed 1-word opcodes: the base IS the complete instruction.
    case InstrEncoding::kFixed:
      emit_word(entry.base);
      return true;

    // MOVEQ #imm,Dn — encoding: 0111 DDD0 iiiiiiii
    case InstrEncoding::kMoveq: {
      if (line.operands.size() < 2) {
        *error_msg = "MOVEQ requires two operands";
        return false;
      }
      const Operand& src = line.operands[0];
      const Operand& dst = line.operands[1];
      if (src.mode != AddressMode::kImmediate) {
        *error_msg = "MOVEQ source must be immediate";
        return false;
      }
      if (dst.mode != AddressMode::kDnDirect) {
        *error_msg = "MOVEQ destination must be data register";
        return false;
      }
      int8_t imm8 = static_cast<int8_t>(src.data & 0xFF);
      uint16_t word =
          static_cast<uint16_t>(0x7000 | ((dst.reg & 7) << 9) | static_cast<uint8_t>(imm8));
      emit_word(word);
      return true;
    }

    default:
      // Stub: emit base opcode word only.  Full encoding is implemented in
      // Task 8.5.3 (instruction handlers).
      emit_word(entry.base);
      return true;
  }
}

}  // namespace easym68k::asm_
