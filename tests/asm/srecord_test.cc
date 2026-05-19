// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// S-Record Generator Tests for EASy68K 68000 Assembler

#include <gtest/gtest.h>

#include "easym68k/asm/assembler.h"
#include "easym68k/asm/srecord_generator.h"

namespace easym68k::asm_ {

TEST(SRecordGeneratorTest, ChecksumVerification) {
  SRecordGenerator gen;
  // Use Generate() to output S0 header and S8 termination
  gen.SetStartAddress(0x1000);
  std::string srec = gen.Generate();

  // The first line should be the S0 header:
  // S021000036384B50524F47202020323043524541544544204259204541535936384B + Checksum + \n
  EXPECT_NE(srec.find("S021000036384B50524F47202020323043524541544544204259204541535936384B6D\n"),
            std::string::npos);

  // The last line should be the S8 termination record:
  // S8 + byteCount 04 + startAddress 001000 + checksum
  // sum = 4 + 0x00 + 0x10 + 0x00 = 20 = 0x14
  // checksum = ~0x14 & 0xFF = 0xEB
  EXPECT_NE(srec.find("S804001000EB\n"), std::string::npos);
}

TEST(SRecordGeneratorTest, ContiguousAndNonContiguousGrouping) {
  SRecordGenerator gen;
  gen.SetStartAddress(0x2000);

  // Add contiguous bytes
  gen.AddByte(0x1000, 0x12);
  gen.AddByte(0x1001, 0x34);
  gen.AddByte(0x1002, 0x56);

  // Add non-contiguous byte
  gen.AddByte(0x1005, 0x78);

  std::string srec = gen.Generate();

  // First record (S1): Address 0x1000, data [0x12, 0x34, 0x56]
  // byteCount = 2 (addr) + 3 (data) + 1 (checksum) = 6
  // sum = 6 + 0x10 + 0x00 + 0x12 + 0x34 + 0x56 = 0xAC
  // checksum = ~0xAC & 0xFF = 0x4D
  EXPECT_NE(srec.find("S10610001234564D\n"), std::string::npos);

  // Second record (S1): Address 0x1005, data [0x78]
  // byteCount = 2 (addr) + 1 (data) + 1 (checksum) = 4
  // sum = 4 + 0x10 + 0x05 + 0x78 = 0x91
  // checksum = ~0x91 & 0xFF = 0x6E
  EXPECT_NE(srec.find("S1041005786E\n"), std::string::npos);
}

TEST(SRecordGeneratorTest, AddressSizeSelection) {
  SRecordGenerator gen;
  gen.SetStartAddress(0x0);

  // S1 (16-bit address)
  gen.AddByte(0x1234, 0xAA);

  // S2 (24-bit address)
  gen.AddByte(0x123456, 0xBB);

  // S3 (32-bit address)
  gen.AddByte(0x12345678, 0xCC);

  std::string srec = gen.Generate();

  // S1041234AA...
  // sum = 4 + 0x12 + 0x34 + 0xAA = 0xF4
  // checksum = ~0xF4 & 0xFF = 0x0B
  EXPECT_NE(srec.find("S1041234AA0B\n"), std::string::npos);

  // S205123456BB...
  // sum = 5 + 0x12 + 0x34 + 0x56 + 0xBB = 0x15C
  // checksum = ~0x5C & 0xFF = 0xA3
  EXPECT_NE(srec.find("S205123456BBA3\n"), std::string::npos);

  // S30612345678CC...
  // sum = 6 + 0x12 + 0x34 + 0x56 + 0x78 + 0xCC = 0x1E6
  // checksum = ~0xE6 & 0xFF = 0x19
  EXPECT_NE(srec.find("S30612345678CC19\n"), std::string::npos);
}

TEST(SRecordGeneratorTest, MemoryMapsVerification) {
  SRecordGenerator gen;
  gen.SetStartAddress(0x1000);
  gen.SetMemoryMaps(true, 0x1000, 0x2000, true, 0x3000, 0x4000, true, 0x5000, 0x6000, true, 0x7000,
                    0x8000);

  std::string srec = gen.Generate();

  // ROM map: name="      ROM ", start=0x001000, end=0x002000
  // name_hex = "202020202020524F4D20" -> "      ROM "
  // start_hex (ASCII) = "303031303030" -> "001000"
  // end_hex (ASCII) = "303032303030" -> "002000"
  EXPECT_NE(srec.find("S01A0000202020202020524F4D203030313030302C303032303030"), std::string::npos);

  // READ map: name="     READ ", start=0x003000, end=0x004000
  EXPECT_NE(srec.find("S01A0000202020202052454144203030333030302C303034303030"), std::string::npos);

  // PROTECTED map: name="PROTECTED ", start=0x005000, end=0x006000
  EXPECT_NE(srec.find("S01A000050524F544543544544203030353030302C303036303030"), std::string::npos);

  // INVALID map: name="  INVALID ", start=0x007000, end=0x008000
  EXPECT_NE(srec.find("S01A00002020494E56414C4944203030373030302C303038303030"), std::string::npos);
}

TEST(SRecordGeneratorTest, AssemblerIntegration) {
  Assembler assembler;
  std::string source = R"(
    ORG $1000
START:
    NOP
    END START
  )";

  AssemblyResult result = assembler.Assemble(source);
  ASSERT_TRUE(result.success);
  EXPECT_EQ(result.org, 0x1000);
  EXPECT_FALSE(result.srecord.empty());

  // Check that the output S-Record contains:
  // - S0 header
  // - S1 data record with NOP opcode (0x4E71)
  // - S8 termination record
  EXPECT_NE(result.srecord.find(
                "S021000036384B50524F47202020323043524541544544204259204541535936384B6D\n"),
            std::string::npos);
  EXPECT_NE(result.srecord.find("S10510004E712B\n"), std::string::npos);
  EXPECT_NE(result.srecord.find("S804001000EB\n"), std::string::npos);
}

}  // namespace easym68k::asm_
