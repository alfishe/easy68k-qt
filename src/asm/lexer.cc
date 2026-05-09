// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/asm/lexer.h"

#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace easym68k::asm_ {

namespace {

// 68000 instruction mnemonics (case-insensitive match via upper-case lookup).
const std::unordered_set<std::string> kOpcodes = {
    // Data movement
    "MOVE",
    "MOVEA",
    "MOVEM",
    "MOVEP",
    "MOVEQ",
    "LEA",
    "PEA",
    "EXG",
    "SWAP",
    "LINK",
    "UNLK",
    // Arithmetic
    "ADD",
    "ADDA",
    "ADDI",
    "ADDQ",
    "ADDX",
    "SUB",
    "SUBA",
    "SUBI",
    "SUBQ",
    "SUBX",
    "MULS",
    "MULU",
    "DIVS",
    "DIVU",
    "NEG",
    "NEGX",
    "CLR",
    "CMP",
    "CMPA",
    "CMPI",
    "CMPM",
    "EXT",
    "EXTB",
    "ABCD",
    "SBCD",
    "NBCD",
    "CHK",
    // Logic
    "AND",
    "ANDI",
    "OR",
    "ORI",
    "EOR",
    "EORI",
    "NOT",
    "TAS",
    // Shift/Rotate
    "ASL",
    "ASR",
    "LSL",
    "LSR",
    "ROL",
    "ROR",
    "ROXL",
    "ROXR",
    // Bit manipulation
    "BTST",
    "BSET",
    "BCLR",
    "BCHG",
    // Branch
    "BRA",
    "BSR",
    "BCC",
    "BCS",
    "BEQ",
    "BNE",
    "BGE",
    "BGT",
    "BHI",
    "BLE",
    "BLS",
    "BLT",
    "BMI",
    "BPL",
    "BVC",
    "BVS",
    "BHS",
    "BLO",
    // DBcc
    "DBCC",
    "DBCS",
    "DBEQ",
    "DBNE",
    "DBGE",
    "DBGT",
    "DBHI",
    "DBLE",
    "DBLS",
    "DBLT",
    "DBMI",
    "DBPL",
    "DBVC",
    "DBVS",
    "DBF",
    "DBT",
    "DBRA",
    "DBHS",
    "DBLO",
    // Scc
    "SCC",
    "SCS",
    "SEQ",
    "SNE",
    "SGE",
    "SGT",
    "SHI",
    "SLE",
    "SLS",
    "SLT",
    "SMI",
    "SPL",
    "SVC",
    "SVS",
    "SF",
    "ST",
    "SHS",
    "SLO",
    // Jump / call
    "JMP",
    "JSR",
    "RTS",
    "RTR",
    "RTE",
    // Trap
    "TRAP",
    "TRAPV",
    // Test
    "TST",
    // Miscellaneous
    "NOP",
    "RESET",
    "STOP",
    "ILLEGAL",
    // 68010+
    "MOVEC",
    "MOVES",
    "RTD",
    "BFCHG",
    "BFCLR",
    "BFEXTS",
    "BFEXTU",
    "BFFFO",
    "BFINS",
    "BFSET",
    "BFTST",
};

// Assembler directives recognised by the EASy68K assembler.
const std::unordered_set<std::string> kDirectives = {
    // Data / storage
    "DC",
    "DCB",
    "DS",
    // Symbol definition
    "EQU",
    "SET",
    "REG",
    // Origin / section
    "ORG",
    "SECTION",
    "OFFSET",
    "MEMORY",
    // End of source
    "END",
    "SIMHALT",
    // File inclusion
    "INCLUDE",
    "INCBIN",
    // Listing control
    "LIST",
    "NOLIST",
    "PAGE",
    "OPT",
    // Macro
    "MACRO",
    "ENDM",
    "MEND",
    "MEXIT",
    // Conditionals (standard)
    "IF",
    "ELSE",
    "ENDI",
    "ENDIF",
    "ENDC",
    "IFEQ",
    "IFNE",
    "IFGT",
    "IFGE",
    "IFLT",
    "IFLE",
    "IFC",
    "IFNC",
    "IFD",
    "IFND",
    // Structured control flow (EASy68K extensions)
    "WHILE",
    "ENDW",
    "REPEAT",
    "UNTIL",
    "FOR",
    "ENDF",
    "IF",
    "THEN",
    "ELSE",
    "ENDI",
    "DBLOOP",
    "UNLESS",
    "SWITCH",
    "ENDSW",
    "DO",
    // External symbol declarations
    "XDEF",
    "XREF",
    "PUBLIC",
    "EXTERN",
    "GLOBAL",
    // Miscellaneous
    "FAIL",
    "EVEN",
    "CNOP",
    "ALIGN",
    "EQUR",
    "REPT",
    "ENDR",
    "TITLE",
    "TTL",
    "SPC",
    // Execution-plan directives (not all verified in original source)
    "COMMENT",
    "LOAD",
};

}  // namespace

