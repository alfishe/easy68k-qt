// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/asm/expression.h"

#include <gtest/gtest.h>

#include "easym68k/asm/symbol_table.h"

namespace easym68k::asm_ {
namespace {

// ---------------------------------------------------------------------------
// Literals
// ---------------------------------------------------------------------------

TEST(ExpressionTest, DecimalLiteral) {
  ExpressionEvaluator e;
  auto r = e.Evaluate("42");
  EXPECT_TRUE(r.is_valid);
  EXPECT_EQ(r.value, 42);
}

TEST(ExpressionTest, HexLiteral) {
  ExpressionEvaluator e;
  auto r = e.Evaluate("$FF");
  EXPECT_TRUE(r.is_valid);
  EXPECT_EQ(r.value, 0xFF);
}

TEST(ExpressionTest, BinaryLiteral) {
  ExpressionEvaluator e;
  auto r = e.Evaluate("%1010");
  EXPECT_TRUE(r.is_valid);
  EXPECT_EQ(r.value, 10);
}

TEST(ExpressionTest, OctalLiteral) {
  ExpressionEvaluator e;
  auto r = e.Evaluate("@17");
  EXPECT_TRUE(r.is_valid);
  EXPECT_EQ(r.value, 15);
}

TEST(ExpressionTest, StringOneChar) {
  ExpressionEvaluator e;
  auto r = e.Evaluate("'A'");
  EXPECT_TRUE(r.is_valid);
  EXPECT_EQ(r.value, 0x41);
}

TEST(ExpressionTest, StringTwoChars) {
  ExpressionEvaluator e;
  auto r = e.Evaluate("'AB'");
  EXPECT_TRUE(r.is_valid);
  EXPECT_EQ(r.value, 0x4142);
}

TEST(ExpressionTest, StringThreeChars) {
  // 3-char string: pack then shift left 8 (EVAL.CPP 'ABC' = 0x41424300)
  ExpressionEvaluator e;
  auto r = e.Evaluate("'ABC'");
  EXPECT_TRUE(r.is_valid);
  EXPECT_EQ(r.value, static_cast<int32_t>(0x41424300));
}

TEST(ExpressionTest, StringFourChars) {
  ExpressionEvaluator e;
  auto r = e.Evaluate("'ABCD'");
  EXPECT_TRUE(r.is_valid);
  EXPECT_EQ(r.value, static_cast<int32_t>(0x41424344));
}

// ---------------------------------------------------------------------------
// Arithmetic operators
// ---------------------------------------------------------------------------

TEST(ExpressionTest, Addition) {
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("1+2").value, 3);
}

TEST(ExpressionTest, Subtraction) {
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("5-3").value, 2);
}

TEST(ExpressionTest, Multiplication) {
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("3*4").value, 12);
}

TEST(ExpressionTest, Division) {
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("8/2").value, 4);
}

TEST(ExpressionTest, Modulo) {
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("10\\3").value, 1);
}

// ---------------------------------------------------------------------------
// Bitwise operators
// ---------------------------------------------------------------------------

TEST(ExpressionTest, BitwiseAnd) {
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("$FF&$0F").value, 0x0F);
}

TEST(ExpressionTest, BitwiseOr) {
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("$F0|$0F").value, 0xFF);
}

TEST(ExpressionTest, BitwiseXor) {
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("$FF^$0F").value, 0xF0);
}

TEST(ExpressionTest, BitwiseOrAlias) {
  // EASy68K uses ! as an alias for |; lexer normalizes ! to |.
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("$F0!$0F").value, 0xFF);
}

// ---------------------------------------------------------------------------
// Shift operators
// ---------------------------------------------------------------------------

TEST(ExpressionTest, ShiftLeft) {
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("1<<4").value, 16);
}

TEST(ExpressionTest, ShiftRight) {
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("256>>4").value, 16);
}

TEST(ExpressionTest, ShiftRightIsLogical) {
  // >> must be unsigned (logical) shift, not arithmetic.
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("$80000000>>1").value, static_cast<int32_t>(0x40000000));
}

// ---------------------------------------------------------------------------
// Unary operators
// ---------------------------------------------------------------------------

TEST(ExpressionTest, UnaryMinus) {
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("-5").value, -5);
}

TEST(ExpressionTest, UnaryComplement) {
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("~0").value, -1);
}

TEST(ExpressionTest, DoubleUnaryMinus) {
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("--5").value, 5);
}

TEST(ExpressionTest, DoubleComplement) {
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("~~$FF").value, 0xFF);
}

// ---------------------------------------------------------------------------
// Operator precedence (matches EVAL.CPP: +/- < */\\ < &|^ < <<>>)
// ---------------------------------------------------------------------------

TEST(ExpressionTest, PrecMulBeforeAdd) {
  // 2+3*4 = 2+(3*4) = 14, not (2+3)*4 = 20.
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("2+3*4").value, 14);
}

TEST(ExpressionTest, PrecBitwiseBeforeAdd) {
  // $FF&$0F+1: & (prec 3) binds tighter than + (prec 1).
  // Result: ($FF&$0F)+1 = $0F+1 = $10.
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("$FF&$0F+1").value, 0x10);
}

TEST(ExpressionTest, PrecShiftBeforeAdd) {
  // 1+2<<3: << (prec 4) binds tighter than + (prec 1).
  // Result: 1+(2<<3) = 1+16 = 17.
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("1+2<<3").value, 17);
}

TEST(ExpressionTest, PrecShiftBeforeBitwise) {
  // 1<<4|2: << (prec 4) binds tighter than | (prec 3).
  // Result: (1<<4)|2 = 16|2 = 18.
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("1<<4|2").value, 18);
}

TEST(ExpressionTest, PrecParenthesesOverride) {
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("(2+3)*4").value, 20);
}

