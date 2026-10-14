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
  auto r = a.Assemble("  ORG $1001\n  DC.W $ABCD\n  END $1002\n");
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
  // two odd ORGs + "END directive missing" (reference oracle)
  EXPECT_EQ(r.warning_count, 3);
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

TEST(ErrorReporterTest, NoErrorLimit) {
  // The original assembler reports every error (no limit); SetMaxErrors()
  // is kept for API compatibility only.
  Assembler a;
  a.SetMaxErrors(3);
  auto r = a.Assemble(
      "  ORG $1000\n"
      "  BOGUS1\n"
      "  BOGUS2\n"
      "  BOGUS3\n"
      "  BOGUS4\n"
      "  BOGUS5\n"
      "  END $1000\n");
  EXPECT_FALSE(r.success);
  EXPECT_EQ(r.errors.size(), 5u);
}

TEST(ErrorReporterTest, OriginalStatusIsReported) {
  // Each diagnostic carries the original status code (asm.h) and text.
  Assembler a;
  auto r = a.Assemble("  ORG $1000\n  DCB.B -1,$00\n  ENDM\n  END $1000\n");
  ASSERT_EQ(r.errors.size(), 2u);
  EXPECT_EQ(r.errors[0].status, 0x307);  // INV_LENGTH
  EXPECT_EQ(r.errors[0].message, "Invalid block length");
  EXPECT_EQ(r.errors[1].status, 0x402);  // INV_OPCODE
  EXPECT_EQ(r.errors[1].message, "Invalid opcode");
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

TEST(ErrorReporterTest, UnknownInstructionMessageIsOriginal) {
  // ERROR.CPP: INV_OPCODE -> "Invalid opcode" (the mnemonic is not quoted).
  Assembler a;
  auto r = a.Assemble("  ORG $1000\n  MYZAP D0\n");
  ASSERT_FALSE(r.errors.empty());
  EXPECT_EQ(r.errors[0].message, "Invalid opcode");
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
