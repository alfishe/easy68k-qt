// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/sim/memory.h"

#include <gtest/gtest.h>

namespace easym68k::sim {
namespace {

class MemoryTest : public ::testing::Test {
 protected:
  static Memory memory;
  void SetUp() override { memory.Clear(); }
};

Memory MemoryTest::memory;

// ---------------------------------------------------------------------------
// Ported from EASy68K-qt memory_test.cc (adapted for new API)
// ---------------------------------------------------------------------------

TEST_F(MemoryTest, InitiallyZero) {
  EXPECT_EQ(memory.ReadByte(0).value, 0u);
  EXPECT_EQ(memory.ReadByte(0x1000).value, 0u);
  EXPECT_EQ(memory.ReadByte(0xFFFFFF).value, 0u);
}

TEST_F(MemoryTest, WriteByte) {
  EXPECT_EQ(memory.WriteByte(0x1000, 0x42), SimResult::kOk);
  auto r = memory.ReadByte(0x1000);
  EXPECT_TRUE(r.success);
  EXPECT_EQ(r.value, 0x42u);
}

TEST_F(MemoryTest, WriteWord) {
  EXPECT_EQ(memory.WriteWord(0x1000, 0x1234), SimResult::kOk);
  auto r = memory.ReadWord(0x1000);
  EXPECT_TRUE(r.success);
  EXPECT_EQ(r.value, 0x1234u);
}

TEST_F(MemoryTest, WriteLong) {
  EXPECT_EQ(memory.WriteLong(0x1000, 0x12345678), SimResult::kOk);
  auto r = memory.ReadLong(0x1000);
  EXPECT_TRUE(r.success);
  EXPECT_EQ(r.value, 0x12345678u);
}

TEST_F(MemoryTest, OddAddressWordError) {
  auto r = memory.ReadWord(0x1001);
  EXPECT_FALSE(r.success);
  EXPECT_EQ(r.error, SimResult::kAddressError);
  EXPECT_EQ(memory.WriteWord(0x1001, 0x1234), SimResult::kAddressError);
}

TEST_F(MemoryTest, OddAddressLongError) {
  auto r = memory.ReadLong(0x1001);
  EXPECT_FALSE(r.success);
  EXPECT_EQ(r.error, SimResult::kAddressError);
  EXPECT_EQ(memory.WriteLong(0x1001, 0x12345678), SimResult::kAddressError);
}

TEST_F(MemoryTest, AddressWrapping) {
  memory.WriteByte(0x1000000, 0x42);  // wraps to 0x000000
  EXPECT_EQ(memory.ReadByte(0x000000).value, 0x42u);
}

TEST_F(MemoryTest, RomProtection) {
  memory.SetFlags(0x1000, 0x1FFF, kMemRom);
  EXPECT_EQ(memory.WriteByte(0x1000, 0x42), SimResult::kBusError);
  EXPECT_EQ(memory.WriteWord(0x1000, 0x1234), SimResult::kBusError);
  EXPECT_EQ(memory.WriteLong(0x1000, 0x12345678), SimResult::kBusError);
  // Reads still succeed
  EXPECT_TRUE(memory.ReadByte(0x1000).success);
}

TEST_F(MemoryTest, ProtectedMemory) {
  memory.SetFlags(0x2000, 0x2FFF, kMemProtected);
  EXPECT_EQ(memory.WriteByte(0x2000, 0x42), SimResult::kBusError);
}

TEST_F(MemoryTest, LoadData) {
  std::vector<uint8_t> data = {0x12, 0x34, 0x56, 0x78};
  memory.LoadData(0x1000, data);
  EXPECT_EQ(memory.ReadByte(0x1000).value, 0x12u);
  EXPECT_EQ(memory.ReadByte(0x1001).value, 0x34u);
  EXPECT_EQ(memory.ReadByte(0x1002).value, 0x56u);
  EXPECT_EQ(memory.ReadByte(0x1003).value, 0x78u);
}

TEST_F(MemoryTest, Clear) {
  memory.WriteByte(0x1000, 0x42);
  memory.Clear();
  EXPECT_EQ(memory.ReadByte(0x1000).value, 0u);
}

// ---------------------------------------------------------------------------
// New tests required by Task 1.4
// ---------------------------------------------------------------------------

TEST_F(MemoryTest, ReadByteInvalidAddress) {
  memory.SetFlags(0x3000, 0x3FFF, kMemInvalid);
  auto r = memory.ReadByte(0x3000);
  EXPECT_FALSE(r.success);
  EXPECT_EQ(r.error, SimResult::kBusError);
}

TEST_F(MemoryTest, ReadByteBoundary) {
  memory.WriteByte(0, 0xAB);
  memory.WriteByte(0xFFFFFF, 0xCD);
  EXPECT_EQ(memory.ReadByte(0).value, 0xABu);
  EXPECT_EQ(memory.ReadByte(0xFFFFFF).value, 0xCDu);
}

TEST_F(MemoryTest, WriteThenReadWordEndian) {
  memory.WriteWord(0x2000, 0xBEEF);
  // 68000 big-endian: high byte at lower address
  EXPECT_EQ(memory.ReadByte(0x2000).value, 0xBEu);
  EXPECT_EQ(memory.ReadByte(0x2001).value, 0xEFu);
}

TEST_F(MemoryTest, WriteThenReadLongEndian) {
  memory.WriteLong(0x4000, 0xDEADBEEF);
  EXPECT_EQ(memory.ReadByte(0x4000).value, 0xDEu);
  EXPECT_EQ(memory.ReadByte(0x4001).value, 0xADu);
  EXPECT_EQ(memory.ReadByte(0x4002).value, 0xBEu);
  EXPECT_EQ(memory.ReadByte(0x4003).value, 0xEFu);
}

TEST_F(MemoryTest, DoubleAlignmentError) {
  auto rw = memory.ReadWord(0x0101);
  EXPECT_FALSE(rw.success);
  EXPECT_EQ(rw.error, SimResult::kAddressError);
  EXPECT_FALSE(rw.error_message.empty());

  auto rl = memory.ReadLong(0x0101);
  EXPECT_FALSE(rl.success);
  EXPECT_EQ(rl.error, SimResult::kAddressError);

  EXPECT_EQ(memory.WriteWord(0x0101, 0), SimResult::kAddressError);
  EXPECT_EQ(memory.WriteLong(0x0101, 0), SimResult::kAddressError);
}

TEST_F(MemoryTest, ProtectedThenUnprotect) {
  memory.SetFlags(0x5000, 0x5FFF, kMemProtected);
  EXPECT_EQ(memory.WriteByte(0x5000, 1), SimResult::kBusError);

  memory.SetFlags(0x5000, 0x5FFF, kMemNormal);
  EXPECT_EQ(memory.WriteByte(0x5000, 1), SimResult::kOk);
  EXPECT_EQ(memory.ReadByte(0x5000).value, 1u);
}

}  // namespace
}  // namespace easym68k::sim