Lexer::Lexer() : pos_(0), line_(1), column_(1), has_error_(false), has_peeked_(false) {}

Lexer::~Lexer() = default;

void Lexer::SetSource(const std::string& source) {
  source_ = source;
  pos_ = 0;
  line_ = 1;
  column_ = 1;
  has_error_ = false;
  error_message_.clear();
  has_peeked_ = false;
}

Token Lexer::NextToken() {
  if (has_peeked_) {
    has_peeked_ = false;
    return peeked_token_;
  }
  return ScanToken();
}

Token Lexer::PeekToken() {
  if (!has_peeked_) {
    peeked_token_ = ScanToken();
    has_peeked_ = true;
  }
  return peeked_token_;
}

std::vector<Token> Lexer::TokenizeLine(const std::string& line, int line_number) {
  std::vector<Token> tokens;
  SetSource(line);
  line_ = line_number;

  while (true) {
    Token token = NextToken();
    tokens.push_back(token);
    if (token.type == TokenType::kEof || token.type == TokenType::kNewline ||
        token.type == TokenType::kError) {
      break;
    }
  }

  return tokens;
}

char Lexer::Current() const {
  if (IsAtEnd())
    return '\0';
  return source_[pos_];
}

char Lexer::Peek(int offset) const {
  if (pos_ + static_cast<size_t>(offset) >= source_.size())
    return '\0';
  return source_[pos_ + offset];
}

char Lexer::Advance() {
  char c = Current();
  if (!IsAtEnd()) {
    pos_++;
    if (c == '\n') {
      line_++;
      column_ = 1;
    } else {
      column_++;
    }
  }
  return c;
}

void Lexer::SkipWhitespace() {
  while (!IsAtEnd()) {
    char c = Current();
    if (c == ' ' || c == '\t' || c == '\r') {
      Advance();
    } else {
      break;
    }
  }
}

bool Lexer::IsAtEnd() const {
  return pos_ >= source_.size();
}

