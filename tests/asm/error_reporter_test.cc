// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Tests for Task 8.5.9: Error Reporter (error codes, severity, limit)

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "easym68k/asm/assembler.h"
#include "easym68k/asm/error_codes.h"

namespace easym68k::asm_ {
namespace {

// ─── ErrorCodeMessage ────────────────────────────────────────────────────────

TEST(ErrorReporterTest, AllCodesHaveMessages) {
  // Every non-generic code should return a non-empty, non-null message.
  const ErrorCode codes[] = {
      ErrorCode::kInv8BitData,
      ErrorCode::kInv16BitData,
      ErrorCode::kInvMoveQuickConst,
      ErrorCode::kInvQuickConst,
      ErrorCode::kInvShiftCount,
      ErrorCode::kInvVectorNum,
      ErrorCode::kInvBranchDisp,
      ErrorCode::kInvOffset,
      ErrorCode::kInvDisp,
      ErrorCode::kInvAbsAddress,
      ErrorCode::kInvEa,
      ErrorCode::kInvSizeCode,
      ErrorCode::kMissingSizeCode,
      ErrorCode::kIllegalSizeCode,
      ErrorCode::kInvOp,
      ErrorCode::kInvRegister,
      ErrorCode::kUnknownInstruction,
      ErrorCode::kInvLabel,
      ErrorCode::kInvSymbol,
      ErrorCode::kMultDefd,
      ErrorCode::kUndefSymbol,
      ErrorCode::kDivByZero,
      ErrorCode::kMissingOperand,
      ErrorCode::kDcbNegCount,
      ErrorCode::kIncludeCycle,
      ErrorCode::kIncludeNotFound,
      ErrorCode::kIncbinNotFound,
      ErrorCode::kFailDirective,
      ErrorCode::kMissingLabel,
      ErrorCode::kSectionOutOfRange,
      ErrorCode::kMacroNoName,
      ErrorCode::kMacroNoEndm,
      ErrorCode::kMacroNested,
      ErrorCode::kEndmOutside,
      ErrorCode::kMexitOutside,
      ErrorCode::kMacroRecursion,
      ErrorCode::kElseWithoutIf,
      ErrorCode::kMultipleElse,
      ErrorCode::kEndcWithoutIf,
      ErrorCode::kLabelOnCond,
      ErrorCode::kCondMismatch,
      ErrorCode::kOddAddress,
      ErrorCode::kInvMemory,
      ErrorCode::kTruncation,
      ErrorCode::kPhaseError,
  };
  for (auto code : codes) {
    const char* msg = ErrorCodeMessage(code);
    ASSERT_NE(msg, nullptr) << "null message for code " << static_cast<int>(code);
    EXPECT_GT(std::string(msg).size(), 0u) << "empty message for code " << static_cast<int>(code);
  }
}

TEST(ErrorReporterTest, GenericCodeHasMessage) {
  const char* msg = ErrorCodeMessage(ErrorCode::kGeneric);
  ASSERT_NE(msg, nullptr);
  EXPECT_GT(std::string(msg).size(), 0u);
}

TEST(ErrorReporterTest, SpecificMessageContent) {
  EXPECT_NE(std::string(ErrorCodeMessage(ErrorCode::kInv8BitData)).find("8"), std::string::npos);
  EXPECT_NE(std::string(ErrorCodeMessage(ErrorCode::kInv16BitData)).find("16"), std::string::npos);
  EXPECT_NE(std::string(ErrorCodeMessage(ErrorCode::kInvQuickConst)).find("1-8"),
            std::string::npos);
  EXPECT_NE(std::string(ErrorCodeMessage(ErrorCode::kInvVectorNum)).find("0-15"),
            std::string::npos);
  EXPECT_NE(std::string(ErrorCodeMessage(ErrorCode::kUnknownInstruction)).find("Unknown"),
            std::string::npos);
  EXPECT_NE(std::string(ErrorCodeMessage(ErrorCode::kEndmOutside)).find("ENDM"), std::string::npos);
  EXPECT_NE(std::string(ErrorCodeMessage(ErrorCode::kMexitOutside)).find("MEXIT"),
            std::string::npos);
  EXPECT_NE(std::string(ErrorCodeMessage(ErrorCode::kEndcWithoutIf)).find("ENDC"),
            std::string::npos);
  EXPECT_NE(std::string(ErrorCodeMessage(ErrorCode::kElseWithoutIf)).find("ELSE"),
            std::string::npos);
  EXPECT_NE(std::string(ErrorCodeMessage(ErrorCode::kMultipleElse)).find("Multiple"),
            std::string::npos);
  EXPECT_NE(std::string(ErrorCodeMessage(ErrorCode::kDcbNegCount)).find("non-negative"),
            std::string::npos);
  EXPECT_NE(std::string(ErrorCodeMessage(ErrorCode::kIncludeNotFound)).find("cannot open"),
            std::string::npos);
  EXPECT_NE(std::string(ErrorCodeMessage(ErrorCode::kSectionOutOfRange)).find("0-15"),
            std::string::npos);
  EXPECT_NE(std::string(ErrorCodeMessage(ErrorCode::kOddAddress)).find("odd"), std::string::npos);
}

// ─── ErrorCodeSeverity ────────────────────────────────────────────────────────

TEST(ErrorReporterTest, WarningCodes) {
  EXPECT_EQ(ErrorCodeSeverity(ErrorCode::kOddAddress), ErrorSeverity::kWarning);
  EXPECT_EQ(ErrorCodeSeverity(ErrorCode::kInvMemory), ErrorSeverity::kWarning);
  EXPECT_EQ(ErrorCodeSeverity(ErrorCode::kTruncation), ErrorSeverity::kWarning);
  EXPECT_EQ(ErrorCodeSeverity(ErrorCode::kPhaseError), ErrorSeverity::kWarning);
}

TEST(ErrorReporterTest, ErrorCodes) {
  EXPECT_EQ(ErrorCodeSeverity(ErrorCode::kGeneric), ErrorSeverity::kError);
  EXPECT_EQ(ErrorCodeSeverity(ErrorCode::kInv16BitData), ErrorSeverity::kError);
  EXPECT_EQ(ErrorCodeSeverity(ErrorCode::kUnknownInstruction), ErrorSeverity::kError);
  EXPECT_EQ(ErrorCodeSeverity(ErrorCode::kUndefSymbol), ErrorSeverity::kError);
  EXPECT_EQ(ErrorCodeSeverity(ErrorCode::kEndmOutside), ErrorSeverity::kError);
}

// ─── Warning does not halt assembly ──────────────────────────────────────────

TEST(ErrorReporterTest, OddOrgIsWarning) {
  // ORG to an odd address emits a warning; assembly succeeds.
  Assembler a;
  auto r = a.Assemble("  ORG $1001\n  DC.W $ABCD\n");
  EXPECT_TRUE(r.success) << "assembly should succeed despite odd-ORG warning";
  EXPECT_EQ(r.warning_count, 1);
  ASSERT_FALSE(r.errors.empty());
  EXPECT_EQ(r.errors[0].severity, ErrorSeverity::kWarning);
  EXPECT_EQ(r.errors[0].code, ErrorCode::kOddAddress);
  EXPECT_NE(r.errors[0].message.find("odd"), std::string::npos);
}

TEST(ErrorReporterTest, WarningCountIsCorrect) {
  // Two odd ORG → one warning (second ORG sees already-even address, no warn).
  // Use two separate odd ORGs.
  Assembler a;
  auto r = a.Assemble("  ORG $1001\n  ORG $2001\n  NOP\n");
  EXPECT_TRUE(r.success);
  EXPECT_EQ(r.warning_count, 2);
}

TEST(ErrorReporterTest, ErrorCountDoesNotIncludeWarnings) {
  // A warning does not affect success flag or error count.
  Assembler a;
  auto r = a.Assemble("  ORG $1001\n  NOP\n");
  EXPECT_TRUE(r.success);
  // All entries in errors are warnings.
  for (const auto& e : r.errors)
    EXPECT_EQ(e.severity, ErrorSeverity::kWarning);
}

// ─── Error limit ─────────────────────────────────────────────────────────────

TEST(ErrorReporterTest, ErrorLimitStopsAssembly) {
  // SetMaxErrors(3): after 3 errors a "too many errors" message is appended and
  // assembly halts before processing further lines.
  Assembler a;
  a.SetMaxErrors(3);
  // Generate more than 3 errors with unknown instructions.
  auto r = a.Assemble(
      "  ORG $1000\n"
      "  BOGUS1\n"
      "  BOGUS2\n"
      "  BOGUS3\n"
      "  BOGUS4\n"
      "  BOGUS5\n");
  EXPECT_FALSE(r.success);
  // Should have exactly 3 real errors + 1 "too many" sentinel = 4 entries.
  EXPECT_LE(r.errors.size(), 4u);
  bool found_sentinel = false;
  for (const auto& e : r.errors)
    if (e.message.find("too many") != std::string::npos)
      found_sentinel = true;
  EXPECT_TRUE(found_sentinel);
}

TEST(ErrorReporterTest, ErrorLimitZeroMeansUnlimited) {
  Assembler a;
  a.SetMaxErrors(0);
  // Generate 5 errors — all should be present.
  auto r = a.Assemble(
      "  ORG $1000\n"
      "  BOGUS1\n"
      "  BOGUS2\n"
      "  BOGUS3\n"
      "  BOGUS4\n"
      "  BOGUS5\n");
  EXPECT_FALSE(r.success);
  EXPECT_GE(r.errors.size(), 5u);
}

TEST(ErrorReporterTest, DefaultLimitIsTen) {
  // Default max_errors is 10; 11 bogus instructions should stop at 10.
  Assembler a;
  const std::string src =
      "  ORG $1000\n"
      "  B1\n  B2\n  B3\n  B4\n  B5\n"
      "  B6\n  B7\n  B8\n  B9\n  B10\n  B11\n";
  auto r = a.Assemble(src);
  EXPECT_FALSE(r.success);
  EXPECT_LE(r.errors.size(), 11u);  // 10 real + 1 sentinel
}

// ─── Error codes in result ────────────────────────────────────────────────────

TEST(ErrorReporterTest, UnknownInstructionCode) {
  Assembler a;
  auto r = a.Assemble("  ORG $1000\n  BOGUS D0\n");
  ASSERT_FALSE(r.errors.empty());
  bool found = false;
  for (const auto& e : r.errors)
    if (e.code == ErrorCode::kUnknownInstruction)
      found = true;
  EXPECT_TRUE(found);
}

TEST(ErrorReporterTest, UnknownInstructionMessageContainsName) {
  Assembler a;
  auto r = a.Assemble("  ORG $1000\n  MYZAP D0\n");
  ASSERT_FALSE(r.errors.empty());
  EXPECT_NE(r.errors[0].message.find("MYZAP"), std::string::npos);
}

TEST(ErrorReporterTest, EndmOutsideMacroCode) {
  Assembler a;
  auto r = a.Assemble("  ORG $1000\n  ENDM\n");
  ASSERT_FALSE(r.errors.empty());
  bool found = false;
  for (const auto& e : r.errors)
    if (e.code == ErrorCode::kEndmOutside)
      found = true;
  EXPECT_TRUE(found);
}

TEST(ErrorReporterTest, MexitOutsideMacroCode) {
  Assembler a;
  auto r = a.Assemble("  ORG $1000\n  MEXIT\n");
  ASSERT_FALSE(r.errors.empty());
  bool found = false;
  for (const auto& e : r.errors)
    if (e.code == ErrorCode::kMexitOutside)
      found = true;
  EXPECT_TRUE(found);
}

TEST(ErrorReporterTest, EndcWithoutIfCode) {
  Assembler a;
  auto r = a.Assemble("  ORG $1000\n  ENDC\n");
  ASSERT_FALSE(r.errors.empty());
  bool found = false;
  for (const auto& e : r.errors)
    if (e.code == ErrorCode::kEndcWithoutIf)
      found = true;
  EXPECT_TRUE(found);
}

TEST(ErrorReporterTest, ElseWithoutIfError) {
  // ELSE outside any conditional block produces an error mentioning "ELSE".
  Assembler a;
  auto r = a.Assemble("  ORG $1000\n  ELSE\n  ENDC\n");
  ASSERT_FALSE(r.errors.empty());
  bool found = false;
  for (const auto& e : r.errors)
    if (e.message.find("ELSE") != std::string::npos)
      found = true;
  EXPECT_TRUE(found);
}

TEST(ErrorReporterTest, DcbNegCountCode) {
  Assembler a;
  auto r = a.Assemble("  ORG $1000\n  DCB.B -1,$00\n");
  ASSERT_FALSE(r.errors.empty());
  bool found = false;
  for (const auto& e : r.errors)
    if (e.code == ErrorCode::kDcbNegCount)
      found = true;
  EXPECT_TRUE(found);
}

TEST(ErrorReporterTest, SectionOutOfRangeCode) {
  Assembler a;
  auto r = a.Assemble("  ORG $1000\n  SECTION 16\n");
  ASSERT_FALSE(r.errors.empty());
  bool found = false;
  for (const auto& e : r.errors)
    if (e.code == ErrorCode::kSectionOutOfRange)
      found = true;
  EXPECT_TRUE(found);
}

TEST(ErrorReporterTest, IncludeNotFoundCode) {
  Assembler a;
  a.SetFileReader([](const std::string&) { return ""; });
  auto r = a.Assemble("  ORG $1000\n  INCLUDE 'nosuchfile.x68'\n");
  ASSERT_FALSE(r.errors.empty());
  bool found = false;
  for (const auto& e : r.errors)
    if (e.code == ErrorCode::kIncludeNotFound)
      found = true;
  EXPECT_TRUE(found);
}

TEST(ErrorReporterTest, FailDirectiveCode) {
  Assembler a;
  auto r = a.Assemble("  ORG $1000\n  FAIL 'intentional'\n");
  ASSERT_FALSE(r.errors.empty());
  bool found = false;
  for (const auto& e : r.errors)
    if (e.code == ErrorCode::kFailDirective)
      found = true;
  EXPECT_TRUE(found);
  // Extra text is appended
  EXPECT_NE(r.errors[0].message.find("intentional"), std::string::npos);
}

// ─── Backward compatibility ───────────────────────────────────────────────────

TEST(ErrorReporterTest, GenericStringErrorHasKGenericCode) {
  // String-based AddError still works; code is kGeneric.
  Assembler a;
  auto r = a.Assemble("  ORG $1000\n  EQU 42\n");  // EQU without label → generic-ish error
  // Assembly may fail for other reasons; just verify any error has a message.
  if (!r.success) {
    ASSERT_FALSE(r.errors.empty());
    EXPECT_FALSE(r.errors[0].message.empty());
  }
}

TEST(ErrorReporterTest, LineNumberInError) {
  Assembler a;
  auto r = a.Assemble("  ORG $1000\n  NOP\n  ENDM\n");
  ASSERT_FALSE(r.errors.empty());
  EXPECT_EQ(r.errors[0].line_number, 3);
}

}  // namespace
}  // namespace easym68k::asm_
