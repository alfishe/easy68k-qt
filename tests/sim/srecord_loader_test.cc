// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Checksum derivation for each record:
//   sum = byte_count + every body byte (addr + data); checksum = ~sum & 0xFF
//   Valid record satisfies: (sum + checksum) & 0xFF == 0xFF
//
//  S107010012345678E3  S1, addr=0x0100, data={12,34,56,78}
//    sum=07+01+00+12+34+56+78=0x1C  chk=0xE3
//  S206010000AABB93    S2, addr=0x010000, data={AA,BB}
//    sum=06+01+00+00+AA+BB=0x6C  chk=0x93
//  S30700001000CCDD3F  S3, addr=0x00001000, data={CC,DD}
//    sum=07+00+00+10+00+CC+DD=0xC0  chk=0x3F
//  S9030100FB          S9, start=0x0100  sum=03+01+00=0x04  chk=0xFB
//  S804010000FA        S8, start=0x010000  sum=04+01+00+00=0x05  chk=0xFA
//  S70500001000EA      S7, start=0x00001000  sum=05+00+00+10+00=0x15  chk=0xEA
//  S10701049ABCDEF0CF  S1, addr=0x0104, data={9A,BC,DE,F0}
//    sum=07+01+04+9A+BC+DE+F0=0x30  chk=0xCF

#include "easym68k/sim/srecord_loader.h"

#include <gtest/gtest.h>

#include <sstream>

namespace easym68k::sim {
namespace {

class SRecordLoaderTest : public ::testing::Test {
 protected:
  static Memory memory;
  SRecordLoader loader;