Token Lexer::ScanToken() {
  SkipWhitespace();

  if (IsAtEnd()) {
    return MakeToken(TokenType::kEof, "");
  }

  int start_line = line_;
  int start_column = column_;

  char c = Current();

  if (c == '\n') {
    Advance();
    Token token = MakeToken(TokenType::kNewline, "\n");
    token.line = start_line;
    token.column = start_column;
    return token;
  }

  // Comment: semicolon anywhere, or * at column 1
  if (c == ';' || (c == '*' && column_ == 1)) {
    return ScanComment();
  }

  if (c == '\'')
    return ScanString();

  // Numeric literals
  if (c == '$')
    return ScanHexNumber();
  if (c == '%')
    return ScanBinaryNumber();
  if (c == '@')
    return ScanOctalNumber();
  if (IsDigit(c))
    return ScanDecimalNumber();

  // Identifiers, opcodes, directives, registers, labels
  if (IsIdentifierStart(c))
    return ScanIdentifier();

  // Single- and double-character punctuation / operators
  Token token;
  token.line = start_line;
  token.column = start_column;

  switch (c) {
    case ',':
      Advance();
      return MakeToken(TokenType::kComma, ",");
    case '(':
      Advance();
      return MakeToken(TokenType::kLParen, "(");
    case ')':
      Advance();
      return MakeToken(TokenType::kRParen, ")");
    case '#':
      Advance();
      return MakeToken(TokenType::kHash, "#");
    case '.':
      Advance();
      return MakeToken(TokenType::kDot, ".");
    case '+':
      Advance();
      return MakeToken(TokenType::kPlus, "+");
    case '-':
      Advance();
      return MakeToken(TokenType::kMinus, "-");
    case '*':
      // * as current-location counter in expressions
      Advance();
      return MakeToken(TokenType::kOperator, "*");
    case '/':
      Advance();
      return MakeToken(TokenType::kOperator, "/");
    case '&':
      Advance();
      return MakeToken(TokenType::kOperator, "&");
    case '|':
    case '!':  // EASy68K uses ! as an OR operator
      Advance();
      return MakeToken(TokenType::kOperator, "|");
    case '^':
      Advance();
      return MakeToken(TokenType::kOperator, "^");
    case '~':
      Advance();
      return MakeToken(TokenType::kOperator, "~");
    case '\\':  // Modulo in EASy68K
      Advance();
      return MakeToken(TokenType::kOperator, "\\");
    case '<':
      Advance();
      if (Current() == '<') {
        Advance();
        return MakeToken(TokenType::kOperator, "<<");
      }
      return MakeError("Expected << for left shift");
    case '>':
      Advance();
      if (Current() == '>') {
        Advance();
        return MakeToken(TokenType::kOperator, ">>");
      }
      return MakeError("Expected >> for right shift");
    default:
      Advance();
      return MakeError(std::string("Unexpected character: ") + c);
  }
}

Token Lexer::ScanIdentifier() {
  int start_line = line_;
  int start_column = column_;
  size_t start = pos_;

  while (IsIdentifierChar(Current())) {
    Advance();
  }

  std::string text = source_.substr(start, pos_ - start);
  std::string upper_text = text;
  std::transform(upper_text.begin(), upper_text.end(), upper_text.begin(),
                 [](unsigned char c) { return std::toupper(c); });

  Token token;
  token.line = start_line;
  token.column = start_column;
  token.text = text;

  // Label: identifier immediately followed by colon
  if (Current() == ':') {
    Advance();
    token.type = TokenType::kLabel;
    return token;
  }

  // Optional size specifier (.B / .W / .L / .S)
  SizeSpec size = SizeSpec::kNone;
  if (Current() == '.') {
    size_t saved_pos = pos_;
    int saved_line = line_;
    int saved_column = column_;
    Advance();
    char size_char = ToUpper(Current());
    if (size_char == 'B' || size_char == 'W' || size_char == 'L' || size_char == 'S') {
      Advance();
      switch (size_char) {
        case 'B':
          size = SizeSpec::kByte;
          break;
        case 'W':
          size = SizeSpec::kWord;
          break;
        case 'L':
          size = SizeSpec::kLong;
          break;
        case 'S':
          size = SizeSpec::kShort;
          break;
      }
    } else {
      pos_ = saved_pos;
      line_ = saved_line;
      column_ = saved_column;
    }
  }

  // Register?
  RegisterType reg_type;
  int reg_num;
  if (IsRegister(upper_text, reg_type, reg_num)) {
    token.type = TokenType::kRegister;
    token.reg_type = reg_type;
    token.reg_num = reg_num;
    token.size = size;
    return token;
  }

  // Opcode?
  if (IsOpcode(upper_text)) {
    token.type = TokenType::kOpcode;
    token.size = size;
    return token;
  }

  // Directive?
  if (IsDirective(upper_text)) {
    token.type = TokenType::kDirective;
    token.size = size;
    return token;
  }

  // General symbol / identifier
  token.type = TokenType::kSymbol;
  token.size = size;
  return token;
}

