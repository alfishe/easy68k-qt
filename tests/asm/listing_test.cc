// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Listing Generator Tests for EASy68K Port

#include <gtest/gtest.h>

#include "easym68k/asm/assembler.h"
#include "easym68k/asm/listing_generator.h"

namespace easym68k::asm_ {
namespace {

TEST(ListingGeneratorTest, TabStopExpansion) {
  ListingGenerator gen;
  gen.Configure("test.X68", "05/19/2026", 60, true);

  // Tab stop 1 is column 12. Let's test standard smart tab stop.
  // Input: "A\tB"
  // Horizontal index j starts at 0. 'A' at j=0. '\t' at j=1.
  // Smart tab stop 1 is at 12. So 12 - 1 = 11 spaces.
  // Result should have 'A' followed by 11 spaces, then 'B'.
  std::string expanded = gen.ExpandTabs("A\tB");
  EXPECT_EQ(expanded, "A           B");

  // Input: "ABC\t" -> 'ABC' (len 3), tab expands to column 12 -> 9 spaces.
  expanded = gen.ExpandTabs("ABC\t");
  EXPECT_EQ(expanded, "ABC         ");

  // Input: past TAB1 (12) but before TAB2 (20).
  // "0123456789012\t" (len 13), expands to column 20 -> 7 spaces.
  expanded = gen.ExpandTabs("0123456789012\t");
  EXPECT_EQ(expanded, "0123456789012       ");

  // Input: past TAB2 (20) but before TAB3 (44).
  // "0123456789012345678901\t" (len 22), expands to column 44 -> 22 spaces.
  expanded = gen.ExpandTabs("0123456789012345678901\t");
  EXPECT_EQ(expanded, "0123456789012345678901                      ");
}

TEST(ListingGeneratorTest, LeftColumnFormatterWithoutCex) {
  ListingGenerator gen;
  gen.Configure("test.X68", "05/19/2026", 60, false);  // Cex flag false

  // Emitting items: two words and one long (total 8 bytes).
  // Fits in 32 chars:
  // "00001000  1234 5678 12345678    "
  // Length is 32.
  std::vector<EmittedItem> emitted = {{0x1234, 2}, {0x5678, 2}, {0x12345678, 4}};

  std::vector<std::string> cols = gen.FormatLeftColumns(0x1000, emitted, false, false, 0, false);
  ASSERT_EQ(cols.size(), 1u);
  EXPECT_EQ(cols[0].length(), 32u);
  EXPECT_EQ(cols[0].substr(0, 10), "00001000  ");
  EXPECT_EQ(cols[0], "00001000  1234 5678 12345678    ");

  // Test truncation when exceeding boundaries with cex_flag = false
  // Emitting five words: 5 * 5 = 25 chars of code.
  // 10 chars of address + 25 = 35 chars > 31.
  // It should place "..." at index 26 since it is WORD_SIZE (size == 2).
  std::vector<EmittedItem> many_words = {
      {0x1111, 2}, {0x2222, 2}, {0x3333, 2}, {0x4444, 2}, {0x5555, 2}};
  std::vector<std::string> trunc_cols =
      gen.FormatLeftColumns(0x1000, many_words, false, false, 0, false);
  ASSERT_EQ(trunc_cols.size(), 1u);
  EXPECT_EQ(trunc_cols[0].length(), 32u);
  EXPECT_EQ(trunc_cols[0].substr(26, 3), "...");
}

TEST(ListingGeneratorTest, LeftColumnFormatterWithCex) {
  ListingGenerator gen;
  gen.Configure("test.X68", "05/19/2026", 60, true);  // Cex flag true

  // Emitting five words.
  // Address + 4 words: "00001000  1111 2222 3333 4444   " (32 chars)
  // Continuation line: "          5555                  " (32 chars)
  std::vector<EmittedItem> many_words = {
      {0x1111, 2}, {0x2222, 2}, {0x3333, 2}, {0x4444, 2}, {0x5555, 2}};
  std::vector<std::string> cex_cols =
      gen.FormatLeftColumns(0x1000, many_words, false, false, 0, false);
  ASSERT_EQ(cex_cols.size(), 2u);
  EXPECT_EQ(cex_cols[0], "00001000  1111 2222 3333 4444   ");
  EXPECT_EQ(cex_cols[1], "          5555                  ");
}

TEST(ListingGeneratorTest, EquSetDirectivesLeftColumn) {
  ListingGenerator gen;
  gen.Configure("test.X68", "05/19/2026", 60, true);

  // EQU directive formatting: =XXXXXXXX
  std::vector<std::string> cols = gen.FormatLeftColumns(0x1000, {}, false, false, 0x1234BEEF, true);
  ASSERT_EQ(cols.size(), 1u);
  EXPECT_EQ(cols[0], "00001000  =1234BEEF             ");
}

TEST(ListingGeneratorTest, AssemblerGetListingValidation) {
  Assembler a;
  a.ConfigureListing("my_test_program.X68", "May 19, 2026", 60, true);
  auto r = a.Assemble(
      "         ORG    $1000\n"
      "MY_VAL   EQU    $42\n"
      "         NOP\n"
      "         DC.W   $1234,$5678,$9ABC,$DEF0,$1111\n"
      "         END\n");
  EXPECT_TRUE(r.success);

  std::string listing = a.GetListing();
  // Ensure basic header items exist
  EXPECT_NE(listing.find("00001000 Starting Address"), std::string::npos);
  EXPECT_NE(listing.find("Assembler used: EASy68K"), std::string::npos);
  EXPECT_NE(listing.find("Created On: May 19, 2026"), std::string::npos);
  EXPECT_NE(listing.find("Page 1"), std::string::npos);

  // Check EQU listing format
  EXPECT_NE(listing.find("00001000  =00000042                  2  MY_VAL   EQU    $42"),
            std::string::npos);

  // Check NOP machine code
  EXPECT_NE(listing.find("00001000  4E71                       3           NOP"),
            std::string::npos);

  // Check CEX continuation for DC.W list of values
  // Word 1-4: 1234 5678 9ABC DEF0
  // Continuation line word 5: 1111
  EXPECT_NE(
      listing.find(
          "00001002  1234 5678 9ABC DEF0        4           DC.W   $1234,$5678,$9ABC,$DEF0,$1111"),
      std::string::npos);
  EXPECT_NE(listing.find("          1111                  "), std::string::npos);
}

TEST(ListingGeneratorTest, ErrorAndWarningListing) {
  Assembler a;
  a.ConfigureListing("errors.X68", "May 19, 2026", 60, true);
  auto r = a.Assemble(
      "         ORG    $1000\n"
      "         BOGUS_OPCODE\n");
  EXPECT_FALSE(r.success);

  std::string listing = a.GetListing();
  // Check that the error is formatted directly under or inside the listing correctly
  EXPECT_NE(listing.find("Line 2 ERROR:"), std::string::npos);
  EXPECT_NE(listing.find("1 error detected"), std::string::npos);
}

}  // namespace
}  // namespace easym68k::asm_