  void SetUp() override { memory.Clear(); }
};

Memory SRecordLoaderTest::memory;

// ---------------------------------------------------------------------------
// Valid records — Task 2.2 required tests
// ---------------------------------------------------------------------------

// SIMOPS2.CPP loadSrec(): each S0 record's data as text, '.' for bytes
// outside ' '..'~' (the Sim68K window reads the memory map from these).
TEST_F(SRecordLoaderTest, S0TextsAreKept) {
  std::vector<std::string> lines = {
      "S021000036384B50524F47202020323043524541544544204259204541535936384B6D",
      "S00500000141B8",
      "S9030100FB",
  };
  auto r = loader.LoadFromLines(lines, memory);
  ASSERT_TRUE(r.success);
  ASSERT_EQ(r.s0_texts.size(), 2u);
  EXPECT_EQ(r.s0_texts[0], "S0 = 68KPROG   20CREATED BY EASY68K");
  EXPECT_EQ(r.s0_texts[1], "S0 = .A");
}

TEST_F(SRecordLoaderTest, ChecksumValid) {
  auto r = loader.LoadFromLines({"S107010012345678E3", "S9030100FB"}, memory);
  EXPECT_TRUE(r.success);
  EXPECT_EQ(memory.ReadByte(0x0100).value, 0x12u);
}

TEST_F(SRecordLoaderTest, ChecksumInvalid) {
  // loadSrec(): a checksum error is reported and loading goes on.
  auto r = loader.LoadFromLines({"S10701001234567800", "S10701049ABCDEF0CF", "S9030100FB"}, memory);
  EXPECT_TRUE(r.success);
  EXPECT_EQ(r.error_line, 0);
  EXPECT_EQ(memory.ReadByte(0x0100).value, 0x12u);
  EXPECT_EQ(memory.ReadByte(0x0104).value, 0x9Au);
  EXPECT_EQ(r.messages,
            (std::vector<std::string>{"Checksum error on line 1...", ".S68 file read successful"}));
}

TEST_F(SRecordLoaderTest, S1Record) {
  auto r = loader.LoadFromLines({"S107010012345678E3", "S9030100FB"}, memory);
  EXPECT_TRUE(r.success);
  EXPECT_EQ(memory.ReadByte(0x0100).value, 0x12u);
  EXPECT_EQ(memory.ReadByte(0x0101).value, 0x34u);
  EXPECT_EQ(memory.ReadByte(0x0102).value, 0x56u);
  EXPECT_EQ(memory.ReadByte(0x0103).value, 0x78u);
  EXPECT_TRUE(r.has_start_address);
  EXPECT_EQ(r.start_address, 0x0100u);
}

TEST_F(SRecordLoaderTest, S2Record) {
  auto r = loader.LoadFromLines({"S206010000AABB93", "S804010000FA"}, memory);
  EXPECT_TRUE(r.success);
  EXPECT_EQ(memory.ReadByte(0x010000).value, 0xAAu);
  EXPECT_EQ(memory.ReadByte(0x010001).value, 0xBBu);
  EXPECT_TRUE(r.has_start_address);
  EXPECT_EQ(r.start_address, 0x010000u);
}

TEST_F(SRecordLoaderTest, S3Record) {
  auto r = loader.LoadFromLines({"S30700001000CCDD3F", "S70500001000EA"}, memory);
  EXPECT_TRUE(r.success);
  EXPECT_EQ(memory.ReadByte(0x001000).value, 0xCCu);
  EXPECT_EQ(memory.ReadByte(0x001001).value, 0xDDu);
  EXPECT_TRUE(r.has_start_address);
  EXPECT_EQ(r.start_address, 0x001000u);
}

TEST_F(SRecordLoaderTest, S7StartAddress) {
  auto r = loader.LoadFromLines({"S70500001000EA"}, memory);
  EXPECT_TRUE(r.success);
  EXPECT_TRUE(r.has_start_address);
  EXPECT_EQ(r.start_address, 0x001000u);
}

TEST_F(SRecordLoaderTest, S8StartAddress) {
  auto r = loader.LoadFromLines({"S804010000FA"}, memory);
  EXPECT_TRUE(r.success);
  EXPECT_TRUE(r.has_start_address);
  EXPECT_EQ(r.start_address, 0x010000u);
}

TEST_F(SRecordLoaderTest, S9StartAddress) {
  auto r = loader.LoadFromLines({"S9030100FB"}, memory);
  EXPECT_TRUE(r.success);
  EXPECT_TRUE(r.has_start_address);
  EXPECT_EQ(r.start_address, 0x0100u);
}

TEST_F(SRecordLoaderTest, EmptyFile) {
  auto r = loader.LoadFromLines({}, memory);
  EXPECT_TRUE(r.success);
  EXPECT_FALSE(r.has_start_address);
  EXPECT_EQ(r.messages, (std::vector<std::string>{".S68 file read successful"}));
}

TEST_F(SRecordLoaderTest, MissingSRecordHeader) {
  // A line that is not a record stops the load; the data before it stays.
  auto r = loader.LoadFromLines({"S107010012345678E3", "not a record", "S9030100FB"}, memory);
  EXPECT_TRUE(r.success);
  EXPECT_EQ(r.error_line, 2);
  EXPECT_FALSE(r.has_start_address);
  EXPECT_EQ(memory.ReadByte(0x0100).value, 0x12u);
  EXPECT_EQ(r.messages,
            (std::vector<std::string>{"Invalid data on line 2 of .S68 file...", "2: not a record",
                                      "Remainder of load stopped..."}));
}

TEST_F(SRecordLoaderTest, TruncatedRecord) {
  // byte_count = 07 but only partial data follows
  auto r = loader.LoadFromLines({"S1070100123456"}, memory);
  EXPECT_TRUE(r.success);
  EXPECT_EQ(r.error_line, 1);
  EXPECT_EQ(r.error_message, "Invalid data on line 1 of .S68 file...");
}

TEST_F(SRecordLoaderTest, MultipleDataRecords) {
  auto r = loader.LoadFromLines({"S107010012345678E3", "S10701049ABCDEF0CF", "S9030100FB"}, memory);
  EXPECT_TRUE(r.success);
  EXPECT_EQ(memory.ReadByte(0x0100).value, 0x12u);
  EXPECT_EQ(memory.ReadByte(0x0104).value, 0x9Au);
  EXPECT_EQ(memory.ReadByte(0x0107).value, 0xF0u);
}

TEST_F(SRecordLoaderTest, LoadFromStream) {
  std::istringstream ss("S107010012345678E3\nS9030100FB\n");
  auto r = loader.LoadFromStream(ss, memory);
  EXPECT_TRUE(r.success);
  EXPECT_EQ(memory.ReadByte(0x0100).value, 0x12u);
  EXPECT_TRUE(r.has_start_address);
  EXPECT_EQ(r.start_address, 0x0100u);
}

// ---------------------------------------------------------------------------
// Additional tests ported and adapted from EASy68K-qt srecord_test.cc
// ---------------------------------------------------------------------------

TEST_F(SRecordLoaderTest, EmptyLinesSkipped) {
  auto r = loader.LoadFromLines({"", "S107010012345678E3", "", "S9030100FB"}, memory);
  EXPECT_TRUE(r.success);
  EXPECT_EQ(memory.ReadByte(0x0100).value, 0x12u);
}

TEST_F(SRecordLoaderTest, LowercaseSPrefix) {
  // loadSrec() reads "S%c": a lower-case 's' is not a record.
  auto r = loader.LoadFromLines({"s107010012345678E3", "S9030100FB"}, memory);
  EXPECT_EQ(r.error_line, 1);
  EXPECT_NE(memory.ReadByte(0x0100).value, 0x12u);
}

TEST_F(SRecordLoaderTest, LoadingStopsAtTheStartAddress) {
  // Lines after S7/S8/S9 are not read; their checksum is not tested.
  auto r = loader.LoadFromLines({"S9030100FF", "S107010012345678E3"}, memory);
  EXPECT_TRUE(r.has_start_address);
  EXPECT_EQ(r.start_address, 0x0100u);
  EXPECT_NE(memory.ReadByte(0x0100).value, 0x12u);
  EXPECT_EQ(r.messages, (std::vector<std::string>{".S68 file read successful"}));
}

TEST_F(SRecordLoaderTest, S5IgnoredOtherTypesStop) {
  auto r = loader.LoadFromLines({"S5030001FB", "S107010012345678E3", "S6040000010A"}, memory);
  EXPECT_EQ(memory.ReadByte(0x0100).value, 0x12u);
  EXPECT_EQ(r.error_line, 3);
}

TEST_F(SRecordLoaderTest, DataPastMemoryStops) {
  // S3 FFFFFFFE: bytes past $FFFFFF are refused ("Invalid Address").
  // sum = 07+00+FF+FF+FE+11+22 = 0x236 -> checksum C9
  auto r = loader.LoadFromLines({"S30700FFFFFE1122C9", "S9030100FB"}, memory);
  EXPECT_EQ(memory.ReadByte(0xFFFFFE).value, 0x11u);
  EXPECT_EQ(memory.ReadByte(0xFFFFFF).value, 0x22u);
  auto r2 = loader.LoadFromLines({"S30701000000AABB92", "S9030100FB"}, memory);
  EXPECT_EQ(r2.error_line, 1);
  EXPECT_EQ(r2.messages.front(), "Invalid Address on line 1...");
  EXPECT_EQ(r2.messages[1], "Invalid data on line 1 of .S68 file...");
  EXPECT_TRUE(r.messages.back() == ".S68 file read successful");
}

TEST_F(SRecordLoaderTest, CannotOpen) {
  auto r = loader.LoadFromFile("/nonexistent/x.S68", memory);
  EXPECT_FALSE(r.success);
  EXPECT_EQ(r.messages, (std::vector<std::string>{"error: cannot open file /nonexistent/x.S68"}));
}

TEST_F(SRecordLoaderTest, NoStartAddress) {
  // A valid S1 record with no S9 terminator — has_start_address stays false
  auto r = loader.LoadFromLines({"S107010012345678E3"}, memory);
  EXPECT_TRUE(r.success);
  EXPECT_FALSE(r.has_start_address);
}

}  // namespace
}  // namespace easym68k::sim
