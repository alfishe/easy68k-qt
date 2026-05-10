// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Code generator — translates parsed instruction lines to 68000 machine code.
// Ported from Edit68K/CODEGEN.CPP and Edit68K/BUILD.CPP (Paul McKee / Charles Kelly).

#ifndef EASYM68K_ASM_CODE_GENERATOR_H_
#define EASYM68K_ASM_CODE_GENERATOR_H_

#include <cstdint>
#include <functional>
#include <string>

#include "easym68k/asm/instruction_table.h"
#include "easym68k/asm/parser.h"
#include "easym68k/asm/symbol_table.h"

namespace easym68k::asm_ {

class CodeGenerator {
 public:
  // Callback used to write one 16-bit word to the output buffer and advance LC.
  using EmitWordFn = std::function<void(uint16_t)>;

  // Encode one instruction into machine code.
  // Called only on pass 2.  Returns true on success; on failure sets
  // *error_msg and returns false.
  bool Encode(const InstrEntry& entry, const ParsedLine& line, uint32_t location_counter,
              const SymbolTable& symbols, const EmitWordFn& emit_word, std::string* error_msg);

  // ---------------------------------------------------------------------------
  // Foundation helpers — used by instruction encoding handlers (Task 8.5.3).
  // ---------------------------------------------------------------------------

  // EffAddr — maps an operand to the 6-bit effective address code used by
  // most 68000 instruction encodings.  Returns -1 for unsupported modes
  // (SR, CCR, USP, etc.) which are handled directly by their instructions.
  // Ported from CODEGEN.CPP effAddr().
  static int EffAddr(const Operand& op);

  // ExtWords — emits the extension word(s) for operand op.
  // loc is passed by reference: it starts as the address of the first
  // extension word slot and is advanced by 2 for each word emitted.
  // Returns false on a range error; sets *error_msg.
  // Ported from CODEGEN.CPP extWords().
  static bool ExtWords(const Operand& op, SizeSpec size, uint32_t& loc, const EmitWordFn& emit_fn,
                       std::string* error_msg);
};

}  // namespace easym68k::asm_

#endif  // EASYM68K_ASM_CODE_GENERATOR_H_
