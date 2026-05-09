// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/asm/parser.h"

#include <gtest/gtest.h>

#include "easym68k/asm/symbol_table.h"

namespace easym68k::asm_ {
namespace {

// ---------------------------------------------------------------------------
// ParseLine — empty / comment
// ---------------------------------------------------------------------------

TEST(ParserTest, EmptyLineProducesNoOpcode) {
  Parser p;
  auto line = p.ParseLine("", 1);
  EXPECT_TRUE(line.opcode.empty());
  EXPECT_TRUE(line.label.empty());
  EXPECT_FALSE(line.has_error);
}

TEST(ParserTest, CommentOnlyLine) {
  Parser p;
  auto line = p.ParseLine("* full line comment", 1);
  EXPECT_TRUE(line.opcode.empty());
  EXPECT_FALSE(line.comment.empty());
}

TEST(ParserTest, SemicolonComment) {
  Parser p;
  auto line = p.ParseLine("  NOP ; inline comment", 1);
  EXPECT_EQ(line.opcode, "NOP");
  EXPECT_FALSE(line.comment.empty());
}

// ---------------------------------------------------------------------------
// ParseLine — label
// ---------------------------------------------------------------------------

TEST(ParserTest, LabelWithColon) {
  Parser p;
  auto line = p.ParseLine("start: NOP", 1);
  EXPECT_EQ(line.label, "start");
  EXPECT_EQ(line.opcode, "NOP");
}

TEST(ParserTest, LabelWithoutColon) {
  Parser p;
  auto line = p.ParseLine("start NOP", 1);
  EXPECT_EQ(line.label, "start");
  EXPECT_EQ(line.opcode, "NOP");
}

TEST(ParserTest, NoLabel) {
  Parser p;
  auto line = p.ParseLine("  NOP", 1);
  EXPECT_TRUE(line.label.empty());
  EXPECT_EQ(line.opcode, "NOP");
}

// ---------------------------------------------------------------------------
// ParseLine — opcode / directive
// ---------------------------------------------------------------------------

TEST(ParserTest, OpcodeUppercased) {
  Parser p;
  auto line = p.ParseLine("  move.w D0,D1", 1);
  EXPECT_EQ(line.opcode, "MOVE");
  EXPECT_EQ(line.size, SizeSpec::kWord);
}

TEST(ParserTest, DirectiveParsed) {
  Parser p;
  auto line = p.ParseLine("  ORG $1000", 1);
  EXPECT_EQ(line.opcode, "ORG");
  ASSERT_EQ(line.operands.size(), 1u);
  EXPECT_EQ(line.operands[0].data, 0x1000);
}

TEST(ParserTest, LineNumber) {
  Parser p;
  auto line = p.ParseLine("NOP", 42);
  EXPECT_EQ(line.line_number, 42);
}

// ---------------------------------------------------------------------------
// ParseOperand — register direct
// ---------------------------------------------------------------------------

TEST(ParserTest, DataRegisterDirect) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("D3", op));
  EXPECT_EQ(op.mode, AddressMode::kDnDirect);
  EXPECT_EQ(op.reg, 3);
}

TEST(ParserTest, AddressRegisterDirect) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("A5", op));
  EXPECT_EQ(op.mode, AddressMode::kAnDirect);
  EXPECT_EQ(op.reg, 5);
}

TEST(ParserTest, SPRegister) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("SP", op));
  EXPECT_EQ(op.mode, AddressMode::kAnDirect);
  EXPECT_EQ(op.reg, 7);
}

TEST(ParserTest, SRRegister) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("SR", op));
  EXPECT_EQ(op.mode, AddressMode::kSRDirect);
}

TEST(ParserTest, CCRRegister) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("CCR", op));
  EXPECT_EQ(op.mode, AddressMode::kCCRDirect);
}

TEST(ParserTest, USPRegister) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("USP", op));
  EXPECT_EQ(op.mode, AddressMode::kUSPDirect);
}

// ---------------------------------------------------------------------------
// ParseOperand — immediate
// ---------------------------------------------------------------------------

TEST(ParserTest, ImmediateDecimal) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("#42", op));
  EXPECT_EQ(op.mode, AddressMode::kImmediate);
  EXPECT_EQ(op.data, 42);
}

TEST(ParserTest, ImmediateHex) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("#$FF", op));
  EXPECT_EQ(op.mode, AddressMode::kImmediate);
  EXPECT_EQ(op.data, 0xFF);
}

TEST(ParserTest, ImmediateNegative) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("#-1", op));
  EXPECT_EQ(op.mode, AddressMode::kImmediate);
  EXPECT_EQ(op.data, -1);
}

