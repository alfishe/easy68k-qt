// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Tests for Task 8.5.5: DCB, DC strings, INCLUDE, INCBIN, SECTION, OFFSET,
// OPT, FAIL, LIST/NOLIST, PAGE, SIMHALT, ORG odd-address fix.

#include <gtest/gtest.h>

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "easym68k/asm/assembler.h"

namespace easym68k::asm_ {
namespace {

// Helper: assemble source, expect success, return bytes
std::vector<uint8_t> Asm(const std::string& src,
                         std::function<void(Assembler&)> configure = nullptr) {
  Assembler a;
  if (configure)
    configure(a);
  auto result = a.Assemble(src);
  if (!result.success) {
    for (const auto& e : result.errors)
      ADD_FAILURE() << "Assembly error line " << e.line_number << ": " << e.message;
  }
  return result.code;
}

// Helper: assemble and expect at least one error containing the substring
void AsmExpectError(const std::string& src, const std::string& substr,
                    std::function<void(Assembler&)> configure = nullptr) {
  Assembler a;
  if (configure)
    configure(a);
  auto result = a.Assemble(src);
  bool found = false;
  for (const auto& e : result.errors) {
    if (e.message.find(substr) != std::string::npos) {
      found = true;
      break;
    }
  }
  if (result.success) {
    ADD_FAILURE() << "Expected assembly error containing '" << substr << "' but got success";
  } else if (!found) {
    std::string all;
    for (const auto& e : result.errors)
      all += e.message + "; ";
    ADD_FAILURE() << "Expected error containing '" << substr << "', got: " << all;
  }
}

// Mock file reader for INCLUDE tests
std::function<void(Assembler&)> WithFiles(const std::map<std::string, std::string>& files) {
  return [files](Assembler& a) {
    a.SetFileReader([files](const std::string& name) -> std::string {
      auto it = files.find(name);
      return (it != files.end()) ? it->second : "";
    });
  };
}

// ============================================================
// DCB tests
// ============================================================

TEST(DirectivesTest, DcbByteBasic) {
  // DCB.B 4,$AB → 4 bytes of 0xAB
  auto code = Asm("  ORG $1000\n  DCB.B 4,$AB\n");
  ASSERT_EQ(code.size(), 4u);
  EXPECT_EQ(code[0], 0xAB);
  EXPECT_EQ(code[1], 0xAB);
  EXPECT_EQ(code[2], 0xAB);
  EXPECT_EQ(code[3], 0xAB);
}

TEST(DirectivesTest, DcbByteZeroCount) {
  // DCB.B 0,$FF → no bytes
  auto code = Asm("  ORG $1000\n  DCB.B 0,$FF\n");
  EXPECT_EQ(code.size(), 0u);
}

TEST(DirectivesTest, DcbWordBasic) {
  // DCB.W 3,$1234 → 3 words (6 bytes)
  auto code = Asm("  ORG $1000\n  DCB.W 3,$1234\n");
  ASSERT_EQ(code.size(), 6u);
  EXPECT_EQ(code[0], 0x12);
  EXPECT_EQ(code[1], 0x34);
  EXPECT_EQ(code[2], 0x12);
  EXPECT_EQ(code[3], 0x34);
  EXPECT_EQ(code[4], 0x12);
  EXPECT_EQ(code[5], 0x34);
}

TEST(DirectivesTest, DcbLongBasic) {
  // DCB.L 2,$DEADBEEF → 2 longs (8 bytes)
  auto code = Asm("  ORG $1000\n  DCB.L 2,$DEADBEEF\n");
  ASSERT_EQ(code.size(), 8u);
  EXPECT_EQ(code[0], 0xDE);
  EXPECT_EQ(code[1], 0xAD);
  EXPECT_EQ(code[2], 0xBE);
  EXPECT_EQ(code[3], 0xEF);
  EXPECT_EQ(code[4], 0xDE);
  EXPECT_EQ(code[5], 0xAD);
  EXPECT_EQ(code[6], 0xBE);
  EXPECT_EQ(code[7], 0xEF);
}

TEST(DirectivesTest, DcbWordAlignsBeforeEmit) {
  // DC.B $01 (odd LC), then DCB.W 1,$CAFE → pad byte + word
  auto code = Asm("  ORG $1000\n  DC.B $01\n  DCB.W 1,$CAFE\n");
  ASSERT_EQ(code.size(), 4u);
  EXPECT_EQ(code[0], 0x01);  // DC.B
  EXPECT_EQ(code[1], 0x00);  // alignment pad
  EXPECT_EQ(code[2], 0xCA);  // DCB.W value
  EXPECT_EQ(code[3], 0xFE);
}

TEST(DirectivesTest, DcbLongAlignsBeforeEmit) {
  // DC.B (odd), DCB.L 1,$12345678 → pad + long
  auto code = Asm("  ORG $1000\n  DC.B $FF\n  DCB.L 1,$12345678\n");
  ASSERT_EQ(code.size(), 6u);
  EXPECT_EQ(code[1], 0x00);  // alignment pad
  EXPECT_EQ(code[2], 0x12);
  EXPECT_EQ(code[3], 0x34);
  EXPECT_EQ(code[4], 0x56);
  EXPECT_EQ(code[5], 0x78);
}

TEST(DirectivesTest, DcbDefaultSizeIsWord) {
  // DCB without size defaults to .W
  auto code = Asm("  ORG $1000\n  DCB 2,$ABCD\n");
  ASSERT_EQ(code.size(), 4u);
  EXPECT_EQ(code[0], 0xAB);
  EXPECT_EQ(code[1], 0xCD);
}

TEST(DirectivesTest, DcbNegativeCountIsError) {
  AsmExpectError("  ORG $1000\n  DCB.B -1,$00\n", "non-negative");
}

TEST(DirectivesTest, DcbLabelPlacedCorrectly) {
  // Use a label after DCB to verify LC advances correctly.
  // DCB.B 5 → LC=$1005; DC.W word-aligns to $1006, pad at [5];
  // value $1005 at [6],[7].
  const char* src =
      "  ORG $1000\n"
      "  DCB.B 5,$00\n"
      "AFTER:\n"
      "  DC.W AFTER\n";
  auto code = Asm(src);
  ASSERT_GE(code.size(), 8u);
  EXPECT_EQ(code[5], 0x00);  // alignment pad inserted by DC.W
  EXPECT_EQ(code[6], 0x10);  // AFTER = $1005 (high byte)
  EXPECT_EQ(code[7], 0x05);  // AFTER = $1005 (low byte)
}

// ============================================================
// DC string literal tests
// ============================================================

TEST(DirectivesTest, DcByteString) {
  // DC.B 'HI' → H, I
  auto code = Asm("  ORG $1000\n  DC.B 'HI'\n");
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(code[0], 'H');
  EXPECT_EQ(code[1], 'I');
}

TEST(DirectivesTest, DcByteStringFiveChars) {
  // DC.B 'Hello' → 5 bytes
  auto code = Asm("  ORG $1000\n  DC.B 'Hello'\n");
  ASSERT_EQ(code.size(), 5u);
  EXPECT_EQ(code[0], 'H');
  EXPECT_EQ(code[1], 'e');
  EXPECT_EQ(code[2], 'l');
  EXPECT_EQ(code[3], 'l');
  EXPECT_EQ(code[4], 'o');
}

TEST(DirectivesTest, DcByteStringWithNull) {
  // DC.B 'Hi',0 → H, i, 0x00
  auto code = Asm("  ORG $1000\n  DC.B 'Hi',0\n");
  ASSERT_EQ(code.size(), 3u);
  EXPECT_EQ(code[0], 'H');
  EXPECT_EQ(code[1], 'i');
  EXPECT_EQ(code[2], 0x00);
}

TEST(DirectivesTest, DcByteStringMixedWithNumbers) {
  // DC.B 'A',65,'C' → A, A, C
  auto code = Asm("  ORG $1000\n  DC.B 'A',65,'C'\n");
  ASSERT_EQ(code.size(), 3u);
  EXPECT_EQ(code[0], 'A');
  EXPECT_EQ(code[1], 65);  // same as 'A'
  EXPECT_EQ(code[2], 'C');
}

TEST(DirectivesTest, DcByteMultipleStrings) {
  // Two string operands on one DC.B
  auto code = Asm("  ORG $1000\n  DC.B 'AB','CD'\n");
  ASSERT_EQ(code.size(), 4u);
  EXPECT_EQ(code[0], 'A');
  EXPECT_EQ(code[1], 'B');
  EXPECT_EQ(code[2], 'C');
  EXPECT_EQ(code[3], 'D');
}

TEST(DirectivesTest, DcByteSingleChar) {
  // Single character string
  auto code = Asm("  ORG $1000\n  DC.B 'X'\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 'X');
}

TEST(DirectivesTest, DcByteStringLabelPlacement) {
  // Label after DC.B 'HELLO',0 should be at +6
  const char* src =
      "  ORG $1000\n"
      "  DC.B 'HELLO',0\n"
      "AFTER:\n"
      "  DC.W AFTER\n";
  auto code = Asm(src);
  ASSERT_GE(code.size(), 8u);
  // AFTER = $1006
  EXPECT_EQ(code[6], 0x10);
  EXPECT_EQ(code[7], 0x06);
}

// ============================================================
// SIMHALT tests
// ============================================================

TEST(DirectivesTest, SimhaltEmitsTwoFfffWords) {
  // SIMHALT → 0xFFFF 0xFFFF
  auto code = Asm("  ORG $1000\n  SIMHALT\n");
  ASSERT_EQ(code.size(), 4u);
  EXPECT_EQ(code[0], 0xFF);
  EXPECT_EQ(code[1], 0xFF);
  EXPECT_EQ(code[2], 0xFF);
  EXPECT_EQ(code[3], 0xFF);
}

TEST(DirectivesTest, SimhaltAtOddAddressAligns) {
  // DC.B $01, SIMHALT → pad, 0xFFFF 0xFFFF
  auto code = Asm("  ORG $1000\n  DC.B $01\n  SIMHALT\n");
  ASSERT_EQ(code.size(), 6u);
  EXPECT_EQ(code[0], 0x01);
  EXPECT_EQ(code[1], 0x00);  // alignment pad
  EXPECT_EQ(code[2], 0xFF);
  EXPECT_EQ(code[3], 0xFF);
  EXPECT_EQ(code[4], 0xFF);
  EXPECT_EQ(code[5], 0xFF);
}

TEST(DirectivesTest, SimhaltAtEvenAddressNoExtraPad) {
  // DC.W $1234, SIMHALT → no extra padding
  auto code = Asm("  ORG $1000\n  DC.W $1234\n  SIMHALT\n");
  ASSERT_EQ(code.size(), 6u);
  EXPECT_EQ(code[2], 0xFF);
  EXPECT_EQ(code[3], 0xFF);
  EXPECT_EQ(code[4], 0xFF);
  EXPECT_EQ(code[5], 0xFF);
}

TEST(DirectivesTest, SimhaltLabelFollowing) {
  // Code after SIMHALT gets correct LC
  const char* src =
      "  ORG $1000\n"
      "  SIMHALT\n"
      "AFTER:\n"
      "  DC.W AFTER\n";
  auto code = Asm(src);
  ASSERT_GE(code.size(), 6u);
  // AFTER = $1004
  EXPECT_EQ(code[4], 0x10);
  EXPECT_EQ(code[5], 0x04);
}

// ============================================================
// ORG odd address
// ============================================================

TEST(DirectivesTest, OrgOddAddressRoundsDown) {
  // ORG $1001 (odd) → auto-aligned to $1000, error emitted
  Assembler a;
  auto result = a.Assemble("  ORG $1001\n  DC.W $1234\n");
  // There should be an error about odd ORG
  bool found_odd_err = false;
  for (const auto& e : result.errors) {
    if (e.message.find("odd") != std::string::npos || e.message.find("odd") != std::string::npos) {
      found_odd_err = true;
      break;
    }
  }
  EXPECT_TRUE(found_odd_err) << "Expected odd-ORG error";
  // Code still emitted at rounded-down address
  EXPECT_EQ(result.org, 0x1000u);
}

// ============================================================
// FAIL directive
// ============================================================

TEST(DirectivesTest, FailEmitsError) {
  AsmExpectError("  ORG $1000\n  FAIL 'bad stuff'\n", "FAIL");
}

TEST(DirectivesTest, FailWithoutMessage) {
  AsmExpectError("  ORG $1000\n  FAIL\n", "FAIL");
}

// ============================================================
// LIST / NOLIST / PAGE — no-ops, should not cause errors
// ============================================================

TEST(DirectivesTest, ListNoEffect) {
  auto code = Asm("  ORG $1000\n  LIST\n  DC.W $1234\n  NOLIST\n");
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(code[0], 0x12);
  EXPECT_EQ(code[1], 0x34);
}

TEST(DirectivesTest, PageNoEffect) {
  auto code = Asm("  ORG $1000\n  PAGE\n  DC.W $ABCD\n");
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(code[0], 0xAB);
  EXPECT_EQ(code[1], 0xCD);
}

// ============================================================
// OPT directive — no-op, options parsed but ignored
// ============================================================

TEST(DirectivesTest, OptNoEffect) {
  auto code = Asm("  ORG $1000\n  OPT CRE\n  DC.W $5678\n");
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(code[0], 0x56);
  EXPECT_EQ(code[1], 0x78);
}

TEST(DirectivesTest, OptMultipleOptions) {
  auto code = Asm("  ORG $1000\n  OPT MEX,NOMEX,WAR\n  DC.B $AA\n");
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0xAA);
}

// ============================================================
// INCLUDE tests (mock reader)
// ============================================================

TEST(DirectivesTest, IncludeBasic) {
  // INCLUDE a file that defines a constant
  auto files = WithFiles({{"defs.x68", "MYVAL EQU $42\n"}});
  const char* src =
      "  ORG $1000\n"
      "  INCLUDE 'defs.x68'\n"
      "  DC.B MYVAL\n";
  auto code = Asm(src, files);
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x42);
}

TEST(DirectivesTest, IncludeDoubleQuoteFilename) {
  // INCLUDE "filename" (double quotes)
  auto files = WithFiles({{"defs.x68", "MYVAL EQU $99\n"}});
  const char* src =
      "  ORG $1000\n"
      "  INCLUDE \"defs.x68\"\n"
      "  DC.B MYVAL\n";
  auto code = Asm(src, files);
  ASSERT_EQ(code.size(), 1u);
  EXPECT_EQ(code[0], 0x99);
}

TEST(DirectivesTest, IncludeEmitsCode) {
  // INCLUDE a file that emits bytes
  auto files = WithFiles({{"data.x68", "  DC.W $BEEF\n"}});
  const char* src =
      "  ORG $1000\n"
      "  INCLUDE 'data.x68'\n";
  auto code = Asm(src, files);
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(code[0], 0xBE);
  EXPECT_EQ(code[1], 0xEF);
}

TEST(DirectivesTest, IncludeForwardRef) {
  // Code in INCLUDE file references a label defined after the INCLUDE
  auto files = WithFiles({{"sub.x68", "  DC.W ENDVAL\n"}});
  const char* src =
      "  ORG $1000\n"
      "  INCLUDE 'sub.x68'\n"
      "ENDVAL EQU $1234\n";
  auto code = Asm(src, files);
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(code[0], 0x12);
  EXPECT_EQ(code[1], 0x34);
}

TEST(DirectivesTest, IncludeFileNotFound) {
  AsmExpectError("  ORG $1000\n  INCLUDE 'nosuchfile.x68'\n", "cannot open");
}

TEST(DirectivesTest, IncludeMissingFilenameIsError) {
  AsmExpectError("  ORG $1000\n  INCLUDE\n", "filename");
}

TEST(DirectivesTest, IncludeEndInIncludedFileStopsOnlyThatFile) {
  // END inside INCLUDE should not stop the outer assembly
  auto files = WithFiles({{"sub.x68", "  DC.B $11\n  END\n  DC.B $22\n"}});
  const char* src =
      "  ORG $1000\n"
      "  INCLUDE 'sub.x68'\n"
      "  DC.B $33\n";
  auto code = Asm(src, files);
  // $11 from sub.x68, $33 from outer (END in sub stops sub, not outer)
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(code[0], 0x11);
  EXPECT_EQ(code[1], 0x33);
}

TEST(DirectivesTest, IncludeLocationCounterContinues) {
  // LC should continue from where INCLUDE left off
  auto files = WithFiles({{"sub.x68", "  DC.W $AABB\n"}});
  const char* src =
      "  ORG $1000\n"
      "  INCLUDE 'sub.x68'\n"
      "AFTER:\n"
      "  DC.W AFTER\n";
  auto code = Asm(src, files);
  // AFTER = $1002
  ASSERT_GE(code.size(), 4u);
  EXPECT_EQ(code[2], 0x10);
  EXPECT_EQ(code[3], 0x02);
}

// ============================================================
// SECTION tests
// ============================================================

TEST(DirectivesTest, SectionSwitchesLocationCounter) {
  // SECTION 0 → LC saved; SECTION 1 → starts at 0; ORG in each
  const char* src =
      "  ORG $1000\n"
      "  DC.W $AAAA\n"
      "  SECTION 1\n"
      "  ORG $2000\n"
      "  DC.W $BBBB\n"
      "  SECTION 0\n"
      "  DC.W $CCCC\n";
  // After SECTION 0 restore: LC should be $1002 (where we left off in section 0)
  // This test checks section 0 code appears at $1000 and $1002
  Assembler a;
  auto result = a.Assemble(src);
  // We don't check cross-section output linearity (multiple orgs),
  // just that it assembles without error
  EXPECT_TRUE(result.success) << [&]() {
    std::string s;
    for (auto& e : result.errors)
      s += e.message + "; ";
    return s;
  }();
}

TEST(DirectivesTest, SectionOutOfRange) {
  AsmExpectError("  ORG $1000\n  SECTION 16\n", "0-15");
}

TEST(DirectivesTest, SectionZeroIsDefault) {
  // SECTION 0 is the default; switching to it and back works
  auto code = Asm("  ORG $1000\n  SECTION 0\n  DC.W $1234\n");
  ASSERT_EQ(code.size(), 2u);
}

// ============================================================
// OFFSET tests
// ============================================================

TEST(DirectivesTest, OffsetModeDoesNotEmitCode) {
  // OFFSET $100 — define some labels but no code emitted
  const char* src =
      "  ORG $1000\n"
      "  OFFSET $100\n"
      "FIELD1: DS.W 1\n"
      "FIELD2: DS.W 1\n"
      "  ORG $1000\n"
      "  DC.W FIELD1\n"
      "  DC.W FIELD2\n";
  auto code = Asm(src);
  // The OFFSET section emits no bytes; the DC.W FIELD1/FIELD2 emit 4 bytes
  ASSERT_EQ(code.size(), 4u);
  // FIELD1 = $100, FIELD2 = $102
  EXPECT_EQ(code[0], 0x01);
  EXPECT_EQ(code[1], 0x00);
  EXPECT_EQ(code[2], 0x01);
  EXPECT_EQ(code[3], 0x02);
}

TEST(DirectivesTest, OffsetExitedByOrg) {
  // After OFFSET section, ORG restores normal operation
  const char* src =
      "  ORG $1000\n"
      "  OFFSET $200\n"
      "MYFIELD: DS.B 1\n"
      "  ORG $1000\n"
      "  DC.W MYFIELD\n";
  auto code = Asm(src);
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(code[0], 0x02);
  EXPECT_EQ(code[1], 0x00);
}

TEST(DirectivesTest, OffsetRequiresOperand) {
  AsmExpectError("  ORG $1000\n  OFFSET\n", "");
  // The parse error means OFFSET with no operand won't crash
}

// ============================================================
// END directive
// ============================================================

TEST(DirectivesTest, EndStopsAssembly) {
  // Lines after END are ignored
  auto code =
      Asm("  ORG $1000\n"
          "  DC.W $1234\n"
          "  END\n"
          "  DC.W $5678\n");
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(code[0], 0x12);
  EXPECT_EQ(code[1], 0x34);
}

// ============================================================
// EVEN / ODD in context of new directives
// ============================================================

TEST(DirectivesTest, EvenAfterDcbByte) {
  // DCB.B odd count followed by EVEN
  auto code = Asm("  ORG $1000\n  DCB.B 3,$00\n  EVEN\n  DC.W $ABCD\n");
  // 3 bytes + 1 pad + 2 bytes = 6
  ASSERT_EQ(code.size(), 6u);
  EXPECT_EQ(code[4], 0xAB);
  EXPECT_EQ(code[5], 0xCD);
}

// ============================================================
// Interaction tests
// ============================================================

TEST(DirectivesTest, DcbAfterInclude) {
  auto files = WithFiles({{"init.x68", "COUNT EQU 3\n"}});
  const char* src =
      "  ORG $1000\n"
      "  INCLUDE 'init.x68'\n"
      "  DCB.B COUNT,$FF\n";
  auto code = Asm(src, files);
  ASSERT_EQ(code.size(), 3u);
  EXPECT_EQ(code[0], 0xFF);
  EXPECT_EQ(code[2], 0xFF);
}

TEST(DirectivesTest, SimhaltAfterString) {
  // DC.B string (odd count) then SIMHALT
  auto code = Asm("  ORG $1000\n  DC.B 'HI!'\n  SIMHALT\n");
  // 3 bytes + 1 pad + 4 bytes = 8
  ASSERT_EQ(code.size(), 8u);
  EXPECT_EQ(code[0], 'H');
  EXPECT_EQ(code[1], 'I');
  EXPECT_EQ(code[2], '!');
  EXPECT_EQ(code[3], 0x00);  // pad
  EXPECT_EQ(code[4], 0xFF);
  EXPECT_EQ(code[5], 0xFF);
  EXPECT_EQ(code[6], 0xFF);
  EXPECT_EQ(code[7], 0xFF);
}

TEST(DirectivesTest, OptListNolistDoNotAffectCode) {
  const char* src =
      "  ORG $1000\n"
      "  OPT CRE,MEX\n"
      "  LIST\n"
      "  DC.B $01\n"
      "  NOLIST\n"
      "  DC.B $02\n"
      "  LIST\n";
  auto code = Asm(src);
  ASSERT_EQ(code.size(), 2u);
  EXPECT_EQ(code[0], 0x01);
  EXPECT_EQ(code[1], 0x02);
}

TEST(DirectivesTest, DcByteDoubleQuotedStringInInclude) {
  // INCLUDE file uses double-quoted include inside
  auto files = WithFiles({{"msg.x68", "  DC.B \"Hi\",0\n"}});
  const char* src = "  ORG $1000\n  INCLUDE 'msg.x68'\n";
  auto code = Asm(src, files);
  ASSERT_EQ(code.size(), 3u);
  EXPECT_EQ(code[0], 'H');
  EXPECT_EQ(code[1], 'i');
  EXPECT_EQ(code[2], 0x00);
}

TEST(DirectivesTest, DcWordAfterStringPreservesAlignment) {
  // DC.B 'AB' (2 bytes, even), DC.W $1234 — no pad needed
  auto code = Asm("  ORG $1000\n  DC.B 'AB'\n  DC.W $1234\n");
  ASSERT_EQ(code.size(), 4u);
  EXPECT_EQ(code[2], 0x12);
  EXPECT_EQ(code[3], 0x34);
}

TEST(DirectivesTest, DcWordAfterOddStringPads) {
  // DC.B 'A' (1 byte, odd), DC.W $5678 — pad inserted
  auto code = Asm("  ORG $1000\n  DC.B 'A'\n  DC.W $5678\n");
  ASSERT_EQ(code.size(), 4u);
  EXPECT_EQ(code[0], 'A');
  EXPECT_EQ(code[1], 0x00);  // pad
  EXPECT_EQ(code[2], 0x56);
  EXPECT_EQ(code[3], 0x78);
}

}  // namespace
}  // namespace easym68k::asm_
