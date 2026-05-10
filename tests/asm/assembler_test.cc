// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/asm/assembler.h"

#include <gtest/gtest.h>

namespace easym68k::asm_ {
namespace {

// ---------------------------------------------------------------------------
// NOP
// ---------------------------------------------------------------------------

TEST(AssemblerTest, NopEncoding) {
  Assembler a;
  auto r = a.Assemble("    ORG $1000\n    NOP\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 2u);
  EXPECT_EQ(r.code[0], 0x4E);
  EXPECT_EQ(r.code[1], 0x71);
}

TEST(AssemblerTest, TwoNops) {
  Assembler a;
  auto r = a.Assemble("    ORG $1000\n    NOP\n    NOP\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 4u);
  EXPECT_EQ(r.code[0], 0x4E);
  EXPECT_EQ(r.code[1], 0x71);
  EXPECT_EQ(r.code[2], 0x4E);
  EXPECT_EQ(r.code[3], 0x71);
}

// ---------------------------------------------------------------------------
// ORG
// ---------------------------------------------------------------------------

TEST(AssemblerTest, OrgSetsOrigin) {
  Assembler a;
  auto r = a.Assemble("    ORG $2000\n    NOP\n");
  EXPECT_TRUE(r.success);
  EXPECT_EQ(r.org, 0x2000u);
}

TEST(AssemblerTest, OrgDefaultsToZeroWithoutDirective) {
  Assembler a;
  auto r = a.Assemble("    NOP\n");
  EXPECT_TRUE(r.success);
  EXPECT_EQ(r.org, 0u);
}

// ---------------------------------------------------------------------------
// Labels
// ---------------------------------------------------------------------------

TEST(AssemblerTest, LabelAtInstruction) {
  // FOO is defined at the NOP address ($1000).
  // A subsequent DC.W FOO must resolve to $1000.
  Assembler a;
  auto r = a.Assemble(
      "    ORG $1000\n"
      "FOO: NOP\n"
      "    DC.W FOO\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 4u);
  // Bytes 0-1: NOP = 4E 71
  EXPECT_EQ(r.code[0], 0x4E);
  EXPECT_EQ(r.code[1], 0x71);
  // Bytes 2-3: DC.W FOO = $1000
  EXPECT_EQ(r.code[2], 0x10);
  EXPECT_EQ(r.code[3], 0x00);
}

TEST(AssemblerTest, ForwardLabelInDcw) {
  // DC.W DONE where DONE is defined two bytes later.
  Assembler a;
  auto r = a.Assemble(
      "    ORG $1000\n"
      "    DC.W DONE\n"
      "    NOP\n"
      "DONE: NOP\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 6u);
  // DC.W DONE: DONE = $1000+2+2 = $1004
  EXPECT_EQ(r.code[0], 0x10);
  EXPECT_EQ(r.code[1], 0x04);
  // NOP
  EXPECT_EQ(r.code[2], 0x4E);
  EXPECT_EQ(r.code[3], 0x71);
  // NOP at DONE
  EXPECT_EQ(r.code[4], 0x4E);
  EXPECT_EQ(r.code[5], 0x71);
}

TEST(AssemblerTest, LabelWithColon) {
  Assembler a;
  auto r = a.Assemble(
      "    ORG $1000\n"
      "START:\n"
      "    DC.W START\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 2u);
  EXPECT_EQ(r.code[0], 0x10);
  EXPECT_EQ(r.code[1], 0x00);
}

// ---------------------------------------------------------------------------
// MOVEQ
// ---------------------------------------------------------------------------

TEST(AssemblerTest, MoveqPositive) {
  // MOVEQ #5,D0 → 0x7005
  Assembler a;
  auto r = a.Assemble("    ORG $1000\n    MOVEQ #5,D0\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 2u);
  EXPECT_EQ(r.code[0], 0x70);
  EXPECT_EQ(r.code[1], 0x05);
}

TEST(AssemblerTest, MoveqNegative) {
  // MOVEQ #-1,D7 → 0x7EFF
  Assembler a;
  auto r = a.Assemble("    ORG $1000\n    MOVEQ #-1,D7\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 2u);
  EXPECT_EQ(r.code[0], 0x7E);
  EXPECT_EQ(r.code[1], 0xFF);
}

TEST(AssemblerTest, MoveqAllRegisters) {
  // Encoding bit pattern: 0111 DDD0 iiiiiiii
  // MOVEQ #1,D3 → 0x7601
  Assembler a;
  auto r = a.Assemble("    ORG $1000\n    MOVEQ #1,D3\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 2u);
  // D3 = 011 → bits 10-8 = 110 → 0x7601 → hi=0x76 lo=0x01
  EXPECT_EQ(r.code[0], 0x76);
  EXPECT_EQ(r.code[1], 0x01);
}

// ---------------------------------------------------------------------------
// DC.W
// ---------------------------------------------------------------------------

TEST(AssemblerTest, DcwSingleValue) {
  Assembler a;
  auto r = a.Assemble("    ORG $1000\n    DC.W $ABCD\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 2u);
  EXPECT_EQ(r.code[0], 0xAB);
  EXPECT_EQ(r.code[1], 0xCD);
}

TEST(AssemblerTest, DcwMultipleValues) {
  Assembler a;
  auto r = a.Assemble("    ORG $1000\n    DC.W $1234,$5678\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 4u);
  EXPECT_EQ(r.code[0], 0x12);
  EXPECT_EQ(r.code[1], 0x34);
  EXPECT_EQ(r.code[2], 0x56);
  EXPECT_EQ(r.code[3], 0x78);
}

TEST(AssemblerTest, DcbSingleValue) {
  Assembler a;
  auto r = a.Assemble("    ORG $1000\n    DC.B $42\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 1u);
  EXPECT_EQ(r.code[0], 0x42);
}

TEST(AssemblerTest, DclSingleValue) {
  Assembler a;
  auto r = a.Assemble("    ORG $1000\n    DC.L $12345678\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 4u);
  EXPECT_EQ(r.code[0], 0x12);
  EXPECT_EQ(r.code[1], 0x34);
  EXPECT_EQ(r.code[2], 0x56);
  EXPECT_EQ(r.code[3], 0x78);
}

// ---------------------------------------------------------------------------
// END
// ---------------------------------------------------------------------------

TEST(AssemblerTest, EndStopsAssembly) {
  // Code after END is not assembled.
  Assembler a;
  auto r = a.Assemble(
      "    ORG $1000\n"
      "    NOP\n"
      "    END\n"
      "    NOP\n");  // This NOP should not be assembled
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 2u);  // Only the first NOP
}

// ---------------------------------------------------------------------------
// EQU
// ---------------------------------------------------------------------------

TEST(AssemblerTest, EquConstantInExpression) {
  Assembler a;
  auto r = a.Assemble(
      "SIZE    EQU $10\n"
      "    ORG $1000\n"
      "    DC.W SIZE\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 2u);
  EXPECT_EQ(r.code[0], 0x00);
  EXPECT_EQ(r.code[1], 0x10);
}

TEST(AssemblerTest, EquUsedInArithmetic) {
  Assembler a;
  auto r = a.Assemble(
      "BASE    EQU $1000\n"
      "    ORG BASE\n"
      "    DC.W BASE+4\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 2u);
  EXPECT_EQ(r.code[0], 0x10);
  EXPECT_EQ(r.code[1], 0x04);
}

// ---------------------------------------------------------------------------
// Comments and blank lines
// ---------------------------------------------------------------------------

TEST(AssemblerTest, CommentLinesIgnored) {
  Assembler a;
  auto r = a.Assemble(
      "* This is a comment\n"
      "    ORG $1000\n"
      "; Another comment\n"
      "    NOP\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 2u);
}

TEST(AssemblerTest, BlankLinesIgnored) {
  Assembler a;
  auto r = a.Assemble(
      "\n"
      "    ORG $1000\n"
      "\n"
      "    NOP\n"
      "\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 2u);
}

// ---------------------------------------------------------------------------
// SET
// ---------------------------------------------------------------------------

TEST(AssemblerTest, SetDirectiveRedefinable) {
  Assembler a;
  auto r = a.Assemble(
      "COUNT   SET 1\n"
      "COUNT   SET 2\n"
      "    ORG $1000\n"
      "    DC.W COUNT\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 2u);
  EXPECT_EQ(r.code[0], 0x00);
  EXPECT_EQ(r.code[1], 0x02);
}

// ---------------------------------------------------------------------------
// DS
// ---------------------------------------------------------------------------

TEST(AssemblerTest, DswReservesSpace) {
  // DS.W 3 reserves 6 bytes (3 words).
  Assembler a;
  auto r = a.Assemble(
      "    ORG $1000\n"
      "    DS.W 3\n"
      "    DC.W $BEEF\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 8u);
  // Bytes 0-5: zeroed storage
  EXPECT_EQ(r.code[0], 0x00);
  EXPECT_EQ(r.code[5], 0x00);
  // Bytes 6-7: DC.W $BEEF
  EXPECT_EQ(r.code[6], 0xBE);
  EXPECT_EQ(r.code[7], 0xEF);
}

TEST(AssemblerTest, DswAutoAlignsAfterOddByte) {
  // DC.B $01 leaves LC at odd address; DS.W must word-align before reserving.
  // DIRECTIV.CPP:529-533: loc++ before DS.W/L if loc is odd.
  Assembler a;
  auto r = a.Assemble(
      "    ORG $1000\n"
      "    DC.B $01\n"    // 1 byte: LC = $1001 (odd)
      "    DS.W 1\n"      // word-align pad + 2 reserved bytes
      "    DC.B $FF\n");  // 1 byte
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 5u);  // $01 + pad + $00 $00 + $FF
  EXPECT_EQ(r.code[0], 0x01);    // DC.B
  EXPECT_EQ(r.code[1], 0x00);    // alignment pad
  EXPECT_EQ(r.code[2], 0x00);    // DS.W reserved byte 0
  EXPECT_EQ(r.code[3], 0x00);    // DS.W reserved byte 1
  EXPECT_EQ(r.code[4], 0xFF);    // DC.B $FF
}

TEST(AssemblerTest, DcwAutoAlignsAfterOddByte) {
  // DC.B $AB leaves LC odd; DC.W must insert a pad byte first.
  // DIRECTIV.CPP:351-355: loc++ before DC.W/L if loc is odd.
  Assembler a;
  auto r = a.Assemble(
      "    ORG $1000\n"
      "    DC.B $AB\n"      // 1 byte: LC = $1001 (odd)
      "    DC.W $1234\n");  // pad + word
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 4u);
  EXPECT_EQ(r.code[0], 0xAB);  // DC.B
  EXPECT_EQ(r.code[1], 0x00);  // alignment pad
  EXPECT_EQ(r.code[2], 0x12);  // DC.W high
  EXPECT_EQ(r.code[3], 0x34);  // DC.W low
}

TEST(AssemblerTest, DclAutoAlignsAfterOddByte) {
  Assembler a;
  auto r = a.Assemble(
      "    ORG $1000\n"
      "    DC.B $FF\n"
      "    DC.L $12345678\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 6u);
  EXPECT_EQ(r.code[0], 0xFF);
  EXPECT_EQ(r.code[1], 0x00);  // pad
  EXPECT_EQ(r.code[2], 0x12);
  EXPECT_EQ(r.code[3], 0x34);
  EXPECT_EQ(r.code[4], 0x56);
  EXPECT_EQ(r.code[5], 0x78);
}

TEST(AssemblerTest, DcbsAreContiguousNoAlignment) {
  // DC.B's must NOT trigger word alignment between them (original comment:
  // "so DC.B's can be contiguous").
  Assembler a;
  auto r = a.Assemble(
      "    ORG $1000\n"
      "    DC.B $01,$02,$03\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 3u);
  EXPECT_EQ(r.code[0], 0x01);
  EXPECT_EQ(r.code[1], 0x02);
  EXPECT_EQ(r.code[2], 0x03);
}

TEST(AssemblerTest, DsbReservesBytes) {
  Assembler a;
  auto r = a.Assemble(
      "    ORG $1000\n"
      "    DS.B 4\n"
      "    DC.B $FF\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 5u);
  EXPECT_EQ(r.code[4], 0xFF);
}

// ---------------------------------------------------------------------------
// EVEN
// ---------------------------------------------------------------------------

TEST(AssemblerTest, EvenAlignsOddAddress) {
  // DC.B $01 leaves LC at an odd address; EVEN adds a pad byte.
  Assembler a;
  auto r = a.Assemble(
      "    ORG $1000\n"
      "    DC.B $01\n"
      "    EVEN\n"
      "    DC.W $BEEF\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 4u);
  EXPECT_EQ(r.code[0], 0x01);  // DC.B
  EXPECT_EQ(r.code[1], 0x00);  // EVEN pad
  EXPECT_EQ(r.code[2], 0xBE);  // DC.W high
  EXPECT_EQ(r.code[3], 0xEF);  // DC.W low
}

TEST(AssemblerTest, EvenOnAlignedAddressIsNoop) {
  Assembler a;
  auto r = a.Assemble(
      "    ORG $1000\n"
      "    DC.W $1234\n"  // already word-aligned after this
      "    EVEN\n"
      "    DC.W $5678\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 4u);
}

// ---------------------------------------------------------------------------
// DC.B string literal
// ---------------------------------------------------------------------------

TEST(AssemblerTest, DcbCharLiteral) {
  Assembler a;
  auto r = a.Assemble("    ORG $1000\n    DC.B 'A'\n");
  EXPECT_TRUE(r.success);
  ASSERT_EQ(r.code.size(), 1u);
  EXPECT_EQ(r.code[0], 0x41);
}

// ---------------------------------------------------------------------------
// Error cases
// ---------------------------------------------------------------------------

TEST(AssemblerTest, UnknownInstructionError) {
  Assembler a;
  auto r = a.Assemble("    ORG $1000\n    BOGUS\n");
  EXPECT_FALSE(r.success);
  ASSERT_FALSE(r.errors.empty());
  EXPECT_NE(r.errors[0].message.find("BOGUS"), std::string::npos);
}

TEST(AssemblerTest, EmptySourceSucceeds) {
  Assembler a;
  auto r = a.Assemble("");
  EXPECT_TRUE(r.success);
  EXPECT_TRUE(r.code.empty());
}

TEST(AssemblerTest, OnlyCommentsSucceeds) {
  Assembler a;
  auto r = a.Assemble("* just a comment\n; another comment\n");
  EXPECT_TRUE(r.success);
  EXPECT_TRUE(r.code.empty());
}

}  // namespace
}  // namespace easym68k::asm_