TEST(ParserTest, ImmediateExpression) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("#4+2", op));
  EXPECT_EQ(op.data, 6);
}

// ---------------------------------------------------------------------------
// ParseOperand — indirect modes
// ---------------------------------------------------------------------------

TEST(ParserTest, AnIndirect) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("(A2)", op));
  EXPECT_EQ(op.mode, AddressMode::kAnIndirect);
  EXPECT_EQ(op.reg, 2);
}

TEST(ParserTest, AnIndirectPostincrement) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("(A3)+", op));
  EXPECT_EQ(op.mode, AddressMode::kAnIndirectPost);
  EXPECT_EQ(op.reg, 3);
}

TEST(ParserTest, AnIndirectPredecrement) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("-(A4)", op));
  EXPECT_EQ(op.mode, AddressMode::kAnIndirectPre);
  EXPECT_EQ(op.reg, 4);
}

TEST(ParserTest, AnIndirectDisplacement) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("8(A1)", op));
  EXPECT_EQ(op.mode, AddressMode::kAnIndirectDisp);
  EXPECT_EQ(op.reg, 1);
  EXPECT_EQ(op.data, 8);
}

TEST(ParserTest, AnIndirectIndex) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("4(A0,D1)", op));
  EXPECT_EQ(op.mode, AddressMode::kAnIndirectIdx);
  EXPECT_EQ(op.reg, 0);
  EXPECT_EQ(op.data, 4);
  EXPECT_EQ(op.index_reg, 1);  // D1 = index 1
}

TEST(ParserTest, AnIndirectIndexLongSize) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("4(A0,D1.L)", op));
  EXPECT_EQ(op.mode, AddressMode::kAnIndirectIdx);
  EXPECT_EQ(op.reg, 0);
  EXPECT_EQ(op.data, 4);
  EXPECT_EQ(op.index_reg, 1);
  EXPECT_EQ(op.index_size, SizeSpec::kLong);
}

TEST(ParserTest, AnIndirectIndexDefaultsToWordSize) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("4(A0,D1)", op));
  EXPECT_EQ(op.index_size, SizeSpec::kWord);
}

TEST(ParserTest, PCDisplacement) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("(PC)", op));
  EXPECT_EQ(op.mode, AddressMode::kPCDisp);
}

TEST(ParserTest, PCDisplacementWithValue) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("10(PC)", op));
  EXPECT_EQ(op.mode, AddressMode::kPCDisp);
  EXPECT_EQ(op.data, 10);
}

TEST(ParserTest, PCIndexNoDisplacement) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("(PC,D0)", op));
  EXPECT_EQ(op.mode, AddressMode::kPCIndex);
  EXPECT_EQ(op.index_reg, 0);
}

TEST(ParserTest, PCIndexWithDisplacement) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("10(PC,D0)", op));
  EXPECT_EQ(op.mode, AddressMode::kPCIndex);
  EXPECT_EQ(op.data, 10);
  EXPECT_EQ(op.index_reg, 0);
}

// ---------------------------------------------------------------------------
// ParseOperand — absolute addressing
// ---------------------------------------------------------------------------

TEST(ParserTest, AbsShortFromSmallValue) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("$100", op));
  EXPECT_EQ(op.mode, AddressMode::kAbsShort);
  EXPECT_EQ(op.data, 0x100);
}

TEST(ParserTest, AbsLongFromLargeValue) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("$10000", op));
  EXPECT_EQ(op.mode, AddressMode::kAbsLong);
}

TEST(ParserTest, AbsShortForcedByDotW) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("$8000.W", op));
  EXPECT_EQ(op.mode, AddressMode::kAbsShort);
  EXPECT_EQ(op.forced_size, SizeSpec::kWord);
}

TEST(ParserTest, AbsLongForcedByDotL) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("$100.L", op));
  EXPECT_EQ(op.mode, AddressMode::kAbsLong);
  EXPECT_EQ(op.forced_size, SizeSpec::kLong);
}

// ---------------------------------------------------------------------------
// Expression evaluation
// ---------------------------------------------------------------------------

TEST(ParserTest, ExpressionArithmetic) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("#3*4+2", op));
  EXPECT_EQ(op.data, 14);
}

TEST(ParserTest, ExpressionPrecedenceAddMul) {
  Parser p;
  Operand op;
  // Without precedence: (2+3)*4 = 20. With correct precedence: 2+(3*4) = 14.
  EXPECT_TRUE(p.ParseOperand("#2+3*4", op));
  EXPECT_EQ(op.data, 14);
}

TEST(ParserTest, ExpressionPrecedenceBitwiseOverAdd) {
  Parser p;
  Operand op;
  // EASy68K precedence: & (3) > + (1), so 1+($F0&$FF) = 1+$F0 = 0xF1.
  EXPECT_TRUE(p.ParseOperand("#1+$F0&$FF", op));
  EXPECT_EQ(op.data, 0xF1);
}

