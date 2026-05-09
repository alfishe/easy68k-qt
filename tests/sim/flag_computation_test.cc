// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/sim/flag_computation.h"

#include <gtest/gtest.h>

namespace easym68k::sim {
namespace {

// ---------------------------------------------------------------------------
// Fixture: fresh CpuState with X bit initially set to detect accidental clears
// ---------------------------------------------------------------------------

class FlagTest : public ::testing::Test {
 protected:
  CpuState state_{};

  void SetUp() override {
    state_ = CpuState{};
    state_.sr = kSrExtend;  // X set initially
  }
};

// ---------------------------------------------------------------------------
// 14. FlagAddOverflow
// ---------------------------------------------------------------------------

TEST_F(FlagTest, FlagAddOverflow_Set) {
  // 0x7F + 0x01 = 0x80 (byte): positive + positive = negative → V=1
  UpdateFlagsAdd(state_, 0x7F, 0x01, 0x80, DataSize::kByte);
  EXPECT_TRUE(state_.GetFlag(kSrOverflow));
  EXPECT_TRUE(state_.GetFlag(kSrNegative));
  EXPECT_FALSE(state_.GetFlag(kSrZero));
  EXPECT_FALSE(state_.GetFlag(kSrCarry));
}

TEST_F(FlagTest, FlagAddOverflow_Clear) {
  // 0x01 + 0x02 = 0x03 (byte): no overflow
  UpdateFlagsAdd(state_, 0x01, 0x02, 0x03, DataSize::kByte);
  EXPECT_FALSE(state_.GetFlag(kSrOverflow));
}

TEST_F(FlagTest, FlagAddOverflow_NegPlusNeg) {
  // 0x80 + 0x80 = 0x00 (byte): negative + negative = positive → V=1
  UpdateFlagsAdd(state_, 0x80, 0x80, 0x00, DataSize::kByte);
  EXPECT_TRUE(state_.GetFlag(kSrOverflow));
  EXPECT_TRUE(state_.GetFlag(kSrCarry));
  EXPECT_TRUE(state_.GetFlag(kSrZero));
}

// ---------------------------------------------------------------------------
// 15. FlagAddCarry
// ---------------------------------------------------------------------------

TEST_F(FlagTest, FlagAddCarry_Set) {
  // 0xFF + 0x01 = 0x00 (byte): carry out → C=1, X=1
  UpdateFlagsAdd(state_, 0xFF, 0x01, 0x00, DataSize::kByte);
  EXPECT_TRUE(state_.GetFlag(kSrCarry));
  EXPECT_TRUE(state_.GetFlag(kSrExtend));
  EXPECT_TRUE(state_.GetFlag(kSrZero));
}

TEST_F(FlagTest, FlagAddCarry_Clear) {
  // 0x01 + 0x01 = 0x02 (byte): no carry
  UpdateFlagsAdd(state_, 0x01, 0x01, 0x02, DataSize::kByte);
  EXPECT_FALSE(state_.GetFlag(kSrCarry));
  EXPECT_FALSE(state_.GetFlag(kSrExtend));
}

TEST_F(FlagTest, FlagAddCarry_Word) {
  // 0xFFFF + 0x0001 = 0x0000 (word): carry
  UpdateFlagsAdd(state_, 0xFFFF, 0x0001, 0x0000, DataSize::kWord);
  EXPECT_TRUE(state_.GetFlag(kSrCarry));
  EXPECT_TRUE(state_.GetFlag(kSrZero));
}

// ---------------------------------------------------------------------------
// 16. FlagSubBorrow
// ---------------------------------------------------------------------------

TEST_F(FlagTest, FlagSubBorrow_Set) {
  // 0x01 - 0x02 = 0xFF (byte): borrow → C=1
  UpdateFlagsSub(state_, 0x02, 0x01, 0xFF, DataSize::kByte);
  EXPECT_TRUE(state_.GetFlag(kSrCarry));
  EXPECT_TRUE(state_.GetFlag(kSrExtend));
  EXPECT_TRUE(state_.GetFlag(kSrNegative));
}

TEST_F(FlagTest, FlagSubBorrow_Clear) {
  // 0x03 - 0x01 = 0x02 (byte): no borrow
  UpdateFlagsSub(state_, 0x01, 0x03, 0x02, DataSize::kByte);
  EXPECT_FALSE(state_.GetFlag(kSrCarry));
  EXPECT_FALSE(state_.GetFlag(kSrExtend));
  EXPECT_FALSE(state_.GetFlag(kSrNegative));
}

TEST_F(FlagTest, FlagSubOverflow) {
  // 0x80 - 0x01 = 0x7F (byte): neg - pos = pos → V=1
  UpdateFlagsSub(state_, 0x01, 0x80, 0x7F, DataSize::kByte);
  EXPECT_TRUE(state_.GetFlag(kSrOverflow));
}

// ---------------------------------------------------------------------------
// 17. FlagLogicClearsVC
// ---------------------------------------------------------------------------

TEST_F(FlagTest, FlagLogicClearsVC) {
  state_.sr |= kSrOverflow | kSrCarry;  // set V and C
  UpdateFlagsLogic(state_, 0x42, DataSize::kByte);
  EXPECT_FALSE(state_.GetFlag(kSrOverflow));
  EXPECT_FALSE(state_.GetFlag(kSrCarry));
  EXPECT_FALSE(state_.GetFlag(kSrNegative));
  EXPECT_FALSE(state_.GetFlag(kSrZero));
  EXPECT_TRUE(state_.GetFlag(kSrExtend));  // X unchanged
}

TEST_F(FlagTest, FlagLogicSetsNZ) {
  UpdateFlagsLogic(state_, 0x00, DataSize::kWord);
  EXPECT_TRUE(state_.GetFlag(kSrZero));
  EXPECT_FALSE(state_.GetFlag(kSrNegative));

  UpdateFlagsLogic(state_, 0x8000, DataSize::kWord);
  EXPECT_TRUE(state_.GetFlag(kSrNegative));
  EXPECT_FALSE(state_.GetFlag(kSrZero));
}

// ---------------------------------------------------------------------------
// 18. FlagMoveClearsVC
// ---------------------------------------------------------------------------

TEST_F(FlagTest, FlagMoveClearsVC) {
  state_.sr |= kSrOverflow | kSrCarry;
  UpdateFlagsMove(state_, 0x1234, DataSize::kWord);
  EXPECT_FALSE(state_.GetFlag(kSrOverflow));
  EXPECT_FALSE(state_.GetFlag(kSrCarry));
  EXPECT_FALSE(state_.GetFlag(kSrNegative));
  EXPECT_FALSE(state_.GetFlag(kSrZero));
  EXPECT_TRUE(state_.GetFlag(kSrExtend));  // X unchanged
}

TEST_F(FlagTest, FlagMoveNegative) {
  UpdateFlagsMove(state_, 0x80000000, DataSize::kLong);
  EXPECT_TRUE(state_.GetFlag(kSrNegative));
  EXPECT_FALSE(state_.GetFlag(kSrZero));
}

}  // namespace
}  // namespace easym68k::sim
