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
    if (line.opcode != "EQU" && line.opcode != "SET" && line.opcode != "REG") {
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
    uint32_t new_loc = static_cast<uint32_t>(line.operands[0].data);
    // TODO: Warn and auto-align on odd ORG address (DIRECTIV.CPP:112-115).
    location_counter_ = new_loc;
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

    // Word-align before DC.W or DC.L (DIRECTIV.CPP:351-355).
    // DC.B is exempt so consecutive DC.B's can be contiguous.
    if (bytes_per > 1 && (location_counter_ & 1))
      EmitByte(0);

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

    // Word-align before DS.W or DS.L (DIRECTIV.CPP:529-533).
    if (bytes_per > 1 && (location_counter_ & 1))
      EmitByte(0);

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

  if (op == "REG") {
    if (pass_ == 1) {
      if (line.label.empty()) {
        AddError(line.line_number, "REG requires a label");
      } else if (line.operands.empty()) {
        AddError(line.line_number, "REG requires a register list operand");
      } else {
        uint16_t mask = static_cast<uint16_t>(line.operands[0].data & 0xFFFF);
        symbols_.DefineRegisterList(line.label, mask, line.line_number);
      }
    }
    return true;
  }

  // XDEF/XREF are linker annotations with no code-generation effect.
  return true;
}

bool Assembler::HandleInstruction(const ParsedLine& line) {
  if (pass_ == 1) {
    location_counter_ += static_cast<uint32_t>(InstructionSize(line));
    return true;
  }

  // Pass 2: look up instruction and dispatch to code generator.
  const InstrEntry* entry = instruction_table_.Lookup(line.opcode);
  if (!entry) {
    AddError(line.line_number, "Unknown instruction: " + line.opcode);
    return true;
  }

  std::string error_msg;
  if (!code_generator_.Encode(
          *entry, line, location_counter_, symbols_, [this](uint16_t w) { EmitWord(w); },
          &error_msg)) {
    AddError(line.line_number, error_msg);
  }
  return true;
}