TEST(ParserTest, ExpressionPrecedenceShiftOverBitwise) {
  Parser p;
  Operand op;
  // EASy68K precedence: << (4) > | (3), so (1<<4)|2 = 0x12.
  EXPECT_TRUE(p.ParseOperand("#1<<4|2", op));
  EXPECT_EQ(op.data, 0x12);
}

TEST(ParserTest, ExpressionBitwiseAnd) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("#$FF&$0F", op));
  EXPECT_EQ(op.data, 0x0F);
}

TEST(ParserTest, ExpressionBitwiseOr) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("#$F0|$0F", op));
  EXPECT_EQ(op.data, 0xFF);
}

TEST(ParserTest, ExpressionShiftLeft) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("#1<<8", op));
  EXPECT_EQ(op.data, 256);
}

TEST(ParserTest, ExpressionModulo) {
  Parser p;
  Operand op;
  EXPECT_TRUE(p.ParseOperand("#10\\3", op));
  EXPECT_EQ(op.data, 1);
}

TEST(ParserTest, ExpressionDivisionByZeroIsError) {
  Parser p;
  p.SetPass(2);  // division-by-zero only reported in pass 2 with full checks
  Operand op;
  // Actually EvaluateExpression checks for /0 in both passes
  EXPECT_FALSE(p.ParseOperand("#5/0", op));
  EXPECT_TRUE(p.HasError());
}

// ---------------------------------------------------------------------------
// Symbol resolution
// ---------------------------------------------------------------------------

TEST(ParserTest, ImmediateFromSymbol) {
  SymbolTable st;
  st.DefineEqu("SIZE", 256, 1);
  Parser p;
  p.SetSymbolTable(&st);
  Operand op;
  EXPECT_TRUE(p.ParseOperand("#SIZE", op));
  EXPECT_EQ(op.data, 256);
}

// ---------------------------------------------------------------------------
// Full line parse
// ---------------------------------------------------------------------------

TEST(ParserTest, MoveW_D0_D1) {
  Parser p;
  auto line = p.ParseLine("  MOVE.W D0,D1", 1);
  EXPECT_EQ(line.opcode, "MOVE");
  EXPECT_EQ(line.size, SizeSpec::kWord);
  ASSERT_EQ(line.operands.size(), 2u);
  EXPECT_EQ(line.operands[0].mode, AddressMode::kDnDirect);
  EXPECT_EQ(line.operands[0].reg, 0);
  EXPECT_EQ(line.operands[1].mode, AddressMode::kDnDirect);
  EXPECT_EQ(line.operands[1].reg, 1);
}

TEST(ParserTest, MoveqImmediate) {
  Parser p;
  auto line = p.ParseLine("  MOVEQ #5,D0", 1);
  EXPECT_EQ(line.opcode, "MOVEQ");
  ASSERT_EQ(line.operands.size(), 2u);
  EXPECT_EQ(line.operands[0].mode, AddressMode::kImmediate);
  EXPECT_EQ(line.operands[0].data, 5);
}

TEST(ParserTest, LabelOpcodeOperands) {
  Parser p;
  auto line = p.ParseLine("loop: BRA loop", 1);
  EXPECT_EQ(line.label, "loop");
  EXPECT_EQ(line.opcode, "BRA");
  ASSERT_EQ(line.operands.size(), 1u);
}

// ---------------------------------------------------------------------------
// EA encoding helpers
// ---------------------------------------------------------------------------

TEST(ParserTest, GetEAEncodingDnDirect) {
  Operand op;
  op.mode = AddressMode::kDnDirect;
  op.reg = 3;
  EXPECT_EQ(GetEAMode(op), 0);
  EXPECT_EQ(GetEAReg(op), 3);
  EXPECT_EQ(GetEAEncoding(op), 3);
}

TEST(ParserTest, GetEAEncodingAbsShort) {
  Operand op;
  op.mode = AddressMode::kAbsShort;
  EXPECT_EQ(GetEAMode(op), 7);
  EXPECT_EQ(GetEAReg(op), 0);
  EXPECT_EQ(GetEAEncoding(op), 0x38);
}

TEST(ParserTest, GetEAEncodingImmediate) {
  Operand op;
  op.mode = AddressMode::kImmediate;
  EXPECT_EQ(GetEAMode(op), 7);
  EXPECT_EQ(GetEAReg(op), 4);
  EXPECT_EQ(GetEAEncoding(op), 0x3C);
}

}  // namespace
}  // namespace easym68k::asm_
