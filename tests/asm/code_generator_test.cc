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

}  // namespace
}  // namespace easym68k::asm_
