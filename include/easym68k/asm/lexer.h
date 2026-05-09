// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Lexer/Tokenizer for EASy68K 68000 Assembler

#ifndef EASYM68K_ASM_LEXER_H_
#define EASYM68K_ASM_LEXER_H_

#include <cstdint>
#include <string>
#include <vector>

namespace easym68k::asm_ {

enum class TokenType {
  kLabel,      // Identifier followed by colon
  kOpcode,     // Instruction mnemonic (MOVE, ADD, etc.)
  kDirective,  // Assembler directive (ORG, DC, DS, EQU, etc.)
  kRegister,   // Register (D0-D7, A0-A7, SP, PC, SR, CCR, USP)
  kNumber,     // Numeric literal (decimal, hex $, binary %, octal @)
  kString,     // String literal ('...')
  kSymbol,     // Symbol/identifier
  kOperator,   // Arithmetic/logical operators (+, -, *, /, &, |, ^, ~, <<, >>)
  kComma,      // ,
  kLParen,     // (
  kRParen,     // )
  kPlus,       // + (also operator, but distinct for addressing modes)
  kMinus,      // - (also operator, but distinct for addressing modes)
  kHash,       // # (immediate mode prefix)
  kDot,        // . (size specifier prefix)
  kNewline,    // End of line
  kComment,    // Comment (* at column 1, or ;)
  kEof,        // End of file
  kError       // Lexer error
};

enum class SizeSpec {
  kNone,
  kByte,  // .B
  kWord,  // .W
  kLong,  // .L
  kShort  // .S (for branches)
};

enum class RegisterType {
  kNone,
  kData,     // D0-D7
  kAddress,  // A0-A7
  kSP,       // Stack Pointer (A7 alias)
  kPC,       // Program Counter
  kSR,       // Status Register
  kCCR,      // Condition Code Register
  kUSP,      // User Stack Pointer
  kSFC,      // Source Function Code (68010)
  kDFC,      // Destination Function Code (68010)
  kVBR       // Vector Base Register (68010)
};

struct Token {
  TokenType type;
  std::string text;       // Original text of the token
  int32_t int_value;      // Numeric value (for kNumber)
  SizeSpec size;          // Size specifier (for kOpcode/kDirective)
  RegisterType reg_type;  // Register type (for kRegister)
  int reg_num;            // Register number 0-7 (for kRegister)
  int line;               // Line number (1-based)
  int column;             // Column number (1-based)

  Token()
      : type(TokenType::kEof),
        int_value(0),
        size(SizeSpec::kNone),
        reg_type(RegisterType::kNone),
        reg_num(-1),
        line(0),
        column(0) {}
};

class Lexer {
 public:
  Lexer();
  ~Lexer();

  void SetSource(const std::string& source);

  Token NextToken();
  Token PeekToken();

  int GetLine() const { return line_; }
  int GetColumn() const { return column_; }

  bool HasError() const { return has_error_; }
  const std::string& GetErrorMessage() const { return error_message_; }

  // Tokenize an entire line; resets source to the given line.
  std::vector<Token> TokenizeLine(const std::string& line, int line_number = 1);

 private:
  std::string source_;
  size_t pos_;
  int line_;
  int column_;
  bool has_error_;
  std::string error_message_;
  Token peeked_token_;
  bool has_peeked_;

  char Current() const;
  char Peek(int offset = 1) const;
  char Advance();
  void SkipWhitespace();
  bool IsAtEnd() const;

  Token ScanToken();
  Token ScanIdentifier();
  Token ScanHexNumber();
  Token ScanBinaryNumber();
  Token ScanOctalNumber();
  Token ScanDecimalNumber();
  Token ScanString();
  Token ScanComment();

  bool IsOpcode(const std::string& upper_name) const;
  bool IsDirective(const std::string& upper_name) const;
  bool IsRegister(const std::string& upper_name, RegisterType& type, int& num) const;

  Token MakeToken(TokenType type, const std::string& text);
  Token MakeError(const std::string& message);

  static bool IsAlpha(char c);
  static bool IsDigit(char c);
  static bool IsHexDigit(char c);
  static bool IsAlphaNum(char c);
  static bool IsIdentifierStart(char c);
  static bool IsIdentifierChar(char c);
  static char ToUpper(char c);
};

const char* TokenTypeToString(TokenType type);

}  // namespace easym68k::asm_

#endif  // EASYM68K_ASM_LEXER_H_
