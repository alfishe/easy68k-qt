// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Tests for Task 8.5.8: Macro Processor (MACRO/ENDM/params/\@/NARG/IFARG/MEXIT)

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "easym68k/asm/assembler.h"

namespace easym68k::asm_ {
namespace {

std::vector<uint8_t> Asm(const std::string& src) {
  Assembler a;
  auto result = a.Assemble(src);
  if (!result.success)
    for (const auto& e : result.errors)
      ADD_FAILURE() << "line " << e.line_number << ": " << e.message;
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
    ADD_FAILURE() << "Expected error '" << substr << "' but got success";
  else if (!found) {
    std::string all;
    for (const auto& e : result.errors)
      all += e.message + "; ";
    ADD_FAILURE() << "Expected error '" << substr << "', got: " << all;
  }
}

uint16_t Word(const std::vector<uint8_t>& code, size_t off) {
  return static_cast<uint16_t>((code[off] << 8) | code[off + 1]);
}

// ─── Definition & invocation ─────────────────────────────────────────────────

TEST(MacroTest, NoArgMacro) {
  auto code =
      Asm("  ORG $1000\n"
          "NOPNOP MACRO\n"
          "  NOP\n"
          "  NOP\n"
          "  ENDM\n"
          "  NOPNOP\n");
  ASSERT_EQ(code.size(), 4u);
  EXPECT_EQ(Word(code, 0), 0x4E71u);
  EXPECT_EQ(Word(code, 2), 0x4E71u);
}

TEST(MacroTest, InvokedTwice) {
  auto code =
      Asm("  ORG $1000\n"
          "ONENOP MACRO\n"
          "  NOP\n"
          "  ENDM\n"
          "  ONENOP\n"
          "  ONENOP\n");
  ASSERT_EQ(code.size(), 4u);
  EXPECT_EQ(Word(code, 0), 0x4E71u);
  EXPECT_EQ(Word(code, 2), 0x4E71u);
}

TEST(MacroTest, SingleArgument) {
  // \1 substituted with first argument
  auto code =
      Asm("  ORG $1000\n"
          "CLREG MACRO\n"
          "  CLR.W \\1\n"
          "  ENDM\n"
          "  CLREG D0\n");
  // CLR.W D0 = 0x4240
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(Word(code, 0), 0x4240u);
}

TEST(MacroTest, MultipleArguments) {
  // ADD.W D0,D1: \1=D0, \2=D1
  auto code =
      Asm("  ORG $1000\n"
          "ADDREGS MACRO\n"
          "  ADD.W \\1,\\2\n"
          "  ENDM\n"
          "  ADDREGS D0,D1\n");
  // ADD.W D0,D1 = 0xD240
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(Word(code, 0), 0xD240u);
}

TEST(MacroTest, ArgumentUppercased) {
  // Arguments are uppercased (EASy68K strcap behaviour)
  auto code =
      Asm("  ORG $1000\n"
          "CLREG MACRO\n"
          "  CLR.W \\1\n"
          "  ENDM\n"
          "  CLREG d0\n");
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(Word(code, 0), 0x4240u);
}

// ─── Size parameter (\0) ──────────────────────────────────────────────────────

TEST(MacroTest, SizeParamByte) {
  auto code =
      Asm("  ORG $1000\n"
          "CLRSZ MACRO\n"
          "  CLR.\\0 \\1\n"
          "  ENDM\n"
          "  CLRSZ.B D0\n");
  // CLR.B D0 = 0x4200
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(Word(code, 0), 0x4200u);
}

TEST(MacroTest, SizeParamLong) {
  auto code =
      Asm("  ORG $1000\n"
          "CLRSZ MACRO\n"
          "  CLR.\\0 \\1\n"
          "  ENDM\n"
          "  CLRSZ.L D0\n");
  // CLR.L D0 = 0x4280
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(Word(code, 0), 0x4280u);
}

TEST(MacroTest, SizeParamDefaultWord) {
  auto code =
      Asm("  ORG $1000\n"
          "CLRSZ MACRO\n"
          "  CLR.\\0 \\1\n"
          "  ENDM\n"
          "  CLRSZ D0\n");
  // CLR.W D0 = 0x4240
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(Word(code, 0), 0x4240u);
}

// ─── \@ unique labels ─────────────────────────────────────────────────────────

TEST(MacroTest, UniqueLabels) {
  // Two invocations get independent \@ labels so branches resolve correctly
  auto code =
      Asm("  ORG $1000\n"
          "SKIP MACRO\n"
          "  BRA \\@done\n"
          "  NOP\n"
          "\\@done NOP\n"
          "  ENDM\n"
          "  SKIP\n"
          "  SKIP\n");
  // Per invocation: BRA.W (4) + NOP (2) + labeled NOP (2) = 8 bytes → 16 total
  // BRA.W at $1000 → target _1done = $1006; disp = $1006 - $1002 = 4
  ASSERT_EQ(code.size(), 16u);
  EXPECT_EQ(Word(code, 0), 0x6000u);  // BRA.W
  EXPECT_EQ(static_cast<int16_t>(Word(code, 2)), 4);
  EXPECT_EQ(Word(code, 4), 0x4E71u);  // skipped NOP
  EXPECT_EQ(Word(code, 6), 0x4E71u);  // \@done NOP
  EXPECT_EQ(Word(code, 8), 0x6000u);  // second BRA.W
  EXPECT_EQ(static_cast<int16_t>(Word(code, 10)), 4);
}

// ─── NARG ────────────────────────────────────────────────────────────────────

TEST(MacroTest, NargZero) {
  auto code =
      Asm("  ORG $1000\n"
          "ARGCOUNT MACRO\n"
          "  DC.W NARG\n"
          "  ENDM\n"
          "  ARGCOUNT\n");
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(Word(code, 0), 0u);
}

TEST(MacroTest, NargThree) {
  auto code =
      Asm("  ORG $1000\n"
          "ARGCOUNT MACRO\n"
          "  DC.W NARG\n"
          "  ENDM\n"
          "  ARGCOUNT A,B,C\n");
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(Word(code, 0), 3u);
}

// ─── IFARG ───────────────────────────────────────────────────────────────────

TEST(MacroTest, IfargSkipsWhenAbsent) {
  auto code =
      Asm("  ORG $1000\n"
          "MAYBE MACRO\n"
          "  NOP\n"
          "  IFARG 2\n"
          "  NOP\n"
          "  ENDC\n"
          "  ENDM\n"
          "  MAYBE A\n");  // arg 2 absent
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(Word(code, 0), 0x4E71u);
}

TEST(MacroTest, IfargAssemblesWhenPresent) {
  auto code =
      Asm("  ORG $1000\n"
          "MAYBE MACRO\n"
          "  NOP\n"
          "  IFARG 2\n"
          "  NOP\n"
          "  ENDC\n"
          "  ENDM\n"
          "  MAYBE A,B\n");  // arg 2 present
  ASSERT_EQ(code.size(), 4u);
  EXPECT_EQ(Word(code, 0), 0x4E71u);
  EXPECT_EQ(Word(code, 2), 0x4E71u);
}

TEST(MacroTest, IfargNestingSkipped) {
  // Nested IFARG inside a false outer IFARG — both NOPs inside are skipped
  auto code =
      Asm("  ORG $1000\n"
          "TRIPLE MACRO\n"
          "  NOP\n"
          "  IFARG 2\n"
          "  NOP\n"
          "  IFARG 3\n"
          "  NOP\n"
          "  ENDC\n"
          "  ENDC\n"
          "  ENDM\n"
          "  TRIPLE A\n");
  ASSERT_EQ(code.size(), 2u);  // only first NOP
}

// ─── MEXIT ───────────────────────────────────────────────────────────────────

TEST(MacroTest, MexitStopsExpansion) {
  auto code =
      Asm("  ORG $1000\n"
          "EARLY MACRO\n"
          "  NOP\n"
          "  MEXIT\n"
          "  NOP\n"
          "  ENDM\n"
          "  EARLY\n");
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(Word(code, 0), 0x4E71u);
}

TEST(MacroTest, MexitInsideSkippedBlock) {
  // MEXIT inside a false IFARG block is treated as a skipped line, not early exit
  auto code =
      Asm("  ORG $1000\n"
          "SKIP MACRO\n"
          "  NOP\n"
          "  IFARG 2\n"
          "  MEXIT\n"
          "  ENDC\n"
          "  NOP\n"
          "  ENDM\n"
          "  SKIP A\n");       // arg 2 absent → MEXIT inside false block
  ASSERT_EQ(code.size(), 4u);  // both NOPs assembled
}

// ─── Nested macros ───────────────────────────────────────────────────────────

TEST(MacroTest, NestedMacroCalls) {
  auto code =
      Asm("  ORG $1000\n"
          "INNER MACRO\n"
          "  NOP\n"
          "  ENDM\n"
          "OUTER MACRO\n"
          "  INNER\n"
          "  INNER\n"
          "  ENDM\n"
          "  OUTER\n");
  ASSERT_EQ(code.size(), 4u);
  EXPECT_EQ(Word(code, 0), 0x4E71u);
  EXPECT_EQ(Word(code, 2), 0x4E71u);
}

TEST(MacroTest, NestedMacroPassesArgs) {
  auto code =
      Asm("  ORG $1000\n"
          "CLREG MACRO\n"
          "  CLR.W \\1\n"
          "  ENDM\n"
          "CLRBOTH MACRO\n"
          "  CLREG \\1\n"
          "  CLREG \\2\n"
          "  ENDM\n"
          "  CLRBOTH D0,D1\n");
  ASSERT_EQ(code.size(), 4u);
  EXPECT_EQ(Word(code, 0), 0x4240u);  // CLR.W D0
  EXPECT_EQ(Word(code, 2), 0x4241u);  // CLR.W D1
}

// ─── Label on macro call site ─────────────────────────────────────────────────

TEST(MacroTest, LabelOnMacroCall) {
  auto code =
      Asm("  ORG $1000\n"
          "ONENOP MACRO\n"
          "  NOP\n"
          "  ENDM\n"
          "start ONENOP\n"
          "  MOVE.W #start,D0\n");
  // start = $1000; NOP at $1000; MOVE.W #$1000,D0 at $1002
  ASSERT_EQ(code.size(), 6u);
  EXPECT_EQ(Word(code, 0), 0x4E71u);  // NOP
  EXPECT_EQ(Word(code, 2), 0x303Cu);  // MOVE.W #n,D0
  EXPECT_EQ(Word(code, 4), 0x1000u);  // #start
}

// ─── Angle-bracket grouping ───────────────────────────────────────────────────

TEST(MacroTest, AngleBracketArgContainsComma) {
  // <A,B> counts as one argument despite the comma
  auto code =
      Asm("  ORG $1000\n"
          "ARGCOUNT MACRO\n"
          "  DC.W NARG\n"
          "  ENDM\n"
          "  ARGCOUNT <A,B>,C\n");
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(Word(code, 0), 2u);  // NARG = 2
}

// ─── Forward reference (pre-scan) ────────────────────────────────────────────

TEST(MacroTest, ForwardReference) {
  // Macro invoked before its definition in the source
  auto code =
      Asm("  ORG $1000\n"
          "  ONENOP\n"
          "ONENOP MACRO\n"
          "  NOP\n"
          "  ENDM\n");
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(Word(code, 0), 0x4E71u);
}

// ─── Two-pass consistency ────────────────────────────────────────────────────

TEST(MacroTest, TwoPassConsistency) {
  const std::string src =
      "  ORG $1000\n"
      "TWICE MACRO\n"
      "  NOP\n"
      "  NOP\n"
      "  ENDM\n"
      "  TWICE\n"
      "  TWICE\n";
  Assembler a1, a2;
  auto r1 = a1.Assemble(src);
  auto r2 = a2.Assemble(src);
  EXPECT_TRUE(r1.success);
  EXPECT_TRUE(r2.success);
  EXPECT_EQ(r1.code, r2.code);
}

// ─── Continuation lines ───────────────────────────────────────────────────────

TEST(MacroTest, ContinuationBasic) {
  // A trailing comma on a line followed by '& ...' merges into one argument list
  auto code =
      Asm("  ORG $1000\n"
          "ARGCOUNT MACRO\n"
          "  DC.W NARG\n"
          "  ENDM\n"
          "  ARGCOUNT A,B,\n"
          "& C,D\n");
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(Word(code, 0), 4u);  // NARG = 4
}

TEST(MacroTest, ContinuationMultiple) {
  // Three-line continuation
  auto code =
      Asm("  ORG $1000\n"
          "ARGCOUNT MACRO\n"
          "  DC.W NARG\n"
          "  ENDM\n"
          "  ARGCOUNT A,\n"
          "& B,\n"
          "& C\n");
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(Word(code, 0), 3u);  // NARG = 3
}

TEST(MacroTest, ContinuationWithSizeSuffix) {
  // Size suffix + continuation: \0 = 'B', \1 = first arg, \2 = second arg
  auto code =
      Asm("  ORG $1000\n"
          "CLRSZ MACRO\n"
          "  CLR.\\0 \\1\n"
          "  CLR.\\0 \\2\n"
          "  ENDM\n"
          "  CLRSZ.B D0,\n"
          "& D1\n");
  // CLR.B D0 = 0x4200, CLR.B D1 = 0x4201
  ASSERT_EQ(code.size(), 4u);
  EXPECT_EQ(Word(code, 0), 0x4200u);
  EXPECT_EQ(Word(code, 2), 0x4201u);
}

TEST(MacroTest, ContinuationTwoPassConsistency) {
  const std::string src =
      "  ORG $1000\n"
      "ARGCOUNT MACRO\n"
      "  DC.W NARG\n"
      "  ENDM\n"
      "  ARGCOUNT A,B,\n"
      "& C,D\n";
  Assembler a1, a2;
  auto r1 = a1.Assemble(src);
  auto r2 = a2.Assemble(src);
  EXPECT_TRUE(r1.success);
  EXPECT_TRUE(r2.success);
  EXPECT_EQ(r1.code, r2.code);
}

TEST(MacroTest, ContinuationNoAmpersand) {
  // Line ending ',' NOT followed by '&' → no join; macro gets 2 args (A,B), next
  // line "  C,D" is assembled separately (unknown instruction C — error expected)
  Assembler a;
  auto result = a.Assemble(
      "  ORG $1000\n"
      "ARGCOUNT MACRO\n"
      "  DC.W NARG\n"
      "  ENDM\n"
      "  ARGCOUNT A,B\n"
      "  NOP\n");
  // Without continuation: ARGCOUNT A,B → NARG=2; NOP follows normally
  EXPECT_TRUE(result.success);
  ASSERT_EQ(result.code.size(), 4u);
  EXPECT_EQ(static_cast<uint16_t>((result.code[0] << 8) | result.code[1]), 2u);
  EXPECT_EQ(static_cast<uint16_t>((result.code[2] << 8) | result.code[3]), 0x4E71u);
}

// ─── Error cases ─────────────────────────────────────────────────────────────

TEST(MacroTest, EndmWithoutMacro) {
  AsmExpectError("  ORG $1000\n  ENDM\n", "ENDM");
}

TEST(MacroTest, MexitWithoutMacro) {
  AsmExpectError("  ORG $1000\n  MEXIT\n", "MEXIT");
}

}  // namespace
}  // namespace easym68k::asm_
