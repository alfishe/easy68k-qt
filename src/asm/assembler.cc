// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/asm/assembler.h"

#include <fstream>
#include <sstream>
#include <unordered_set>

namespace easym68k::asm_ {

Assembler::Assembler()
    : pass_(1),
      location_counter_(0),
      org_(0),
      org_set_(false),
      end_seen_(false),
      listing_enabled_(true),
      offset_mode_(false),
      offset_save_loc_(0),
      section_locs_{},
      current_section_(0) {
  // Default file reader: read from the real filesystem
  file_reader_ = [this](const std::string& filename) -> std::string {
    return ReadIncludeFile(filename);
  };
}

Assembler::~Assembler() = default;

void Assembler::AddIncludePath(const std::string& path) {
  include_paths_.push_back(path);
}

// static
std::vector<std::string> Assembler::SplitLines(const std::string& source) {
  std::vector<std::string> lines;
  std::istringstream ss(source);
  std::string line;
  while (std::getline(ss, line))
    lines.push_back(line);
  if (lines.empty())
    lines.push_back("");
  return lines;
}

std::string Assembler::ReadIncludeFile(const std::string& filename) {
  // Search include paths first
  for (const auto& dir : include_paths_) {
    std::string path = dir + "/" + filename;
    std::ifstream f(path);
    if (f.is_open()) {
      std::ostringstream buf;
      buf << f.rdbuf();
      return buf.str();
    }
  }
  // Fallback: try filename as-is
  std::ifstream f(filename);
  if (!f.is_open())
    return "";
  std::ostringstream buf;
  buf << f.rdbuf();
  return buf.str();
}

std::string Assembler::ReadBinaryFile(const std::string& filename) {
  std::ifstream f(filename, std::ios::binary);
  if (!f.is_open())
    return "";
  std::ostringstream buf;
  buf << f.rdbuf();
  return buf.str();
}

AssemblyResult Assembler::Assemble(const std::string& source) {
  auto lines = SplitLines(source);
  errors_.clear();

  auto reset_state = [this]() {
    location_counter_ = 0;
    org_ = 0;
    org_set_ = false;
    end_seen_ = false;
    listing_enabled_ = true;
    offset_mode_ = false;
    offset_save_loc_ = 0;
    current_section_ = 0;
    for (auto& s : section_locs_)
      s = 0;
    cond_stack_.clear();
  };

  // Pass 1: define all labels / EQU symbols; compute location counter.
  pass_ = 1;
  reset_state();
  symbols_.Clear();
  parser_.SetPass(1);
  parser_.SetSymbolTable(&symbols_);
  RunPass(lines);

  // Pass 2: emit object code using the symbol table built in pass 1.
  pass_ = 2;
  reset_state();
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

bool Assembler::IsAssembling() const {
  for (const auto& f : cond_stack_)
    if (!f.active)
      return false;
  return true;
}

bool Assembler::OuterIsAssembling() const {
  for (size_t i = 0; i + 1 < cond_stack_.size(); i++)
    if (!cond_stack_[i].active)
      return false;
  return true;
}

bool Assembler::IsConditional(const std::string& opcode) const {
  return opcode == "IFC" || opcode == "IFNC" || opcode == "IFEQ" || opcode == "IFNE" ||
         opcode == "IFLT" || opcode == "IFLE" || opcode == "IFGT" || opcode == "IFGE" ||
         opcode == "ELSE" || opcode == "ENDC";
}

void Assembler::HandleConditional(const ParsedLine& line) {
  const std::string& op = line.opcode;
  const bool outer = OuterIsAssembling();

  // The original rejects labels on conditional directive lines (ASSEMBLE.CPP: LABEL_ERROR).
  if (!line.label.empty() && pass_ == 2)
    AddError(line.line_number, "Label not permitted on conditional directive: " + line.label);

  // Helper: extract string value from an operand for IFC/IFNC comparison.
  // kStringLiteral → symbol_name; kImmediate → decimal; otherwise → symbol_name
  auto operand_str = [](const Operand& o) -> std::string {
    if (o.mode == AddressMode::kStringLiteral)
      return o.symbol_name;
    if (o.mode == AddressMode::kImmediate)
      return std::to_string(o.data);
    return o.symbol_name;
  };

  if (op == "IFC") {
    // IFC 'str1','str2' — assemble if strings are equal (case-sensitive)
    if (!IsAssembling()) {
      // Nested inside an already-skipped block — track depth only
      cond_stack_.push_back({false, false});
      return;
    }
    bool cond = false;
    if (line.operands.size() >= 2) {
      cond = (operand_str(line.operands[0]) == operand_str(line.operands[1]));
    } else if (pass_ == 2) {
      AddError(line.line_number, "IFC requires two operands");
    }
    cond_stack_.push_back({cond, false});

  } else if (op == "IFNC") {
    // IFNC 'str1','str2' — assemble if strings are NOT equal
    if (!IsAssembling()) {
      cond_stack_.push_back({false, false});
      return;
    }
    bool cond = true;
    if (line.operands.size() >= 2) {
      cond = (operand_str(line.operands[0]) != operand_str(line.operands[1]));
    } else if (pass_ == 2) {
      AddError(line.line_number, "IFNC requires two operands");
    }
    cond_stack_.push_back({cond, false});

  } else if (op == "IFEQ" || op == "IFNE" || op == "IFLT" || op == "IFLE" || op == "IFGT" ||
             op == "IFGE") {
    // IFxx expr — assemble based on expr compared to 0
    if (!IsAssembling()) {
      cond_stack_.push_back({false, false});
      return;
    }
    bool cond = false;
    if (line.operands.empty()) {
      if (pass_ == 2)
        AddError(line.line_number, op + " requires an expression");
    } else {
      int32_t val = line.operands[0].data;
      if (op == "IFEQ")
        cond = (val == 0);
      else if (op == "IFNE")
        cond = (val != 0);
      else if (op == "IFLT")
        cond = (val < 0);
      else if (op == "IFLE")
        cond = (val <= 0);
      else if (op == "IFGT")
        cond = (val > 0);
      else if (op == "IFGE")
        cond = (val >= 0);
    }
    cond_stack_.push_back({cond, false});

  } else if (op == "ELSE") {
    if (cond_stack_.empty()) {
      if (pass_ == 2)
        AddError(line.line_number, "ELSE without matching IF");
      return;
    }
    CondFrame& top = cond_stack_.back();
    if (top.seen_else) {
      if (pass_ == 2)
        AddError(line.line_number, "Multiple ELSE in same conditional block");
      return;
    }
    top.seen_else = true;
    // Only toggle active state when the outer context is assembling.
    // If the outer is skipped, the whole block is dead regardless.
    if (outer)
      top.active = !top.active;

  } else if (op == "ENDC") {
    if (cond_stack_.empty()) {
      if (pass_ == 2)
        AddError(line.line_number, "ENDC without matching IF");
      return;
    }
    cond_stack_.pop_back();
  }
}

void Assembler::RunPass(const std::vector<std::string>& lines) {
  for (int i = 0; i < static_cast<int>(lines.size()); i++) {
    if (end_seen_)
      break;
    parser_.SetLocationCounter(location_counter_);
    ParsedLine line = parser_.ParseLine(lines[i], i + 1);

    if (line.has_error) {
      if (pass_ == 2 && IsAssembling())
        AddError(line.line_number, line.error_message);
      continue;
    }

    // Conditional directives are always evaluated to maintain the nesting stack.
    if (!line.opcode.empty() && IsConditional(line.opcode)) {
      HandleConditional(line);
      continue;
    }

    // Skip non-conditional content when inside an inactive conditional block.
    if (!IsAssembling())
      continue;

    if (!ProcessLine(line))
      break;
  }
}

bool Assembler::ProcessLine(const ParsedLine& line) {
  // Regular labels (not EQU/SET/REG) are defined at the current location counter.
  if (!line.label.empty() && pass_ == 1) {
    if (line.opcode != "EQU" && line.opcode != "SET" && line.opcode != "REG") {
      symbols_.DefineLabel(line.label, location_counter_, line.line_number);
    }
    symbols_.SetCurrentParentLabel(line.label);
  }

  if (line.opcode.empty())
    return true;

  static const std::unordered_set<std::string> kDirectiveOpcodes = {
      "ORG", "EQU",  "SET",  "DC",     "DS",      "DCB",     "END",     "EVEN",
      "ODD", "XDEF", "XREF", "REG",    "INCLUDE", "INCBIN",  "SECTION", "OFFSET",
      "OPT", "FAIL", "LIST", "NOLIST", "PAGE",    "SIMHALT", "MEMORY",
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
    if (new_loc & 1) {
      // Odd ORG: warn and align (DIRECTIV.CPP:112-115)
      if (pass_ == 2)
        AddError(line.line_number, "ORG address is odd — auto-aligned to even");
      new_loc &= ~1u;
    }
    if (offset_mode_) {
      // Exiting OFFSET mode via ORG
      offset_mode_ = false;
      location_counter_ = offset_save_loc_;
    }
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
      sz = SizeSpec::kWord;

    int bytes_per = (sz == SizeSpec::kByte) ? 1 : (sz == SizeSpec::kLong) ? 4 : 2;

    // Word-align before DC.W or DC.L (DIRECTIV.CPP:351-355)
    if (bytes_per > 1 && (location_counter_ & 1))
      EmitByte(0);

    if (pass_ == 1) {
      for (const auto& operand : line.operands) {
        if (operand.mode == AddressMode::kStringLiteral) {
          location_counter_ += static_cast<uint32_t>(operand.symbol_name.size());
        } else {
          location_counter_ += static_cast<uint32_t>(bytes_per);
        }
      }
    } else {
      for (const auto& op_val : line.operands) {
        if (op_val.mode == AddressMode::kStringLiteral) {
          for (unsigned char ch : op_val.symbol_name)
            EmitByte(ch);
        } else {
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
    }
    return true;
  }

  if (op == "DCB") {
    SizeSpec sz = line.size;
    if (sz == SizeSpec::kNone)
      sz = SizeSpec::kWord;

    int bytes_per = (sz == SizeSpec::kByte) ? 1 : (sz == SizeSpec::kLong) ? 4 : 2;

    // Word-align before DCB.W or DCB.L
    if (bytes_per > 1 && (location_counter_ & 1))
      EmitByte(0);

    int32_t count = 0;
    uint32_t fill_val = 0;
    if (line.operands.size() >= 1)
      count = line.operands[0].data;
    if (line.operands.size() >= 2)
      fill_val = static_cast<uint32_t>(line.operands[1].data);

    if (count < 0) {
      AddError(line.line_number, "DCB count must be non-negative");
      return true;
    }

    uint32_t total = static_cast<uint32_t>(bytes_per * count);
    if (pass_ == 2) {
      for (int32_t j = 0; j < count; j++) {
        if (bytes_per == 1) {
          EmitByte(static_cast<uint8_t>(fill_val & 0xFF));
        } else if (bytes_per == 2) {
          EmitWord(static_cast<uint16_t>(fill_val & 0xFFFF));
        } else {
          EmitLong(fill_val);
        }
      }
    } else {
      location_counter_ += total;
    }
    return true;
  }

  if (op == "DS") {
    SizeSpec sz = line.size;
    if (sz == SizeSpec::kNone)
      sz = SizeSpec::kWord;

    int bytes_per = (sz == SizeSpec::kByte) ? 1 : (sz == SizeSpec::kLong) ? 4 : 2;

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
    end_seen_ = true;
    return false;
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

  if (op == "INCLUDE") {
    if (line.operands.empty() || line.operands[0].mode != AddressMode::kStringLiteral) {
      AddError(line.line_number, "INCLUDE requires a filename string");
      return true;
    }
    const std::string& filename = line.operands[0].symbol_name;

    // Cycle detection
    if (active_includes_.count(filename)) {
      AddError(line.line_number, "INCLUDE cycle detected: " + filename);
      return true;
    }

    std::string content = file_reader_(filename);
    if (content.empty()) {
      // Only error in pass 2 so pass 1 can still build the symbol table
      if (pass_ == 2)
        AddError(line.line_number, "INCLUDE: cannot open file: " + filename);
      return true;
    }

    auto sub_lines = SplitLines(content);
    active_includes_.insert(filename);
    bool saved_end = end_seen_;
    end_seen_ = false;
    RunPass(sub_lines);
    end_seen_ = saved_end;  // END inside INCLUDE ends only that file
    active_includes_.erase(filename);
    return true;
  }

  if (op == "INCBIN") {
    if (line.operands.empty() || line.operands[0].mode != AddressMode::kStringLiteral) {
      AddError(line.line_number, "INCBIN requires a filename string");
      return true;
    }
    const std::string& filename = line.operands[0].symbol_name;
    std::string content = ReadBinaryFile(filename);
    if (content.empty() && pass_ == 2) {
      AddError(line.line_number, "INCBIN: cannot open file: " + filename);
      return true;
    }
    uint32_t sz = static_cast<uint32_t>(content.size());
    if (pass_ == 2) {
      for (unsigned char ch : content)
        EmitByte(ch);
    } else {
      location_counter_ += sz;
    }
    return true;
  }

  if (op == "SECTION") {
    // SECTION n — switch to section 0-15, saving current LC
    int32_t n = line.operands.empty() ? 0 : line.operands[0].data;
    if (n < 0 || n >= kNumSections) {
      AddError(line.line_number, "SECTION number must be 0-15");
      return true;
    }
    if (offset_mode_) {
      // Exiting OFFSET mode via SECTION
      offset_mode_ = false;
      location_counter_ = offset_save_loc_;
    }
    // Save current section's LC and restore new section's LC
    section_locs_[current_section_] = location_counter_;
    current_section_ = static_cast<int>(n);
    location_counter_ = section_locs_[current_section_];
    return true;
  }

  if (op == "OFFSET") {
    // OFFSET expr — starts OFFSET mode: LC = expr, code not emitted
    if (line.operands.empty()) {
      AddError(line.line_number, "OFFSET requires an expression");
      return true;
    }
    if (!offset_mode_)
      offset_save_loc_ = location_counter_;
    offset_mode_ = true;
    location_counter_ = static_cast<uint32_t>(line.operands[0].data);
    return true;
  }

  if (op == "OPT") {
    // Options control listing / assembly behavior — no-op in this implementation
    return true;
  }

  if (op == "FAIL") {
    // FAIL 'message' — unconditional user error
    if (pass_ == 2) {
      std::string msg = "FAIL";
      if (!line.operands.empty() && line.operands[0].mode == AddressMode::kStringLiteral) {
        msg += ": " + line.operands[0].symbol_name;
      }
      AddError(line.line_number, msg);
    }
    return true;
  }

  if (op == "LIST") {
    listing_enabled_ = true;
    return true;
  }

  if (op == "NOLIST") {
    listing_enabled_ = false;
    return true;
  }

  if (op == "PAGE") {
    // Page break in listing — no-op
    return true;
  }

  if (op == "SIMHALT") {
    // Word-align then emit two 0xFFFF words (= 4 bytes)
    WordAlign();
    EmitWord(0xFFFF);
    EmitWord(0xFFFF);
    return true;
  }

  if (op == "MEMORY") {
    // MEMORY directive: no-op (memory region definition for simulator)
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

  auto can_use_quick = [&]() -> bool {
    if (line.operands.empty())
      return false;
    const Operand& src = line.operands[0];
    return src.mode == AddressMode::kImmediate && src.is_back_ref && src.data >= 1 &&
           src.data <= 8 && src.forced_size == SizeSpec::kNone && sz != SizeSpec::kByte;
  };

  switch (entry->encoding) {
    case InstrEncoding::kFixed:
    case InstrEncoding::kRegOp:
    case InstrEncoding::kAnOp:
    case InstrEncoding::kMoveUsp:
    case InstrEncoding::kAddxSubx:
    case InstrEncoding::kAbcdSbcd:
    case InstrEncoding::kCmpm:
    case InstrEncoding::kExg:
    case InstrEncoding::kMoveq:
    case InstrEncoding::kTrap:
      break;

    case InstrEncoding::kBranch:
      words = (line.size == SizeSpec::kShort || line.size == SizeSpec::kByte) ? 1 : 2;
      break;

    case InstrEncoding::kDBcc:
      words = 2;
      break;

    case InstrEncoding::kQuick:
      if (line.operands.size() >= 2)
        words += InstructionTable::ExtWordCount(line.operands[1].mode, sz);
      break;

    case InstrEncoding::kShift:
      if (line.operands.size() == 1)
        words += InstructionTable::ExtWordCount(line.operands[0].mode, sz);
      break;

    case InstrEncoding::kMoveM: {
      words = 2;
      for (const auto& operand : line.operands) {
        if (operand.mode != AddressMode::kRegisterList) {
          words += InstructionTable::ExtWordCount(operand.mode, sz);
          break;
        }
      }
      break;
    }

    case InstrEncoding::kMove: {
      if (line.operands.size() >= 2) {
        const Operand& src = line.operands[0];
        const Operand& dst = line.operands[1];
        if (src.mode == AddressMode::kImmediate && src.is_back_ref && src.data >= -128 &&
            src.data <= 127 && dst.mode == AddressMode::kDnDirect && sz == SizeSpec::kLong &&
            src.forced_size == SizeSpec::kNone) {
          break;
        }
      }
      for (const auto& op : line.operands)
        words += InstructionTable::ExtWordCount(op.mode, sz);
      break;
    }

    case InstrEncoding::kEaBidirect: {
      if (line.operands.size() >= 2 && line.operands[0].mode == AddressMode::kImmediate) {
        uint16_t top = entry->base & 0xF000;
        if ((top == 0xD000 || top == 0x9000) && can_use_quick()) {
          words += InstructionTable::ExtWordCount(line.operands[1].mode, sz);
          break;
        }
      }
      for (const auto& op : line.operands)
        words += InstructionTable::ExtWordCount(op.mode, sz);
      break;
    }

    case InstrEncoding::kImmedToEa: {
      if (line.operands.size() >= 2) {
        uint16_t itype = entry->base & 0xFF00;
        if ((itype == 0x0600 || itype == 0x0400) && can_use_quick()) {
          words += InstructionTable::ExtWordCount(line.operands[1].mode, sz);
          break;
        }
      }
      for (const auto& op : line.operands)
        words += InstructionTable::ExtWordCount(op.mode, sz);
      break;
    }

    case InstrEncoding::kAddrEa: {
      if (line.operands.size() >= 2) {
        uint16_t top = entry->base & 0xF000;
        if ((top == 0xD000 || top == 0x9000) && can_use_quick()) {
          words += InstructionTable::ExtWordCount(line.operands[1].mode, sz);
          break;
        }
      }
      for (const auto& op : line.operands)
        words += InstructionTable::ExtWordCount(op.mode, sz);
      break;
    }

    default:
      for (const auto& op : line.operands)
        words += InstructionTable::ExtWordCount(op.mode, sz);
      break;
  }

  return words * 2;
}

void Assembler::EmitByte(uint8_t b) {
  if (pass_ == 2 && !offset_mode_) {
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

void Assembler::WordAlign() {
  if (location_counter_ & 1) {
    EmitByte(0);
  }
}

void Assembler::AddError(int line_number, const std::string& message) {
  errors_.push_back({line_number, message});
}

}  // namespace easym68k::asm_
