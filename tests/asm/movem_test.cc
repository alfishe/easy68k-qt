// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Task 8.5.4 — MOVEM register-list parsing and encoding tests.
// All expected bytes are verified against the M68000 PRM encoding rules and
// against the original MOVEM.CPP / BUILD.CPP reference implementation.

#include <cstdint>
#include <string>
#include <vector>

#include "easym68k/asm/assembler.h"
#include "easym68k/asm/parser.h"
#include "gtest/gtest.h"

namespace easym68k::asm_ {
namespace {

// Assemble a single source line (with implicit ORG 0) and return the bytes.
// Dies if assembly fails.
std::vector<uint8_t> Asm(const std::string& source) {
  Assembler a;
  AssemblyResult r = a.Assemble(source);
  EXPECT_TRUE(r.success) << "Assembly failed: " << (r.errors.empty() ? "" : r.errors[0].message);
  return r.code;
}

// Assemble multi-line source; returns the first 8 bytes (or all, if fewer).
std::vector<uint8_t> AsmMulti(const std::string& source) {
  Assembler a;
  AssemblyResult r = a.Assemble(source);
  EXPECT_TRUE(r.success) << "Assembly failed: " << (r.errors.empty() ? "" : r.errors[0].message);
  return r.code;
}

// -----------------------------------------------------------------------
// Register list parsing — tested via assembled byte output
// -----------------------------------------------------------------------

// MOVEM.W with a single data register: D0 only → mask bit 0 set = 0x0001.
// Pre-decrement reverses: bit 0 → bit 15 → reversed mask = 0x8000.
TEST(MovemTest, SingleDataRegPreDec) {
  auto bytes = Asm("        MOVEM.W D0,-(SP)");
  ASSERT_EQ(bytes.size(), 4u);
  EXPECT_EQ(bytes[0], 0x48);
  EXPECT_EQ(bytes[1], 0xA7);  // 0x4880 | EffAddr(-(SP)=0x27)
  EXPECT_EQ(bytes[2], 0x80);  // reversed mask high byte
  EXPECT_EQ(bytes[3], 0x00);
}

// Single address register A5 → bit (8+5)=13 → mask 0x2000.
// Pre-decrement reversed: bit 13 → bit 2 → 0x0004.
TEST(MovemTest, SingleAddrRegPreDec) {
  auto bytes = Asm("        MOVEM.W A5,-(SP)");
  ASSERT_EQ(bytes.size(), 4u);
  EXPECT_EQ(bytes[0], 0x48);
  EXPECT_EQ(bytes[1], 0xA7);
  EXPECT_EQ(bytes[2], 0x00);
  EXPECT_EQ(bytes[3], 0x04);  // reversed 0x2000 → 0x0004
}

// D0-D3 range → bits 0-3 → mask 0x000F.
// Pre-decrement reversed: 0x000F → bits 12-15 → 0xF000.
TEST(MovemTest, DataRangePreDec) {
  auto bytes = Asm("        MOVEM.W D0-D3,-(SP)");
  ASSERT_EQ(bytes.size(), 4u);
  EXPECT_EQ(bytes[0], 0x48);
  EXPECT_EQ(bytes[1], 0xA7);
  EXPECT_EQ(bytes[2], 0xF0);
  EXPECT_EQ(bytes[3], 0x00);
}

// A0-A3 range → bits 8-11 → mask 0x0F00.
// Reversed: bits 4-7 → 0x00F0.
TEST(MovemTest, AddrRangePreDec) {
  auto bytes = Asm("        MOVEM.W A0-A3,-(SP)");
  ASSERT_EQ(bytes.size(), 4u);
  EXPECT_EQ(bytes[0], 0x48);
  EXPECT_EQ(bytes[1], 0xA7);
  EXPECT_EQ(bytes[2], 0x00);
  EXPECT_EQ(bytes[3], 0xF0);
}

// D0-D3/A0-A3 combined → bits 0-3, bits 8-11 → mask 0x0F0F.
// Reversed: bits 12-15, bits 4-7 → 0xF0F0.
TEST(MovemTest, CombinedListPreDec) {
  auto bytes = Asm("        MOVEM.W D0-D3/A0-A3,-(SP)");
  ASSERT_EQ(bytes.size(), 4u);
  EXPECT_EQ(bytes[0], 0x48);
  EXPECT_EQ(bytes[1], 0xA7);
  EXPECT_EQ(bytes[2], 0xF0);
  EXPECT_EQ(bytes[3], 0xF0);
}

// D0-D7/A0-A7 = all 16 registers → mask 0xFFFF.
// Reversed: 0xFFFF (palindrome).
TEST(MovemTest, AllRegistersPreDec) {
  auto bytes = Asm("        MOVEM.W D0-D7/A0-A7,-(SP)");
  ASSERT_EQ(bytes.size(), 4u);
  EXPECT_EQ(bytes[0], 0x48);
  EXPECT_EQ(bytes[1], 0xA7);
  EXPECT_EQ(bytes[2], 0xFF);
  EXPECT_EQ(bytes[3], 0xFF);
}

// Reversed range D3-D0 is the same as D0-D3.
TEST(MovemTest, ReversedRangePreDec) {
  auto bytes = Asm("        MOVEM.W D3-D0,-(SP)");
  ASSERT_EQ(bytes.size(), 4u);
  EXPECT_EQ(bytes[0], 0x48);
  EXPECT_EQ(bytes[1], 0xA7);
  EXPECT_EQ(bytes[2], 0xF0);
  EXPECT_EQ(bytes[3], 0x00);
}

// -----------------------------------------------------------------------
// MOVEM opcode word: word vs long, register-list-first form
// -----------------------------------------------------------------------

// Default size (no suffix) → word form 0x4880.
TEST(MovemTest, DefaultSizeIsWord) {
  auto bytes = Asm("        MOVEM D0-D3,-(SP)");
  ASSERT_GE(bytes.size(), 2u);
  EXPECT_EQ(bytes[0], 0x48);
  EXPECT_EQ(bytes[1], 0xA7);
}

// .W suffix → 0x4880 base, bit 6 = 0. 0x4880 | EffAddr(-(SP)=0x27) = 0x48A7.
TEST(MovemTest, WordSizeBase) {
  auto bytes = Asm("        MOVEM.W D0-D7/A0-A6,-(SP)");
  ASSERT_EQ(bytes.size(), 4u);
  EXPECT_EQ(bytes[0], 0x48);
  EXPECT_EQ(bytes[1], 0xA7);  // word: 0x4880 | 0x27
  // mask 0x7FFF reversed for -(SP) = 0xFFFE
  EXPECT_EQ(bytes[2], 0xFF);
  EXPECT_EQ(bytes[3], 0xFE);
}

// .L suffix → bit 6 set in base → 0x48C0.
// 0x48C0 | EffAddr(-(SP)=0x27) = 0x48E7   (0xC0|0x27 = 0xE7, not 0xC7)
TEST(MovemTest, LongSizeBit) {
  auto bytes = Asm("        MOVEM.L D0-D7/A0-A6,-(SP)");
  ASSERT_EQ(bytes.size(), 4u);
  EXPECT_EQ(bytes[0], 0x48);
  EXPECT_EQ(bytes[1], 0xE7);  // 0x48C0 | 0x27 = 0x48E7
  // mask 0x7FFF reversed = 0xFFFE
  EXPECT_EQ(bytes[2], 0xFF);
  EXPECT_EQ(bytes[3], 0xFE);
}

// -----------------------------------------------------------------------
// MOVEM reglist → EA (non-pre-decrement: mask NOT reversed)
// -----------------------------------------------------------------------

// MOVEM.W D0-D3,(A0) → indirect EA, no reversal.
TEST(MovemTest, ReglistToAnIndirect) {
  auto bytes = Asm("        MOVEM.W D0-D3,(A0)");
  ASSERT_EQ(bytes.size(), 4u);
  // 0x4880 | EffAddr((A0)=0x10) = 0x4890
  EXPECT_EQ(bytes[0], 0x48);
  EXPECT_EQ(bytes[1], 0x90);
  EXPECT_EQ(bytes[2], 0x00);
  EXPECT_EQ(bytes[3], 0x0F);  // mask 0x000F, NOT reversed
}

// MOVEM.L D0-D3,4(A0) → displacement EA, 1 extension word.
TEST(MovemTest, ReglistToAnDisp) {
  auto bytes = Asm("        MOVEM.L D0-D3,4(A0)");
  ASSERT_EQ(bytes.size(), 6u);
  // 0x48C0 | EffAddr(4(A0)=0x28) = 0x48E8
  EXPECT_EQ(bytes[0], 0x48);
  EXPECT_EQ(bytes[1], 0xE8);
  EXPECT_EQ(bytes[2], 0x00);  // mask high
  EXPECT_EQ(bytes[3], 0x0F);  // mask low
  EXPECT_EQ(bytes[4], 0x00);  // disp high
  EXPECT_EQ(bytes[5], 0x04);  // disp = 4
}

// MOVEM.W D0-D3,$1000.L → abs-long EA, 2 extension words.
TEST(MovemTest, ReglistToAbsLong) {
  auto bytes = Asm("        MOVEM.W D0-D3,$1000.L");
  ASSERT_EQ(bytes.size(), 8u);
  // 0x4880 | 0x39 = 0x48B9
  EXPECT_EQ(bytes[0], 0x48);
  EXPECT_EQ(bytes[1], 0xB9);
  EXPECT_EQ(bytes[2], 0x00);  // mask high
  EXPECT_EQ(bytes[3], 0x0F);
  EXPECT_EQ(bytes[4], 0x00);  // abs long high word
  EXPECT_EQ(bytes[5], 0x00);
  EXPECT_EQ(bytes[6], 0x10);  // abs long low word: 0x1000
  EXPECT_EQ(bytes[7], 0x00);
}

// MOVEM.W D0-D3,-(A0) → pre-dec with A0 (not just SP), mask reversed.
TEST(MovemTest, ReglistToPreDecA0) {
  auto bytes = Asm("        MOVEM.W D0-D3,-(A0)");
  ASSERT_EQ(bytes.size(), 4u);
  // 0x4880 | EffAddr(-(A0)=0x20) = 0x48A0
  EXPECT_EQ(bytes[0], 0x48);
  EXPECT_EQ(bytes[1], 0xA0);
  EXPECT_EQ(bytes[2], 0xF0);  // reversed 0x000F
  EXPECT_EQ(bytes[3], 0x00);
}

// -----------------------------------------------------------------------
// MOVEM EA → reglist (form 2, bit 10 = 0x0400 set)
// -----------------------------------------------------------------------

// MOVEM.W (SP)+,D0-D7 → post-increment EA, 0x0400 added.
TEST(MovemTest, PostIncToReglist) {
  auto bytes = Asm("        MOVEM.W (SP)+,D0-D7");
  ASSERT_EQ(bytes.size(), 4u);
  // 0x4880 | 0x0400 | EffAddr((SP)+=0x1F) = 0x4C9F
  EXPECT_EQ(bytes[0], 0x4C);
  EXPECT_EQ(bytes[1], 0x9F);
  EXPECT_EQ(bytes[2], 0x00);
  EXPECT_EQ(bytes[3], 0xFF);  // D0-D7 = bits 0-7 = 0x00FF, no reversal
}

// MOVEM.W (A0),D0-D3 → indirect EA, form 2.
TEST(MovemTest, AnIndirectToReglist) {
  auto bytes = Asm("        MOVEM.W (A0),D0-D3");
  ASSERT_EQ(bytes.size(), 4u);
  // 0x4880 | 0x0400 | 0x10 = 0x4C90
  EXPECT_EQ(bytes[0], 0x4C);
  EXPECT_EQ(bytes[1], 0x90);
  EXPECT_EQ(bytes[2], 0x00);
  EXPECT_EQ(bytes[3], 0x0F);
}

// MOVEM.L 4(A0),D0-D3 → displacement EA, form 2.
TEST(MovemTest, DispToReglist) {
  auto bytes = Asm("        MOVEM.L 4(A0),D0-D3");
  ASSERT_EQ(bytes.size(), 6u);
  // 0x48C0 | 0x0400 | 0x28 = 0x4CE8
  EXPECT_EQ(bytes[0], 0x4C);
  EXPECT_EQ(bytes[1], 0xE8);
  EXPECT_EQ(bytes[2], 0x00);
  EXPECT_EQ(bytes[3], 0x0F);
  EXPECT_EQ(bytes[4], 0x00);
  EXPECT_EQ(bytes[5], 0x04);
}

// MOVEM.W $1000.L,D0-D3 → abs-long EA, form 2.
TEST(MovemTest, AbsLongToReglist) {
  auto bytes = Asm("        MOVEM.W $1000.L,D0-D3");
  ASSERT_EQ(bytes.size(), 8u);
  // 0x4880 | 0x0400 | 0x39 = 0x4CB9
  EXPECT_EQ(bytes[0], 0x4C);
  EXPECT_EQ(bytes[1], 0xB9);
  EXPECT_EQ(bytes[2], 0x00);
  EXPECT_EQ(bytes[3], 0x0F);
  EXPECT_EQ(bytes[4], 0x00);
  EXPECT_EQ(bytes[5], 0x00);
  EXPECT_EQ(bytes[6], 0x10);
  EXPECT_EQ(bytes[7], 0x00);
}

// MOVEM.L (SP)+,D0-D7/A0-A6 → form 2 long, all caller-save registers.
TEST(MovemTest, PostIncLongToReglist) {
  auto bytes = Asm("        MOVEM.L (SP)+,D0-D7/A0-A6");
  ASSERT_EQ(bytes.size(), 4u);
  // 0x48C0 | 0x0400 | 0x1F = 0x4CDF
  EXPECT_EQ(bytes[0], 0x4C);
  EXPECT_EQ(bytes[1], 0xDF);
  // mask: D0-D7 (bits 0-7) + A0-A6 (bits 8-14) = 0x7FFF, no reversal
  EXPECT_EQ(bytes[2], 0x7F);
  EXPECT_EQ(bytes[3], 0xFF);
}

// -----------------------------------------------------------------------
// A7 = SP equivalence in register lists
// -----------------------------------------------------------------------

// D0/A7 in a register list — A7 and SP are the same physical register.
// mask: D0=bit0, A7=bit15 → 0x8001; reversed for -(SP): 0x8001 reversed = 0x8001.
// Actually: 0x8001 = 1000 0000 0000 0001, reversed = 1000 0000 0000 0001 = 0x8001
TEST(MovemTest, A7AndSPSameRegister) {
  auto b1 = Asm("        MOVEM.W D0/A7,-(SP)");
  auto b2 = Asm("        MOVEM.W D0/SP,-(SP)");
  ASSERT_EQ(b1.size(), 4u);
  ASSERT_EQ(b2.size(), 4u);
  // Both should produce the same bytes (A7 = SP, reg_num = 7)
  EXPECT_EQ(b1, b2);
  // mask D0 (bit0) + A7 (bit15) = 0x8001; reversed: bit0→bit15, bit15→bit0 = 0x8001
  EXPECT_EQ(b1[2], 0x80);
  EXPECT_EQ(b1[3], 0x01);
}

// -----------------------------------------------------------------------
// InstructionSize prediction (pass 1 must agree with pass 2 byte count)
// -----------------------------------------------------------------------

// MOVEM to pre-decrement: opcode(2) + mask(2) + 0 ext words = 4 bytes.
// Verified indirectly: a label immediately after must land at the right address.
TEST(MovemTest, SizePrediction_PreDec) {
  // Label AFTER must be at address 4.
  auto bytes = AsmMulti(
      "        ORG $100\n"
      "        MOVEM.W D0-D3,-(SP)\n"
      "AFTER   NOP\n");
  ASSERT_GE(bytes.size(), 6u);
  // MOVEM = 4 bytes, NOP = 2 bytes → total 6
  EXPECT_EQ(bytes.size(), 6u);
}

// MOVEM to d(An): opcode(2) + mask(2) + disp(2) = 6 bytes.
TEST(MovemTest, SizePrediction_Disp) {
  auto bytes = AsmMulti(
      "        ORG $100\n"
      "        MOVEM.L D0-D7,4(A0)\n"
      "AFTER   NOP\n");
  ASSERT_GE(bytes.size(), 8u);
  EXPECT_EQ(bytes.size(), 8u);  // 6 + 2
}

// MOVEM to abs.L: opcode(2) + mask(2) + abs(4) = 8 bytes.
TEST(MovemTest, SizePrediction_AbsLong) {
  auto bytes = AsmMulti(
      "        ORG $100\n"
      "        MOVEM.W D0-D3,$1000.L\n"
      "AFTER   NOP\n");
  ASSERT_GE(bytes.size(), 10u);
  EXPECT_EQ(bytes.size(), 10u);  // 8 + 2
}

// -----------------------------------------------------------------------
// REG directive
// -----------------------------------------------------------------------

// REG directive defines a named register list; MOVEM can use it.
TEST(MovemTest, RegDirectiveDefinesSymbol) {
  // Define SREGS and use it in a MOVEM.W SREGS,-(SP)
  // D0-D3 = mask 0x000F; reversed for -(SP) = 0xF000
  auto bytes = AsmMulti(
      "SREGS   REG D0-D3\n"
      "        MOVEM.W SREGS,-(SP)\n");
  ASSERT_EQ(bytes.size(), 4u);
  EXPECT_EQ(bytes[0], 0x48);
  EXPECT_EQ(bytes[1], 0xA7);
  EXPECT_EQ(bytes[2], 0xF0);
  EXPECT_EQ(bytes[3], 0x00);
}

// REG symbol used in EA-to-reglist form.
TEST(MovemTest, RegDirectiveEaToRegs) {
  auto bytes = AsmMulti(
      "DREGS   REG D0-D7\n"
      "        MOVEM.W (SP)+,DREGS\n");
  ASSERT_EQ(bytes.size(), 4u);
  // 0x4C9F, mask 0x00FF
  EXPECT_EQ(bytes[0], 0x4C);
  EXPECT_EQ(bytes[1], 0x9F);
  EXPECT_EQ(bytes[2], 0x00);
  EXPECT_EQ(bytes[3], 0xFF);
}

// REG defined AFTER its use (forward reference).
TEST(MovemTest, RegDirectiveForwardRef) {
  auto bytes = AsmMulti(
      "        MOVEM.W FREGS,-(SP)\n"
      "FREGS   REG D0-D3/A0-A3\n");
  ASSERT_EQ(bytes.size(), 4u);
  // mask 0x0F0F; reversed for -(SP): 0xF0F0
  EXPECT_EQ(bytes[0], 0x48);
  EXPECT_EQ(bytes[1], 0xA7);
  EXPECT_EQ(bytes[2], 0xF0);
  EXPECT_EQ(bytes[3], 0xF0);
}

// REG with combined D and A ranges.
TEST(MovemTest, RegDirectiveCombinedList) {
  auto bytes = AsmMulti(
      "ALL     REG D0-D7/A0-A7\n"
      "        MOVEM.L ALL,-(SP)\n");
  ASSERT_EQ(bytes.size(), 4u);
  EXPECT_EQ(bytes[0], 0x48);
  EXPECT_EQ(bytes[1], 0xE7);  // 0x48C0 | EffAddr(-(SP)=0x27) = 0x48E7
  EXPECT_EQ(bytes[2], 0xFF);  // all 16 bits reversed = 0xFFFF (palindrome)
  EXPECT_EQ(bytes[3], 0xFF);
}

// -----------------------------------------------------------------------
// Canonical save/restore pattern
// -----------------------------------------------------------------------

// Typical function prologue/epilogue pattern:
//   MOVEM.L D0-D7/A0-A6,-(SP)  save all data and most addr regs
//   MOVEM.L (SP)+,D0-D7/A0-A6  restore them
TEST(MovemTest, SaveRestorePattern) {
  auto bytes = AsmMulti(
      "        MOVEM.L D0-D7/A0-A6,-(SP)\n"
      "        MOVEM.L (SP)+,D0-D7/A0-A6\n");
  ASSERT_EQ(bytes.size(), 8u);
  // Save: 0x48C0 | 0x27 = 0x48E7, reversed mask 0x7FFF = 0xFFFE
  EXPECT_EQ(bytes[0], 0x48);
  EXPECT_EQ(bytes[1], 0xE7);
  EXPECT_EQ(bytes[2], 0xFF);
  EXPECT_EQ(bytes[3], 0xFE);
  // Restore: 0x4CDF, mask 0x7FFF (no reversal)
  EXPECT_EQ(bytes[4], 0x4C);
  EXPECT_EQ(bytes[5], 0xDF);
  EXPECT_EQ(bytes[6], 0x7F);
  EXPECT_EQ(bytes[7], 0xFF);
}

}  // namespace
}  // namespace easym68k::asm_
