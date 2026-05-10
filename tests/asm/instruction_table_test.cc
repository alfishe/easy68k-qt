// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/asm/instruction_table.h"

#include <gtest/gtest.h>

namespace easym68k::asm_ {
namespace {

// ---------------------------------------------------------------------------
// Lookup: kFixed encoding
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupNop) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("NOP");
  ASSERT_NE(e, nullptr);
  EXPECT_STREQ(e->name, "NOP");
  EXPECT_EQ(e->encoding, InstrEncoding::kFixed);
  EXPECT_EQ(e->nops, 0);
  EXPECT_EQ(e->sizes, kSzNone);
  EXPECT_EQ(e->base, 0x4E71);
}

TEST(InstrTableTest, LookupRts) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("RTS");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kFixed);
  EXPECT_EQ(e->base, 0x4E75);
}

TEST(InstrTableTest, LookupIllegal) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("ILLEGAL");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kFixed);
  EXPECT_EQ(e->base, 0x4AFC);
}

TEST(InstrTableTest, LookupReset) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("RESET");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kFixed);
  EXPECT_EQ(e->base, 0x4E70);
}

TEST(InstrTableTest, LookupTrapv) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("TRAPV");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kFixed);
  EXPECT_EQ(e->base, 0x4E76);
}

// ---------------------------------------------------------------------------
// Lookup: kFixedWord encoding
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupStop) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("STOP");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kFixedWord);
  EXPECT_EQ(e->nops, 1);
  EXPECT_EQ(e->sizes, kSzNone);
  EXPECT_EQ(e->base, 0x4E72);
}

// ---------------------------------------------------------------------------
// Lookup: kRegOp encoding
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupSwap) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("SWAP");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kRegOp);
  EXPECT_EQ(e->sizes, kSzWord);
  EXPECT_EQ(e->nops, 1);
  EXPECT_EQ(e->base, 0x4840);
}

TEST(InstrTableTest, LookupExt) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("EXT");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kRegOp);
  EXPECT_EQ(e->sizes, kSzWL);
  EXPECT_EQ(e->base, 0x4880);
}

// ---------------------------------------------------------------------------
// Lookup: kAnOp encoding
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupUnlk) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("UNLK");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kAnOp);
  EXPECT_EQ(e->nops, 1);
  EXPECT_EQ(e->base, 0x4E58);
}

// ---------------------------------------------------------------------------
// Lookup: kEaOnly encoding
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupClr) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("CLR");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kEaOnly);
  EXPECT_EQ(e->sizes, kSzBWL);
  EXPECT_EQ(e->nops, 1);
  EXPECT_EQ(e->base, 0x4200);
}

TEST(InstrTableTest, LookupPea) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("PEA");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kEaOnly);
  EXPECT_EQ(e->sizes, kSzLong);
  EXPECT_EQ(e->base, 0x4840);
}

// ---------------------------------------------------------------------------
// Lookup: kEaBidirect encoding
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupAdd) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("ADD");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kEaBidirect);
  EXPECT_EQ(e->sizes, kSzBWL);
  EXPECT_EQ(e->nops, 2);
  EXPECT_EQ(e->base, 0xD000);
}

TEST(InstrTableTest, LookupOr) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("OR");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kEaBidirect);
  EXPECT_EQ(e->base, 0x8000);
}

// ---------------------------------------------------------------------------
// Lookup: kEaToReg encoding
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupCmp) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("CMP");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kEaToReg);
  EXPECT_EQ(e->sizes, kSzBWL);
  EXPECT_EQ(e->nops, 2);
  EXPECT_EQ(e->base, 0xB000);
}

// ---------------------------------------------------------------------------
// Lookup: kRegToEa encoding
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupEor) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("EOR");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kRegToEa);
  EXPECT_EQ(e->base, 0xB100);
}

// ---------------------------------------------------------------------------
// Lookup: kAddrEa encoding
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupAdda) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("ADDA");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kAddrEa);
  EXPECT_EQ(e->sizes, kSzWL);
  EXPECT_EQ(e->base, 0xD0C0);
}

TEST(InstrTableTest, LookupCmpa) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("CMPA");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kAddrEa);
  EXPECT_EQ(e->base, 0xB0C0);
}

// ---------------------------------------------------------------------------
// Lookup: kMove / kMoveA / kMoveM / kMoveP / kMoveq
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupMove) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("MOVE");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kMove);
  EXPECT_EQ(e->sizes, kSzBWL);
  EXPECT_EQ(e->base, 0x1000);
}

