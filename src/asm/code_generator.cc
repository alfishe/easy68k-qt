// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Code Generator — Task 8.5.1 stub.
// kFixed and kMoveq are fully implemented.  All other encodings emit the base
// opcode word as a placeholder until Tasks 8.5.2 and 8.5.3.

#include "easym68k/asm/code_generator.h"

namespace easym68k::asm_ {

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
      // Tasks 8.5.2 (EA encoding) and 8.5.3 (instruction handlers).
      emit_word(entry.base);
      return true;
  }
}

}  // namespace easym68k::asm_