Token Lexer::ScanHexNumber() {
  int start_line = line_;
  int start_column = column_;
  size_t start = pos_;

  Advance();  // skip $

  if (!IsHexDigit(Current())) {
    return MakeError("Expected hexadecimal digit after $");
  }

  int32_t value = 0;
  while (IsHexDigit(Current())) {
    char d = ToUpper(Current());
    int digit = (d >= 'A') ? (d - 'A' + 10) : (d - '0');
    value = (value << 4) | digit;
    Advance();
  }

  Token token;
  token.type = TokenType::kNumber;
  token.text = source_.substr(start, pos_ - start);
  token.int_value = value;
  token.line = start_line;
  token.column = start_column;
  return token;
}

Token Lexer::ScanBinaryNumber() {
  int start_line = line_;
  int start_column = column_;
  size_t start = pos_;

  Advance();  // skip %

  if (Current() != '0' && Current() != '1') {
    return MakeError("Expected binary digit after %");
  }

  int32_t value = 0;
  while (Current() == '0' || Current() == '1') {
    value = (value << 1) | (Current() - '0');
    Advance();
  }

  Token token;
  token.type = TokenType::kNumber;
  token.text = source_.substr(start, pos_ - start);
  token.int_value = value;
  token.line = start_line;
  token.column = start_column;
  return token;
}

Token Lexer::ScanOctalNumber() {
  int start_line = line_;
  int start_column = column_;
  size_t start = pos_;

  Advance();  // skip @

  if (Current() < '0' || Current() > '7') {
    return MakeError("Expected octal digit after @");
  }

  int32_t value = 0;
  while (Current() >= '0' && Current() <= '7') {
    value = (value << 3) | (Current() - '0');
    Advance();
  }

  Token token;
  token.type = TokenType::kNumber;
  token.text = source_.substr(start, pos_ - start);
  token.int_value = value;
  token.line = start_line;
  token.column = start_column;
  return token;
}

Token Lexer::ScanDecimalNumber() {
  int start_line = line_;
  int start_column = column_;
  size_t start = pos_;

  int32_t value = 0;
  while (IsDigit(Current())) {
    value = value * 10 + (Current() - '0');
    Advance();
  }

  Token token;
  token.type = TokenType::kNumber;
  token.text = source_.substr(start, pos_ - start);
  token.int_value = value;
  token.line = start_line;
  token.column = start_column;
  return token;
}

Token Lexer::ScanString() {
  int start_line = line_;
  int start_column = column_;

  Advance();  // skip opening '

  std::string value;
  while (!IsAtEnd() && Current() != '\n') {
    if (Current() == '\'') {
      if (Peek() == '\'') {
        // Escaped single-quote ''
        value += '\'';
        Advance();
        Advance();
      } else {
        Advance();  // skip closing '
        break;
      }
    } else {
      value += Current();
      Advance();
    }
  }

  Token token;
  token.type = TokenType::kString;
  token.text = value;
  // Pack up to 4 bytes into int_value (big-endian)
  token.int_value = 0;
  for (size_t i = 0; i < value.size() && i < 4; ++i) {
    token.int_value = (token.int_value << 8) | static_cast<unsigned char>(value[i]);
  }
  if (value.size() == 3) {
    token.int_value <<= 8;
  }
  token.line = start_line;
  token.column = start_column;
  return token;
}

Token Lexer::ScanComment() {
  int start_line = line_;
  int start_column = column_;
  size_t start = pos_;

  while (!IsAtEnd() && Current() != '\n') {
    Advance();
  }

  Token token;
  token.type = TokenType::kComment;
  token.text = source_.substr(start, pos_ - start);
  token.line = start_line;
  token.column = start_column;
  return token;
}

bool Lexer::IsOpcode(const std::string& upper_name) const {
  return kOpcodes.count(upper_name) > 0;
}