TEST(InstrTableTest, LookupMovea) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("MOVEA");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kMoveA);
  EXPECT_EQ(e->sizes, kSzWL);
  EXPECT_EQ(e->base, 0x3000);
}

TEST(InstrTableTest, LookupMovem) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("MOVEM");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kMoveM);
  EXPECT_EQ(e->sizes, kSzWL);
}

TEST(InstrTableTest, LookupMovep) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("MOVEP");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kMoveP);
  EXPECT_EQ(e->sizes, kSzWL);
  EXPECT_EQ(e->base, 0x0188);
}

TEST(InstrTableTest, LookupMoveq) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("MOVEQ");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kMoveq);
  EXPECT_EQ(e->nops, 2);
  EXPECT_EQ(e->base, 0x7000);
}

// ---------------------------------------------------------------------------
// Lookup: kImmedToEa encoding
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupAddi) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("ADDI");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kImmedToEa);
  EXPECT_EQ(e->sizes, kSzBWL);
  EXPECT_EQ(e->nops, 2);
  EXPECT_EQ(e->base, 0x0600);
}

TEST(InstrTableTest, LookupCmpi) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("CMPI");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kImmedToEa);
  EXPECT_EQ(e->base, 0x0C00);
}

// ---------------------------------------------------------------------------
// Lookup: kQuick encoding
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupAddq) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("ADDQ");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kQuick);
  EXPECT_EQ(e->sizes, kSzBWL);
  EXPECT_EQ(e->base, 0x5000);
}

TEST(InstrTableTest, LookupSubq) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("SUBQ");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kQuick);
  EXPECT_EQ(e->base, 0x5100);
}

// ---------------------------------------------------------------------------
// Lookup: kBranch encoding + condition codes
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupBra) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("BRA");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kBranch);
  EXPECT_EQ(e->cond, 0);
  EXPECT_EQ(e->base, 0x6000);
  EXPECT_EQ(e->nops, 1);
}

TEST(InstrTableTest, LookupBsr) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("BSR");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kBranch);
  EXPECT_EQ(e->cond, 0);
  EXPECT_EQ(e->base, 0x6100);
}

TEST(InstrTableTest, LookupBeq) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("BEQ");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kBranch);
  EXPECT_EQ(e->cond, 7);
  EXPECT_EQ(e->base, 0x6700);
}

TEST(InstrTableTest, LookupBne) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("BNE");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->cond, 6);
  EXPECT_EQ(e->base, 0x6600);
}

TEST(InstrTableTest, LookupBge) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("BGE");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->cond, 12);
  EXPECT_EQ(e->base, 0x6C00);
}

// ---------------------------------------------------------------------------
// Lookup: kDBcc encoding + condition codes
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupDbf) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("DBF");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kDBcc);
  EXPECT_EQ(e->cond, 1);
  EXPECT_EQ(e->base, 0x51C8);
  EXPECT_EQ(e->nops, 2);
}

TEST(InstrTableTest, LookupDbra) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("DBRA");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kDBcc);
  EXPECT_EQ(e->cond, 1);
  EXPECT_EQ(e->base, 0x51C8);
}

TEST(InstrTableTest, LookupDbeq) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("DBEQ");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->cond, 7);
  EXPECT_EQ(e->base, 0x57C8);
}

// ---------------------------------------------------------------------------
// Lookup: kScc encoding + condition codes
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupSt) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("ST");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kScc);
  EXPECT_EQ(e->cond, 0);
  EXPECT_EQ(e->base, 0x50C0);
}

TEST(InstrTableTest, LookupSeq) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("SEQ");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kScc);
  EXPECT_EQ(e->cond, 7);
  EXPECT_EQ(e->base, 0x57C0);
}

// ---------------------------------------------------------------------------
// Lookup: kShift encoding
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupAsl) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("ASL");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kShift);
  EXPECT_EQ(e->sizes, kSzBWL);
  EXPECT_EQ(e->base, 0xE160);
}

TEST(InstrTableTest, LookupRol) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("ROL");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kShift);
  EXPECT_EQ(e->base, 0xE178);
}

// ---------------------------------------------------------------------------
// Lookup: kBitReg encoding
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupBtst) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("BTST");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kBitReg);
  EXPECT_EQ(e->nops, 2);
  EXPECT_EQ(e->base, 0x0100);
}

