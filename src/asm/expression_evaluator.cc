// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/asm/expression.h"
#include "easym68k/asm/symbol_table.h"

namespace easym68k::asm_ {

ExpressionEvaluator::ExpressionEvaluator()
    : symbols_(nullptr), location_counter_(0), pass_(1), pos_(0) {}

ExpressionEvaluator::~ExpressionEvaluator() = default;

void ExpressionEvaluator::SetSymbolTable(SymbolTable* symbols) {
  symbols_ = symbols;
}

void ExpressionEvaluator::SetLocationCounter(uint32_t loc) {
  location_counter_ = loc;
}

void ExpressionEvaluator::SetPass(int pass) {
  pass_ = pass;
}

ExpressionResult ExpressionEvaluator::Evaluate(const std::string& expr) {
  // Prefix with a space so '*' at the start isn't misread as a column-1 comment.
  tokens_ = lexer_.TokenizeLine(" " + expr, 1);
  pos_ = 0;
  return ParseExprPrec(1);
}

ExpressionResult ExpressionEvaluator::Evaluate(const std::vector<Token>& tokens, size_t start,
                                               size_t& end) {
  tokens_ = tokens;
  pos_ = start;
  ExpressionResult result = ParseExprPrec(1);
  end = pos_;
  return result;
}

Token ExpressionEvaluator::Current() const {
  if (pos_ >= tokens_.size()) {
    Token eof;
    eof.type = TokenType::kEof;
    return eof;
  }
  return tokens_[pos_];
}

Token ExpressionEvaluator::Advance() {
  Token token = Current();
  if (pos_ < tokens_.size())
    pos_++;
  return token;
}

// Returns 1-4 for binary operators matching EVAL.CPP precedence():
//   +/- → 1, */\\ → 2, &|^ → 3, <<>> → 4. Returns 0 for non-operators.
int ExpressionEvaluator::BinOpPrec(const Token& tok) {
  if (tok.type == TokenType::kPlus || tok.type == TokenType::kMinus)
    return 1;
  if (tok.type == TokenType::kOperator) {
    const std::string& s = tok.text;
    if (s == "*" || s == "/" || s == "\\")
      return 2;
    if (s == "&" || s == "|" || s == "^")
      return 3;
    if (s == "<<" || s == ">>")
      return 4;
  }
  return 0;
}

ExpressionResult ExpressionEvaluator::MakeError(const std::string& message) {
  ExpressionResult result;
  result.is_valid = false;
  result.is_back_ref = false;
  result.error_message = message;
  return result;
}

ExpressionResult ExpressionEvaluator::ParsePrimary() {
  // Unary minus and bitwise complement may chain (e.g. --5, ~~$FF).
  if (Current().type == TokenType::kMinus) {
    Advance();
    ExpressionResult inner = ParsePrimary();
    if (!inner.is_valid)
      return inner;
    inner.value = -inner.value;
    return inner;
  }
  if (Current().type == TokenType::kOperator && Current().text == "~") {
    Advance();
    ExpressionResult inner = ParsePrimary();
    if (!inner.is_valid)
      return inner;
    inner.value = ~inner.value;
    return inner;
  }

  Token token = Current();

  if (token.type == TokenType::kNumber) {
    Advance();
    ExpressionResult result;
    result.value = token.int_value;
    return result;
  }

  if (token.type == TokenType::kString) {
    Advance();
    ExpressionResult result;
    result.value = token.int_value;
    return result;
  }

  // * = current location counter (EVAL.CPP: "* is current address")
  if (token.type == TokenType::kOperator && token.text == "*") {
    Advance();
    ExpressionResult result;
    result.value = static_cast<int32_t>(location_counter_);
    return result;
  }

  if (token.type == TokenType::kSymbol) {
    Advance();
    ExpressionResult result;
    if (symbols_) {
      SymbolInfo info;
      if (symbols_->Lookup(token.text, info)) {
        if (HasFlag(info.flags, SymbolFlags::kRegisterList))
          return MakeError("Register list symbol not valid in expression: " + token.text);
        result.value = info.value;
        result.is_back_ref = info.is_defined;
      } else {
        result.is_back_ref = false;
        if (pass_ == 2) {
          // Structured-control jump labels; see EVAL.CPP lines 338-348.
          if (token.text == ".0")
            return MakeError("ENDI expected");
          if (token.text == ".1")
            return MakeError("ENDW expected");
          if (token.text == ".2")
            return MakeError("ENDF expected");
          if (token.text == ".3")
            return MakeError("REPEAT expected");
          return MakeError("Undefined symbol: " + token.text);
        }
        result.value = 0;
      }
    } else {
      result.is_back_ref = false;
      result.value = 0;
    }
    return result;
  }

  if (token.type == TokenType::kLParen) {
    Advance();
    ExpressionResult inner = ParseExprPrec(1);
    if (!inner.is_valid)
      return inner;
    if (Current().type != TokenType::kRParen)
      return MakeError("Expected )");
    Advance();
    return inner;
  }

  TokenType t = token.type;
  if (t == TokenType::kEof || t == TokenType::kNewline || t == TokenType::kComment)
    return MakeError("Unexpected end of expression");

  return MakeError("Unexpected token in expression: " + token.text);
}

ExpressionResult ExpressionEvaluator::ParseExprPrec(int min_prec) {
  ExpressionResult lhs = ParsePrimary();
  if (!lhs.is_valid)
    return lhs;

  while (true) {
    Token op_tok = Current();
    int prec = BinOpPrec(op_tok);
    if (prec == 0 || prec < min_prec)
      break;
    Advance();

    ExpressionResult rhs = ParseExprPrec(prec + 1);
    if (!rhs.is_valid)
      return rhs;

    lhs.is_back_ref = lhs.is_back_ref && rhs.is_back_ref;

    std::string op = op_tok.text;
    if (op_tok.type == TokenType::kPlus) {
      lhs.value += rhs.value;
    } else if (op_tok.type == TokenType::kMinus) {
      lhs.value -= rhs.value;
    } else if (op == "*") {
      lhs.value *= rhs.value;
    } else if (op == "/") {
      if (rhs.value == 0)
        return MakeError("Division by zero");
      lhs.value /= rhs.value;
    } else if (op == "\\") {
      if (rhs.value == 0)
        return MakeError("Modulo by zero");
      lhs.value %= rhs.value;
    } else if (op == "&") {
      lhs.value &= rhs.value;
    } else if (op == "|") {
      lhs.value |= rhs.value;
    } else if (op == "^") {
      lhs.value ^= rhs.value;
    } else if (op == "<<") {
      lhs.value <<= rhs.value;
    } else if (op == ">>") {
      lhs.value = static_cast<int32_t>(static_cast<uint32_t>(lhs.value) >> rhs.value);
    }
  }
  return lhs;
}

}  // namespace easym68k::asm_