bool Lexer::IsDirective(const std::string& upper_name) const {
  return kDirectives.count(upper_name) > 0;
}

bool Lexer::IsRegister(const std::string& upper_name, RegisterType& type, int& num) const {
  if (upper_name.size() == 2) {
    if (upper_name[0] == 'D' && upper_name[1] >= '0' && upper_name[1] <= '7') {
      type = RegisterType::kData;
      num = upper_name[1] - '0';
      return true;
    }
    if (upper_name[0] == 'A' && upper_name[1] >= '0' && upper_name[1] <= '7') {
      type = RegisterType::kAddress;
      num = upper_name[1] - '0';
      return true;
    }
    if (upper_name == "SP") {
      type = RegisterType::kSP;
      num = 7;
      return true;
    }
    if (upper_name == "PC") {
      type = RegisterType::kPC;
      num = -1;
      return true;
    }
    if (upper_name == "SR") {
      type = RegisterType::kSR;
      num = -1;
      return true;
    }
  } else if (upper_name.size() == 3) {
    if (upper_name == "CCR") {
      type = RegisterType::kCCR;
      num = -1;
      return true;
    }
    if (upper_name == "USP") {
      type = RegisterType::kUSP;
      num = -1;
      return true;
    }
    if (upper_name == "SFC") {
      type = RegisterType::kSFC;
      num = -1;
      return true;
    }
    if (upper_name == "DFC") {
      type = RegisterType::kDFC;
      num = -1;
      return true;
    }
    if (upper_name == "VBR") {
      type = RegisterType::kVBR;
      num = -1;
      return true;
    }
  }
  return false;
}

Token Lexer::MakeToken(TokenType type, const std::string& text) {
  Token token;
  token.type = type;
  token.text = text;
  token.line = line_;
  token.column = column_ - static_cast<int>(text.size());
  return token;
}

Token Lexer::MakeError(const std::string& message) {
  has_error_ = true;
  error_message_ = message;
  Token token;
  token.type = TokenType::kError;
  token.text = message;
  token.line = line_;
  token.column = column_;
  return token;
}

bool Lexer::IsAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool Lexer::IsDigit(char c) {
  return c >= '0' && c <= '9';
}

bool Lexer::IsHexDigit(char c) {
  return IsDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool Lexer::IsAlphaNum(char c) {
  return IsAlpha(c) || IsDigit(c);
}

bool Lexer::IsIdentifierStart(char c) {
  return IsAlpha(c) || c == '_';
}

bool Lexer::IsIdentifierChar(char c) {
  return IsAlphaNum(c) || c == '_' || c == '$';
}

char Lexer::ToUpper(char c) {
  if (c >= 'a' && c <= 'z')
    return static_cast<char>(c - 'a' + 'A');
  return c;
}

const char* TokenTypeToString(TokenType type) {
  switch (type) {
    case TokenType::kLabel:
      return "LABEL";
    case TokenType::kOpcode:
      return "OPCODE";
    case TokenType::kDirective:
      return "DIRECTIVE";
    case TokenType::kRegister:
      return "REGISTER";
    case TokenType::kNumber:
      return "NUMBER";
    case TokenType::kString:
      return "STRING";
    case TokenType::kSymbol:
      return "SYMBOL";
    case TokenType::kOperator:
      return "OPERATOR";
    case TokenType::kComma:
      return "COMMA";
    case TokenType::kLParen:
      return "LPAREN";
    case TokenType::kRParen:
      return "RPAREN";
    case TokenType::kPlus:
      return "PLUS";
    case TokenType::kMinus:
      return "MINUS";
    case TokenType::kHash:
      return "HASH";
    case TokenType::kDot:
      return "DOT";
    case TokenType::kNewline:
      return "NEWLINE";
    case TokenType::kComment:
      return "COMMENT";
    case TokenType::kEof:
      return "EOF";
    case TokenType::kError:
      return "ERROR";
  }
  return "UNKNOWN";
}

}  // namespace easym68k::asm_
