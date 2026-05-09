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

  void SetUp() override {
    memory.Clear();
  }
};

Memory SRecordLoaderTest::memory;

// ---------------------------------------------------------------------------
// Valid records — Task 2.2 required tests
// ---------------------------------------------------------------------------

TEST_F(SRecordLoaderTest, ChecksumValid) {
  auto r = loader.LoadFromLines({"S107010012345678E3", "S9030100FB"}, memory);
  EXPECT_TRUE(r.success);
  EXPECT_EQ(memory.ReadByte(0x0100).value, 0x12u);
}

TEST_F(SRecordLoaderTest, ChecksumInvalid) {
  // Corrupt the checksum byte (E3 → 00)
  auto r = loader.LoadFromLines({"S10701001234567800"}, memory);
  EXPECT_FALSE(r.success);
  EXPECT_GT(r.error_line, 0);
  EXPECT_FALSE(r.error_message.empty());
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
  EXPECT_FALSE(r.success);
  EXPECT_FALSE(r.error_message.empty());
}

TEST_F(SRecordLoaderTest, MissingSRecordHeader) {
  // Lines without 'S' prefix are silently skipped; load still succeeds
  auto r = loader.LoadFromLines({"not a record", "; comment", "S9030100FB"}, memory);
  EXPECT_TRUE(r.success);
  EXPECT_TRUE(r.has_start_address);
}

TEST_F(SRecordLoaderTest, TruncatedRecord) {
  // byte_count = 07 but only partial data follows
  auto r = loader.LoadFromLines({"S1070100123456"}, memory);
  EXPECT_FALSE(r.success);
  EXPECT_EQ(r.error_line, 1);
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
  // 's' (lowercase) is accepted as well as 'S'
  // s1 07 01 00 12 34 56 78 E3 — same record with lowercase 's'
  auto r = loader.LoadFromLines({"s107010012345678E3", "S9030100FB"}, memory);
  EXPECT_TRUE(r.success);
  EXPECT_EQ(memory.ReadByte(0x0100).value, 0x12u);
}

TEST_F(SRecordLoaderTest, NoStartAddress) {
  // A valid S1 record with no S9 terminator — has_start_address stays false
  auto r = loader.LoadFromLines({"S107010012345678E3"}, memory);
  EXPECT_TRUE(r.success);
  EXPECT_FALSE(r.has_start_address);
}

}  // namespace
}  // namespace easym68k::sim