TEST(ExpressionTest, PrecLeftAssociativity) {
  // 2-3-4 = (2-3)-4 = -5, not 2-(3-4) = 3.
  ExpressionEvaluator e;
  EXPECT_EQ(e.Evaluate("2-3-4").value, -5);
}

// ---------------------------------------------------------------------------
// Symbol resolution
// ---------------------------------------------------------------------------

TEST(ExpressionTest, SymbolBackRef) {
  SymbolTable st;
  st.DefineEqu("SIZE", 256, 1);
  ExpressionEvaluator e;
  e.SetSymbolTable(&st);
  auto r = e.Evaluate("SIZE");
  EXPECT_TRUE(r.is_valid);
  EXPECT_TRUE(r.is_back_ref);
  EXPECT_EQ(r.value, 256);
}

TEST(ExpressionTest, SymbolInExpression) {
  SymbolTable st;
  st.DefineEqu("BASE", 0x1000, 1);
  ExpressionEvaluator e;
  e.SetSymbolTable(&st);
  EXPECT_EQ(e.Evaluate("BASE+4").value, 0x1004);
}

TEST(ExpressionTest, LocationCounter) {
  ExpressionEvaluator e;
  e.SetLocationCounter(0x2000);
  auto r = e.Evaluate("*");
  EXPECT_TRUE(r.is_valid);
  EXPECT_EQ(r.value, 0x2000);
}

TEST(ExpressionTest, LocationCounterInExpression) {
  ExpressionEvaluator e;
  e.SetLocationCounter(0x100);
  EXPECT_EQ(e.Evaluate("*+4").value, 0x104);
}

TEST(ExpressionTest, Pass1UndefinedSymbol) {
  // Pass 1: undefined symbol → value=0, is_valid=true, is_back_ref=false.
  ExpressionEvaluator e;
  e.SetPass(1);
  SymbolTable st;
  e.SetSymbolTable(&st);
  auto r = e.Evaluate("UNKNOWN");
  EXPECT_TRUE(r.is_valid);
  EXPECT_FALSE(r.is_back_ref);
  EXPECT_EQ(r.value, 0);
}

TEST(ExpressionTest, Pass2UndefinedSymbol) {
  ExpressionEvaluator e;
  e.SetPass(2);
  SymbolTable st;
  e.SetSymbolTable(&st);
  auto r = e.Evaluate("UNKNOWN");
  EXPECT_FALSE(r.is_valid);
}

TEST(ExpressionTest, RegisterListSymbolIsError) {
  SymbolTable st;
  st.DefineRegisterList("REGS", 0x00FF, 1);
  ExpressionEvaluator e;
  e.SetSymbolTable(&st);
  auto r = e.Evaluate("REGS");
  EXPECT_FALSE(r.is_valid);
}

// ---------------------------------------------------------------------------
// Error cases
// ---------------------------------------------------------------------------

TEST(ExpressionTest, DivisionByZero) {
  ExpressionEvaluator e;
  auto r = e.Evaluate("5/0");
  EXPECT_FALSE(r.is_valid);
}

TEST(ExpressionTest, ModuloByZero) {
  ExpressionEvaluator e;
  auto r = e.Evaluate("5\\0");
  EXPECT_FALSE(r.is_valid);
}

TEST(ExpressionTest, EmptyExpression) {
  ExpressionEvaluator e;
  auto r = e.Evaluate("");
  EXPECT_FALSE(r.is_valid);
}

TEST(ExpressionTest, MismatchedParenthesis) {
  ExpressionEvaluator e;
  auto r = e.Evaluate("(1+2");
  EXPECT_FALSE(r.is_valid);
}

TEST(ExpressionTest, StructuredControlEndiError) {
  ExpressionEvaluator e;
  e.SetPass(2);
  SymbolTable st;
  e.SetSymbolTable(&st);
  auto r = e.Evaluate(".0");
  EXPECT_FALSE(r.is_valid);
  EXPECT_NE(r.error_message.find("ENDI"), std::string::npos);
}

TEST(ExpressionTest, StructuredControlEndwError) {
  ExpressionEvaluator e;
  e.SetPass(2);
  SymbolTable st;
  e.SetSymbolTable(&st);
  auto r = e.Evaluate(".1");
  EXPECT_FALSE(r.is_valid);
  EXPECT_NE(r.error_message.find("ENDW"), std::string::npos);
}

TEST(ExpressionTest, StructuredControlEndfError) {
  ExpressionEvaluator e;
  e.SetPass(2);
  SymbolTable st;
  e.SetSymbolTable(&st);
  auto r = e.Evaluate(".2");
  EXPECT_FALSE(r.is_valid);
  EXPECT_NE(r.error_message.find("ENDF"), std::string::npos);
}

TEST(ExpressionTest, StructuredControlRepeatError) {
  ExpressionEvaluator e;
  e.SetPass(2);
  SymbolTable st;
  e.SetSymbolTable(&st);
  auto r = e.Evaluate(".3");
  EXPECT_FALSE(r.is_valid);
  EXPECT_NE(r.error_message.find("REPEAT"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Token-slice overload
// ---------------------------------------------------------------------------

TEST(ExpressionTest, EvaluateFromTokenSlice) {
  // Tokenize "42,D0" and evaluate only the first token as an expression.
  Lexer lexer;
  auto tokens = lexer.TokenizeLine("42,D0", 1);
  ExpressionEvaluator e;
  size_t end = 0;
  auto r = e.Evaluate(tokens, 0, end);
  EXPECT_TRUE(r.is_valid);
  EXPECT_EQ(r.value, 42);
  // end should stop at the comma token.
  EXPECT_EQ(tokens[end].type, TokenType::kComma);
}

}  // namespace
}  // namespace easym68k::asm_
