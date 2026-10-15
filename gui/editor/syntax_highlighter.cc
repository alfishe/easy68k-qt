// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "editor/syntax_highlighter.h"

#include <cctype>
#include <cstring>
#include <QFont>
#include <string>

#include "easym68k/asm/instruction_set.h"

namespace easym68k::editor {
namespace {

constexpr int kMaxTokens = 256;
constexpr int kMaxSize = 512;

enum class TokenLineType {
  kUnknown,
  kCode,
  kOpcodeFound,
  kComment,
  kErrorStyle,
  kStructure,
};

constexpr int kRelCount = 14;
constexpr int kRelWidth = 9;
constexpr char kRelational[kRelCount][kRelWidth] = {
    {'*', '<', '*', '&', '.', '*', '<', '*', ' '}, {'*', '<', '*', '|', '.', '*', '<', '*', ' '},
    {'*', '<', '*', '&', '*', '<', '*', ' ', ' '}, {'*', '<', '*', '|', '*', '<', '*', ' ', ' '},
    {'<', '&', '.', '*', '<', '*', ' ', ' ', ' '}, {'<', '|', '.', '*', '<', '*', ' ', ' ', ' '},
    {'<', '&', '*', '<', '*', ' ', ' ', ' ', ' '}, {'<', '|', '*', '<', '*', ' ', ' ', ' ', ' '},
    {'*', '<', '*', '&', '<', ' ', ' ', ' ', ' '}, {'*', '<', '*', '|', '<', ' ', ' ', ' ', ' '},
    {'<', '&', '<', ' ', ' ', ' ', ' ', ' ', ' '}, {'<', '|', '<', ' ', ' ', ' ', ' ', ' ', ' '},
    {'*', '<', '*', ' ', ' ', ' ', ' ', ' ', ' '}, {'<', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
};

char* SkipSpace(char* p) {
  while (*p && std::isspace(static_cast<unsigned char>(*p))) {
    ++p;
  }
  return p;
}

int Tokenize(char* instr, const char* delim, char* token[], char* tokens, char* token_end[],
             char* empty_str) {
  int tok_n = 0;
  int tok_count = 0;
  char* start = instr;
  int paren_count = 0;
  const bool dot_delimiter = (std::strchr(delim, '.') != nullptr);
  bool quoted = false;

  for (int i = 0; i < kMaxTokens; ++i) {
    token[i] = empty_str;
    token_end[i] = nullptr;
  }

  while (*instr && std::isspace(static_cast<unsigned char>(*instr))) {
    ++instr;
  }
  if (*instr != '*' && *instr != ';') {
    if (start != instr) {
      tok_n = 1;
    }
    int size = 0;
    while (*instr && tok_n < kMaxTokens && size < kMaxSize) {
      paren_count = 0;
      token[tok_n] = &tokens[size];
      while (*instr && std::isspace(static_cast<unsigned char>(*instr))) {
        ++instr;
      }
      if (*instr == '\'' && *(instr + 1) == '\'') {
        tokens[size++] = '\0';
        instr += 2;
      }
      if (dot_delimiter && *instr == '.') {
        tokens[size++] = *instr++;
      }
      while (*instr && ((std::strchr(delim, *instr) == nullptr) || paren_count > 0 || quoted) &&
             (size < kMaxSize - 1)) {
        if (*instr == '\'') {
          quoted = !quoted;
        }
        if (*instr == '(') {
          ++paren_count;
        } else if (*instr == ')') {
          --paren_count;
        }
        tokens[size++] = *instr++;
      }

      tokens[size++] = '\0';
      token_end[tok_n] = instr;
      if (*instr && (!dot_delimiter || *instr != '.')) {
        ++instr;
      }
      ++tok_count;
      ++tok_n;
      while (*instr && std::isspace(static_cast<unsigned char>(*instr))) {
        ++instr;
      }
    }
  }
  return tok_count;
}

int CheckRelational(int n, char* const token_s[]) {
  for (int row = 0; row < kRelCount; ++row) {
    for (int col = 0; col < kRelWidth; ++col) {
      if (n + col >= kMaxTokens)
        break;
      const char pat = kRelational[row][col];
      const char* tok = token_s[n + col];
      if (pat == '<') {
        if (std::strlen(tok) < 4 || tok[0] != '<' || tok[3] != '>') {
          break;
        }
      }
      if (pat == '|') {
        if (std::strcmp(tok, "OR") != 0) {
          break;
        }
      }
      if (pat == '&') {
        if (std::strcmp(tok, "AND") != 0) {
          break;
        }
      }
      if (pat == '.') {
        if (tok[0] != '.') {
          break;
        }
      }
      if (pat == '*') {
        if (tok[0] == '\0') {
          break;
        }
      }
      if (pat == ' ') {
        return n + col;
      }
    }
  }
  return 0;
}

// Looks up whether `mnemonic` is a valid 68000 instruction (flavor table or
// MOVEM) and whether it accepts operands (`wants_operands`).
bool LookupOpcodeForHighlight(const char* token1, const char* token2, bool* wants_operands) {
  if (token1 == nullptr || token1[0] == '\0')
    return false;
  if (token2 != nullptr && token2[0] == '.') {
    // Size code must be single letter (.B, .W, .L, .S) or invalid minor error;
    // multi-character size suffix is a severe syntax error in instLookup.
    if (std::strlen(token2) > 2) {
      return false;
    }
  }
  if (std::strcmp(token1, "MOVEM") == 0) {
    *wants_operands = true;
    return true;
  }
  const asm_::Instruction* inst = asm_::LookupInstruction(token1);
  if (inst == nullptr) {
    return false;
  }
  *wants_operands =
      (inst->flavors != nullptr && inst->flavor_count > 0 && inst->flavors[0].source != 0);
  return true;
}

char* HighlightOperand(char* p, const char* cap_line, char*& syntax_start,
                       QVector<M68kSyntaxHighlighter::Span>* spans) {
  using SpanKind = M68kSyntaxHighlighter::SpanKind;
  bool more = false;
  do {
    int paren_count = 0;
    while (*p && *p != '(' && *p != '\'' && *p != '\"' && *p != ',' && *p != ' ' && *p != '\t') {
      ++p;
    }
    if (!*p) {
      return p;
    } else if (*p == '(') {
      ++p;
      ++paren_count;
      while (*p && paren_count > 0) {
        if (*p == '(') {
          ++paren_count;
        } else if (*p == ')') {
          --paren_count;
        }
        ++p;
      }
      if (!*p && paren_count > 0) {
        spans->push_back({static_cast<int>(syntax_start - cap_line),
                          static_cast<int>(p - syntax_start), SpanKind::kError});
        return p;
      }
      if (*p && !std::isspace(static_cast<unsigned char>(*p))) {
        ++p;
        more = true;
        continue;
      }
    } else if (*p == '\'' || *p == '\"') {
      const char quote = *p;
      syntax_start = p;
      do {
        ++p;
        while (*p == '\'' && *(p + 1) == '\'') {
          p += 2;
        }
      } while (*p && *p != quote);
      if (!*p) {
        spans->push_back({static_cast<int>(syntax_start - cap_line),
                          static_cast<int>(p - syntax_start), SpanKind::kError});
        return p;
      } else {
        ++p;
        spans->push_back({static_cast<int>(syntax_start - cap_line),
                          static_cast<int>(p - syntax_start), SpanKind::kText});
      }
      syntax_start = p;
      if (*p && !std::isspace(static_cast<unsigned char>(*p))) {
        ++p;
        more = true;
        continue;
      }
    }
    p = SkipSpace(p);
    if (*p == ',') {
      ++p;
      p = SkipSpace(p);
      more = true;
    } else {
      more = false;
    }
  } while (more);
  return p;
}

}  // namespace

SyntaxTheme SyntaxTheme::FromPreset(int preset_index, const QColor& bg) {
  SyntaxTheme t;
  t.background = bg;
  t.preset = preset_index;
  t.disabled = false;

  switch (preset_index) {
    case kColor1:
      // Default Color 1 (Edit68K/asm.h DEFAULT_*_COLOR).
      break;
    case kColor2:
      t.code.color = QColor(0, 0, 0);
      t.unknown.color = QColor(128, 0, 128);  // clPurple
      t.directive.color = QColor(0, 0, 255);  // clBlue
      t.comment.color = QColor(0, 128, 0);    // clGreen
      t.label.color = QColor(0, 0, 128);      // clNavy
      t.structure.color = QColor(0, 0, 255);  // clBlue
      t.error.color = QColor(255, 0, 0);      // clRed
      t.text.color = QColor(128, 0, 0);       // clMaroon
      break;
    case kMono:
      t.code.color = QColor(0, 0, 0);
      t.unknown = {QColor(0, 0, 0), false, true, true};
      t.directive = {QColor(0, 0, 0), true, false, false};
      t.comment = {QColor(0, 0, 0), false, true, false};
      t.label = {QColor(0, 0, 0), true, false, false};
      t.structure = {QColor(0, 0, 0), true, false, false};
      t.error = {QColor(0, 0, 0), false, false, true};
      t.text = {QColor(0, 0, 0), false, false, false};
      break;
    case kCustom:
      break;
    default:  // kDisabled
      t.code.color = QColor(0, 0, 0);
      t.unknown.color = QColor(0, 0, 0);
      t.directive.color = QColor(0, 0, 0);
      t.comment.color = QColor(0, 0, 0);
      t.label.color = QColor(0, 0, 0);
      t.structure.color = QColor(0, 0, 0);
      t.error.color = QColor(0, 0, 0);
      t.text.color = QColor(0, 0, 0);
      t.disabled = true;
      t.preset = kDisabled;
      break;
  }
  return t;
}

M68kSyntaxHighlighter::M68kSyntaxHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {}

void M68kSyntaxHighlighter::SetTheme(const SyntaxTheme& theme) {
  theme_ = theme;
  rehighlight();
}

void M68kSyntaxHighlighter::SetEnabled(bool enabled) {
  if (enabled_ == enabled)
    return;
  enabled_ = enabled;
  rehighlight();
}

QTextCharFormat M68kSyntaxHighlighter::FormatForStyle(const TextStyle& style) {
  QTextCharFormat fmt;
  fmt.setForeground(style.color);
  fmt.setFontWeight(style.bold ? QFont::Bold : QFont::Normal);
  fmt.setFontItalic(style.italic);
  fmt.setFontUnderline(style.underline);
  return fmt;
}

QVector<M68kSyntaxHighlighter::Span> M68kSyntaxHighlighter::ClassifyLine(const QString& line) {
  QVector<Span> spans;
  if (line.isEmpty())
    return spans;

  const QByteArray upper_bytes = line.toUpper().toLatin1();
  const int line_len = upper_bytes.size();
  std::string cap_buf(upper_bytes.constData(), static_cast<size_t>(line_len));
  cap_buf.push_back('\0');
  char* const cap_line = cap_buf.data();

  char* start = SkipSpace(cap_line);
  char* p = start;
  if (!*p)
    return spans;

  if (*p == '*' || *p == ';') {
    spans.push_back({0, line_len, SpanKind::kComment});
    return spans;
  }

  while (std::isalnum(static_cast<unsigned char>(*p)) || *p == '.' || *p == '_' || *p == '$') {
    ++p;
  }
  if (start == cap_line || *p == ':') {
    if (*p == ':') {
      ++p;
    }
    spans.push_back({0, static_cast<int>(p - cap_line), SpanKind::kLabel});
    p = SkipSpace(p);
    if (!*p)
      return spans;
  } else {
    p = start;
    start = cap_line;
  }

  if (*p == '*' || *p == ';') {
    const int offset = static_cast<int>(p - cap_line);
    spans.push_back({offset, line_len - offset, SpanKind::kComment});
    return spans;
  }

  char empty_str[1] = {'\0'};
  char* token_s[kMaxTokens];
  char* token_end[kMaxTokens];
  char tokens_s[kMaxSize];
  Tokenize(start, ". \t\n", token_s, tokens_s, token_end, empty_str);

  TokenLineType token_type = TokenLineType::kUnknown;
  char* syntax_start = p;

  constexpr int kDirectiveCount = 32;
  constexpr int kNoOperandDirectives = 7;
  static constexpr const char* kDirectives[kDirectiveCount] = {
      "ENDC",   "ENDM", "LIST",   "NOLIST", "PAGE",    "MEXIT",   "SIMHALT", "DC",
      "DCB",    "DS",   "END",    "EQU",    "FAIL",    "INCLUDE", "MACRO",   "MEMORY",
      "OFFSET", "OPT",  "ORG",    "REG",    "SECTION", "SET",     "IFC",     "IFNC",
      "IFEQ",   "IFNE", "INCBIN", "IFLT",   "IFLE",    "IFGT",    "IFGE",    "NARG",
  };
  constexpr int kStructureSingles = 5;
  static constexpr const char* kSingleStructures[kStructureSingles] = {"ELSE", "ENDI", "ENDF",
                                                                       "ENDW", "REPEAT"};

  int n = 1;
  if (token_s[2][0] == '.') {
    n = 2;
  }

  bool handled = false;
  for (int i = 0; i < kDirectiveCount; ++i) {
    if (std::strcmp(token_s[1], kDirectives[i]) == 0) {
      if (token_end[n] != nullptr) {
        p = token_end[n];
      }
      spans.push_back({static_cast<int>(syntax_start - cap_line),
                       static_cast<int>(p - syntax_start), SpanKind::kDirective});
      p = SkipSpace(p);
      syntax_start = p;
      if (std::strcmp(token_s[1], "FAIL") == 0) {
        token_type = TokenLineType::kOpcodeFound;
        handled = true;
        break;
      }
      if (i < kNoOperandDirectives) {
        token_type = TokenLineType::kComment;
        syntax_start = p;
        handled = true;
        break;
      }
      p = HighlightOperand(p, cap_line, syntax_start, &spans);
      if (std::strcmp(token_s[1], "MEMORY") == 0) {
        p = HighlightOperand(p, cap_line, syntax_start, &spans);
      }
      token_type = TokenLineType::kComment;
      syntax_start = p;
      handled = true;
      break;
    }
  }

  if (!handled) {
    if (std::strcmp(token_s[1], "IF") == 0) {
      n = CheckRelational(n + 1, token_s);
      if (n != 0 && std::strcmp(token_s[n], "THEN") == 0) {
        token_type = TokenLineType::kStructure;
      } else {
        token_type = TokenLineType::kErrorStyle;
      }
    } else if (std::strcmp(token_s[1], "WHILE") == 0) {
      if (std::strcmp(token_s[n + 1], "<T>") == 0) {
        n += 2;
      } else {
        n = CheckRelational(n + 1, token_s);
      }
      if (n != 0 && std::strcmp(token_s[n], "DO") == 0) {
        token_type = TokenLineType::kStructure;
      } else {
        token_type = TokenLineType::kErrorStyle;
      }
    } else if (std::strcmp(token_s[1], "FOR") == 0) {
      if (std::strcmp(token_s[n + 2], "=") == 0 &&
          (std::strcmp(token_s[n + 4], "TO") == 0 || std::strcmp(token_s[n + 4], "DOWNTO") == 0)) {
        if (std::strcmp(token_s[n + 6], "BY") == 0) {
          n += 8;
        } else {
          n += 6;
        }
        if (std::strcmp(token_s[n], "DO") == 0) {
          token_type = TokenLineType::kStructure;
        } else {
          token_type = TokenLineType::kErrorStyle;
        }
      } else {
        token_type = TokenLineType::kErrorStyle;
      }
    } else if (std::strcmp(token_s[1], "DBLOOP") == 0) {
      if (token_s[n + 1][0] == 'D' && std::isdigit(static_cast<unsigned char>(token_s[n + 1][1])) &&
          token_s[n + 2][0] == '=' && token_s[n + 3][0] != '\0') {
        n += 3;
        token_type = TokenLineType::kStructure;
      } else {
        token_type = TokenLineType::kErrorStyle;
      }
    } else if (std::strcmp(token_s[1], "UNLESS") == 0) {
      if (token_s[n + 1][0] == '\0') {
        token_type = TokenLineType::kStructure;
      } else if (std::strcmp(token_s[n + 1], "<F>") == 0) {
        token_type = TokenLineType::kStructure;
        n += 1;
      } else if (std::strlen(token_s[n + 2]) >= 4 && token_s[n + 2][0] == '<' &&
                 token_s[n + 2][3] == '>' && token_s[n + 3][0] != '\0') {
        token_type = TokenLineType::kStructure;
        n += 3;
      } else {
        token_type = TokenLineType::kErrorStyle;
      }
    } else if (std::strcmp(token_s[1], "UNTIL") == 0) {
      n = CheckRelational(n + 1, token_s);
      if (n != 0 && std::strcmp(token_s[n], "DO") == 0) {
        token_type = TokenLineType::kStructure;
      } else {
        token_type = TokenLineType::kErrorStyle;
      }
    } else {
      token_type = TokenLineType::kUnknown;
      for (int i = 0; i < kStructureSingles; ++i) {
        if (std::strcmp(token_s[1], kSingleStructures[i]) == 0) {
          if (i == 0 && token_s[n + 1][0] == '.') {
            ++n;
          }
          token_type = TokenLineType::kStructure;
          break;
        }
      }

      if (token_type == TokenLineType::kUnknown) {
        bool wants_operands = false;
        if (LookupOpcodeForHighlight(token_s[1], token_s[2], &wants_operands)) {
          token_type = TokenLineType::kOpcodeFound;
          if (token_end[n] != nullptr) {
            p = token_end[n];
          }
          p = SkipSpace(p);
          if (token_s[n + 1][0] != '\0') {
            if (wants_operands) {
              p = HighlightOperand(p, cap_line, syntax_start, &spans);
            }
            p = SkipSpace(p);
            if (*p) {
              syntax_start = p;
              token_type = TokenLineType::kComment;
            }
          }
        }
      }
    }

    if (token_type == TokenLineType::kStructure) {
      if (token_s[n + 1][0] == '.') {
        ++n;
      }
      if (token_end[n] != nullptr) {
        p = token_end[n];
      }
      spans.push_back({static_cast<int>(syntax_start - cap_line),
                       static_cast<int>(p - syntax_start), SpanKind::kStructure});
      p = SkipSpace(p);
      syntax_start = p;
      token_type = TokenLineType::kComment;
    }
  }

  if (token_type == TokenLineType::kErrorStyle) {
    const int offset = static_cast<int>(syntax_start - cap_line);
    if (offset < line_len) {
      spans.push_back({offset, line_len - offset, SpanKind::kError});
    }
  } else if (token_type == TokenLineType::kComment) {
    const int offset = static_cast<int>(p - cap_line);
    if (offset < line_len) {
      spans.push_back({offset, line_len - offset, SpanKind::kComment});
    }
  } else if (token_type == TokenLineType::kUnknown) {
    const int offset = static_cast<int>(syntax_start - cap_line);
    if (offset < line_len) {
      spans.push_back({offset, line_len - offset, SpanKind::kUnknown});
    }
  }

  return spans;
}

void M68kSyntaxHighlighter::highlightBlock(const QString& text) {
  if (text.isEmpty())
    return;

  const QTextCharFormat code_fmt = FormatForStyle(theme_.code);
  setFormat(0, text.size(), code_fmt);
  if (!enabled_ || theme_.disabled) {
    return;
  }

  const QVector<Span> spans = ClassifyLine(text);
  for (const Span& span : spans) {
    if (span.length <= 0)
      continue;
    switch (span.kind) {
      case SpanKind::kCode:
        setFormat(span.start, span.length, code_fmt);
        break;
      case SpanKind::kComment:
        setFormat(span.start, span.length, FormatForStyle(theme_.comment));
        break;
      case SpanKind::kDirective:
        setFormat(span.start, span.length, FormatForStyle(theme_.directive));
        break;
      case SpanKind::kLabel:
        setFormat(span.start, span.length, FormatForStyle(theme_.label));
        break;
      case SpanKind::kUnknown:
        setFormat(span.start, span.length, FormatForStyle(theme_.unknown));
        break;
      case SpanKind::kStructure:
        setFormat(span.start, span.length, FormatForStyle(theme_.structure));
        break;
      case SpanKind::kError:
        setFormat(span.start, span.length, FormatForStyle(theme_.error));
        break;
      case SpanKind::kText:
        setFormat(span.start, span.length, FormatForStyle(theme_.text));
        break;
    }
  }
}

}  // namespace easym68k::editor
