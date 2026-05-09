// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/asm/assembler.h"

#include <sstream>
#include <unordered_set>

namespace easym68k::asm_ {

Assembler::Assembler() : pass_(1), location_counter_(0), org_(0), org_set_(false) {}

Assembler::~Assembler() = default;

// static
std::vector<std::string> Assembler::SplitLines(const std::string& source) {
  std::vector<std::string> lines;
  std::istringstream ss(source);
  std::string line;
  while (std::getline(ss, line))
    lines.push_back(line);
  // Guarantee at least one line so single-expression sources are processed.
  if (lines.empty())
    lines.push_back("");
  return lines;
}

AssemblyResult Assembler::Assemble(const std::string& source) {
  auto lines = SplitLines(source);
  errors_.clear();

  // Pass 1: define all labels / EQU symbols; compute location counter.
  pass_ = 1;
  location_counter_ = 0;
  org_ = 0;
  org_set_ = false;
  symbols_.Clear();
  parser_.SetPass(1);
  parser_.SetSymbolTable(&symbols_);
  RunPass(lines);

  // Pass 2: emit object code using the symbol table built in pass 1.
  pass_ = 2;
  location_counter_ = 0;
  org_ = 0;
  org_set_ = false;
  code_.clear();
  parser_.SetPass(2);
  RunPass(lines);

  AssemblyResult result;
  result.success = errors_.empty();
  result.org = org_;
  result.code = code_;
  result.errors = errors_;
  return result;
}

void Assembler::RunPass(const std::vector<std::string>& lines) {
  for (int i = 0; i < static_cast<int>(lines.size()); i++) {
    parser_.SetLocationCounter(location_counter_);
    ParsedLine line = parser_.ParseLine(lines[i], i + 1);

    if (line.has_error) {
      // Pass-1 errors are silently skipped: the assembler finishes pass 1 to
      // collect all label addresses, then re-parses and reports errors in
      // pass 2.  Structural errors that corrupt the symbol table (e.g.
      // multiply-defined labels) will surface as errors in pass 2.
      if (pass_ == 2)
        AddError(line.line_number, line.error_message);
      continue;
    }

    if (!ProcessLine(line))
      break;
  }
}

bool Assembler::ProcessLine(const ParsedLine& line) {
  // Regular labels (not EQU/SET) are defined at the current location counter.
  if (!line.label.empty() && pass_ == 1) {
    if (line.opcode != "EQU" && line.opcode != "SET") {
      symbols_.DefineLabel(line.label, location_counter_, line.line_number);
    }
    symbols_.SetCurrentParentLabel(line.label);
  }

  if (line.opcode.empty())
    return true;

  static const std::unordered_set<std::string> kDirectiveOpcodes = {
      "ORG", "EQU", "SET", "DC", "DS", "END", "EVEN", "ODD", "XDEF", "XREF", "REG",
  };
  if (kDirectiveOpcodes.count(line.opcode))
    return HandleDirective(line);

  return HandleInstruction(line);
}

bool Assembler::HandleDirective(const ParsedLine& line) {
  const std::string& op = line.opcode;

  if (op == "ORG") {
    if (line.operands.empty()) {
      AddError(line.line_number, "ORG requires an address");
      return true;
    }
    location_counter_ = static_cast<uint32_t>(line.operands[0].data);
    // TODO(Task 8.5): Handle ORG discontinuities. Currently AssemblyResult
    // exposes a single flat code vector starting at org_, so a second ORG
    // that jumps to a non-contiguous address will silently produce incorrect
    // output.  The fix requires a segment map: map<uint32_t, vector<uint8_t>>.
    return true;
  }

  if (op == "EQU" || op == "SET") {
    if (pass_ == 1) {
      if (line.label.empty()) {
        AddError(line.line_number, "EQU/SET requires a label");
      } else if (line.operands.empty()) {
        AddError(line.line_number, "EQU/SET requires a value");
      } else {
        int32_t val = line.operands[0].data;
        if (op == "SET") {
          symbols_.DefineSet(line.label, val, line.line_number);
        } else {
          symbols_.DefineEqu(line.label, val, line.line_number);
        }
      }
    }
    return true;
  }

  if (op == "DC") {
    SizeSpec sz = line.size;
    if (sz == SizeSpec::kNone)
      sz = SizeSpec::kWord;  // DC without size → DC.W

    int bytes_per = (sz == SizeSpec::kByte)   ? 1
                    : (sz == SizeSpec::kLong) ? 4
                                              : 2;  // kWord / kShort

    if (pass_ == 1) {
      location_counter_ +=
          static_cast<uint32_t>(bytes_per * static_cast<int>(line.operands.size()));
    } else {
      for (const auto& op_val : line.operands) {
        uint32_t val = static_cast<uint32_t>(op_val.data);
        if (bytes_per == 1) {
          EmitByte(static_cast<uint8_t>(val & 0xFF));
        } else if (bytes_per == 2) {
          EmitWord(static_cast<uint16_t>(val & 0xFFFF));
        } else {
          EmitLong(val);
        }
      }
    }
    return true;
  }

  if (op == "DS") {
    SizeSpec sz = line.size;
    if (sz == SizeSpec::kNone)
      sz = SizeSpec::kWord;

    int bytes_per = (sz == SizeSpec::kByte) ? 1 : (sz == SizeSpec::kLong) ? 4 : 2;

    int32_t count = line.operands.empty() ? 1 : line.operands[0].data;
    uint32_t total = static_cast<uint32_t>(bytes_per * count);

    if (pass_ == 2) {
      for (uint32_t j = 0; j < total; j++)
        EmitByte(0);
    } else {
      location_counter_ += total;
    }
    return true;
  }

  if (op == "EVEN") {
    if (location_counter_ & 1) {
      if (pass_ == 2) {
        EmitByte(0);
      } else {
        location_counter_++;
      }
    }
    return true;
  }

  if (op == "ODD") {
    if (!(location_counter_ & 1)) {
      if (pass_ == 2) {
        EmitByte(0);
      } else {
        location_counter_++;
      }
    }
    return true;
  }

  if (op == "END") {
    return false;  // Signal end of assembly
  }

  // XDEF/XREF/REG are linker annotations with no code-generation effect.
  // They are recognized as directives so HandleInstruction does not emit
  // an "Unknown instruction" error for them.
  return true;
}

bool Assembler::HandleInstruction(const ParsedLine& line) {
  const std::string& op = line.opcode;

  if (pass_ == 1) {
    location_counter_ += static_cast<uint32_t>(InstructionSize(line));
    return true;
  }

  // Pass 2: emit encoding
  if (op == "NOP") {
    EmitWord(0x4E71);
    return true;
  }

  if (op == "MOVEQ") {
    // Syntax: MOVEQ #imm,Dn
    // Encoding: 0111 DDD0 iiiiiiii
    if (line.operands.size() < 2) {
      AddError(line.line_number, "MOVEQ requires two operands");
      return true;
    }
    const Operand& src = line.operands[0];
    const Operand& dst = line.operands[1];
    if (src.mode != AddressMode::kImmediate) {
      AddError(line.line_number, "MOVEQ source must be immediate");
      return true;
    }
    if (dst.mode != AddressMode::kDnDirect) {
      AddError(line.line_number, "MOVEQ destination must be data register");
      return true;
    }
    int8_t imm8 = static_cast<int8_t>(src.data & 0xFF);
    uint16_t word =
        static_cast<uint16_t>(0x7000 | ((dst.reg & 7) << 9) | (static_cast<uint8_t>(imm8)));
    EmitWord(word);
    return true;
  }

  AddError(line.line_number, "Unknown instruction: " + op);
  return true;
}

int Assembler::InstructionSize(const ParsedLine& line) const {
  const std::string& op = line.opcode;
  if (op == "NOP")
    return 2;
  if (op == "MOVEQ")
    return 2;
  // TODO(Task 8.5): Replace with instruction_table lookup.  The default of 2
  // bytes is correct for single-word opcodes but wrong for instructions with
  // immediate or absolute-long operands (e.g. ADDI, BRA.W, LINK).  Any code
  // that uses unimplemented instructions will get incorrect label addresses.
  return 2;
}

void Assembler::EmitByte(uint8_t b) {
  if (pass_ == 2) {
    if (!org_set_) {
      org_ = location_counter_;
      org_set_ = true;
    }
    code_.push_back(b);
  }
  location_counter_++;
}

void Assembler::EmitWord(uint16_t w) {
  EmitByte(static_cast<uint8_t>((w >> 8) & 0xFF));
  EmitByte(static_cast<uint8_t>(w & 0xFF));
}

void Assembler::EmitLong(uint32_t l) {
  EmitByte(static_cast<uint8_t>((l >> 24) & 0xFF));
  EmitByte(static_cast<uint8_t>((l >> 16) & 0xFF));
  EmitByte(static_cast<uint8_t>((l >> 8) & 0xFF));
  EmitByte(static_cast<uint8_t>(l & 0xFF));
}

void Assembler::AddError(int line_number, const std::string& message) {
  errors_.push_back({line_number, message});
}

}  // namespace easym68k::asm_
