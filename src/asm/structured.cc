// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Structured control-flow expansion (Task 8.5.7).
// Ported from EASy68K/Edit68K/STRUCTURED.CPP (Charles Kelly).
//
// Each structured directive is expanded into a sequence of ordinary 68000
// instructions that are injected back through the assembler.  The label
// counters are reset at the start of every pass so both passes produce
// identical label names.

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <vector>

#include "easym68k/asm/assembler.h"

namespace easym68k::asm_ {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string FormatStcLabel(uint32_t n) {
  char buf[16];
  std::snprintf(buf, sizeof(buf), "_%08X", n);
  return std::string(buf);
}

// Tokenizer matching the original tokenize(capLine, ". \t\n", ...) behaviour:
//   - Splits on space / tab / newline.
//   - A '.' encountered between tokens starts the next token with '.'.
//   - Items inside ( ) stay as one token.
//   - If the line starts with whitespace, token[0] is "" (no label present).
//   - Result is always padded to at least 20 slots.
static std::vector<std::string> TokenizeStc(const std::string& raw) {
  std::vector<std::string> toks;

  // Uppercase first
  std::string line = raw;
  for (char& c : line)
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

  const char* p = line.c_str();

  // If line begins with whitespace → no label → slot 0 stays ""
  bool leading_space = (*p && std::isspace(static_cast<unsigned char>(*p)));
  if (leading_space)
    while (*p && std::isspace(static_cast<unsigned char>(*p)))
      p++;

  toks.push_back(leading_space ? "" : "");  // slot 0 (label placeholder)

  // If no leading space, slot 0 is the label; collect it now
  if (!leading_space && *p) {
    std::string label_tok;
    while (*p && !std::isspace(static_cast<unsigned char>(*p)) && *p != '.')
      label_tok += *p++;
    toks[0] = label_tok;
    while (*p && std::isspace(static_cast<unsigned char>(*p)))
      p++;
  }

  while (*p) {
    // Skip whitespace
    while (*p && std::isspace(static_cast<unsigned char>(*p)))
      p++;
    if (!*p)
      break;

    std::string tok;

    // Dot delimiter: include '.' at start of this token
    if (*p == '.') {
      tok += *p++;
    }

    // Collect chars until space/tab/newline/dot (respect parentheses)
    int depth = 0;
    while (*p) {
      if (*p == '(')
        depth++;
      else if (*p == ')')
        depth--;
      if (depth == 0 && (std::isspace(static_cast<unsigned char>(*p)) || *p == '.'))
        break;
      tok += *p++;
    }

    if (!tok.empty())
      toks.push_back(tok);

    while (*p && std::isspace(static_cast<unsigned char>(*p)))
      p++;
  }

  // Pad to 24 slots for safe indexed access
  while (toks.size() < 24)
    toks.push_back("");

  return toks;
}

// ---------------------------------------------------------------------------
// BccCodes table — ported verbatim from STRUCTURED.CPP
//
// Columns (0-based):
//   0: <cc> token
//   1: Rn<cc>ea  / ea<cc>#n  / IF <cc>   (skip when false)
//   2: OR version of col 1                (jump when true)
//   3: ea<cc>Rn  / #n<cc>ea              (skip when false, inverted CMP side)
//   4: OR version of col 3               (jump when true)
// ---------------------------------------------------------------------------
static const char* kBccCodes[16][5] = {
    {"<GT>", "BLE", "BGT", "BGE", "BLT"}, {"<GE>", "BLT", "BGE", "BGT", "BLE"},
    {"<LT>", "BGE", "BLT", "BLE", "BGT"}, {"<LE>", "BGT", "BLE", "BLT", "BGE"},
    {"<EQ>", "BNE", "BEQ", "BNE", "BEQ"}, {"<NE>", "BEQ", "BNE", "BEQ", "BNE"},
    {"<HI>", "BLS", "BHI", "BHS", "BLO"}, {"<HS>", "BLO", "BHS", "BHI", "BLS"},
    {"<CC>", "BLO", "BCC", "BHI", "BLS"}, {"<LO>", "BHS", "BLO", "BLS", "BHI"},
    {"<CS>", "BHS", "BCS", "BLS", "BHI"}, {"<LS>", "BHI", "BLS", "BLO", "BHS"},
    {"<MI>", "BPL", "BMI", "BPL", "BMI"}, {"<PL>", "BMI", "BPL", "BMI", "BPL"},
    {"<VC>", "BVS", "BVC", "BVS", "BVC"}, {"<VS>", "BVC", "BVS", "BVC", "BVS"},
};

// mode is IF_CC=1, RN_EA=1, EA_IM=1, EA_RN=3, IM_EA=3; or_flag is 0 or 1.
static std::string GetBcc(const std::string& cc, int mode, int or_flag) {
  for (int i = 0; i < 16; i++) {
    if (cc == kBccCodes[i][0])
      return kBccCodes[i][mode + or_flag];
  }
  return "B??";
}

static bool IsRegister(const std::string& t) {
  return t.size() >= 2 && (t[0] == 'D' || t[0] == 'A') && std::isdigit(t[1]);
}

// ---------------------------------------------------------------------------
// GenCmpBcc — generate CMP + Bcc lines for one clause of a structured
// condition expression.
//
// toks: the full token vector; arg0 is the index of the first argument
//       (after any optional size spec).
// extent: branch size suffix including tab, e.g. "\t" or ".S\t".
// label: branch target label string.
// or_flag: 0 for normal (skip-when-false), 1 for OR (jump-when-true).
// ---------------------------------------------------------------------------
static std::vector<std::string> GenCmpBcc(const std::vector<std::string>& toks, int arg0,
                                          const std::string& extent, const std::string& label,
                                          int or_flag, bool& error) {
  std::vector<std::string> lines;
  error = false;

  // Optional size spec at toks[arg0-1] (already consumed by caller; use default .W)
  // The size is whatever was passed in from the enclosing call.
  // Here we deduce it from a token just before arg0 if it starts with '.'.
  std::string cmp = "\tCMP.W\t";
  int a = arg0;
  // Note: the caller passes arg0 already past the optional size token.
  // Reconstruct cmp size from toks[arg0-1] when it is ".B"/".W"/".L".
  // (handled by the caller — we receive arg0 pointing at the first operand)

  if (toks[a].empty()) {
    error = true;
    return lines;
  }

  if (toks[a][0] == '<') {
    // Simple condition code — no CMP, just Bcc
    lines.push_back("\t" + GetBcc(toks[a], 1, or_flag) + extent + label + "\n");
  } else if (toks[a][0] == '#') {
    // #n <cc> ea  →  CMP.x #n,ea  then getBcc col IM_EA=3
    lines.push_back(cmp + toks[a] + "," + toks[a + 2] + "\n");
    lines.push_back("\t" + GetBcc(toks[a + 1], 3, or_flag) + extent + label + "\n");
  } else if (toks[a + 2].size() > 0 && toks[a + 2][0] == '#') {
    // ea <cc> #n  →  CMP.x #n,ea  then getBcc col EA_IM=1
    lines.push_back(cmp + toks[a + 2] + "," + toks[a] + "\n");
    lines.push_back("\t" + GetBcc(toks[a + 1], 1, or_flag) + extent + label + "\n");
  } else if (IsRegister(toks[a])) {
    // Rn <cc> ea  →  CMP.x ea,Rn  then getBcc col RN_EA=1
    lines.push_back(cmp + toks[a + 2] + "," + toks[a] + "\n");
    lines.push_back("\t" + GetBcc(toks[a + 1], 1, or_flag) + extent + label + "\n");
  } else if (IsRegister(toks[a + 2])) {
    // ea <cc> Rn  →  CMP.x ea,Rn  then getBcc col EA_RN=3
    lines.push_back(cmp + toks[a] + "," + toks[a + 2] + "\n");
    lines.push_back("\t" + GetBcc(toks[a + 1], 3, or_flag) + extent + label + "\n");
  } else {
    error = true;
  }
  return lines;
}

// Apply CMP size from a size token (".B"/".W"/".L") to a GenCmpBcc-style
// result by patching "CMP.W" to the correct size.
static std::vector<std::string> GenCmpBccSized(const std::vector<std::string>& toks, int size_tok,
                                               int arg0, const std::string& extent,
                                               const std::string& label, int or_flag, bool& error) {
  // Determine size string from toks[size_tok] if it starts with '.'
  std::string size_str = "W";
  if (!toks[size_tok].empty() && toks[size_tok][0] == '.') {
    char s = toks[size_tok].size() > 1 ? toks[size_tok][1] : '?';
    if (s == 'B')
      size_str = "B";
    else if (s == 'W')
      size_str = "W";
    else if (s == 'L')
      size_str = "L";
    else {
      error = true;
      return {};
    }
  }

  auto lines = GenCmpBcc(toks, arg0, extent, label, or_flag, error);
  if (size_str != "W") {
    for (auto& l : lines) {
      // Patch "CMP.W" → "CMP.x"
      size_t pos = l.find("CMP.W");
      if (pos != std::string::npos)
        l[pos + 4] = size_str[0];
    }
  }
  return lines;
}

// Find keyword (THEN / DO) in toks starting from index 3; return its index or
// -1.  Also extract extent (branch size suffix) from the token after it.
static int FindKeyword(const std::vector<std::string>& toks, const char* kw, std::string& extent) {
  for (int i = 3; i <= 11; i++) {
    if (toks[i] == kw) {
      // token after keyword: ".S" or ".L" or absent
      if (!toks[i + 1].empty() && toks[i + 1][0] == '.') {
        char c = toks[i + 1].size() > 1 ? toks[i + 1][1] : '\0';
        if (c == 'S')
          extent = ".S\t";
        else if (c == 'L')
          extent = ".L\t";
        else
          extent = "\t";
      } else {
        extent = "\t";
      }
      return i;
    }
  }
  return -1;
}

// ---------------------------------------------------------------------------
// HandleStructured — dispatches to per-construct logic, generates assembly
// lines, and injects them back through RunPass.
// ---------------------------------------------------------------------------
void Assembler::HandleStructured(const std::string& raw_line, int line_number) {
  auto toks = TokenizeStc(raw_line);

  // toks[0] = label (or ""), toks[1] = keyword, toks[2..] = arguments
  // n: operand start index (2, or 3 if toks[2] is a size spec ".B"/".W"/".L")
  int n = 2;
  if (!toks[2].empty() && toks[2][0] == '.')
    n = 3;

  const std::string& kw = toks[1];
  std::vector<std::string> inject;

  // Optionally define a label that was on this structured directive line.
  if (!toks[0].empty()) {
    inject.push_back(toks[0] + ":\n");
  }

  auto add_error = [&](const std::string& msg) {
    if (pass_ == 2)
      AddError(line_number, msg);
  };

  // ==================== IF ====================
  if (kw == "IF") {
    std::string extent;
    int then_idx = FindKeyword(toks, "THEN", extent);
    if (then_idx < 0) {
      add_error("IF: THEN expected");
      return;
    }

    std::string lbl1 = FormatStcLabel(stc_label_i_);
    bool err = false;

    // Check for OR / AND compound expressions
    bool has_or_simple = (toks[n + 1] == "OR");
    bool has_or_ea = (toks[n + 3] == "OR");
    bool has_and_simple = (toks[n + 1] == "AND");
    bool has_and_ea = (toks[n + 3] == "AND");

    int or_flag = (has_or_simple || has_or_ea) ? 1 : 0;
    if (or_flag)
      extent = ".S\t";  // first OR branch always short

    auto first = GenCmpBccSized(toks, 2, n, extent, lbl1, or_flag, err);
    if (err) {
      add_error("IF: syntax error in condition");
      return;
    }
    for (auto& l : first)
      inject.push_back(l);

    if (has_or_simple || has_or_ea) {
      std::string lbl2 = lbl1;
      stc_label_i_++;
      lbl1 = FormatStcLabel(stc_label_i_);
      // Second clause at n+2 (simple) or n+4 (ea form)
      int start2 = has_or_simple ? n + 2 : n + 4;
      // Second branch uses normal extent (from THEN)
      std::string extent2;
      FindKeyword(toks, "THEN", extent2);
      auto second = GenCmpBccSized(toks, 2, start2, extent2, lbl1, 0, err);
      if (err) {
        add_error("IF: syntax error in second condition");
        return;
      }
      for (auto& l : second)
        inject.push_back(l);
      inject.push_back(lbl2 + ":\n");
    } else if (has_and_simple || has_and_ea) {
      int start2 = has_and_simple ? n + 2 : n + 4;
      auto second = GenCmpBccSized(toks, 2, start2, extent, lbl1, 0, err);
      if (err) {
        add_error("IF: syntax error in second condition");
        return;
      }
      for (auto& l : second)
        inject.push_back(l);
    }

    stc_stack_.push_back(stc_label_i_);
    stc_label_i_++;
    InjectLines(inject);
    return;
  }

  // ==================== ELSE ====================
  if (kw == "ELSE") {
    if (stc_stack_.empty() || (stc_stack_.back() & kStcTypeMask) != kStcMaskI) {
      add_error("ELSE without matching IF");
      return;
    }
    uint32_t if_lbl = stc_stack_.back();
    stc_stack_.pop_back();

    std::string extent;
    if (!toks[2].empty() && toks[2][0] == '.') {
      char c = toks[2].size() > 1 ? toks[2][1] : '\0';
      extent = (c == 'S') ? ".S\t" : (c == 'L') ? ".L\t" : "\t";
    } else {
      extent = "\t";
    }

    std::string new_lbl = FormatStcLabel(stc_label_i_);
    inject.push_back("\tBRA" + extent + new_lbl + "\n");
    inject.push_back(FormatStcLabel(if_lbl) + ":\n");
    stc_stack_.push_back(stc_label_i_);
    stc_label_i_++;
    InjectLines(inject);
    return;
  }

  // ==================== ENDI ====================
  if (kw == "ENDI") {
    if (stc_stack_.empty() || (stc_stack_.back() & kStcTypeMask) != kStcMaskI) {
      add_error("ENDI without matching IF");
      return;
    }
    uint32_t lbl = stc_stack_.back();
    stc_stack_.pop_back();
    inject.push_back(FormatStcLabel(lbl) + ":\n");
    InjectLines(inject);
    return;
  }

  // ==================== WHILE ====================
  if (kw == "WHILE") {
    std::string extent;
    int do_idx = FindKeyword(toks, "DO", extent);
    if (do_idx < 0) {
      add_error("WHILE: DO expected");
      return;
    }

    // Define loop-top label and push it (tagged WHILE)
    std::string top_lbl = FormatStcLabel(stc_label_w_);
    inject.push_back(top_lbl + ":\n");
    stc_stack_.push_back(stc_label_w_);
    stc_label_w_++;

    std::string exit_lbl = FormatStcLabel(stc_label_w_);

    // Generate condition (unless <T> = infinite loop)
    if (toks[n] != "<T>") {
      bool has_or_simple = (toks[n + 1] == "OR");
      bool has_or_ea = (toks[n + 3] == "OR");
      bool has_and_simple = (toks[n + 1] == "AND");
      bool has_and_ea = (toks[n + 3] == "AND");

      int or_flag = (has_or_simple || has_or_ea) ? 1 : 0;
      std::string first_extent = or_flag ? ".S\t" : extent;

      bool err = false;
      auto first = GenCmpBccSized(toks, 2, n, first_extent, exit_lbl, or_flag, err);
      if (err) {
        add_error("WHILE: syntax error in condition");
        return;
      }
      for (auto& l : first)
        inject.push_back(l);

      if (has_or_simple || has_or_ea) {
        std::string lbl2 = exit_lbl;
        stc_label_w_++;
        exit_lbl = FormatStcLabel(stc_label_w_);
        int start2 = has_or_simple ? n + 2 : n + 4;
        auto second = GenCmpBccSized(toks, 2, start2, extent, exit_lbl, 0, err);
        if (err) {
          add_error("WHILE: syntax error in second condition");
          return;
        }
        for (auto& l : second)
          inject.push_back(l);
        inject.push_back(lbl2 + ":\n");
      } else if (has_and_simple || has_and_ea) {
        int start2 = has_and_simple ? n + 2 : n + 4;
        auto second = GenCmpBccSized(toks, 2, start2, extent, exit_lbl, 0, err);
        if (err) {
          add_error("WHILE: syntax error in second condition");
          return;
        }
        for (auto& l : second)
          inject.push_back(l);
      }
    }

    stc_stack_.push_back(stc_label_w_);
    stc_label_w_++;
    InjectLines(inject);
    return;
  }

  // ==================== ENDW ====================
  if (kw == "ENDW") {
    if (stc_stack_.size() < 2 || (stc_stack_.back() & kStcTypeMask) != kStcMaskW) {
      add_error("ENDW without matching WHILE");
      return;
    }
    uint32_t exit_lbl = stc_stack_.back();
    stc_stack_.pop_back();
    if ((stc_stack_.back() & kStcTypeMask) != kStcMaskW) {
      add_error("ENDW: mismatched stack");
      return;
    }
    uint32_t top_lbl = stc_stack_.back();
    stc_stack_.pop_back();
    inject.push_back("\tBRA\t" + FormatStcLabel(top_lbl) + "\n");
    inject.push_back(FormatStcLabel(exit_lbl) + ":\n");
    InjectLines(inject);
    return;
  }

  // ==================== REPEAT ====================
  if (kw == "REPEAT") {
    std::string top_lbl = FormatStcLabel(stc_label_r_);
    inject.push_back(top_lbl + ":\n");
    stc_stack_.push_back(stc_label_r_);
    stc_label_r_++;
    InjectLines(inject);
    return;
  }

  // ==================== UNTIL ====================
  if (kw == "UNTIL") {
    if (stc_stack_.empty() || (stc_stack_.back() & kStcTypeMask) != kStcMaskR) {
      add_error("UNTIL without matching REPEAT");
      return;
    }
    uint32_t top_lbl_val = stc_stack_.back();
    stc_stack_.pop_back();
    std::string top_lbl = FormatStcLabel(top_lbl_val);

    std::string extent;
    FindKeyword(toks, "DO", extent);  // DO is optional for UNTIL
    if (extent.empty())
      extent = "\t";

    // Temporary label for OR logic
    std::string tmp_lbl = FormatStcLabel(stc_label_r_);

    bool has_or_simple = (toks[n + 1] == "OR");
    bool has_or_ea = (toks[n + 3] == "OR");
    bool has_and_simple = (toks[n + 1] == "AND");
    bool has_and_ea = (toks[n + 3] == "AND");

    bool err = false;

    if (has_or_simple || has_or_ea) {
      // First condition branches to tmp_lbl (OR true-path) when true
      auto first = GenCmpBccSized(toks, 2, n, ".S\t", tmp_lbl, 1, err);
      if (err) {
        add_error("UNTIL: syntax error in first condition");
        return;
      }
      for (auto& l : first)
        inject.push_back(l);
      // Second condition branches back to top when condition false (loop back)
      int start2 = has_or_simple ? n + 2 : n + 4;
      auto second = GenCmpBccSized(toks, 2, start2, extent, top_lbl, 0, err);
      if (err) {
        add_error("UNTIL: syntax error in second condition");
        return;
      }
      for (auto& l : second)
        inject.push_back(l);
      inject.push_back(tmp_lbl + ":\n");
      stc_label_r_++;
    } else {
      // Single or AND: branch BACK when condition is false (keep looping)
      auto first = GenCmpBccSized(toks, 2, n, extent, top_lbl, 0, err);
      if (err) {
        add_error("UNTIL: syntax error in condition");
        return;
      }
      for (auto& l : first)
        inject.push_back(l);
      if (has_and_simple || has_and_ea) {
        int start2 = has_and_simple ? n + 2 : n + 4;
        auto second = GenCmpBccSized(toks, 2, start2, extent, top_lbl, 0, err);
        if (err) {
          add_error("UNTIL: syntax error in second condition");
          return;
        }
        for (auto& l : second)
          inject.push_back(l);
      }
    }

    InjectLines(inject);
    return;
  }

  // ==================== FOR ====================
  if (kw == "FOR") {
    std::string extent;
    if (FindKeyword(toks, "DO", extent) < 0) {
      add_error("FOR: DO expected");
      return;
    }
    if (extent.empty())
      extent = "\t";

    // Determine size
    std::string size_str = ".W\t";
    if (!toks[2].empty() && toks[2][0] == '.') {
      char s = toks[2].size() > 1 ? toks[2][1] : '?';
      if (s == 'B')
        size_str = ".B\t";
      else if (s == 'W')
        size_str = ".W\t";
      else if (s == 'L')
        size_str = ".L\t";
      else {
        add_error("FOR: invalid size");
        return;
      }
    }

    // token layout: toks[n]=op1  toks[n+1]='='  toks[n+2]=op2
    //               toks[n+3]=TO|DOWNTO  toks[n+4]=op3 (limit)
    //               toks[n+5]=BY|DO      toks[n+6]=op4 (step, if BY)
    const std::string& op1 = toks[n];
    const std::string& op2 = toks[n + 2];
    const std::string& dir = toks[n + 3];  // "TO" or "DOWNTO"
    const std::string& op3 = toks[n + 4];  // limit
    bool has_by = (toks[n + 5] == "BY");
    const std::string& op4 = has_by ? toks[n + 6] : "";  // step

    bool down = (dir == "DOWNTO");

    // Labels: body_lbl = loop body top, check_lbl = limit check
    std::string body_lbl = FormatStcLabel(stc_label_f_);
    stc_label_f_++;
    std::string check_lbl = FormatStcLabel(stc_label_f_);

    // Initialize: MOVE.x op2,op1 (skip if op1 == op2)
    if (op1 != op2)
      inject.push_back("\tMOVE" + size_str + op2 + "," + op1 + "\n");

    // Jump to limit check first
    inject.push_back("\tBRA" + extent + check_lbl + "\n");
    stc_stack_.push_back(stc_label_f_);  // push check_lbl value

    // Define body top
    inject.push_back(body_lbl + ":\n");

    // Deferred code (assembled at ENDF) — pushed in reverse pop order:
    // Pop order at ENDF: step first, then check_lbl def, then CMP, then Bcc.
    std::string bcc_line = std::string("\t") + (down ? "BGE" : "BLE") + extent + body_lbl + "\n";
    std::string cmp_line = "\tCMP" + size_str + op3 + "," + op1 + "\n";
    std::string step_line;
    if (has_by)
      step_line = std::string("\t") + (down ? "SUB" : "ADD") + size_str + op4 + "," + op1 + "\n";
    else
      step_line = std::string("\t") + (down ? "SUB" : "ADD") + size_str + "#1," + op1 + "\n";

    for_stack_.push_back(bcc_line);
    for_stack_.push_back(cmp_line);
    for_stack_.push_back(step_line);

    stc_label_f_++;
    InjectLines(inject);
    return;
  }

  // ==================== ENDF ====================
  if (kw == "ENDF") {
    if (stc_stack_.empty() || (stc_stack_.back() & kStcTypeMask) != kStcMaskF) {
      add_error("ENDF without matching FOR");
      return;
    }
    if (for_stack_.size() < 3) {
      add_error("ENDF: for_stack_ underflow");
      return;
    }
    uint32_t check_lbl_val = stc_stack_.back();
    stc_stack_.pop_back();

    // Pop order: step, then CMP, then Bcc
    std::string step = for_stack_.back();
    for_stack_.pop_back();
    std::string cmp = for_stack_.back();
    for_stack_.pop_back();
    std::string bcc = for_stack_.back();
    for_stack_.pop_back();

    inject.push_back(step);
    inject.push_back(FormatStcLabel(check_lbl_val) + ":\n");
    inject.push_back(cmp);
    inject.push_back(bcc);
    InjectLines(inject);
    return;
  }

  // ==================== DBLOOP ====================
  if (kw == "DBLOOP") {
    // Syntax: DBLOOP Dn = op2
    if (toks[2].size() < 2 || toks[2][0] != 'D' || !std::isdigit(toks[2][1]) || toks[3] != "=") {
      add_error("DBLOOP: syntax must be DBLOOP Dn = expr");
      return;
    }
    char reg_num = toks[2][1];
    const std::string& op2 = toks[4];

    db_stack_.push_back(reg_num);

    // Initialize counter: MOVE op2,Dn (skip if same)
    if (toks[2] != op2)
      inject.push_back("\tMOVE\t" + op2 + "," + toks[2] + "\n");

    // Define loop top
    std::string top_lbl = FormatStcLabel(stc_label_d_);
    inject.push_back(top_lbl + ":\n");
    stc_stack_.push_back(stc_label_d_);
    stc_label_d_++;
    InjectLines(inject);
    return;
  }

  // ==================== UNLESS ====================
  if (kw == "UNLESS") {
    if (stc_stack_.empty() || (stc_stack_.back() & kStcTypeMask) != kStcMaskD) {
      add_error("UNLESS without matching DBLOOP");
      return;
    }
    if (db_stack_.empty()) {
      add_error("UNLESS: db_stack_ underflow");
      return;
    }
    uint32_t top_lbl_val = stc_stack_.back();
    stc_stack_.pop_back();
    char reg_num = db_stack_.back();
    db_stack_.pop_back();

    // DB-suffix part: "  D<n>,_XXXXXXXX" (used as the register+label part)
    std::string db_suffix = "\tD" + std::string(1, reg_num) + "," + FormatStcLabel(top_lbl_val);

    // UNLESS <F> or UNLESS with no expression → DBRA
    bool no_cond = (toks[n] == "<F>" || toks[2].empty());
    if (no_cond) {
      inject.push_back("\tDBRA" + db_suffix + "\n");
    } else {
      // Determine size for CMP
      std::string size_str = ".W\t";
      if (!toks[2].empty() && toks[2][0] == '.') {
        char s = toks[2].size() > 1 ? toks[2][1] : '?';
        if (s == 'B')
          size_str = ".B\t";
        else if (s == 'W')
          size_str = ".W\t";
        else if (s == 'L')
          size_str = ".L\t";
        else {
          add_error("UNLESS: invalid size");
          return;
        }
      }

      if (toks[n][0] == '<') {
        // UNLESS <cc>  →  DB<cc> Dn,label (uses inverted-false branch, col 1)
        inject.push_back("\tD" + GetBcc(toks[n], 1, 0) + db_suffix + "\n");
      } else if (toks[n][0] == '#') {
        // UNLESS #n <cc> ea
        inject.push_back("\tCMP" + size_str + toks[n] + "," + toks[n + 2] + "\n");
        inject.push_back("\tD" + GetBcc(toks[n + 1], 3, 0) + db_suffix + "\n");
      } else if (toks[n + 2].size() > 0 && toks[n + 2][0] == '#') {
        // UNLESS ea <cc> #n
        inject.push_back("\tCMP" + size_str + toks[n + 2] + "," + toks[n] + "\n");
        inject.push_back("\tD" + GetBcc(toks[n + 1], 1, 0) + db_suffix + "\n");
      } else if (IsRegister(toks[n])) {
        // UNLESS Rn <cc> ea
        inject.push_back("\tCMP" + size_str + toks[n + 2] + "," + toks[n] + "\n");
        inject.push_back("\tD" + GetBcc(toks[n + 1], 1, 0) + db_suffix + "\n");
      } else if (IsRegister(toks[n + 2])) {
        // UNLESS ea <cc> Rn
        inject.push_back("\tCMP" + size_str + toks[n] + "," + toks[n + 2] + "\n");
        inject.push_back("\tD" + GetBcc(toks[n + 1], 3, 0) + db_suffix + "\n");
      } else {
        add_error("UNLESS: syntax error in condition");
        return;
      }
    }

    InjectLines(inject);
    return;
  }
}

}  // namespace easym68k::asm_
