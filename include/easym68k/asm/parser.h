// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Parser for EASy68K 68000 Assembler

#ifndef EASYM68K_ASM_PARSER_H_
#define EASYM68K_ASM_PARSER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "easym68k/asm/expression.h"
#include "easym68k/asm/lexer.h"

namespace easym68k::asm_ {

// Forward declarations
class SymbolTable;

// Addressing mode codes (bit masks matching EASy68K asm.h conventions).
enum class AddressMode {
  kNone = 0x00000,
  kDnDirect = 0x00001,        // Dn
  kAnDirect = 0x00002,        // An
  kAnIndirect = 0x00004,      // (An)
  kAnIndirectPost = 0x00008,  // (An)+
  kAnIndirectPre = 0x00010,   // -(An)
  kAnIndirectDisp = 0x00020,  // d(An)
  kAnIndirectIdx = 0x00040,   // d(An,Xi)
  kAbsShort = 0x00080,        // xxx.W
  kAbsLong = 0x00100,         // xxx.L
  kPCDisp = 0x00200,          // d(PC)
  kPCIndex = 0x00400,         // d(PC,Xi)
  kImmediate = 0x00800,       // #imm
  kSRDirect = 0x01000,        // SR
  kCCRDirect = 0x02000,       // CCR
  kUSPDirect = 0x04000,       // USP
  kSFCDirect = 0x08000,       // SFC (68010)
  kDFCDirect = 0x10000,       // DFC (68010)
  kVBRDirect = 0x20000        // VBR (68010)
};

inline AddressMode operator|(AddressMode a, AddressMode b) {
  return static_cast<AddressMode>(static_cast<int>(a) | static_cast<int>(b));
}

inline AddressMode operator&(AddressMode a, AddressMode b) {
  return static_cast<AddressMode>(static_cast<int>(a) & static_cast<int>(b));
}

inline bool HasMode(AddressMode modes, AddressMode test) {
  return (static_cast<int>(modes) & static_cast<int>(test)) != 0;
}

// Parsed operand descriptor
struct Operand {
  AddressMode mode;
  int32_t data;          // Immediate value, displacement, or absolute address
  int32_t field;         // Bitfield instruction parameters
  int reg;               // Principal register (0-7)
  int index_reg;         // Index register (0-7 = D0-D7, 8-15 = A0-A7)
  SizeSpec index_size;   // Size of index register
  SizeSpec forced_size;  // Forced size for immediate/absolute
  bool is_back_ref;      // True if value is known at this point
  std::string symbol_name;

  Operand()
      : mode(AddressMode::kNone),
        data(0),
        field(0),
        reg(-1),
        index_reg(-1),
        index_size(SizeSpec::kWord),
        forced_size(SizeSpec::kNone),
        is_back_ref(true) {}
};

// Parsed source line
struct ParsedLine {
  std::string label;
  std::string opcode;  // Always uppercase
  SizeSpec size;
  std::vector<Operand> operands;
  std::string comment;
  int line_number;
  bool has_error;
  std::string error_message;

  ParsedLine() : size(SizeSpec::kNone), line_number(0), has_error(false) {}
};

class Parser {
 public:
  Parser();
  ~Parser();

  void SetSymbolTable(SymbolTable* symbols);
  void SetLocationCounter(uint32_t loc);
  void SetPass(int pass);

  ParsedLine ParseLine(const std::string& line, int line_number);
  bool ParseOperand(const std::string& operand_text, Operand& operand);

  bool HasError() const { return has_error_; }
  const std::string& GetErrorMessage() const { return error_message_; }

 private:
  Lexer lexer_;
  ExpressionEvaluator evaluator_;
  SymbolTable* symbols_;
  uint32_t location_counter_;
  int pass_;
  bool has_error_;
  std::string error_message_;

  std::vector<Token> tokens_;
  size_t token_pos_;

  Token Current() const;
  Token Peek(int offset = 1) const;
  Token Advance();
  bool Match(TokenType type);
  bool Check(TokenType type) const;
  bool IsAtEnd() const;

  bool ParseLabel(ParsedLine& line);
  bool ParseOpcodeOrDirective(ParsedLine& line);
  bool ParseOperands(ParsedLine& line);
  bool ParseSingleOperand(Operand& op);

  bool ParseRegisterDirect(Operand& op);
  bool ParseImmediate(Operand& op);
  bool ParseIndirect(Operand& op);
  bool ParseAbsoluteOrDisplacement(Operand& op);
  bool FinishIndexedMode(Operand& out, int32_t disp, bool back_ref, AddressMode base_mode,
                         AddressMode idx_mode);

  bool EvaluateExpression(int32_t& value, bool& is_back_ref);

  void Error(const std::string& message);
  static std::string ToUpper(const std::string& s);
};

int GetEAEncoding(const Operand& op);
int GetEAMode(const Operand& op);
int GetEAReg(const Operand& op);

}  // namespace easym68k::asm_

#endif  // EASYM68K_ASM_PARSER_H_
