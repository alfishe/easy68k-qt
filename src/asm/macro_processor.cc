// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Macro processor for EASy68K — ported from EASy68K/Edit68K/MACRO.CPP

#include <cctype>
#include <string>
#include <vector>

#include "easym68k/asm/assembler.h"
#include "easym68k/asm/lexer.h"

namespace easym68k::asm_ {

// ── Pre-scan ────────────────────────────────────────────────────────────────

// Scan through source lines, find every MACRO/ENDM pair, and store the body.
// This runs once before pass 1 so forward-referenced macros work.
void Assembler::ScanMacros(const std::vector<std::string>& lines) {
  for (int i = 0; i < static_cast<int>(lines.size()); i++) {
    ParsedLine line = parser_.ParseLine(lines[i], i + 1);
    if (line.opcode == "MACRO" && !line.label.empty()) {
      std::vector<std::string> body;
      int j = i + 1;
      while (j < static_cast<int>(lines.size())) {
        ParsedLine bl = parser_.ParseLine(lines[j], j + 1);
        if (bl.opcode == "ENDM")
          break;
        body.push_back(lines[j]);
        j++;
      }
      macros_[line.label] = std::move(body);
      i = j;
    }
  }
}

// ── RunPass helpers ──────────────────────────────────────────────────────────

bool Assembler::IsMacro(const std::string& opcode) const {
  return macros_.count(opcode) > 0;
}

// Scan lines[i+1 ..] for ENDM; store body under name; advance i to ENDM.
// Called from RunPass when we see MACRO while assembling.
void Assembler::CollectMacroBody(const std::string& name, const std::vector<std::string>& lines,
                                 int& i, int line_number) {
  std::vector<std::string> body;
  int j = i + 1;
  bool found = false;
  while (j < static_cast<int>(lines.size())) {
    ParsedLine bl = parser_.ParseLine(lines[j], j + 1);
    if (bl.opcode == "ENDM") {
      found = true;
      break;
    }
    if (bl.opcode == "MACRO") {
      if (pass_ == 2)
        AddError(line_number, ErrorCode::kMacroNested, name);
      break;
    }
    body.push_back(lines[j]);
    if (pass_ == 2 && listing_enabled_) {
      listing_generator_.AddLine(0, {}, lines[j], j + 1, "", offset_mode_, false, 0, false);
    }
    j++;
  }
  if (!found && pass_ == 2)
    AddError(line_number, ErrorCode::kMacroNoEndm, name);
  if (found && pass_ == 2 && listing_enabled_) {
    listing_generator_.AddLine(0, {}, lines[j], j + 1, "", offset_mode_, false, 0, false);
  }
  macros_[name] = std::move(body);
  i = j;
}

// ── Static helpers ────────────────────────────────────────────────────────────

// Extract the text after opcode+size from a raw line.
// Used to get the raw argument string for macro invocation.
// static
std::string Assembler::ExtractRawOperands(const std::string& raw, const std::string& label,
                                          const std::string& opcode) {
  // Build uppercase version for case-insensitive search.
  std::string upper;
  upper.reserve(raw.size());
  for (char c : raw)
    upper += static_cast<char>(toupper(static_cast<unsigned char>(c)));

  // Skip past the label, then find the opcode.
  size_t from = 0;
  if (!label.empty()) {
    size_t lp = upper.find(label);
    if (lp != std::string::npos)
      from = lp + label.size();
  }

  size_t op_pos = upper.find(opcode, from);
  if (op_pos == std::string::npos)
    return "";
  size_t after = op_pos + opcode.size();

  // Skip optional size suffix (.B / .W / .L / .S)
  if (after < raw.size() && raw[after] == '.' && after + 1 < raw.size() &&
      isalpha(static_cast<unsigned char>(raw[after + 1]))) {
    after += 2;
  }

  // Skip whitespace
  while (after < raw.size() && isspace(static_cast<unsigned char>(raw[after])))
    after++;

  std::string result = raw.substr(after);

  // Strip trailing whitespace / CR / LF
  while (!result.empty()) {
    char back = result.back();
    if (back == '\n' || back == '\r' || back == ' ' || back == '\t')
      result.pop_back();
    else
      break;
  }
  return result;
}

// Parse comma-separated macro arguments from raw operand text.
// args[0] = size character ('B'/'W'/'L'/'S').
// args[1..] = actual arguments (uppercased, angle-bracket text preserved).
// static
std::vector<std::string> Assembler::ParseMacroArgs(const std::string& arg_text, char size_char) {
  std::vector<std::string> args;
  args.push_back(std::string(1, size_char));  // \0

  // Uppercase the argument text (matches original strcap behaviour).
  std::string upper;
  upper.reserve(arg_text.size());
  for (char c : arg_text)
    upper += static_cast<char>(toupper(static_cast<unsigned char>(c)));

  // Check for empty / whitespace-only operand text.
  bool any = false;
  for (char c : upper)
    if (!isspace(static_cast<unsigned char>(c))) {
      any = true;
      break;
    }
  if (!any)
    return args;

  bool in_angles = false;
  std::string cur;
  for (size_t i = 0; i < upper.size();) {
    char c = upper[i];
    if (c == '<') {
      if (!in_angles)
        in_angles = true;
      else
        cur += c;
      i++;
    } else if (c == '>') {
      if (in_angles)
        in_angles = false;
      else
        cur += c;
      i++;
    } else if (!in_angles && c == '\'' && i + 1 < upper.size() && upper[i + 1] == '\'') {
      i += 2;  // '' = null argument placeholder; keep cur empty
    } else if (!in_angles && c == ',') {
      args.push_back(cur);
      cur.clear();
      i++;
      while (i < upper.size() && isspace(static_cast<unsigned char>(upper[i])))
        i++;
    } else if (c == '\n' || c == '\r') {
      i++;
    } else {
      cur += c;
      i++;
    }
  }
  args.push_back(cur);
  return args;
}

// Perform parameter substitution on one body line.
//   \0..\9  → args[0..9]
//   \A..\Z  → args[10..35]
//   \@      → '_' + decimal label_num
//   NARG    → decimal narg  (word-boundary, case-insensitive)
// static
std::string Assembler::SubstituteMacroParams(const std::string& body_line,
                                             const std::vector<std::string>& args, int narg,
                                             int label_num) {
  std::string result;
  result.reserve(body_line.size() * 2);
  const std::string narg_str = std::to_string(narg);
  const std::string label_str = '_' + std::to_string(label_num);
  const int argc = static_cast<int>(args.size());

  for (size_t i = 0; i < body_line.size();) {
    char c = body_line[i];

    if (c == '\\' && i + 1 < body_line.size()) {
      char nx = static_cast<char>(toupper(static_cast<unsigned char>(body_line[i + 1])));
      if (nx == '@') {
        result += label_str;
        i += 2;
      } else if (nx >= '0' && nx <= '9') {
        int n = nx - '0';
        if (n < argc)
          result += args[n];
        i += 2;
      } else if (nx >= 'A' && nx <= 'Z') {
        int n = (nx - 'A') + 10;
        if (n < argc)
          result += args[n];
        i += 2;
      } else {
        result += c;
        i++;
      }
      continue;
    }

    // NARG word-boundary substitution (case-insensitive).
    if (i + 4 <= body_line.size()) {
      char n0 = static_cast<char>(toupper(static_cast<unsigned char>(body_line[i])));
      char n1 = static_cast<char>(toupper(static_cast<unsigned char>(body_line[i + 1])));
      char n2 = static_cast<char>(toupper(static_cast<unsigned char>(body_line[i + 2])));
      char n3 = static_cast<char>(toupper(static_cast<unsigned char>(body_line[i + 3])));
      if (n0 == 'N' && n1 == 'A' && n2 == 'R' && n3 == 'G') {
        char after = (i + 4 < body_line.size()) ? body_line[i + 4] : '\0';
        if (!isalnum(static_cast<unsigned char>(after)) && after != '_') {
          result += narg_str;
          i += 4;
          continue;
        }
      }
    }

    result += c;
    i++;
  }
  return result;
}

// Return the uppercase opcode token from a raw body line (pre-substitution).
// Handles leading labels (non-whitespace at column 0) and size suffixes.
// static
std::string Assembler::GetLineOpcode(const std::string& raw) {
  size_t pos = 0;

  // Skip leading whitespace; if none, first word is a label — skip it.
  if (!raw.empty() && !isspace(static_cast<unsigned char>(raw[0]))) {
    if (raw[0] == '*' || raw[0] == ';')
      return "";
    while (pos < raw.size() && !isspace(static_cast<unsigned char>(raw[pos])) && raw[pos] != ':')
      pos++;
    if (pos < raw.size() && raw[pos] == ':')
      pos++;
  }

  while (pos < raw.size() && isspace(static_cast<unsigned char>(raw[pos])))
    pos++;
  if (pos >= raw.size() || raw[pos] == '*' || raw[pos] == ';')
    return "";

  size_t start = pos;
  while (pos < raw.size() && !isspace(static_cast<unsigned char>(raw[pos])) && raw[pos] != '.' &&
         raw[pos] != '\n' && raw[pos] != '\r')
    pos++;

  std::string op = raw.substr(start, pos - start);
  for (auto& ch : op)
    ch = static_cast<char>(toupper(static_cast<unsigned char>(ch)));
  return op;
}

// Extract the integer argument from an IFARG body line (e.g. "IFARG 2" → 2).
// Returns -1 on failure.
// static
int Assembler::ExtractIfargN(const std::string& raw) {
  // Find "IFARG" in the line (case-insensitive), then parse the digit after it.
  for (size_t i = 0; i + 5 <= raw.size(); i++) {
    if (toupper(static_cast<unsigned char>(raw[i])) == 'I' &&
        toupper(static_cast<unsigned char>(raw[i + 1])) == 'F' &&
        toupper(static_cast<unsigned char>(raw[i + 2])) == 'A' &&
        toupper(static_cast<unsigned char>(raw[i + 3])) == 'R' &&
        toupper(static_cast<unsigned char>(raw[i + 4])) == 'G') {
      size_t p = i + 5;
      while (p < raw.size() && isspace(static_cast<unsigned char>(raw[p])))
        p++;
      if (p < raw.size() && isdigit(static_cast<unsigned char>(raw[p])))
        return raw[p] - '0';
      return -1;
    }
  }
  return -1;
}

// ── Invocation ───────────────────────────────────────────────────────────────

void Assembler::InvokeMacro(const std::string& name, const std::string& label,
                            const std::string& raw_operands, SizeSpec size, int line_number) {
  if (macro_nest_depth_ >= kMacroNestLimit) {
    if (pass_ == 2)
      AddError(line_number, ErrorCode::kMacroRecursion, name);
    return;
  }

  auto it = macros_.find(name);
  if (it == macros_.end())
    return;

  // \0 = size character
  char size_char = 'W';
  if (size == SizeSpec::kByte)
    size_char = 'B';
  else if (size == SizeSpec::kLong)
    size_char = 'L';
  else if (size == SizeSpec::kShort)
    size_char = 'S';

  auto args = ParseMacroArgs(raw_operands, size_char);
  const int narg = static_cast<int>(args.size()) - 1;

  // Increment label counter before expansion (both passes must increment identically).
  ++macro_label_num_;
  const int label_num = macro_label_num_;

  // Define the label attached to the call site (original: define(label, loc, ...)).
  if (!label.empty())
    DefineOrMarkLabel(label, location_counter_, line_number);

  macro_nest_depth_++;

  // Unified conditional stack for the macro body.
  // Each frame tracks whether it was injected to RunPass (injected=true → IFC/etc.)
  // or consumed locally (injected=false → IFARG). active=false means we're in a
  // skipped region. ENDC pops the top frame and injects iff injected=true.
  struct LocalFrame {
    bool active;    // false = skip lines in this block
    bool injected;  // true = the opening directive was injected to RunPass
  };
  std::vector<LocalFrame> lstack;

  auto la = [&]() -> bool {
    for (const auto& f : lstack)
      if (!f.active)
        return false;
    return true;
  };

  std::vector<std::string> inject;

  for (const auto& body_line : it->second) {
    const std::string op = GetLineOpcode(body_line);

    if (op == "ENDM")
      break;

    if (op == "MEXIT") {
      if (la())
        break;
      continue;
    }

    if (op == "IFARG") {
      int n = ExtractIfargN(body_line);
      bool present = la() && (n >= 1 && n < static_cast<int>(args.size()) && !args[n].empty());
      lstack.push_back({present, false});
      continue;
    }

    if (op == "IFC" || op == "IFNC" || op == "IFEQ" || op == "IFNE" || op == "IFLT" ||
        op == "IFLE" || op == "IFGT" || op == "IFGE") {
      if (la()) {
        inject.push_back(SubstituteMacroParams(body_line, args, narg, label_num));
        lstack.push_back({true, true});  // injected
      } else {
        lstack.push_back({false, false});  // skipped, local
      }
      continue;
    }

    if (op == "ELSE") {
      if (!lstack.empty()) {
        auto& top = lstack.back();
        if (top.injected) {
          inject.push_back(SubstituteMacroParams(body_line, args, narg, label_num));
        } else {
          top.active = !top.active;
        }
      } else if (la()) {
        inject.push_back(SubstituteMacroParams(body_line, args, narg, label_num));
      }
      continue;
    }

    if (op == "ENDC") {
      if (!lstack.empty()) {
        LocalFrame popped = lstack.back();
        lstack.pop_back();
        if (popped.injected)
          inject.push_back(SubstituteMacroParams(body_line, args, narg, label_num));
      } else {
        // No local frame — orphan ENDC; inject so RunPass can report error
        inject.push_back(SubstituteMacroParams(body_line, args, narg, label_num));
      }
      continue;
    }

    if (!la())
      continue;

    inject.push_back(SubstituteMacroParams(body_line, args, narg, label_num));
  }

  macro_nest_depth_--;
  listing_macro_nest_depth_++;
  InjectLines(inject);
  listing_macro_nest_depth_--;
}

}  // namespace easym68k::asm_
