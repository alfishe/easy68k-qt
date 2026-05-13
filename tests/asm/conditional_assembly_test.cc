// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Tests for Task 8.5.6: IFC/IFNC/IFEQ/IFNE/IFLT/IFLE/IFGT/IFGE/ELSE/ENDC

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "easym68k/asm/assembler.h"

namespace easym68k::asm_ {
namespace {

std::vector<uint8_t> Asm(const std::string& src) {
  Assembler a;
  auto result = a.Assemble(src);
  if (!result.success) {
    for (const auto& e : result.errors)
      ADD_FAILURE() << "line " << e.line_number << ": " << e.message;
  }
  return result.code;
}

void AsmExpectError(const std::string& src, const std::string& substr) {
  Assembler a;
  auto result = a.Assemble(src);
  bool found = false;
  for (const auto& e : result.errors)
    if (e.message.find(substr) != std::string::npos) {
      found = true;
      break;
    }
  if (result.success)
    ADD_FAILURE() << "Expected error containing '" << substr << "' but got success";
  else if (!found) {
    std::string all;
    for (const auto& e : result.errors)
      all += e.message + "; ";
    ADD_FAILURE() << "Expected error '" << substr << "', got: " << all;
  }
}

// ============================================================
// IFC — assemble if strings are equal (case-sensitive)
// ============================================================

TEST(ConditionalTest, IfcEqualStringsAssembles) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFC 'abc','abc'\n"
          "  DC.B $01\n"
          "  ENDC\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x01);
}

TEST(ConditionalTest, IfcUnequalStringsSkips) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFC 'abc','xyz'\n"
          "  DC.B $01\n"
          "  ENDC\n"
          "  DC.B $02\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x02);
}

TEST(ConditionalTest, IfcCaseSensitive) {
  // 'ABC' != 'abc' — case-sensitive
  auto code =
      Asm("  ORG $1000\n"
          "  IFC 'ABC','abc'\n"
          "  DC.B $01\n"
          "  ENDC\n"
          "  DC.B $02\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x02);
}

// ============================================================
// IFNC — assemble if strings are NOT equal
// ============================================================

TEST(ConditionalTest, IfncUnequalStringsAssembles) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFNC 'abc','xyz'\n"
          "  DC.B $01\n"
          "  ENDC\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x01);
}

TEST(ConditionalTest, IfncEqualStringsSkips) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFNC 'same','same'\n"
          "  DC.B $01\n"
          "  ENDC\n"
          "  DC.B $02\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x02);
}

// ============================================================
// IFEQ — assemble if expression == 0
// ============================================================

TEST(ConditionalTest, IfeqZeroAssembles) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFEQ 0\n"
          "  DC.B $01\n"
          "  ENDC\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x01);
}

TEST(ConditionalTest, IfeqNonzeroSkips) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFEQ 1\n"
          "  DC.B $01\n"
          "  ENDC\n"
          "  DC.B $02\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x02);
}

TEST(ConditionalTest, IfeqWithSymbol) {
  auto code =
      Asm("  ORG $1000\n"
          "FLAG EQU 0\n"
          "  IFEQ FLAG\n"
          "  DC.B $AB\n"
          "  ENDC\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0xAB);
}

// ============================================================
// IFNE — assemble if expression != 0
// ============================================================

TEST(ConditionalTest, IfneNonzeroAssembles) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFNE 42\n"
          "  DC.B $01\n"
          "  ENDC\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x01);
}

TEST(ConditionalTest, IfneZeroSkips) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFNE 0\n"
          "  DC.B $01\n"
          "  ENDC\n"
          "  DC.B $02\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x02);
}

// ============================================================
// IFLT / IFLE / IFGT / IFGE
// ============================================================

TEST(ConditionalTest, IfltNegativeAssembles) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFLT -1\n"
          "  DC.B $01\n"
          "  ENDC\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x01);
}

TEST(ConditionalTest, IfltZeroSkips) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFLT 0\n"
          "  DC.B $01\n"
          "  ENDC\n"
          "  DC.B $02\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x02);
}

TEST(ConditionalTest, IfleZeroAssembles) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFLE 0\n"
          "  DC.B $01\n"
          "  ENDC\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x01);
}

TEST(ConditionalTest, IflePositiveSkips) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFLE 1\n"
          "  DC.B $01\n"
          "  ENDC\n"
          "  DC.B $02\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x02);
}

TEST(ConditionalTest, IfgtPositiveAssembles) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFGT 5\n"
          "  DC.B $01\n"
          "  ENDC\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x01);
}

TEST(ConditionalTest, IfgtZeroSkips) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFGT 0\n"
          "  DC.B $01\n"
          "  ENDC\n"
          "  DC.B $02\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x02);
}

TEST(ConditionalTest, IfgeZeroAssembles) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFGE 0\n"
          "  DC.B $01\n"
          "  ENDC\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x01);
}

TEST(ConditionalTest, IfgeNegativeSkips) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFGE -1\n"
          "  DC.B $01\n"
          "  ENDC\n"
          "  DC.B $02\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x02);
}

// ============================================================
// ELSE — toggles assembly state
// ============================================================

TEST(ConditionalTest, ElseAfterTrueCondSkipElseBranch) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFEQ 0\n"
          "  DC.B $01\n"
          "  ELSE\n"
          "  DC.B $02\n"
          "  ENDC\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x01);
}