// ---------------------------------------------------------------------------
// Lookup: kMulDiv / kLea / kJmpJsr / kChk / kLink / kTrap
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupMuls) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("MULS");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kMulDiv);
  EXPECT_EQ(e->sizes, kSzWord);
  EXPECT_EQ(e->base, 0xC1C0);
}

TEST(InstrTableTest, LookupDivs) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("DIVS");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kMulDiv);
  EXPECT_EQ(e->base, 0x81C0);
}

TEST(InstrTableTest, LookupLea) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("LEA");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kLea);
  EXPECT_EQ(e->sizes, kSzLong);
  EXPECT_EQ(e->base, 0x41C0);
}

TEST(InstrTableTest, LookupJmp) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("JMP");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kJmpJsr);
  EXPECT_EQ(e->sizes, kSzNone);
  EXPECT_EQ(e->base, 0x4EC0);
}

TEST(InstrTableTest, LookupJsr) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("JSR");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kJmpJsr);
  EXPECT_EQ(e->base, 0x4E80);
}

TEST(InstrTableTest, LookupChk) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("CHK");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kChk);
  EXPECT_EQ(e->sizes, kSzWord);
  EXPECT_EQ(e->base, 0x4180);
}

TEST(InstrTableTest, LookupLink) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("LINK");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kLink);
  EXPECT_EQ(e->nops, 2);
  EXPECT_EQ(e->base, 0x4E50);
}

TEST(InstrTableTest, LookupTrap) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("TRAP");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kTrap);
  EXPECT_EQ(e->sizes, kSzNone);
  EXPECT_EQ(e->nops, 1);
  EXPECT_EQ(e->base, 0x4E40);
}

// ---------------------------------------------------------------------------
// Lookup: kAddxSubx / kAbcdSbcd / kCmpm / kExg
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupAddx) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("ADDX");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kAddxSubx);
  EXPECT_EQ(e->sizes, kSzBWL);
  EXPECT_EQ(e->base, 0xD100);
}

TEST(InstrTableTest, LookupAbcd) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("ABCD");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kAbcdSbcd);
  EXPECT_EQ(e->sizes, kSzByte);
  EXPECT_EQ(e->base, 0xC100);
}

TEST(InstrTableTest, LookupCmpm) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("CMPM");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kCmpm);
  EXPECT_EQ(e->base, 0xB108);
}

TEST(InstrTableTest, LookupExg) {
  InstructionTable table;
  const InstrEntry* e = table.Lookup("EXG");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(e->encoding, InstrEncoding::kExg);
  EXPECT_EQ(e->sizes, kSzLong);
  EXPECT_EQ(e->base, 0xC140);
}

// ---------------------------------------------------------------------------
// Negative lookup tests
// ---------------------------------------------------------------------------

TEST(InstrTableTest, LookupUnknownReturnsNull) {
  InstructionTable table;
  EXPECT_EQ(table.Lookup("BOGUS"), nullptr);
}

TEST(InstrTableTest, LookupLowercaseReturnsNull) {
  InstructionTable table;
  EXPECT_EQ(table.Lookup("nop"), nullptr);
}

TEST(InstrTableTest, LookupEmptyReturnsNull) {
  InstructionTable table;
  EXPECT_EQ(table.Lookup(""), nullptr);
}

// ---------------------------------------------------------------------------
// ExtWordCount tests
// ---------------------------------------------------------------------------

TEST(InstrTableTest, ExtWordCountZeroExtModes) {
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kDnDirect, SizeSpec::kWord), 0);
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kAnDirect, SizeSpec::kWord), 0);
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kAnIndirect, SizeSpec::kWord), 0);
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kAnIndirectPost, SizeSpec::kWord), 0);
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kAnIndirectPre, SizeSpec::kWord), 0);
}

TEST(InstrTableTest, ExtWordCountAnIndirectDisp) {
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kAnIndirectDisp, SizeSpec::kByte), 1);
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kAnIndirectDisp, SizeSpec::kWord), 1);
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kAnIndirectDisp, SizeSpec::kLong), 1);
}

TEST(InstrTableTest, ExtWordCountAnIndirectIdx) {
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kAnIndirectIdx, SizeSpec::kByte), 1);
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kAnIndirectIdx, SizeSpec::kLong), 1);
}

