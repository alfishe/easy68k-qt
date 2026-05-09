// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Expression Evaluator for EASy68K 68000 Assembler

#ifndef EASYM68K_ASM_EXPRESSION_H_
#define EASYM68K_ASM_EXPRESSION_H_

#include <cstdint>
#include <string>
#include <vector>

#include "easym68k/asm/lexer.h"

namespace easym68k::asm_ {

class SymbolTable;

struct ExpressionResult {
  int32_t value;
  bool is_valid;
  bool is_back_ref;  // True if all symbols are backward references
  std::string error_message;

  ExpressionResult() : value(0), is_valid(true), is_back_ref(true) {}
};

// Evaluates constant expressions from token streams or strings.
// Operator precedence matches EVAL.CPP:
//   +/- (1) < */\\ (2) < &|^ (3) < <<>> (4, highest)
class ExpressionEvaluator {
 public:
  ExpressionEvaluator();
  ~ExpressionEvaluator();

  void SetSymbolTable(SymbolTable* symbols);
  void SetLocationCounter(uint32_t loc);
  void SetPass(int pass);

  // Tokenize expr and evaluate the entire string as one expression.
  ExpressionResult Evaluate(const std::string& expr);

  // Evaluate starting at tokens[start]; sets end to the first unconsumed token.
  ExpressionResult Evaluate(const std::vector<Token>& tokens, size_t start, size_t& end);

 private:
  Lexer lexer_;
  SymbolTable* symbols_;
  uint32_t location_counter_;
  int pass_;

  std::vector<Token> tokens_;
  size_t pos_;

  Token Current() const;
  Token Advance();

  // Parse an expression with all binary operators at prec >= min_prec.
  // Also consumes the leading primary value.
  ExpressionResult ParseExprPrec(int min_prec);
  ExpressionResult ParsePrimary();

  static int BinOpPrec(const Token& tok);
  static ExpressionResult MakeError(const std::string& message);
};

}  // namespace easym68k::asm_

#endif  // EASYM68K_ASM_EXPRESSION_H_