TEST(ConditionalTest, ElseAfterFalseCondAssembleElseBranch) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFEQ 1\n"
          "  DC.B $01\n"
          "  ELSE\n"
          "  DC.B $02\n"
          "  ENDC\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x02);
}

TEST(ConditionalTest, ElseWithIfc) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFC 'a','b'\n"
          "  DC.B $01\n"
          "  ELSE\n"
          "  DC.B $02\n"
          "  ENDC\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x02);
}

// ============================================================
// Nesting
// ============================================================

TEST(ConditionalTest, NestedIfBothTrue) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFEQ 0\n"
          "  IFNE 1\n"
          "  DC.B $AB\n"
          "  ENDC\n"
          "  ENDC\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0xAB);
}

TEST(ConditionalTest, NestedIfOuterFalseInnerNotEvaluated) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFEQ 1\n"  // false — outer block skipped
          "  IFNE 1\n"  // inner IF inside skipped block
          "  DC.B $AB\n"
          "  ENDC\n"
          "  ENDC\n"
          "  DC.B $CD\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0xCD);
}

TEST(ConditionalTest, NestedIfInnerFalse) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFEQ 0\n"  // true
          "  IFEQ 1\n"  // false — inner block skipped
          "  DC.B $AB\n"
          "  ENDC\n"
          "  DC.B $CD\n"  // outer block still active
          "  ENDC\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0xCD);
}

TEST(ConditionalTest, ThreeLevelsDeep) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFEQ 0\n"  // true
          "  IFNE 5\n"  // true
          "  IFGE 0\n"  // true
          "  DC.B $FF\n"
          "  ENDC\n"
          "  ENDC\n"
          "  ENDC\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0xFF);
}

TEST(ConditionalTest, NestedElse) {
  // Outer true, inner false → inner else branch assembled
  auto code =
      Asm("  ORG $1000\n"
          "  IFEQ 0\n"  // outer: true
          "  IFEQ 1\n"  // inner: false
          "  DC.B $01\n"
          "  ELSE\n"
          "  DC.B $02\n"
          "  ENDC\n"
          "  ENDC\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x02);
}

TEST(ConditionalTest, ElseInsideSkippedOuterNotToggled) {
  // Outer false — ELSE inside it should not affect outer skipping
  auto code =
      Asm("  ORG $1000\n"
          "  IFEQ 1\n"  // outer: false
          "  IFEQ 0\n"  // inner: would be true but outer is false
          "  DC.B $01\n"
          "  ELSE\n"
          "  DC.B $02\n"
          "  ENDC\n"
          "  ENDC\n"
          "  DC.B $03\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x03);
}

// ============================================================
// Integration: conditionals with EQU/SET/labels
// ============================================================

TEST(ConditionalTest, ConditionalWithEqu) {
  auto code =
      Asm("  ORG $1000\n"
          "DEBUG EQU 1\n"
          "  IFNE DEBUG\n"
          "  DC.B $AA\n"
          "  ENDC\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0xAA);
}

TEST(ConditionalTest, ConditionalGuardsLabel) {
  // A label inside a false conditional should not be defined
  const char* src =
      "  ORG $1000\n"
      "  IFEQ 1\n"  // false
      "SKIPPED EQU $FF\n"
      "  ENDC\n"
      "  IFEQ 0\n"  // true
      "ACTIVE EQU $42\n"
      "  ENDC\n"
      "  DC.B ACTIVE\n";
  auto code = Asm(src);
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x42);
}

TEST(ConditionalTest, ConditionalWithExpressionArith) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFGT 10-5\n"  // 5 > 0 → true
          "  DC.B $BB\n"
          "  ENDC\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0xBB);
}

TEST(ConditionalTest, EmptyIfBlock) {
  // An if block with nothing inside is valid
  auto code =
      Asm("  ORG $1000\n"
          "  IFEQ 0\n"
          "  ENDC\n"
          "  DC.B $01\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x01);
}

TEST(ConditionalTest, MultipleTopLevelConditions) {
  auto code =
      Asm("  ORG $1000\n"
          "  IFEQ 0\n"
          "  DC.B $01\n"
          "  ENDC\n"
          "  IFNE 1\n"
          "  DC.B $02\n"
          "  ENDC\n"
          "  IFEQ 1\n"
          "  DC.B $03\n"
          "  ENDC\n");
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(code[0], 0x01);
  EXPECT_EQ(code[1], 0x02);
}

// ============================================================
// Error cases
// ============================================================

TEST(ConditionalTest, EndcWithoutIfIsError) {
  AsmExpectError("  ORG $1000\n  ENDC\n", "ENDC without");
}

TEST(ConditionalTest, ElseWithoutIfIsError) {
  AsmExpectError("  ORG $1000\n  ELSE\n  ENDC\n", "ELSE without");
}

TEST(ConditionalTest, DoubleElseIsError) {
  AsmExpectError(
      "  ORG $1000\n"
      "  IFEQ 0\n"
      "  ELSE\n"
      "  ELSE\n"
      "  ENDC\n",
      "Multiple ELSE");
}

TEST(ConditionalTest, LabelOnConditionalLineIsError) {
  AsmExpectError(
      "  ORG $1000\n"
      "MYLAB IFC 'a','a'\n"
      "  DC.B $01\n"
      "  ENDC\n",
      "Label not permitted");
}

}  // namespace
}  // namespace easym68k::asm_
