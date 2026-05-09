// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/asm/parser.h"

#include <algorithm>
#include <cctype>

#include "easym68k/asm/symbol_table.h"

namespace easym68k::asm_ {

Parser::Parser()
    : symbols_(nullptr), location_counter_(0), pass_(1), has_error_(false), token_pos_(0) {}

Parser::~Parser() = default;

void Parser::SetSymbolTable(SymbolTable* symbols) {
  symbols_ = symbols;
  evaluator_.SetSymbolTable(symbols);
}
void Parser::SetLocationCounter(uint32_t loc) {
  location_counter_ = loc;
  evaluator_.SetLocationCounter(loc);
}
void Parser::SetPass(int pass) {
  pass_ = pass;
  evaluator_.SetPass(pass);
}

ParsedLine Parser::ParseLine(const std::string& line, int line_number) {
  ParsedLine result;
  result.line_number = line_number;
  has_error_ = false;
  error_message_.clear();

  tokens_ = lexer_.TokenizeLine(line, line_number);
  token_pos_ = 0;

  if (tokens_.empty() || tokens_[0].type == TokenType::kEof ||
      tokens_[0].type == TokenType::kNewline || tokens_[0].type == TokenType::kComment) {
    if (!tokens_.empty() && tokens_[0].type == TokenType::kComment) {
      result.comment = tokens_[0].text;
    }
    return result;
  }

  ParseLabel(result);

  if (!IsAtEnd() && !Check(TokenType::kComment) && !Check(TokenType::kNewline)) {
    ParseOpcodeOrDirective(result);
  }

  if (!has_error_ && !result.opcode.empty() && !IsAtEnd() && !Check(TokenType::kComment) &&
      !Check(TokenType::kNewline)) {
    ParseOperands(result);
  }

  if (Check(TokenType::kComment)) {
    result.comment = Current().text;
  }

  if (has_error_) {
    result.has_error = true;
    result.error_message = error_message_;
  }

  return result;
}

bool Parser::ParseOperand(const std::string& operand_text, Operand& operand) {
  tokens_ = lexer_.TokenizeLine(operand_text, 1);
  token_pos_ = 0;
  has_error_ = false;
  return ParseSingleOperand(operand);
}

Token Parser::Current() const {
  if (token_pos_ >= tokens_.size()) {
    Token eof;
    eof.type = TokenType::kEof;
    return eof;
  }
  return tokens_[token_pos_];
}

Token Parser::Peek(int offset) const {
  size_t pos = token_pos_ + static_cast<size_t>(offset);
  if (pos >= tokens_.size()) {
    Token eof;
    eof.type = TokenType::kEof;
    return eof;
  }
  return tokens_[pos];
}

Token Parser::Advance() {
  Token token = Current();
  if (token_pos_ < tokens_.size())
    token_pos_++;
  return token;
}

bool Parser::Match(TokenType type) {
  if (Check(type)) {
    Advance();
    return true;
  }
  return false;
}

bool Parser::Check(TokenType type) const {
  return Current().type == type;
}

bool Parser::IsAtEnd() const {
  TokenType t = Current().type;
  return t == TokenType::kEof || t == TokenType::kNewline || t == TokenType::kComment;
}

bool Parser::ParseLabel(ParsedLine& line) {
  if (Check(TokenType::kLabel)) {
    line.label = Current().text;
    Advance();
    return true;
  }

  // Label without colon at column 1 (symbol followed by opcode/directive)
  if (Check(TokenType::kSymbol)) {
    Token next = Peek();
    if (next.type == TokenType::kOpcode || next.type == TokenType::kDirective) {
      line.label = Current().text;
      Advance();
      return true;
    }
  }

  return false;
}

bool Parser::ParseOpcodeOrDirective(ParsedLine& line) {
  Token token = Current();

  if (token.type == TokenType::kOpcode || token.type == TokenType::kDirective) {
    line.opcode = ToUpper(token.text);
    line.size = token.size;
    Advance();
    return true;
  }

  // Symbol that wasn't classified as opcode/directive (e.g., unknown mnemonic)
  if (token.type == TokenType::kSymbol) {
    line.opcode = ToUpper(token.text);
    line.size = token.size;
    Advance();
    return true;
  }

  return false;
}

bool Parser::ParseOperands(ParsedLine& line) {
  Operand op;
  if (!ParseSingleOperand(op))
    return false;
  line.operands.push_back(op);

  while (Match(TokenType::kComma)) {
    Operand next_op;
    if (!ParseSingleOperand(next_op))
      return false;
    line.operands.push_back(next_op);
  }

  return true;
}

bool Parser::ParseSingleOperand(Operand& op) {
  if (Check(TokenType::kHash))
    return ParseImmediate(op);

  // Predecrement -(An)
  if (Check(TokenType::kMinus)) {
    Token next = Peek();
    if (next.type == TokenType::kLParen) {
      Advance();  // consume -
      Advance();  // consume (
      if (!Check(TokenType::kRegister)) {
        Error("Expected address register after -(");
        return false;
      }
      Token reg = Advance();
      if (reg.reg_type != RegisterType::kAddress && reg.reg_type != RegisterType::kSP) {
        Error("Predecrement requires address register");
        return false;
      }
      if (!Match(TokenType::kRParen)) {
        Error("Expected ) after register");
        return false;
      }
      op.mode = AddressMode::kAnIndirectPre;
      op.reg = reg.reg_num;
      return true;
    }
  }

  if (Check(TokenType::kRegister))
    return ParseRegisterDirect(op);
  if (Check(TokenType::kLParen))
    return ParseIndirect(op);
  return ParseAbsoluteOrDisplacement(op);
}

bool Parser::ParseRegisterDirect(Operand& op) {
  Token token = Advance();
  switch (token.reg_type) {
    case RegisterType::kData:
      op.mode = AddressMode::kDnDirect;
      op.reg = token.reg_num;
      break;
    case RegisterType::kAddress:
    case RegisterType::kSP:
      op.mode = AddressMode::kAnDirect;
      op.reg = token.reg_num;
      break;
    case RegisterType::kSR:
      op.mode = AddressMode::kSRDirect;
      break;
    case RegisterType::kCCR:
      op.mode = AddressMode::kCCRDirect;
      break;
    case RegisterType::kUSP:
      op.mode = AddressMode::kUSPDirect;
      break;
    case RegisterType::kSFC:
      op.mode = AddressMode::kSFCDirect;
      break;
    case RegisterType::kDFC:
      op.mode = AddressMode::kDFCDirect;
      break;
    case RegisterType::kVBR:
      op.mode = AddressMode::kVBRDirect;
      break;
    case RegisterType::kPC:
      op.mode = AddressMode::kPCDisp;
      op.data = 0;
      break;
    default:
      Error("Invalid register");
      return false;
  }
  return true;
}

bool Parser::ParseImmediate(Operand& op) {
  Advance();  // consume #
  op.mode = AddressMode::kImmediate;

  if (!EvaluateExpression(op.data, op.is_back_ref))
    return false;

  // Optional forced size: #expr.L  (dot already consumed by lexer as kDot)
  if (Check(TokenType::kDot)) {
    Advance();
    Token size_token = Current();
    if (size_token.type == TokenType::kSymbol || size_token.type == TokenType::kOpcode) {
      std::string s = ToUpper(size_token.text);
      if (s == "L") {
        op.forced_size = SizeSpec::kLong;
        Advance();
      } else if (s == "W") {
        op.forced_size = SizeSpec::kWord;
        Advance();
      } else if (s == "B") {
        op.forced_size = SizeSpec::kByte;
        Advance();
      }
    }
  }

  return true;
}

bool Parser::ParseIndirect(Operand& op) {
  Advance();  // consume (

  Token token = Current();

  if (token.type == TokenType::kRegister) {
    if (token.reg_type == RegisterType::kAddress || token.reg_type == RegisterType::kSP) {
      int reg = token.reg_num;
      Advance();

      // (An,Xi) — index without displacement
      if (Check(TokenType::kComma)) {
        Advance();
        if (!Check(TokenType::kRegister)) {
          Error("Expected index register");
          return false;
        }
        Token idx = Advance();
        op.mode = AddressMode::kAnIndirectIdx;
        op.reg = reg;
        op.data = 0;
        op.is_back_ref = true;
        if (idx.reg_type == RegisterType::kData) {
          op.index_reg = idx.reg_num;
        } else if (idx.reg_type == RegisterType::kAddress || idx.reg_type == RegisterType::kSP) {
          op.index_reg = idx.reg_num + 8;
        } else {
          Error("Invalid index register");
          return false;
        }
        op.index_size = (idx.size != SizeSpec::kNone) ? idx.size : SizeSpec::kWord;
        if (!Match(TokenType::kRParen)) {
          Error("Expected )");
          return false;
        }
        return true;
      }

      if (!Match(TokenType::kRParen)) {
        Error("Expected )");
        return false;
      }

      // (An)+
      if (Check(TokenType::kPlus)) {
        Advance();
        op.mode = AddressMode::kAnIndirectPost;
        op.reg = reg;
        return true;
      }

      op.mode = AddressMode::kAnIndirect;
      op.reg = reg;
      return true;

    } else if (token.reg_type == RegisterType::kPC) {
      Advance();
      if (Check(TokenType::kComma)) {
        Advance();
        if (!Check(TokenType::kRegister)) {
          Error("Expected index register");
          return false;
        }
        Token idx = Advance();
        op.mode = AddressMode::kPCIndex;
        op.data = 0;
        op.is_back_ref = true;
        if (idx.reg_type == RegisterType::kData) {
          op.index_reg = idx.reg_num;
        } else if (idx.reg_type == RegisterType::kAddress || idx.reg_type == RegisterType::kSP) {
          op.index_reg = idx.reg_num + 8;
        }
        op.index_size = (idx.size != SizeSpec::kNone) ? idx.size : SizeSpec::kWord;
        if (!Match(TokenType::kRParen)) {
          Error("Expected )");
          return false;
        }
        return true;
      }
      if (!Match(TokenType::kRParen)) {
        Error("Expected )");
        return false;
      }
      op.mode = AddressMode::kPCDisp;
      op.data = 0;
      op.is_back_ref = true;
      return true;
    }
  }

  // Displacement: (d,An), (d,An,Xi), (d,PC), (d,PC,Xi)
  int32_t disp = 0;
  bool back_ref = true;
  if (!EvaluateExpression(disp, back_ref))
    return false;

  if (!Match(TokenType::kComma)) {
    Error("Expected comma after displacement");
    return false;
  }

  Token reg_token = Current();
  if (reg_token.type != TokenType::kRegister) {
    Error("Expected register after displacement");
    return false;
  }
  Advance();

  if (reg_token.reg_type == RegisterType::kPC) {
    return FinishIndexedMode(op, disp, back_ref, AddressMode::kPCDisp, AddressMode::kPCIndex);
  } else if (reg_token.reg_type == RegisterType::kAddress ||
             reg_token.reg_type == RegisterType::kSP) {
    op.reg = reg_token.reg_num;
    return FinishIndexedMode(op, disp, back_ref, AddressMode::kAnIndirectDisp,
                             AddressMode::kAnIndirectIdx);
  }

  Error("Expected address register or PC");
  return false;
}

bool Parser::ParseAbsoluteOrDisplacement(Operand& op) {
  int32_t value = 0;
  bool back_ref = true;

  if (!EvaluateExpression(value, back_ref))
    return false;

  // d(An) or d(PC) or d(An,Xi) or d(PC,Xi)
  if (Check(TokenType::kLParen)) {
    Advance();
    Token reg_token = Current();
    if (reg_token.type != TokenType::kRegister) {
      Error("Expected register after (");
      return false;
    }
    Advance();

    if (reg_token.reg_type == RegisterType::kPC) {
      return FinishIndexedMode(op, value, back_ref, AddressMode::kPCDisp, AddressMode::kPCIndex);
    } else if (reg_token.reg_type == RegisterType::kAddress ||
               reg_token.reg_type == RegisterType::kSP) {
      op.reg = reg_token.reg_num;
      return FinishIndexedMode(op, value, back_ref, AddressMode::kAnIndirectDisp,
                               AddressMode::kAnIndirectIdx);
    }

    Error("Expected address register or PC");
    return false;
  }

  // Check for forced size suffix: xxx.W or xxx.L
  SizeSpec forced = SizeSpec::kNone;
  if (Check(TokenType::kDot)) {
    Advance();
    Token size_tok = Current();
    if (size_tok.type == TokenType::kSymbol || size_tok.type == TokenType::kOpcode) {
      std::string s = ToUpper(size_tok.text);
      if (s == "W") {
        forced = SizeSpec::kWord;
        Advance();
      } else if (s == "L") {
        forced = SizeSpec::kLong;
        Advance();
      }
    }
  }

  op.data = value;
  op.is_back_ref = back_ref;
  op.forced_size = forced;

  if (forced == SizeSpec::kLong) {
    op.mode = AddressMode::kAbsLong;
  } else if (forced == SizeSpec::kWord) {
    op.mode = AddressMode::kAbsShort;
  } else if (!back_ref || value > 32767 || value < -32768) {
    op.mode = AddressMode::kAbsLong;
  } else {
    op.mode = AddressMode::kAbsShort;
  }

  return true;
}

bool Parser::FinishIndexedMode(Operand& out, int32_t disp, bool back_ref, AddressMode base_mode,
                               AddressMode idx_mode) {
  if (Check(TokenType::kComma)) {
    Advance();
    if (!Check(TokenType::kRegister)) {
      Error("Expected index register");
      return false;
    }
    Token idx = Advance();
    out.mode = idx_mode;
    out.data = disp;
    out.is_back_ref = back_ref;
    if (idx.reg_type == RegisterType::kData) {
      out.index_reg = idx.reg_num;
    } else if (idx.reg_type == RegisterType::kAddress || idx.reg_type == RegisterType::kSP) {
      out.index_reg = idx.reg_num + 8;
    }
    out.index_size = (idx.size != SizeSpec::kNone) ? idx.size : SizeSpec::kWord;
  } else {
    out.mode = base_mode;
    out.data = disp;
    out.is_back_ref = back_ref;
  }
  if (!Match(TokenType::kRParen)) {
    Error("Expected )");
    return false;
  }
  return true;
}

bool Parser::EvaluateExpression(int32_t& value, bool& is_back_ref) {
  ExpressionResult result = evaluator_.Evaluate(tokens_, token_pos_, token_pos_);
  if (!result.is_valid) {
    Error(result.error_message);
    return false;
  }
  value = result.value;
  is_back_ref = result.is_back_ref;
  return true;
}

void Parser::Error(const std::string& message) {
  has_error_ = true;
  error_message_ = message;
}

std::string Parser::ToUpper(const std::string& s) {
  std::string result = s;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return result;
}

int GetEAEncoding(const Operand& op) {
  return (GetEAMode(op) << 3) | GetEAReg(op);
}

int GetEAMode(const Operand& op) {
  switch (op.mode) {
    case AddressMode::kDnDirect:
      return 0;
    case AddressMode::kAnDirect:
      return 1;
    case AddressMode::kAnIndirect:
      return 2;
    case AddressMode::kAnIndirectPost:
      return 3;
    case AddressMode::kAnIndirectPre:
      return 4;
    case AddressMode::kAnIndirectDisp:
      return 5;
    case AddressMode::kAnIndirectIdx:
      return 6;
    case AddressMode::kAbsShort:
      return 7;
    case AddressMode::kAbsLong:
      return 7;
    case AddressMode::kPCDisp:
      return 7;
    case AddressMode::kPCIndex:
      return 7;
    case AddressMode::kImmediate:
      return 7;
    default:
      return 0;
  }
}

int GetEAReg(const Operand& op) {
  switch (op.mode) {
    case AddressMode::kDnDirect:
    case AddressMode::kAnDirect:
    case AddressMode::kAnIndirect:
    case AddressMode::kAnIndirectPost:
    case AddressMode::kAnIndirectPre:
    case AddressMode::kAnIndirectDisp:
    case AddressMode::kAnIndirectIdx:
      return op.reg;
    case AddressMode::kAbsShort:
      return 0;
    case AddressMode::kAbsLong:
      return 1;
    case AddressMode::kPCDisp:
      return 2;
    case AddressMode::kPCIndex:
      return 3;
    case AddressMode::kImmediate:
      return 4;
    default:
      return 0;
  }
}

}  // namespace easym68k::asm_