TEST(InstrTableTest, ExtWordCountAbsShort) {
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kAbsShort, SizeSpec::kWord), 1);
}

TEST(InstrTableTest, ExtWordCountAbsLong) {
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kAbsLong, SizeSpec::kByte), 2);
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kAbsLong, SizeSpec::kWord), 2);
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kAbsLong, SizeSpec::kLong), 2);
}

TEST(InstrTableTest, ExtWordCountPCRel) {
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kPCDisp, SizeSpec::kWord), 1);
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kPCIndex, SizeSpec::kWord), 1);
}

TEST(InstrTableTest, ExtWordCountImmediateByte) {
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kImmediate, SizeSpec::kByte), 1);
}

TEST(InstrTableTest, ExtWordCountImmediateWord) {
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kImmediate, SizeSpec::kWord), 1);
}

TEST(InstrTableTest, ExtWordCountImmediateLong) {
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kImmediate, SizeSpec::kLong), 2);
}

TEST(InstrTableTest, ExtWordCountSrCcrUspZero) {
  // SR/CCR/USP are encoded in the opcode itself — no extension words.
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kSRDirect, SizeSpec::kWord), 0);
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kCCRDirect, SizeSpec::kWord), 0);
  EXPECT_EQ(InstructionTable::ExtWordCount(AddressMode::kUSPDirect, SizeSpec::kWord), 0);
}

// ---------------------------------------------------------------------------
// SizeField tests
// ---------------------------------------------------------------------------

TEST(InstrTableTest, SizeFieldByte) {
  EXPECT_EQ(InstructionTable::SizeField(SizeSpec::kByte), 0);
}

TEST(InstrTableTest, SizeFieldWord) {
  EXPECT_EQ(InstructionTable::SizeField(SizeSpec::kWord), 1);
}

TEST(InstrTableTest, SizeFieldLong) {
  EXPECT_EQ(InstructionTable::SizeField(SizeSpec::kLong), 2);
}

TEST(InstrTableTest, SizeFieldNoneIsNegOne) {
  EXPECT_EQ(InstructionTable::SizeField(SizeSpec::kNone), -1);
}

TEST(InstrTableTest, SizeFieldShortIsNegOne) {
  EXPECT_EQ(InstructionTable::SizeField(SizeSpec::kShort), -1);
}

// ---------------------------------------------------------------------------
// Mode mask constant tests
// ---------------------------------------------------------------------------

TEST(InstrTableTest, ModeMaskValues) {
  EXPECT_EQ(kModeD, 0x001u);
  EXPECT_EQ(kModeA, 0x002u);
  EXPECT_EQ(kModeInd, 0x004u);
  EXPECT_EQ(kModePost, 0x008u);
  EXPECT_EQ(kModePre, 0x010u);
  EXPECT_EQ(kModeDisp, 0x020u);
  EXPECT_EQ(kModeIdx, 0x040u);
  EXPECT_EQ(kModeAbsW, 0x080u);
  EXPECT_EQ(kModeAbsL, 0x100u);
  EXPECT_EQ(kModePCDisp, 0x200u);
  EXPECT_EQ(kModePCIdx, 0x400u);
  EXPECT_EQ(kModeImm, 0x800u);
}

TEST(InstrTableTest, ModeMaskSrCcrUsp) {
  EXPECT_EQ(kModeSR, 0x1000u);
  EXPECT_EQ(kModeCCR, 0x2000u);
  EXPECT_EQ(kModeUSP, 0x4000u);
}

// ---------------------------------------------------------------------------
// Compound mask membership tests
// ---------------------------------------------------------------------------

TEST(InstrTableTest, CompoundMaskAbs) {
  EXPECT_EQ(kModeAbs, kModeAbsW | kModeAbsL);
}

TEST(InstrTableTest, CompoundMaskPCRel) {
  EXPECT_EQ(kModePCRel, kModePCDisp | kModePCIdx);
}

TEST(InstrTableTest, CompoundMaskMemContainsIndirectModes) {
  EXPECT_TRUE(kModeMem & kModeInd);
  EXPECT_TRUE(kModeMem & kModePost);
  EXPECT_TRUE(kModeMem & kModePre);
  EXPECT_TRUE(kModeMem & kModeDisp);
  EXPECT_TRUE(kModeMem & kModeIdx);
  EXPECT_TRUE(kModeMem & kModeAbsW);
  EXPECT_TRUE(kModeMem & kModeAbsL);
  // kModeMem must NOT include Dn, An, PC-relative, or immediate.
  EXPECT_FALSE(kModeMem & kModeD);
  EXPECT_FALSE(kModeMem & kModeA);
  EXPECT_FALSE(kModeMem & kModePCDisp);
  EXPECT_FALSE(kModeMem & kModeImm);
}

