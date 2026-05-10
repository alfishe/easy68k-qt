// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/asm/code_generator.h"

#include <cstdint>
#include <string>
#include <vector>

#include "easym68k/asm/instruction_table.h"
#include "easym68k/asm/parser.h"
#include "gtest/gtest.h"

namespace easym68k::asm_ {
namespace {

// ---------------------------------------------------------------------------
// Helper — build a minimal Operand for a given addressing mode.
// ---------------------------------------------------------------------------

Operand MakeDn(int n) {
  Operand op;
  op.mode = AddressMode::kDnDirect;
  op.reg = n;
  return op;
}

Operand MakeAn(int n) {
  Operand op;
  op.mode = AddressMode::kAnDirect;
  op.reg = n;
  return op;
}

Operand MakeAnInd(int n) {
  Operand op;
  op.mode = AddressMode::kAnIndirect;
  op.reg = n;
  return op;
}

Operand MakeAnPost(int n) {
  Operand op;
  op.mode = AddressMode::kAnIndirectPost;
  op.reg = n;
  return op;
}

Operand MakeAnPre(int n) {
  Operand op;
  op.mode = AddressMode::kAnIndirectPre;
  op.reg = n;
  return op;
}

Operand MakeAnDisp(int n, int32_t disp) {
  Operand op;
  op.mode = AddressMode::kAnIndirectDisp;
  op.reg = n;
  op.data = disp;
  return op;
}

Operand MakeAnIdx(int n, int32_t disp, int idx_reg, SizeSpec idx_size) {
  Operand op;
  op.mode = AddressMode::kAnIndirectIdx;
  op.reg = n;
  op.data = disp;
  op.index_reg = idx_reg;
  op.index_size = idx_size;
  return op;
}

Operand MakeAbsShort(int32_t addr) {
  Operand op;
  op.mode = AddressMode::kAbsShort;
  op.data = addr;
  return op;
}

Operand MakeAbsLong(int32_t addr) {
  Operand op;
  op.mode = AddressMode::kAbsLong;
  op.data = addr;
  return op;
}

Operand MakePCDisp(int32_t abs_target) {
  Operand op;
  op.mode = AddressMode::kPCDisp;
  op.data = abs_target;
  return op;
}

Operand MakePCIdx(int32_t abs_target, int idx_reg, SizeSpec idx_size) {
  Operand op;
  op.mode = AddressMode::kPCIndex;
  op.data = abs_target;
  op.index_reg = idx_reg;
  op.index_size = idx_size;
  return op;
}

Operand MakeImm(int32_t val) {
  Operand op;
  op.mode = AddressMode::kImmediate;
  op.data = val;
  return op;
}

// ---------------------------------------------------------------------------
// EffAddr tests — 13 addressing modes
// ---------------------------------------------------------------------------

TEST(EffAddrTest, DnDirect) {
  for (int i = 0; i < 8; ++i)
    EXPECT_EQ(CodeGenerator::EffAddr(MakeDn(i)), i);
}

TEST(EffAddrTest, AnDirect) {
  for (int i = 0; i < 8; ++i)
    EXPECT_EQ(CodeGenerator::EffAddr(MakeAn(i)), 0x08 | i);
}

TEST(EffAddrTest, AnIndirect) {
  for (int i = 0; i < 8; ++i)
    EXPECT_EQ(CodeGenerator::EffAddr(MakeAnInd(i)), 0x10 | i);
}

TEST(EffAddrTest, AnIndirectPost) {
  for (int i = 0; i < 8; ++i)
    EXPECT_EQ(CodeGenerator::EffAddr(MakeAnPost(i)), 0x18 | i);
}

TEST(EffAddrTest, AnIndirectPre) {
  for (int i = 0; i < 8; ++i)
    EXPECT_EQ(CodeGenerator::EffAddr(MakeAnPre(i)), 0x20 | i);
}

TEST(EffAddrTest, AnIndirectDisp) {
  for (int i = 0; i < 8; ++i)
    EXPECT_EQ(CodeGenerator::EffAddr(MakeAnDisp(i, 0)), 0x28 | i);
}

TEST(EffAddrTest, AnIndirectIdx) {
  for (int i = 0; i < 8; ++i)
    EXPECT_EQ(CodeGenerator::EffAddr(MakeAnIdx(i, 0, 0, SizeSpec::kWord)), 0x30 | i);
}

TEST(EffAddrTest, AbsShort) {
  EXPECT_EQ(CodeGenerator::EffAddr(MakeAbsShort(0x1000)), 0x38);
}

TEST(EffAddrTest, AbsLong) {
  EXPECT_EQ(CodeGenerator::EffAddr(MakeAbsLong(0x12345678)), 0x39);
}

TEST(EffAddrTest, PCDisp) {
  EXPECT_EQ(CodeGenerator::EffAddr(MakePCDisp(0x1000)), 0x3A);
}

TEST(EffAddrTest, PCIndex) {
  EXPECT_EQ(CodeGenerator::EffAddr(MakePCIdx(0x1000, 0, SizeSpec::kWord)), 0x3B);
}

TEST(EffAddrTest, Immediate) {
  EXPECT_EQ(CodeGenerator::EffAddr(MakeImm(42)), 0x3C);
}

TEST(EffAddrTest, SRDirectReturnsMinusOne) {
  Operand op;
  op.mode = AddressMode::kSRDirect;
  EXPECT_EQ(CodeGenerator::EffAddr(op), -1);
}

TEST(EffAddrTest, CCRDirectReturnsMinusOne) {
  Operand op;
  op.mode = AddressMode::kCCRDirect;
  EXPECT_EQ(CodeGenerator::EffAddr(op), -1);
}

TEST(EffAddrTest, USPDirectReturnsMinusOne) {
  Operand op;
  op.mode = AddressMode::kUSPDirect;
  EXPECT_EQ(CodeGenerator::EffAddr(op), -1);
}

// ---------------------------------------------------------------------------
// ExtWords — helper: collect emitted words and track loc
// ---------------------------------------------------------------------------

struct ExtResult {
  bool ok;
  std::string error;
  std::vector<uint16_t> words;
  uint32_t loc;
};

ExtResult RunExtWords(const Operand& op, SizeSpec size, uint32_t start_loc) {
  ExtResult r;
  r.loc = start_loc;
  r.ok = CodeGenerator::ExtWords(
      op, size, r.loc, [&r](uint16_t w) { r.words.push_back(w); }, &r.error);
  return r;
}

// ---------------------------------------------------------------------------
// ExtWords — modes that emit no extension words
// ---------------------------------------------------------------------------

TEST(ExtWordsTest, DnDirectNoWords) {
  auto r = RunExtWords(MakeDn(3), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  EXPECT_TRUE(r.words.empty());
  EXPECT_EQ(r.loc, 0x1000u);
}

TEST(ExtWordsTest, AnDirectNoWords) {
  auto r = RunExtWords(MakeAn(5), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  EXPECT_TRUE(r.words.empty());
}

TEST(ExtWordsTest, AnIndirectNoWords) {
  auto r = RunExtWords(MakeAnInd(2), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  EXPECT_TRUE(r.words.empty());
}

TEST(ExtWordsTest, AnIndirectPostNoWords) {
  auto r = RunExtWords(MakeAnPost(0), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  EXPECT_TRUE(r.words.empty());
}

TEST(ExtWordsTest, AnIndirectPreNoWords) {
  auto r = RunExtWords(MakeAnPre(7), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  EXPECT_TRUE(r.words.empty());
}

// ---------------------------------------------------------------------------
// ExtWords — d(An)
// ---------------------------------------------------------------------------

TEST(ExtWordsTest, AnIndirectDispPositive) {
  auto r = RunExtWords(MakeAnDisp(3, 100), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 100u);
  EXPECT_EQ(r.loc, 0x1002u);
}

TEST(ExtWordsTest, AnIndirectDispNegative) {
  auto r = RunExtWords(MakeAnDisp(0, -4), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], static_cast<uint16_t>(-4));
}

TEST(ExtWordsTest, AnIndirectDispMaxPositive) {
  auto r = RunExtWords(MakeAnDisp(0, 32767), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  EXPECT_EQ(r.words[0], 0x7FFFu);
}

TEST(ExtWordsTest, AnIndirectDispMinNegative) {
  auto r = RunExtWords(MakeAnDisp(0, -32768), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  EXPECT_EQ(r.words[0], 0x8000u);
}

TEST(ExtWordsTest, AnIndirectDispOutOfRangePositive) {
  auto r = RunExtWords(MakeAnDisp(0, 32768), SizeSpec::kWord, 0x1000);
  EXPECT_FALSE(r.ok);
  EXPECT_NE(r.error.find("INV_DISP"), std::string::npos);
}

TEST(ExtWordsTest, AnIndirectDispOutOfRangeNegative) {
  auto r = RunExtWords(MakeAnDisp(0, -32769), SizeSpec::kWord, 0x1000);
  EXPECT_FALSE(r.ok);
  EXPECT_NE(r.error.find("INV_DISP"), std::string::npos);
}

// ---------------------------------------------------------------------------
// ExtWords — d(An,Xi)
// ---------------------------------------------------------------------------

TEST(ExtWordsTest, AnIndirectIdxDnWord) {
  // d(A2, D3.W) with disp=8
  // ext = (D3=3)<<12 | (word→0<<11) | (8&0xFF) = 0x3008
  auto r = RunExtWords(MakeAnIdx(2, 8, 3, SizeSpec::kWord), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x3008u);
  EXPECT_EQ(r.loc, 0x1002u);
}

TEST(ExtWordsTest, AnIndirectIdxAnLong) {
  // d(A0, A5.L) with disp=0  → index_reg=13 (A5=8+5)
  // ext = (13)<<12 | (1<<11) | 0 = 0xD800
  auto r = RunExtWords(MakeAnIdx(0, 0, 13, SizeSpec::kLong), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xD800u);
}

TEST(ExtWordsTest, AnIndirectIdxNegativeDisp) {
  // d(A1, D0.W) disp=-4 → 0xFFFC & 0xFF = 0xFC
  auto r = RunExtWords(MakeAnIdx(1, -4, 0, SizeSpec::kWord), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0] & 0xFF, 0xFCu);
}

TEST(ExtWordsTest, AnIndirectIdxDispOutOfRange) {
  auto r = RunExtWords(MakeAnIdx(0, 128, 0, SizeSpec::kWord), SizeSpec::kWord, 0x1000);
  EXPECT_FALSE(r.ok);
  EXPECT_NE(r.error.find("INV_DISP"), std::string::npos);
}

TEST(ExtWordsTest, AnIndirectIdxDispMinNegOutOfRange) {
  auto r = RunExtWords(MakeAnIdx(0, -129, 0, SizeSpec::kWord), SizeSpec::kWord, 0x1000);
  EXPECT_FALSE(r.ok);
}

// ---------------------------------------------------------------------------
// ExtWords — Abs.W
// ---------------------------------------------------------------------------

TEST(ExtWordsTest, AbsShortPositive) {
  auto r = RunExtWords(MakeAbsShort(0x1234), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x1234u);
  EXPECT_EQ(r.loc, 0x1002u);
}

TEST(ExtWordsTest, AbsShortNegative) {
  // Negative short: -2 = 0xFFFE; valid for kAbsShort
  auto r = RunExtWords(MakeAbsShort(-2), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  EXPECT_EQ(r.words[0], 0xFFFEu);
}

TEST(ExtWordsTest, AbsShortOutOfRange) {
  auto r = RunExtWords(MakeAbsShort(32768), SizeSpec::kWord, 0x1000);
  EXPECT_FALSE(r.ok);
  EXPECT_NE(r.error.find("INV_ABS_ADDRESS"), std::string::npos);
}

TEST(ExtWordsTest, AbsShortOutOfRangeNeg) {
  auto r = RunExtWords(MakeAbsShort(-32769), SizeSpec::kWord, 0x1000);
  EXPECT_FALSE(r.ok);
}

// ---------------------------------------------------------------------------
// ExtWords — Abs.L
// ---------------------------------------------------------------------------

TEST(ExtWordsTest, AbsLongTwoWords) {
  auto r = RunExtWords(MakeAbsLong(0x12345678), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x1234u);
  EXPECT_EQ(r.words[1], 0x5678u);
  EXPECT_EQ(r.loc, 0x1004u);
}

TEST(ExtWordsTest, AbsLongZero) {
  auto r = RunExtWords(MakeAbsLong(0), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0u);
  EXPECT_EQ(r.words[1], 0u);
}

TEST(ExtWordsTest, AbsLongNegative) {
  // -1 = 0xFFFFFFFF → high=0xFFFF, low=0xFFFF
  auto r = RunExtWords(MakeAbsLong(-1), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0xFFFFu);
  EXPECT_EQ(r.words[1], 0xFFFFu);
}

// ---------------------------------------------------------------------------
// ExtWords — PC-relative d(PC)
// ---------------------------------------------------------------------------

TEST(ExtWordsTest, PCDispForwardRef) {
  // loc=0x1000; target=0x1100 → disp = 0x1100 - (0x1000+2) = 0xFE
  auto r = RunExtWords(MakePCDisp(0x1100), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x00FEu);
  EXPECT_EQ(r.loc, 0x1002u);
}

TEST(ExtWordsTest, PCDispBackwardRef) {
  // loc=0x1100; target=0x1000 → disp = 0x1000 - (0x1100+2) = -258 = 0xFEFE
  auto r = RunExtWords(MakePCDisp(0x1000), SizeSpec::kWord, 0x1100);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], static_cast<uint16_t>(-258));
}

TEST(ExtWordsTest, PCDispZeroOffset) {
  // target == loc+2 → disp = 0
  auto r = RunExtWords(MakePCDisp(0x1002), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  EXPECT_EQ(r.words[0], 0u);
}

TEST(ExtWordsTest, PCDispOutOfRange) {
  // loc=0x1000; target=0x9100 → disp = 0x8100 > 32767
  auto r = RunExtWords(MakePCDisp(0x9100), SizeSpec::kWord, 0x1000);
  EXPECT_FALSE(r.ok);
  EXPECT_NE(r.error.find("INV_DISP"), std::string::npos);
}

// ---------------------------------------------------------------------------
// ExtWords — d(PC,Xi)
// ---------------------------------------------------------------------------

TEST(ExtWordsTest, PCIndexForwardRef) {
  // loc=0x1000; target=0x1082; D0.W → disp = 0x1082-(0x1000+2)=0x80
  // But 0x80 = 128 > 127, so out of range
  auto r = RunExtWords(MakePCIdx(0x1082, 0, SizeSpec::kWord), SizeSpec::kWord, 0x1000);
  EXPECT_FALSE(r.ok);
}

TEST(ExtWordsTest, PCIndexSmallDisp) {
  // loc=0x1000; target=0x1050; D1.W → disp = 0x50-2 = 0x4E = 78
  // ext = (1)<<12 | (0<<11) | (0x4E) = 0x104E
  auto r = RunExtWords(MakePCIdx(0x1050, 1, SizeSpec::kWord), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x104Eu);
}

TEST(ExtWordsTest, PCIndexLongIndexReg) {
  // loc=0x1000; target=0x1002; A3.L (index_reg=11) → disp=0
  // ext = (11)<<12 | (1<<11) | 0 = 0xB800
  auto r = RunExtWords(MakePCIdx(0x1002, 11, SizeSpec::kLong), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xB800u);
}

// ---------------------------------------------------------------------------
// ExtWords — Immediate byte
// ---------------------------------------------------------------------------

TEST(ExtWordsTest, ImmediateByte) {
  auto r = RunExtWords(MakeImm(0x42), SizeSpec::kByte, 0x1000);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x0042u);
  EXPECT_EQ(r.loc, 0x1002u);
}

TEST(ExtWordsTest, ImmediateByteNegative) {
  auto r = RunExtWords(MakeImm(-1), SizeSpec::kByte, 0x1000);
  EXPECT_TRUE(r.ok);
  EXPECT_EQ(r.words[0], 0x00FFu);
}

TEST(ExtWordsTest, ImmediateByteOutOfRange) {
  auto r = RunExtWords(MakeImm(256), SizeSpec::kByte, 0x1000);
  EXPECT_FALSE(r.ok);
  EXPECT_NE(r.error.find("INV_8_BIT_DATA"), std::string::npos);
}

TEST(ExtWordsTest, ImmediateByteNegOutOfRange) {
  auto r = RunExtWords(MakeImm(-129), SizeSpec::kByte, 0x1000);
  EXPECT_FALSE(r.ok);
}

// ---------------------------------------------------------------------------
// ExtWords — Immediate word
// ---------------------------------------------------------------------------

TEST(ExtWordsTest, ImmediateWord) {
  auto r = RunExtWords(MakeImm(0x1234), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x1234u);
  EXPECT_EQ(r.loc, 0x1002u);
}

TEST(ExtWordsTest, ImmediateWordNegative) {
  auto r = RunExtWords(MakeImm(-1), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  EXPECT_EQ(r.words[0], 0xFFFFu);
}

TEST(ExtWordsTest, ImmediateWordMaxUnsigned) {
  // 65535 is in range for word (unsigned representation)
  auto r = RunExtWords(MakeImm(65535), SizeSpec::kWord, 0x1000);
  EXPECT_TRUE(r.ok);
  EXPECT_EQ(r.words[0], 0xFFFFu);
}

TEST(ExtWordsTest, ImmediateWordOutOfRange) {
  auto r = RunExtWords(MakeImm(65536), SizeSpec::kWord, 0x1000);
  EXPECT_FALSE(r.ok);
  EXPECT_NE(r.error.find("INV_16_BIT_DATA"), std::string::npos);
}

// ---------------------------------------------------------------------------
// ExtWords — Immediate long
// ---------------------------------------------------------------------------

TEST(ExtWordsTest, ImmediateLongTwoWords) {
  auto r = RunExtWords(MakeImm(0x12345678), SizeSpec::kLong, 0x1000);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x1234u);
  EXPECT_EQ(r.words[1], 0x5678u);
  EXPECT_EQ(r.loc, 0x1004u);
}

TEST(ExtWordsTest, ImmediateLongNegative) {
  auto r = RunExtWords(MakeImm(-1), SizeSpec::kLong, 0x1000);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0xFFFFu);
  EXPECT_EQ(r.words[1], 0xFFFFu);
}

// ---------------------------------------------------------------------------
// ExtWords — loc advancement
// ---------------------------------------------------------------------------

TEST(ExtWordsTest, LocAdvancedByTwoPerWord) {
  uint32_t loc = 0x2000;
  std::vector<uint16_t> words;
  std::string err;
  Operand op = MakeAbsLong(0xABCD1234);
  bool ok = CodeGenerator::ExtWords(
      op, SizeSpec::kWord, loc, [&words](uint16_t w) { words.push_back(w); }, &err);
  EXPECT_TRUE(ok);
  EXPECT_EQ(loc, 0x2004u);
}

// ---------------------------------------------------------------------------
// Encode — kFixed
// ---------------------------------------------------------------------------

TEST(EncodeTest, FixedEmitsBase) {
  InstrEntry entry;
  entry.encoding = InstrEncoding::kFixed;
  entry.base = 0x4E71;  // NOP

  ParsedLine line;
  line.opcode = "NOP";
  SymbolTable syms;

  std::vector<uint16_t> out;
  std::string err;
  CodeGenerator cg;
  bool ok = cg.Encode(entry, line, 0x1000, syms, [&out](uint16_t w) { out.push_back(w); }, &err);
  EXPECT_TRUE(ok);
  ASSERT_EQ(out.size(), 1u);
  EXPECT_EQ(out[0], 0x4E71u);
}

// ---------------------------------------------------------------------------
// Encode — kMoveq
// ---------------------------------------------------------------------------

TEST(EncodeTest, MoveqD0) {
  InstrEntry entry;
  entry.encoding = InstrEncoding::kMoveq;
  entry.base = 0x7000;

  ParsedLine line;
  line.opcode = "MOVEQ";
  line.operands.push_back(MakeImm(1));
  line.operands.push_back(MakeDn(0));

  SymbolTable syms;
  std::vector<uint16_t> out;
  std::string err;
  CodeGenerator cg;
  bool ok = cg.Encode(entry, line, 0x1000, syms, [&out](uint16_t w) { out.push_back(w); }, &err);
  EXPECT_TRUE(ok);
  ASSERT_EQ(out.size(), 1u);
  EXPECT_EQ(out[0], 0x7001u);  // 0111 000 0 00000001
}

TEST(EncodeTest, MoveqD3Imm42) {
  InstrEntry entry;
  entry.encoding = InstrEncoding::kMoveq;
  entry.base = 0x7000;

  ParsedLine line;
  line.opcode = "MOVEQ";
  line.operands.push_back(MakeImm(42));
  line.operands.push_back(MakeDn(3));

  SymbolTable syms;
  std::vector<uint16_t> out;
  std::string err;
  CodeGenerator cg;
  bool ok = cg.Encode(entry, line, 0, syms, [&out](uint16_t w) { out.push_back(w); }, &err);
  EXPECT_TRUE(ok);
  ASSERT_EQ(out.size(), 1u);
  EXPECT_EQ(out[0], 0x762Au);  // 0111 011 0 00101010
}

TEST(EncodeTest, MoveqNegativeImm) {
  InstrEntry entry;
  entry.encoding = InstrEncoding::kMoveq;
  entry.base = 0x7000;

  ParsedLine line;
  line.operands.push_back(MakeImm(-1));
  line.operands.push_back(MakeDn(7));

  SymbolTable syms;
  std::vector<uint16_t> out;
  std::string err;
  CodeGenerator cg;
  bool ok = cg.Encode(entry, line, 0, syms, [&out](uint16_t w) { out.push_back(w); }, &err);
  EXPECT_TRUE(ok);
  ASSERT_EQ(out.size(), 1u);
  EXPECT_EQ(out[0], 0x7EFFu);  // 0111 111 0 11111111
}

TEST(EncodeTest, MoveqErrorNonImmediate) {
  InstrEntry entry;
  entry.encoding = InstrEncoding::kMoveq;
  entry.base = 0x7000;

  ParsedLine line;
  line.operands.push_back(MakeDn(0));  // wrong: should be immediate
  line.operands.push_back(MakeDn(1));

  SymbolTable syms;
  std::vector<uint16_t> out;
  std::string err;
  CodeGenerator cg;
  bool ok = cg.Encode(entry, line, 0, syms, [&out](uint16_t w) { out.push_back(w); }, &err);
  EXPECT_FALSE(ok);
  EXPECT_FALSE(err.empty());
}

TEST(EncodeTest, MoveqErrorNonDataRegDest) {
  InstrEntry entry;
  entry.encoding = InstrEncoding::kMoveq;
  entry.base = 0x7000;

  ParsedLine line;
  line.operands.push_back(MakeImm(0));
  line.operands.push_back(MakeAn(0));  // wrong: should be Dn

  SymbolTable syms;
  std::vector<uint16_t> out;
  std::string err;
  CodeGenerator cg;
  bool ok = cg.Encode(entry, line, 0, syms, [&out](uint16_t w) { out.push_back(w); }, &err);
  EXPECT_FALSE(ok);
}

// ---------------------------------------------------------------------------
// Helpers for concise handler tests
// ---------------------------------------------------------------------------

struct EncodeResult {
  bool ok;
  std::string error;
  std::vector<uint16_t> words;
};

EncodeResult RunEncode(InstrEncoding enc, uint16_t base, uint8_t sizes, SizeSpec sz,
                       std::initializer_list<Operand> ops, uint32_t loc = 0x1000) {
  InstrEntry entry{};
  entry.encoding = enc;
  entry.base = base;
  entry.sizes = sizes;
  entry.name = "";

  ParsedLine line;
  line.size = sz;
  for (const auto& op : ops)
    line.operands.push_back(op);

  EncodeResult r;
  CodeGenerator cg;
  SymbolTable syms;
  r.ok = cg.Encode(entry, line, loc, syms, [&r](uint16_t w) { r.words.push_back(w); }, &r.error);
  return r;
}

Operand MakeCCR() {
  Operand op;
  op.mode = AddressMode::kCCRDirect;
  return op;
}

Operand MakeSR() {
  Operand op;
  op.mode = AddressMode::kSRDirect;
  return op;
}

Operand MakeUSP() {
  Operand op;
  op.mode = AddressMode::kUSPDirect;
  return op;
}

Operand MakeImmFwd(int32_t val) {
  Operand op = MakeImm(val);
  op.is_back_ref = false;
  return op;
}

// ---------------------------------------------------------------------------
// Encode — kFixedWord (STOP)
// ---------------------------------------------------------------------------

TEST(EncodeTest, StopEmitsBaseAndWord) {
  auto r =
      RunEncode(InstrEncoding::kFixedWord, 0x4E72, kSzNone, SizeSpec::kNone, {MakeImm(0x2700)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x4E72u);
  EXPECT_EQ(r.words[1], 0x2700u);
}

// ---------------------------------------------------------------------------
// Encode — kRegOp (SWAP, EXT)
// ---------------------------------------------------------------------------

TEST(EncodeTest, SwapD3) {
  // SWAP D3: 0x4840 | 3 = 0x4843 (word-only, no size bit)
  auto r = RunEncode(InstrEncoding::kRegOp, 0x4840, kSzWord, SizeSpec::kWord, {MakeDn(3)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x4843u);
}

TEST(EncodeTest, ExtWordD2) {
  // EXT.W D2: 0x4880 | 2 = 0x4882 (size W → bit 6 clear)
  auto r = RunEncode(InstrEncoding::kRegOp, 0x4880, kSzWL, SizeSpec::kWord, {MakeDn(2)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x4882u);
}

TEST(EncodeTest, ExtLongD5) {
  // EXT.L D5: 0x4880 | 0x40 | 5 = 0x48C5 (size L → bit 6 set)
  auto r = RunEncode(InstrEncoding::kRegOp, 0x4880, kSzWL, SizeSpec::kLong, {MakeDn(5)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x48C5u);
}

// ---------------------------------------------------------------------------
// Encode — kAnOp (UNLK)
// ---------------------------------------------------------------------------

TEST(EncodeTest, UnlkA2) {
  // UNLK A2: 0x4E58 | 2 = 0x4E5A
  auto r = RunEncode(InstrEncoding::kAnOp, 0x4E58, kSzNone, SizeSpec::kNone, {MakeAn(2)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x4E5Au);
}

// ---------------------------------------------------------------------------
// Encode — kEaOnly (CLR, PEA)
// ---------------------------------------------------------------------------

TEST(EncodeTest, ClrByteD0) {
  // CLR.B D0: base=0x4200, SizeBits(B)=0 → 0x4200
  auto r = RunEncode(InstrEncoding::kEaOnly, 0x4200, kSzBWL, SizeSpec::kByte, {MakeDn(0)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x4200u);
}

TEST(EncodeTest, ClrWordD0) {
  // CLR.W D0: 0x4200 | 0x40 = 0x4240
  auto r = RunEncode(InstrEncoding::kEaOnly, 0x4200, kSzBWL, SizeSpec::kWord, {MakeDn(0)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x4240u);
}

TEST(EncodeTest, ClrLongD0) {
  // CLR.L D0: 0x4200 | 0x80 = 0x4280
  auto r = RunEncode(InstrEncoding::kEaOnly, 0x4200, kSzBWL, SizeSpec::kLong, {MakeDn(0)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x4280u);
}

TEST(EncodeTest, PeaAnIndirect) {
  // PEA (A0): single-size (kSzLong only), no size field; 0x4840 | 0x10 = 0x4850
  auto r = RunEncode(InstrEncoding::kEaOnly, 0x4840, kSzLong, SizeSpec::kLong, {MakeAnInd(0)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x4850u);
}

// ---------------------------------------------------------------------------
// Encode — kJmpJsr (JMP, JSR)
// ---------------------------------------------------------------------------

TEST(EncodeTest, JmpAnIndirect) {
  // JMP (A0): 0x4EC0 | 0x10 = 0x4ED0
  auto r = RunEncode(InstrEncoding::kJmpJsr, 0x4EC0, kSzNone, SizeSpec::kNone, {MakeAnInd(0)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x4ED0u);
}

TEST(EncodeTest, JsrAnIndirect) {
  // JSR (A1): 0x4E80 | 0x11 = 0x4E91
  auto r = RunEncode(InstrEncoding::kJmpJsr, 0x4E80, kSzNone, SizeSpec::kNone, {MakeAnInd(1)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x4E91u);
}

// ---------------------------------------------------------------------------
// Encode — kEaToReg (CMP)
// ---------------------------------------------------------------------------

TEST(EncodeTest, CmpWordD0D2) {
  // CMP.W D0,D2: 0xB000|0x40|(2<<9)|0 = 0xB440
  auto r =
      RunEncode(InstrEncoding::kEaToReg, 0xB000, kSzBWL, SizeSpec::kWord, {MakeDn(0), MakeDn(2)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xB440u);
}

TEST(EncodeTest, CmpByteAnIndD3) {
  // CMP.B (A1),D3: 0xB000|0|(3<<9)|0x11 = 0xB611, (A1) has no ext words
  auto r = RunEncode(InstrEncoding::kEaToReg, 0xB000, kSzBWL, SizeSpec::kByte,
                     {MakeAnInd(1), MakeDn(3)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xB611u);
}

// ---------------------------------------------------------------------------
// Encode — kRegToEa (EOR)
// ---------------------------------------------------------------------------

TEST(EncodeTest, EorWordD1D0) {
  // EOR.W D1,D0: 0xB100|0x40|(1<<9)|0 = 0xB340
  auto r =
      RunEncode(InstrEncoding::kRegToEa, 0xB100, kSzBWL, SizeSpec::kWord, {MakeDn(1), MakeDn(0)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xB340u);
}

// ---------------------------------------------------------------------------
// Encode — kAddrEa (ADDA, SUBA, CMPA)
// ---------------------------------------------------------------------------

TEST(EncodeTest, AddaWordD0A1) {
  // ADDA.W D0,A1: 0xD0C0|W_bit=0|(1<<9)|EffAddr(D0)=0 = 0xD2C0
  auto r =
      RunEncode(InstrEncoding::kAddrEa, 0xD0C0, kSzWL, SizeSpec::kWord, {MakeDn(0), MakeAn(1)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xD2C0u);
}

TEST(EncodeTest, AddaLongD0A1) {
  // ADDA.L D0,A1: 0xD0C0|0x100|(1<<9)|0 = 0xD3C0
  auto r =
      RunEncode(InstrEncoding::kAddrEa, 0xD0C0, kSzWL, SizeSpec::kLong, {MakeDn(0), MakeAn(1)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xD3C0u);
}

TEST(EncodeTest, CmpaWordD0A2) {
  // CMPA.W D0,A2: 0xB0C0|0|(2<<9)|0 = 0xB4C0
  auto r =
      RunEncode(InstrEncoding::kAddrEa, 0xB0C0, kSzWL, SizeSpec::kWord, {MakeDn(0), MakeAn(2)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xB4C0u);
}

// ---------------------------------------------------------------------------
// Encode — kMulDiv (MULS, DIVU)
// ---------------------------------------------------------------------------

TEST(EncodeTest, MulsD0D2) {
  // MULS D0,D2: 0xC1C0|(2<<9)|EffAddr(D0)=0 = 0xC5C0
  auto r =
      RunEncode(InstrEncoding::kMulDiv, 0xC1C0, kSzWord, SizeSpec::kWord, {MakeDn(0), MakeDn(2)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xC5C0u);
}

TEST(EncodeTest, DivuD1D3) {
  // DIVU D1,D3: 0x80C0|(3<<9)|EffAddr(D1)=1 = 0x86C1
  auto r =
      RunEncode(InstrEncoding::kMulDiv, 0x80C0, kSzWord, SizeSpec::kWord, {MakeDn(1), MakeDn(3)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x86C1u);
}

// ---------------------------------------------------------------------------
// Encode — kLea
// ---------------------------------------------------------------------------

TEST(EncodeTest, LeaAnIndA0A1) {
  // LEA (A0),A1: 0x41C0|(1<<9)|EffAddr((A0))=0x10 = 0x43D0
  auto r =
      RunEncode(InstrEncoding::kLea, 0x41C0, kSzLong, SizeSpec::kLong, {MakeAnInd(0), MakeAn(1)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x43D0u);
}

// ---------------------------------------------------------------------------
// Encode — kEaBidirect (ADD, AND, OR)
// ---------------------------------------------------------------------------

TEST(EncodeTest, AddWordEaToDn) {
  // ADD.W D0,D1 (<ea>,Dn): 0xD000|0x40|(1<<9)|0 = 0xD240
  auto r = RunEncode(InstrEncoding::kEaBidirect, 0xD000, kSzBWL, SizeSpec::kWord,
                     {MakeDn(0), MakeDn(1)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xD240u);
}

TEST(EncodeTest, AddWordDnToEa) {
  // ADD.W D0,(A1) (Dn,<ea>): (0xD000|0x100)|0x40|(0<<9)|0x11 = 0xD151
  auto r = RunEncode(InstrEncoding::kEaBidirect, 0xD000, kSzBWL, SizeSpec::kWord,
                     {MakeDn(0), MakeAnInd(1)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xD151u);
}

TEST(EncodeTest, AddWordImmLargeToAddi) {
  // ADD.W #100,D1 (imm, data=100>8 → ADDI form): 0x0641 + immediate word
  auto r = RunEncode(InstrEncoding::kEaBidirect, 0xD000, kSzBWL, SizeSpec::kWord,
                     {MakeImm(100), MakeDn(1)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x0641u);  // ADDI.W Dn=1
  EXPECT_EQ(r.words[1], 100u);
}

TEST(EncodeTest, AddWordImmSmallToAddq) {
  // ADD.W #5,D0 (imm, data=5 ∈ [1,8], is_back_ref=true → ADDQ peephole)
  // ADDQ.W #5,D0: QuickOpcode(0x5000,W,5,0) = 0x5A40
  auto r = RunEncode(InstrEncoding::kEaBidirect, 0xD000, kSzBWL, SizeSpec::kWord,
                     {MakeImm(5), MakeDn(0)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x5A40u);
}

TEST(EncodeTest, AndImmToCCR) {
  // AND.W #3,CCR → ANDI #3,CCR: ccr_base=0x023C, emit 0x023C+0x0003
  auto r = RunEncode(InstrEncoding::kEaBidirect, 0xC000, kSzBWL, SizeSpec::kWord,
                     {MakeImm(3), MakeCCR()});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x023Cu);
  EXPECT_EQ(r.words[1], 0x0003u);
}

TEST(EncodeTest, OrImmToSR) {
  // OR.W #0x0200,SR → ORI #0x0200,SR: sr_base=0x007C
  auto r = RunEncode(InstrEncoding::kEaBidirect, 0x8000, kSzBWL, SizeSpec::kWord,
                     {MakeImm(0x0200), MakeSR()});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x007Cu);
  EXPECT_EQ(r.words[1], 0x0200u);
}

// ---------------------------------------------------------------------------
// Encode — kImmedToEa (ADDI, ANDI, ORI, SUBI)
// ---------------------------------------------------------------------------

TEST(EncodeTest, AddiWordLargeNormal) {
  // ADDI.W #0x42,D0: 0x0640 + 0x0042
  auto r = RunEncode(InstrEncoding::kImmedToEa, 0x0600, kSzBWL, SizeSpec::kWord,
                     {MakeImm(0x42), MakeDn(0)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x0640u);
  EXPECT_EQ(r.words[1], 0x0042u);
}

TEST(EncodeTest, AndiToCCR) {
  // ANDI.B #0x0F,CCR: ccr_base=(0x0200&0xFF00)|0x3C=0x023C
  auto r = RunEncode(InstrEncoding::kImmedToEa, 0x0200, kSzBWL, SizeSpec::kByte,
                     {MakeImm(0x0F), MakeCCR()});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x023Cu);
  EXPECT_EQ(r.words[1], 0x000Fu);
}

TEST(EncodeTest, OriToSR) {
  // ORI.W #0x0100,SR: sr_base=(0x0000&0xFF00)|0x7C=0x007C
  auto r = RunEncode(InstrEncoding::kImmedToEa, 0x0000, kSzBWL, SizeSpec::kWord,
                     {MakeImm(0x0100), MakeSR()});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x007Cu);
  EXPECT_EQ(r.words[1], 0x0100u);
}

TEST(EncodeTest, SubiWordSmallPeephole) {
  // SUBI.W #3,D0 (data=3 → SUBQ peephole): SUBQ.W #3,D0 = 0x5740
  auto r = RunEncode(InstrEncoding::kImmedToEa, 0x0400, kSzBWL, SizeSpec::kWord,
                     {MakeImm(3), MakeDn(0)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x5740u);
}

// ---------------------------------------------------------------------------
// Encode — kImmedToCcr and kImmedToSr
// ---------------------------------------------------------------------------

TEST(EncodeTest, ImmedToCcrEmitsByteWord) {
  // kImmedToCcr: emit base 0x023C + byte immediate
  auto r = RunEncode(InstrEncoding::kImmedToCcr, 0x023C, kSzByte, SizeSpec::kByte, {MakeImm(0x7F)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x023Cu);
  EXPECT_EQ(r.words[1], 0x007Fu);
}

TEST(EncodeTest, ImmedToSrEmitsWordImm) {
  // kImmedToSr: emit base + full word immediate
  auto r =
      RunEncode(InstrEncoding::kImmedToSr, 0x007C, kSzWord, SizeSpec::kWord, {MakeImm(0x0700)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x007Cu);
  EXPECT_EQ(r.words[1], 0x0700u);
}

// ---------------------------------------------------------------------------
// Encode — kQuick (ADDQ, SUBQ)
// ---------------------------------------------------------------------------

TEST(EncodeTest, AddqByteThreeD0) {
  // ADDQ.B #3,D0: QuickOpcode(0x5000,B,3,0) = 0x5600
  auto r =
      RunEncode(InstrEncoding::kQuick, 0x5000, kSzBWL, SizeSpec::kByte, {MakeImm(3), MakeDn(0)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x5600u);
}

TEST(EncodeTest, SubqWordFourD1) {
  // SUBQ.W #4,D1: QuickOpcode(0x5100,W,4,1) = 0x5941
  auto r =
      RunEncode(InstrEncoding::kQuick, 0x5100, kSzBWL, SizeSpec::kWord, {MakeImm(4), MakeDn(1)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x5941u);
}

TEST(EncodeTest, QuickErrorOutOfRange) {
  auto r =
      RunEncode(InstrEncoding::kQuick, 0x5000, kSzBWL, SizeSpec::kWord, {MakeImm(9), MakeDn(0)});
  EXPECT_FALSE(r.ok);
}

// ---------------------------------------------------------------------------
// Encode — kBranch (BRA, BCC)
// ---------------------------------------------------------------------------

TEST(EncodeTest, BraWordExplicit) {
  // BRA.W to loc+0x102 (word displacement 0x100)
  // loc=0x1000, target=0x1102, size=kWord → word form: [0x6000, 0x0100]
  auto r = RunEncode(InstrEncoding::kBranch, 0x6000, kSzByte | kSzWord, SizeSpec::kWord,
                     {MakeImm(0x1102)}, 0x1000);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x6000u);
  EXPECT_EQ(r.words[1], 0x0100u);
}

TEST(EncodeTest, BraShortExplicit) {
  // BRA.S (kShort) backward: loc=0x1010, target=0x100A → disp=-8
  // short form: [0x60F8]
  auto r = RunEncode(InstrEncoding::kBranch, 0x6000, kSzByte | kSzWord, SizeSpec::kShort,
                     {MakeImm(0x100A)}, 0x1010);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x60F8u);
}

TEST(EncodeTest, BraUnsizedIsWordForm) {
  // BRA without explicit size (kNone → kWord inside Encode): always word form.
  // loc=0x1010, target=0x1008 → disp=-10; word form [0x6000, 0xFFF6]
  auto r = RunEncode(InstrEncoding::kBranch, 0x6000, kSzByte | kSzWord, SizeSpec::kNone,
                     {MakeImm(0x1008)}, 0x1010);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x6000u);
  EXPECT_EQ(r.words[1], static_cast<uint16_t>(-10));
}

TEST(EncodeTest, BccWordForwardRef) {
  // BCC to forward ref (is_back_ref=false): word form regardless of disp
  // loc=0x1000, target=0x1200, BCC base=0x6400
  Operand target = MakeImmFwd(0x1200);
  EncodeResult r;
  {
    InstrEntry entry{};
    entry.encoding = InstrEncoding::kBranch;
    entry.base = 0x6400;
    entry.sizes = kSzByte | kSzWord;
    entry.name = "";
    ParsedLine line;
    line.size = SizeSpec::kNone;
    line.operands.push_back(target);
    CodeGenerator cg;
    SymbolTable syms;
    r.ok =
        cg.Encode(entry, line, 0x1000, syms, [&r](uint16_t w) { r.words.push_back(w); }, &r.error);
  }
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x6400u);
  EXPECT_EQ(r.words[1], 0x01FEu);
}

// ---------------------------------------------------------------------------
// Encode — kDBcc (DBRA)
// ---------------------------------------------------------------------------

TEST(EncodeTest, DbraD0Backward) {
  // DBRA D0,loop: loc=0x1010, target=0x1000 → disp=-18=0xFFEE
  auto r = RunEncode(InstrEncoding::kDBcc, 0x51C8, kSzWord, SizeSpec::kWord,
                     {MakeDn(0), MakeImm(0x1000)}, 0x1010);
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x51C8u);
  EXPECT_EQ(r.words[1], static_cast<uint16_t>(-18));
}

// ---------------------------------------------------------------------------
// Encode — kScc (SEQ, SCC)
// ---------------------------------------------------------------------------

TEST(EncodeTest, SeqD0) {
  // SEQ D0: 0x57C0 | EffAddr(D0)=0 = 0x57C0
  auto r = RunEncode(InstrEncoding::kScc, 0x57C0, kSzByte, SizeSpec::kByte, {MakeDn(0)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x57C0u);
}

TEST(EncodeTest, SccAnPost) {
  // SEQ (A0)+: 0x57C0 | EffAddr((A0)+)=0x18 = 0x57D8
  auto r = RunEncode(InstrEncoding::kScc, 0x57C0, kSzByte, SizeSpec::kByte, {MakeAnPost(0)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x57D8u);
}

// ---------------------------------------------------------------------------
// Encode — kShift (ASL/ASR register and memory forms)
// ---------------------------------------------------------------------------

TEST(EncodeTest, AslWordImmD3) {
  // ASL.W #2,D3: dir=1,type=0,sz=1,cnt=2 → 0xE543
  auto r =
      RunEncode(InstrEncoding::kShift, 0xE160, kSzBWL, SizeSpec::kWord, {MakeImm(2), MakeDn(3)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xE543u);
}

TEST(EncodeTest, AsrWordRegD1D3) {
  // ASR.W D1,D3: dir=0,type=0,sz=1,reg-count(bit5=1) → 0xE263
  auto r =
      RunEncode(InstrEncoding::kShift, 0xE060, kSzBWL, SizeSpec::kWord, {MakeDn(1), MakeDn(3)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xE263u);
}

TEST(EncodeTest, AslWordMemAnPost) {
  // ASL.W (A0)+ (memory form): dir=1,type=0 → 0xE0C0|0x100|0x18 = 0xE1D8
  auto r = RunEncode(InstrEncoding::kShift, 0xE160, kSzBWL, SizeSpec::kWord, {MakeAnPost(0)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xE1D8u);
}

TEST(EncodeTest, LslWordImmD0) {
  // LSL.W #3,D0: dir=1,type=1(LS),sz=1,cnt=3 → 0xE748
  auto r =
      RunEncode(InstrEncoding::kShift, 0xE168, kSzBWL, SizeSpec::kWord, {MakeImm(3), MakeDn(0)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xE748u);
}

// ---------------------------------------------------------------------------
// Encode — kBitReg (BTST, BCLR dynamic and static)
// ---------------------------------------------------------------------------

TEST(EncodeTest, BtstDynamicD1D0) {
  // BTST D1,D0 (dynamic): 0x0100|(1<<9)|EffAddr(D0)=0 = 0x0300
  auto r = RunEncode(InstrEncoding::kBitReg, 0x0100, kSzByte | kSzLong, SizeSpec::kLong,
                     {MakeDn(1), MakeDn(0)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x0300u);
}

TEST(EncodeTest, BtstStaticD0) {
  // BTST #3,D0 (static): static_base=0x0800|0=0x0800, then bit# word 3
  auto r = RunEncode(InstrEncoding::kBitReg, 0x0100, kSzByte | kSzLong, SizeSpec::kLong,
                     {MakeImm(3), MakeDn(0)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x0800u);
  EXPECT_EQ(r.words[1], 0x0003u);
}

TEST(EncodeTest, BclrStaticAnInd) {
  // BCLR #5,(A2) (static): static_base=(0x0180&~0x100)|0x0800=0x0880; 0x0880|0x12=0x0892
  auto r = RunEncode(InstrEncoding::kBitReg, 0x0180, kSzByte | kSzLong, SizeSpec::kByte,
                     {MakeImm(5), MakeAnInd(2)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x0892u);
  EXPECT_EQ(r.words[1], 0x0005u);
}

// ---------------------------------------------------------------------------
// Encode — kMove (MOVE — normal, CCR, SR, USP, MOVEQ peephole)
// ---------------------------------------------------------------------------

TEST(EncodeTest, MoveWordD0D1) {
  // MOVE.W D0,D1: move_base=0x3000, dst_ea=1 → 0x3200
  auto r = RunEncode(InstrEncoding::kMove, 0x0000, kSzBWL, SizeSpec::kWord, {MakeDn(0), MakeDn(1)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x3200u);
}

TEST(EncodeTest, MoveByteD0D1) {
  // MOVE.B D0,D1: move_base=0x1000 → 0x1200
  auto r = RunEncode(InstrEncoding::kMove, 0x0000, kSzBWL, SizeSpec::kByte, {MakeDn(0), MakeDn(1)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x1200u);
}

TEST(EncodeTest, MoveLongD0D1) {
  // MOVE.L D0,D1: move_base=0x2000 → 0x2200
  auto r = RunEncode(InstrEncoding::kMove, 0x0000, kSzBWL, SizeSpec::kLong, {MakeDn(0), MakeDn(1)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x2200u);
}

TEST(EncodeTest, MoveToCCR) {
  // MOVE D0,CCR: 0x44C0 | EffAddr(D0)=0 = 0x44C0
  auto r = RunEncode(InstrEncoding::kMove, 0x0000, kSzBWL, SizeSpec::kWord, {MakeDn(0), MakeCCR()});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x44C0u);
}

TEST(EncodeTest, MoveToSR) {
  // MOVE D0,SR: 0x46C0 | 0 = 0x46C0
  auto r = RunEncode(InstrEncoding::kMove, 0x0000, kSzBWL, SizeSpec::kWord, {MakeDn(0), MakeSR()});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x46C0u);
}

TEST(EncodeTest, MoveAnToUSP) {
  // MOVE A0,USP: 0x4E60 | (A0.reg=0) = 0x4E60
  auto r = RunEncode(InstrEncoding::kMove, 0x0000, kSzBWL, SizeSpec::kWord, {MakeAn(0), MakeUSP()});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x4E60u);
}

TEST(EncodeTest, MoveLongImmD3Moveq) {
  // MOVE.L #42,D3 with is_back_ref=true → MOVEQ peephole: 0x762A
  auto r =
      RunEncode(InstrEncoding::kMove, 0x0000, kSzBWL, SizeSpec::kLong, {MakeImm(42), MakeDn(3)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x762Au);
}

TEST(EncodeTest, MoveLongLargeImmD0NoMoveq) {
  // MOVE.L #1000,D0 (1000 > 127 → no peephole): MOVE.L #1000,D0 = 0x203C + 0x0000 + 0x03E8
  auto r =
      RunEncode(InstrEncoding::kMove, 0x0000, kSzBWL, SizeSpec::kLong, {MakeImm(1000), MakeDn(0)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 3u);
  EXPECT_EQ(r.words[0], 0x203Cu);
  EXPECT_EQ(r.words[1], 0x0000u);
  EXPECT_EQ(r.words[2], 0x03E8u);
}

// ---------------------------------------------------------------------------
// Encode — kMoveA (MOVEA)
// ---------------------------------------------------------------------------

TEST(EncodeTest, MoveaWordD0A1) {
  // MOVEA.W D0,A1: 0x3000|((A1_ea=0x09)remix) = 0x3240
  auto r = RunEncode(InstrEncoding::kMoveA, 0x3000, kSzWL, SizeSpec::kWord, {MakeDn(0), MakeAn(1)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x3240u);
}

TEST(EncodeTest, MoveaLongAnPostA2) {
  // MOVEA.L (A0)+,A2: 0x2000 | dest_remap(A2=0x0A) | EffAddr((A0)+)=0x18 = 0x2458
  auto r =
      RunEncode(InstrEncoding::kMoveA, 0x3000, kSzWL, SizeSpec::kLong, {MakeAnPost(0), MakeAn(2)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x2458u);
}

// ---------------------------------------------------------------------------
// Encode — kMoveP (all four sub-forms)
// ---------------------------------------------------------------------------

TEST(EncodeTest, MovepWordDnToMem) {
  // MOVEP.W D1,4(A0) (Dn→d(An) word): 0x0188|(1<<9)|0 = 0x0388, disp=4
  auto r = RunEncode(InstrEncoding::kMoveP, 0x0188, kSzWL, SizeSpec::kWord,
                     {MakeDn(1), MakeAnDisp(0, 4)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x0388u);
  EXPECT_EQ(r.words[1], 4u);
}

TEST(EncodeTest, MovepLongDnToMem) {
  // MOVEP.L D3,6(A2) (Dn→d(An) long): 0x01C8|(3<<9)|2 = 0x07CA, disp=6
  auto r = RunEncode(InstrEncoding::kMoveP, 0x0188, kSzWL, SizeSpec::kLong,
                     {MakeDn(3), MakeAnDisp(2, 6)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x07CAu);
  EXPECT_EQ(r.words[1], 6u);
}

TEST(EncodeTest, MovepWordMemToDn) {
  // MOVEP.W 0(A1),D2 (d(An)→Dn word): 0x0108|(2<<9)|1 = 0x0509, disp=0
  auto r = RunEncode(InstrEncoding::kMoveP, 0x0188, kSzWL, SizeSpec::kWord,
                     {MakeAnDisp(1, 0), MakeDn(2)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x0509u);
  EXPECT_EQ(r.words[1], 0u);
}

TEST(EncodeTest, MovepLongMemToDn) {
  // MOVEP.L 8(A3),D4 (d(An)→Dn long): 0x0148|(4<<9)|3 = 0x094B, disp=8
  auto r = RunEncode(InstrEncoding::kMoveP, 0x0188, kSzWL, SizeSpec::kLong,
                     {MakeAnDisp(3, 8), MakeDn(4)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x094Bu);
  EXPECT_EQ(r.words[1], 8u);
}

// ---------------------------------------------------------------------------
// Encode — kTrap and kLink
// ---------------------------------------------------------------------------

TEST(EncodeTest, TrapVector7) {
  // TRAP #7: 0x4E40 | 7 = 0x4E47
  auto r = RunEncode(InstrEncoding::kTrap, 0x4E40, kSzNone, SizeSpec::kNone, {MakeImm(7)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x4E47u);
}

TEST(EncodeTest, TrapVector0) {
  // TRAP #0: 0x4E40
  auto r = RunEncode(InstrEncoding::kTrap, 0x4E40, kSzNone, SizeSpec::kNone, {MakeImm(0)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x4E40u);
}

TEST(EncodeTest, LinkA0NegDisp) {
  // LINK A0,#-8: emit 0x4E50, 0xFFF8
  auto r =
      RunEncode(InstrEncoding::kLink, 0x4E50, kSzNone, SizeSpec::kNone, {MakeAn(0), MakeImm(-8)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 2u);
  EXPECT_EQ(r.words[0], 0x4E50u);
  EXPECT_EQ(r.words[1], static_cast<uint16_t>(-8));
}

// ---------------------------------------------------------------------------
// Encode — kAddxSubx, kAbcdSbcd, kCmpm, kExg
// ---------------------------------------------------------------------------

TEST(EncodeTest, AddxWordDnForm) {
  // ADDX.W D1,D2: 0xD100|0x40|(2<<9)|1 = 0xD541
  auto r =
      RunEncode(InstrEncoding::kAddxSubx, 0xD100, kSzBWL, SizeSpec::kWord, {MakeDn(1), MakeDn(2)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xD541u);
}

TEST(EncodeTest, AddxWordPreForm) {
  // ADDX.W -(A1),-(A2): 0xD100|0x40|(2<<9)|0x0008|1 = 0xD549
  auto r = RunEncode(InstrEncoding::kAddxSubx, 0xD100, kSzBWL, SizeSpec::kWord,
                     {MakeAnPre(1), MakeAnPre(2)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xD549u);
}

TEST(EncodeTest, AbcdDnForm) {
  // ABCD D0,D1: 0xC100|(1<<9)|0 = 0xC300
  auto r =
      RunEncode(InstrEncoding::kAbcdSbcd, 0xC100, kSzByte, SizeSpec::kByte, {MakeDn(0), MakeDn(1)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xC300u);
}

TEST(EncodeTest, SbcdPreForm) {
  // SBCD -(A0),-(A1): 0x8100|(1<<9)|0x0008|0 = 0x8308
  auto r = RunEncode(InstrEncoding::kAbcdSbcd, 0x8100, kSzByte, SizeSpec::kByte,
                     {MakeAnPre(0), MakeAnPre(1)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0x8308u);
}

TEST(EncodeTest, CmpmWordA0A1) {
  // CMPM.W (A0)+,(A1)+: 0xB108|0x40|(1<<9)|0 = 0xB348
  auto r = RunEncode(InstrEncoding::kCmpm, 0xB108, kSzBWL, SizeSpec::kWord,
                     {MakeAnPost(0), MakeAnPost(1)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xB348u);
}

TEST(EncodeTest, ExgDataData) {
  // EXG D0,D1 (data-data): 0xC140|0|(0<<9)|1 = 0xC141
  auto r = RunEncode(InstrEncoding::kExg, 0xC140, kSzLong, SizeSpec::kLong, {MakeDn(0), MakeDn(1)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xC141u);
}

TEST(EncodeTest, ExgAddrAddr) {
  // EXG A0,A1 (addr-addr): 0xC140|0x0008|(0<<9)|1 = 0xC149
  auto r = RunEncode(InstrEncoding::kExg, 0xC140, kSzLong, SizeSpec::kLong, {MakeAn(0), MakeAn(1)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xC149u);
}

TEST(EncodeTest, ExgDataAddr) {
  // EXG D0,A1 (data-addr): (0xC140&~0x40)|0x0088|(0<<9)|1 = 0xC189
  auto r = RunEncode(InstrEncoding::kExg, 0xC140, kSzLong, SizeSpec::kLong, {MakeDn(0), MakeAn(1)});
  EXPECT_TRUE(r.ok);
  ASSERT_EQ(r.words.size(), 1u);
  EXPECT_EQ(r.words[0], 0xC189u);
}

}  // namespace
}  // namespace easym68k::asm_