int Assembler::InstructionSize(const ParsedLine& line) const {
  const InstrEntry* entry = instruction_table_.Lookup(line.opcode);
  if (!entry)
    return 2;

  SizeSpec sz = (line.size != SizeSpec::kNone) ? line.size : SizeSpec::kWord;
  int words = 1;  // base opcode word

  // Mirrors the CanUseQuick() peephole check in code_generator.cc's anonymous
  // namespace so that pass-1 size prediction matches pass-2 Encode() output.
  auto can_use_quick = [&]() -> bool {
    if (line.operands.empty())
      return false;
    const Operand& src = line.operands[0];
    return src.mode == AddressMode::kImmediate && src.is_back_ref && src.data >= 1 &&
           src.data <= 8 && src.forced_size == SizeSpec::kNone && sz != SizeSpec::kByte;
  };

  switch (entry->encoding) {
    // Fixed 1-word opcodes — no extension words.
    case InstrEncoding::kFixed:
    case InstrEncoding::kRegOp:
    case InstrEncoding::kAnOp:
    case InstrEncoding::kMoveUsp:
    case InstrEncoding::kAddxSubx:
    case InstrEncoding::kAbcdSbcd:
    case InstrEncoding::kCmpm:
    case InstrEncoding::kExg:
      break;

    // MOVEQ: immediate data encoded in the opcode word — no extension words.
    case InstrEncoding::kMoveq:
      break;

    // TRAP: vector encoded in the opcode word — no extension words.
    case InstrEncoding::kTrap:
      break;

    // Bcc/BRA/BSR: explicit .S/.B suffix → short form (1 word);
    // otherwise conservative word form (2 words).
    case InstrEncoding::kBranch:
      words = (line.size == SizeSpec::kShort || line.size == SizeSpec::kByte) ? 1 : 2;
      break;

    // DBcc: always 2 words (opcode + 16-bit displacement word).
    case InstrEncoding::kDBcc:
      words = 2;
      break;

    // ADDQ/SUBQ: quick data (#1-8) lives in the opcode — count only the
    // destination's extension words.
    case InstrEncoding::kQuick:
      if (line.operands.size() >= 2)
        words += InstructionTable::ExtWordCount(line.operands[1].mode, sz);
      break;

    // Shift/rotate: register form (2 operands) has no extension words; memory
    // form (1 operand) adds extension words for the effective address.
    case InstrEncoding::kShift:
      if (line.operands.size() == 1)
        words += InstructionTable::ExtWordCount(line.operands[0].mode, sz);
      break;

    // MOVEM: 1 opcode word + 1 register-list word + EA extension words.
    case InstrEncoding::kMoveM: {
      words = 2;  // opcode + register-mask word
      for (const auto& operand : line.operands) {
        if (operand.mode != AddressMode::kRegisterList) {
          words += InstructionTable::ExtWordCount(operand.mode, sz);
          break;
        }
      }
      break;
    }

    // MOVE: MOVEQ peephole fires when MOVE.L #imm,Dn with imm in [-128,127].
    // All other forms sum extension words normally.
    case InstrEncoding::kMove: {
      if (line.operands.size() >= 2) {
        const Operand& src = line.operands[0];
        const Operand& dst = line.operands[1];
        if (src.mode == AddressMode::kImmediate && src.is_back_ref && src.data >= -128 &&
            src.data <= 127 && dst.mode == AddressMode::kDnDirect && sz == SizeSpec::kLong &&
            src.forced_size == SizeSpec::kNone) {
          break;  // MOVEQ: 1 word, no extension words
        }
      }
      for (const auto& op : line.operands)
        words += InstructionTable::ExtWordCount(op.mode, sz);
      break;
    }

    // ADD/SUB/AND/OR (kEaBidirect): ADDQ/SUBQ peephole for immediate src.
    case InstrEncoding::kEaBidirect: {
      if (line.operands.size() >= 2 && line.operands[0].mode == AddressMode::kImmediate) {
        uint16_t top = entry->base & 0xF000;
        if ((top == 0xD000 || top == 0x9000) && can_use_quick()) {
          // ADDQ/SUBQ: opcode + dst extension words only
          words += InstructionTable::ExtWordCount(line.operands[1].mode, sz);
          break;
        }
      }
      for (const auto& op : line.operands)
        words += InstructionTable::ExtWordCount(op.mode, sz);
      break;
    }

    // ADDI/SUBI/etc. (kImmedToEa): ADDQ/SUBQ peephole for ADDI/SUBI.
    case InstrEncoding::kImmedToEa: {
      if (line.operands.size() >= 2) {
        uint16_t itype = entry->base & 0xFF00;
        if ((itype == 0x0600 || itype == 0x0400) && can_use_quick()) {
          // ADDQ/SUBQ: opcode + dst extension words only
          words += InstructionTable::ExtWordCount(line.operands[1].mode, sz);
          break;
        }
      }
      for (const auto& op : line.operands)
        words += InstructionTable::ExtWordCount(op.mode, sz);
      break;
    }

    // ADDA/SUBA (kAddrEa): ADDQ/SUBQ peephole when immediate fits 1–8.
    case InstrEncoding::kAddrEa: {
      if (line.operands.size() >= 2) {
        uint16_t top = entry->base & 0xF000;
        if ((top == 0xD000 || top == 0x9000) && can_use_quick()) {
          // ADDQ/SUBQ: opcode + dst extension words only
          words += InstructionTable::ExtWordCount(line.operands[1].mode, sz);
          break;
        }
      }
      for (const auto& op : line.operands)
        words += InstructionTable::ExtWordCount(op.mode, sz);
      break;
    }

    // General case: sum extension words across all operands.
    default:
      for (const auto& op : line.operands)
        words += InstructionTable::ExtWordCount(op.mode, sz);
      break;
  }

  return words * 2;
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