TEST(InstrTableTest, CompoundMaskControlContainsExpectedModes) {
  // Control = (An) + d(An) + d(An,Xi) + Abs.W + Abs.L + d(PC) + d(PC,Xi)
  EXPECT_TRUE(kModeControl & kModeInd);
  EXPECT_TRUE(kModeControl & kModeDisp);
  EXPECT_TRUE(kModeControl & kModeIdx);
  EXPECT_TRUE(kModeControl & kModeAbsW);
  EXPECT_TRUE(kModeControl & kModeAbsL);
  EXPECT_TRUE(kModeControl & kModePCDisp);
  EXPECT_TRUE(kModeControl & kModePCIdx);
  // Control must NOT include (An)+, -(An), Dn, An, #imm.
  EXPECT_FALSE(kModeControl & kModePost);
  EXPECT_FALSE(kModeControl & kModePre);
  EXPECT_FALSE(kModeControl & kModeD);
  EXPECT_FALSE(kModeControl & kModeA);
  EXPECT_FALSE(kModeControl & kModeImm);
}

TEST(InstrTableTest, CompoundMaskDataAlt) {
  // DataAlt = Dn + all memory-alterable modes (no PC-relative, no immediate).
  EXPECT_TRUE(kModeDataAlt & kModeD);
  EXPECT_TRUE(kModeDataAlt & kModeInd);
  EXPECT_TRUE(kModeDataAlt & kModeAbsL);
  EXPECT_FALSE(kModeDataAlt & kModeA);
  EXPECT_FALSE(kModeDataAlt & kModePCDisp);
  EXPECT_FALSE(kModeDataAlt & kModeImm);
}

// ---------------------------------------------------------------------------
// Table completeness — every standard 68000 instruction must be found
// ---------------------------------------------------------------------------

TEST(InstrTableTest, AllInstructionsPresent) {
  InstructionTable table;
  static const char* const kAll[] = {
      "ABCD",  "ADD",  "ADDA", "ADDI", "ADDQ", "ADDX",  "AND",   "ANDI",  "ASL",     "ASR",  "BCC",
      "BCHG",  "BCLR", "BCS",  "BEQ",  "BGE",  "BGT",   "BHI",   "BHS",   "BLE",     "BLO",  "BLS",
      "BLT",   "BMI",  "BNE",  "BPL",  "BRA",  "BSET",  "BSR",   "BTST",  "BVC",     "BVS",  "CHK",
      "CLR",   "CMP",  "CMPA", "CMPI", "CMPM", "DBCC",  "DBCS",  "DBEQ",  "DBF",     "DBGE", "DBGT",
      "DBHI",  "DBHS", "DBLE", "DBLO", "DBLS", "DBLT",  "DBMI",  "DBNE",  "DBPL",    "DBRA", "DBT",
      "DBVC",  "DBVS", "DIVS", "DIVU", "EOR",  "EORI",  "EXG",   "EXT",   "ILLEGAL", "JMP",  "JSR",
      "LEA",   "LINK", "LSL",  "LSR",  "MOVE", "MOVEA", "MOVEM", "MOVEP", "MOVEQ",   "MULS", "MULU",
      "NBCD",  "NEG",  "NEGX", "NOP",  "NOT",  "OR",    "ORI",   "PEA",   "RESET",   "ROL",  "ROR",
      "ROXL",  "ROXR", "RTE",  "RTR",  "RTS",  "SBCD",  "SCC",   "SCS",   "SEQ",     "SF",   "SGE",
      "SGT",   "SHI",  "SHS",  "SLE",  "SLO",  "SLS",   "SLT",   "SMI",   "SNE",     "SPL",  "ST",
      "STOP",  "SUB",  "SUBA", "SUBI", "SUBQ", "SUBX",  "SVC",   "SVS",   "SWAP",    "TAS",  "TRAP",
      "TRAPV", "TST",  "UNLK",
  };
  for (const char* name : kAll)
    EXPECT_NE(table.Lookup(name), nullptr) << "Missing instruction: " << name;
}

}  // namespace
}  // namespace easym68k::asm_
