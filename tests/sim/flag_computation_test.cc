// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/sim/flag_computation.h"

#include <gtest/gtest.h>

#include "easym68k/sim/simulator.h"

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

// =============================================================================
// InstrFlagTest — end-to-end N/Z/V/C/X verification via the simulator.
// Each test sets up registers, executes one instruction, and checks all
// relevant flags. X-flag preservation is verified for logic/move/TST/CMP.
// =============================================================================

class InstrFlagTest : public ::testing::Test {
 protected:
  static M68kSimulator sim_;

  void SetUp() override {
    sim_.GetMemory().Clear();
    sim_.GetMemory().WriteLong(0x0, 0x3FFE);
    sim_.GetMemory().WriteLong(0x4, 0x1000);
    sim_.Reset();
    sim_.State().sr = kSrSupervisor;
  }

  void RunOpcode(uint16_t opcode) {
    sim_.GetMemory().WriteWord(0x1000, opcode);
    sim_.Step();
  }
};

M68kSimulator InstrFlagTest::sim_{M68kSimulator::Config{}};

// ---------------------------------------------------------------------------
// ADD — N/Z/V/C/X
// ---------------------------------------------------------------------------

// ADD.L D1,D0: 0xD081
TEST_F(InstrFlagTest, Add_SignedOverflow) {
  sim_.State().d[0] = 0x7FFFFFFF;
  sim_.State().d[1] = 0x00000001;
  RunOpcode(0xD081);
  EXPECT_EQ(sim_.State().d[0], 0x80000000u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_TRUE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_FALSE(sim_.State().GetFlag(kSrExtend));
}

TEST_F(InstrFlagTest, Add_UnsignedOverflow) {
  sim_.State().d[0] = 0xFFFFFFFF;
  sim_.State().d[1] = 0x00000001;
  RunOpcode(0xD081);  // ADD.L D1,D0
  EXPECT_EQ(sim_.State().d[0], 0x00000000u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));
}

// ADD.W D1,D0: 0xD041; both negative → zero with carry and overflow
TEST_F(InstrFlagTest, AddWord_BothNeg_Overflow) {
  sim_.State().d[0] = 0x00008000;
  sim_.State().d[1] = 0x00008000;
  RunOpcode(0xD041);
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x0000u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_TRUE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));
}

// ADD.B D1,D0: 0xD001; simple no-flag case
TEST_F(InstrFlagTest, AddByte_NoFlags) {
  sim_.State().d[0] = 0x01;
  sim_.State().d[1] = 0x02;
  RunOpcode(0xD001);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x03u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_FALSE(sim_.State().GetFlag(kSrExtend));
}

TEST_F(InstrFlagTest, AddByte_Carry) {
  sim_.State().d[0] = 0xFF;
  sim_.State().d[1] = 0x01;
  RunOpcode(0xD001);  // ADD.B D1,D0
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x00u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));
}

// ---------------------------------------------------------------------------
// ADDX — Z cleared only if result != 0; X chains
// ADDX.B D1,D0: 0xD101
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, Addx_NoCarry) {
  sim_.State().sr = kSrSupervisor;  // X=0
  sim_.State().d[0] = 0x01;
  sim_.State().d[1] = 0x01;
  RunOpcode(0xD101);  // ADDX.B D1,D0
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x02u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_FALSE(sim_.State().GetFlag(kSrExtend));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));  // result≠0 → Z cleared
}

TEST_F(InstrFlagTest, Addx_ZeroResultPreservesZ) {
  // $FF + $00 + X=1 = $00: result=0 → Z not cleared (stays 1 from preset)
  sim_.State().sr = kSrSupervisor | kSrExtend | kSrZero;
  sim_.State().d[0] = 0xFF;
  sim_.State().d[1] = 0x00;
  RunOpcode(0xD101);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x00u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));  // Z not cleared (result=0)
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));
}

TEST_F(InstrFlagTest, Addx_NonZeroResultClearsZ) {
  // $10 + $10 + X=0 = $20: result≠0 → Z cleared even if pre-Z=1
  sim_.State().sr = kSrSupervisor | kSrZero;
  sim_.State().d[0] = 0x10;
  sim_.State().d[1] = 0x10;
  RunOpcode(0xD101);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x20u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));  // cleared by non-zero result
}

// ---------------------------------------------------------------------------
// SUB — N/Z/V/C/X
// SUB.W D1,D0: 0x9041; SUB.L D1,D0: 0x9081; SUB.B D1,D0: 0x9001
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, SubWord_Borrow) {
  sim_.State().d[0] = 0x0000;
  sim_.State().d[1] = 0x0001;
  RunOpcode(0x9041);  // SUB.W D1,D0 → $0000-$0001=$FFFF
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0xFFFFu);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));
}

TEST_F(InstrFlagTest, SubWord_SignedOverflow) {
  sim_.State().d[0] = 0x8000;
  sim_.State().d[1] = 0x0001;
  RunOpcode(0x9041);  // $8000-$0001=$7FFF: neg-pos=pos → V=1
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x7FFFu);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_TRUE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_FALSE(sim_.State().GetFlag(kSrExtend));
}

TEST_F(InstrFlagTest, SubLong_NoFlags) {
  sim_.State().d[0] = 0x00000003;
  sim_.State().d[1] = 0x00000001;
  RunOpcode(0x9081);  // SUB.L D1,D0
  EXPECT_EQ(sim_.State().d[0], 0x00000002u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_FALSE(sim_.State().GetFlag(kSrExtend));
}

TEST_F(InstrFlagTest, SubByte_Zero) {
  sim_.State().d[0] = 0x05;
  sim_.State().d[1] = 0x05;
  RunOpcode(0x9001);  // SUB.B D1,D0
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x00u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_FALSE(sim_.State().GetFlag(kSrExtend));
}

// ---------------------------------------------------------------------------
// SUBX — Z cleared only if result != 0
// SUBX.B D1,D0: 0x9101; SUBX.W D1,D0: 0x9141
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, Subx_NoFlags) {
  sim_.State().sr = kSrSupervisor;
  sim_.State().d[0] = 0x05;
  sim_.State().d[1] = 0x03;
  RunOpcode(0x9101);  // SUBX.B
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x02u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_FALSE(sim_.State().GetFlag(kSrExtend));
}

TEST_F(InstrFlagTest, Subx_BorrowChain) {
  // $00 - $00 - X=1 = $FF: C=1, Z cleared (result≠0)
  sim_.State().sr = kSrSupervisor | kSrExtend | kSrZero;
  sim_.State().d[0] = 0x00;
  sim_.State().d[1] = 0x00;
  RunOpcode(0x9101);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0xFFu);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));  // cleared by non-zero result
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));
}

TEST_F(InstrFlagTest, Subx_ZeroResultPreservesZ) {
  // $01 - $01 - X=0 = $00: Z not cleared (result=0)
  sim_.State().sr = kSrSupervisor | kSrZero;
  sim_.State().d[0] = 0x0001;
  sim_.State().d[1] = 0x0001;
  RunOpcode(0x9141);  // SUBX.W
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x0000u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));  // Z preserved
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

// ---------------------------------------------------------------------------
// NEG — N/Z/V/C/X; V set only for min-negative operand
// NEG.B D0: 0x4400
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, NegByte_Zero) {
  sim_.State().d[0] = 0x00;
  RunOpcode(0x4400);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x00u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_FALSE(sim_.State().GetFlag(kSrExtend));
}

TEST_F(InstrFlagTest, NegByte_MinNeg_Overflow) {
  sim_.State().d[0] = 0x80;  // -128 → can't negate → V=1
  RunOpcode(0x4400);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x80u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_TRUE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));
}

TEST_F(InstrFlagTest, NegByte_Positive) {
  sim_.State().d[0] = 0x01;
  RunOpcode(0x4400);  // 0 - 1 = $FF
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0xFFu);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));
}

// ---------------------------------------------------------------------------
// NEGX — Z cleared only if result != 0; C = (result != 0)
// NEGX.B D0: 0x4000
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, NegxByte_ZeroOpZeroX_PreservesZ) {
  sim_.State().sr = kSrSupervisor | kSrZero;  // pre-Z=1, X=0
  sim_.State().d[0] = 0x00;
  RunOpcode(0x4000);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x00u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));  // Z preserved (result=0)
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_FALSE(sim_.State().GetFlag(kSrExtend));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
}

TEST_F(InstrFlagTest, NegxByte_ZeroOpOneX_ClearsZ) {
  // 0 - 0 - X=1 = $FF: C=1, Z cleared
  sim_.State().sr = kSrSupervisor | kSrExtend | kSrZero;
  sim_.State().d[0] = 0x00;
  RunOpcode(0x4000);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0xFFu);
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));
}

TEST_F(InstrFlagTest, NegxByte_MinNeg_Overflow) {
  sim_.State().sr = kSrSupervisor;  // X=0
  sim_.State().d[0] = 0x80;
  RunOpcode(0x4000);  // 0 - $80 - 0 = $80: V=dm&&rm=1
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x80u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
}

// ---------------------------------------------------------------------------
// CMP — flags only, X not modified
// CMP.B D1,D0: 0xB001; CMP.W D1,D0: 0xB041
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, CmpWord_Equal) {
  sim_.State().sr = kSrSupervisor | kSrExtend;  // pre-X=1
  sim_.State().d[0] = 0x1234;
  sim_.State().d[1] = 0x1234;
  RunOpcode(0xB041);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unmodified
}

TEST_F(InstrFlagTest, CmpWord_Borrow) {
  sim_.State().d[0] = 0x0001;
  sim_.State().d[1] = 0x0002;  // D0 - D1 = $FFFF, borrow
  RunOpcode(0xB041);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(InstrFlagTest, CmpWord_SignedOverflow) {
  sim_.State().d[0] = 0x8000;
  sim_.State().d[1] = 0x0001;  // D0 - D1 = $7FFF: neg - pos = pos → V=1
  RunOpcode(0xB041);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_TRUE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(InstrFlagTest, CmpLong_XUnmodified) {
  sim_.State().sr = kSrSupervisor | kSrExtend;
  sim_.State().d[0] = 0x00000002;
  sim_.State().d[1] = 0x00000001;
  RunOpcode(0xB081);  // CMP.L D1,D0 → D0-D1=1, no flags
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unmodified
}

// ---------------------------------------------------------------------------
// TST — N/Z set from result; V=0, C=0; X unchanged
// TST.W D0: 0x4A40
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, TstWord_Zero) {
  sim_.State().sr = kSrSupervisor | kSrExtend | kSrOverflow | kSrCarry;
  sim_.State().d[0] = 0x0000;
  RunOpcode(0x4A40);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unchanged
}

TEST_F(InstrFlagTest, TstWord_Negative) {
  sim_.State().d[0] = 0x8000;
  RunOpcode(0x4A40);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(InstrFlagTest, TstLong_Positive) {
  sim_.State().sr = kSrSupervisor | kSrExtend;
  sim_.State().d[0] = 0x00000001;
  RunOpcode(0x4A80);  // TST.L D0
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unchanged
}

// ---------------------------------------------------------------------------
// CLR — always Z=1, N=0, V=0, C=0; X unchanged
// CLR.B D0: 0x4200
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, ClrByte) {
  sim_.State().sr = kSrSupervisor | kSrExtend | kSrOverflow | kSrCarry;
  sim_.State().d[0] = 0xFF;
  RunOpcode(0x4200);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x00u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unchanged
}

TEST_F(InstrFlagTest, ClrWord_ClearsVC) {
  sim_.State().sr = kSrSupervisor | kSrOverflow | kSrCarry;
  sim_.State().d[0] = 0xABCD;
  RunOpcode(0x4240);  // CLR.W D0
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x0000u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

// ---------------------------------------------------------------------------
// AND — V=0, C=0; X unchanged
// AND.W D1,D0: 0xC041; AND.B D1,D0: 0xC001
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, AndWord_Zero) {
  sim_.State().sr = kSrSupervisor | kSrExtend | kSrOverflow | kSrCarry;
  sim_.State().d[0] = 0x00FF;
  sim_.State().d[1] = 0xFF00;
  RunOpcode(0xC041);  // AND.W D1,D0
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x0000u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unchanged
}

TEST_F(InstrFlagTest, AndWord_Negative) {
  sim_.State().d[0] = 0xFFFF;
  sim_.State().d[1] = 0x8080;
  RunOpcode(0xC041);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(InstrFlagTest, AndByte_ClearsVC) {
  sim_.State().sr = kSrSupervisor | kSrOverflow | kSrCarry;
  sim_.State().d[0] = 0x0F;
  sim_.State().d[1] = 0xFF;
  RunOpcode(0xC001);  // AND.B D1,D0
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

// ---------------------------------------------------------------------------
// OR — V=0, C=0; X unchanged
// OR.B D1,D0: 0x8001; OR.W D1,D0: 0x8041
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, OrByte_Negative) {
  sim_.State().d[0] = 0x01;
  sim_.State().d[1] = 0x80;
  RunOpcode(0x8001);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x81u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(InstrFlagTest, OrWord_Zero) {
  sim_.State().d[0] = 0x0000;
  sim_.State().d[1] = 0x0000;
  RunOpcode(0x8041);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
}

// ---------------------------------------------------------------------------
// EOR — V=0, C=0; X unchanged
// EOR.B D1,D0: 0xB300; EOR.W D1,D0: 0xB340
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, EorByte_Zero) {
  sim_.State().sr = kSrSupervisor | kSrExtend | kSrOverflow | kSrCarry;
  sim_.State().d[0] = 0xAA;
  sim_.State().d[1] = 0xAA;
  RunOpcode(0xB300);  // EOR.B D1,D0
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x00u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unchanged
}

TEST_F(InstrFlagTest, EorWord_ClearsVC) {
  sim_.State().sr = kSrSupervisor | kSrOverflow | kSrCarry;
  sim_.State().d[0] = 0x00FF;
  sim_.State().d[1] = 0xFF00;
  RunOpcode(0xB340);  // EOR.W D1,D0
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0xFFFFu);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

// ---------------------------------------------------------------------------
// NOT — V=0, C=0; X unchanged
// NOT.B D0: 0x4600; NOT.W D0: 0x4640
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, NotByte_AllOnes_Zero) {
  sim_.State().sr = kSrSupervisor | kSrExtend;
  sim_.State().d[0] = 0xFF;
  RunOpcode(0x4600);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x00u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unchanged
}

TEST_F(InstrFlagTest, NotByte_Negative) {
  sim_.State().d[0] = 0x7F;
  RunOpcode(0x4600);  // NOT $7F = $80
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x80u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
}

// ---------------------------------------------------------------------------
// MOVE — V=0, C=0; X unchanged
// MOVE.W D1,D0: 0x3001; MOVE.L D1,D0: 0x2001
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, MoveWord_Zero) {
  sim_.State().sr = kSrSupervisor | kSrExtend | kSrOverflow | kSrCarry;
  sim_.State().d[1] = 0x0000;
  RunOpcode(0x3001);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unchanged
}

TEST_F(InstrFlagTest, MoveWord_Negative) {
  sim_.State().d[1] = 0x8000;
  RunOpcode(0x3001);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
}

TEST_F(InstrFlagTest, MoveLong_Positive) {
  sim_.State().d[1] = 0x00000001;
  RunOpcode(0x2001);  // MOVE.L D1,D0
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(InstrFlagTest, MoveWord_ClearsVC) {
  sim_.State().sr = kSrSupervisor | kSrOverflow | kSrCarry;
  sim_.State().d[1] = 0x0042;
  RunOpcode(0x3001);
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

// ---------------------------------------------------------------------------
// ASL — V set if sign bit changes during shift; C = last bit shifted out
// ASL.W #1,D0: 0xE360
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, AslWord_SignChange_SetsV) {
  sim_.State().d[0] = 0x4000;  // bit15=0 → $8000 bit15=1: sign change
  RunOpcode(0xE360);
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x8000u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_TRUE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));  // old bit15 of $4000 = 0
  EXPECT_FALSE(sim_.State().GetFlag(kSrExtend));
}

TEST_F(InstrFlagTest, AslWord_MSBShiftsOut_NoSignChange) {
  sim_.State().d[0] = 0xC000;  // bit15=1 → $8000 bit15=1: no sign change
  RunOpcode(0xE360);
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x8000u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));  // no sign change
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));      // old bit15=1
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));
}

TEST_F(InstrFlagTest, AslWord_SignBitLost_SetsV) {
  sim_.State().d[0] = 0x8000;  // bit15=1 → $0000 bit15=0: sign change
  RunOpcode(0xE360);
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x0000u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_TRUE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));
}

TEST_F(InstrFlagTest, AslCount0_XUnchanged) {
  sim_.State().sr = kSrSupervisor | kSrExtend;
  sim_.State().d[1] = 0;  // count register = 0
  sim_.State().d[0] = 0x1234;
  sim_.GetMemory().WriteWord(0x1000, 0xE360);  // ASL.W #1,D0
  // Use register-count form instead: ASL.W D1,D0
  // ASL.W D1,D0: bits11-9=001(D1),bit8=1,bits7-6=01(word),bit5=0(reg),bits4-3=00(AS),bits2-0=000
  // = 1110 001 1 01 0 00 000 = 0xE348
  sim_.GetMemory().WriteWord(0x1000, 0xE348);
  sim_.Step();
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x1234u);  // unchanged
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unchanged for count=0
}

TEST_F(InstrFlagTest, AslByte_NoSignChange) {
  sim_.State().d[0] = 0x01;
  RunOpcode(0xE320);  // ASL.B #1,D0 → $02
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x02u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

// ---------------------------------------------------------------------------
// ASR — fills with sign bit; V always 0
// ASR.W #1,D0: 0xE260
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, AsrWord_SignFill) {
  sim_.State().d[0] = 0x8000;  // ASR fills with sign: $C000
  RunOpcode(0xE260);
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0xC000u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));  // LSB of $8000=0
}

TEST_F(InstrFlagTest, AsrWord_Zero) {
  sim_.State().d[0] = 0x0001;
  RunOpcode(0xE260);  // $0001 >> 1 = $0000, C=1
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x0000u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
}

TEST_F(InstrFlagTest, AsrByte_PositiveSignFill) {
  sim_.State().d[0] = 0x04;  // fills with 0, C=0
  RunOpcode(0xE220);         // ASR.B #1 → $02
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x02u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

// ---------------------------------------------------------------------------
// LSL / LSR — fills with 0; V always 0
// LSL.W #1,D0: 0xE368; LSR.W #1,D0: 0xE268
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, LslWord_MSBCarry) {
  sim_.State().d[0] = 0x8000;
  RunOpcode(0xE368);  // LSL shifts in 0: result=$0000, C=1
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x0000u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
}

TEST_F(InstrFlagTest, LsrWord_LSBCarry) {
  sim_.State().d[0] = 0x0001;
  RunOpcode(0xE268);  // LSR: $0001>>1=$0000, C=1
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x0000u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));  // fills with 0, not sign
}

TEST_F(InstrFlagTest, LsrWord_FillsZero) {
  sim_.State().d[0] = 0x8001;
  RunOpcode(0xE268);  // $8001>>1=$4000 (fills with 0, not sign bit)
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x4000u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));  // LSR fills with 0
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
}

// ---------------------------------------------------------------------------
// ROL / ROR — X not affected; C = last bit rotated out
// ROL.W #1,D0: 0xE378; ROR.W #1,D0: 0xE278
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, RolWord_MSBWraps) {
  sim_.State().sr = kSrSupervisor | kSrExtend;
  sim_.State().d[0] = 0x8001;
  RunOpcode(0xE378);  // ROL.W: MSB→LSB: $0003, C=1
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x0003u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unchanged
}

TEST_F(InstrFlagTest, RorWord_LSBWraps) {
  sim_.State().sr = kSrSupervisor | kSrExtend;
  sim_.State().d[0] = 0x0001;
  RunOpcode(0xE278);  // ROR.W: LSB→MSB: $8000, C=1
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x8000u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unchanged
}

TEST_F(InstrFlagTest, RolWord_XPreservedWhenXWas0) {
  sim_.State().sr = kSrSupervisor;  // X=0
  sim_.State().d[0] = 0x0000;
  RunOpcode(0xE378);                              // ROL: $0000→$0000, C=0
  EXPECT_FALSE(sim_.State().GetFlag(kSrExtend));  // X still 0
}

// ---------------------------------------------------------------------------
// ROXL / ROXR — X shifts in, C shifts out to new X
// ROXL.W #1,D0: 0xE370; ROXR.W #1,D0: 0xE270
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, RoxlWord_XShiftsIn) {
  sim_.State().sr = kSrSupervisor;  // X=0
  sim_.State().d[0] = 0x8000;
  RunOpcode(0xE370);  // ROXL: X=0 shifts in at bit0; MSB=1→C=1,X=1
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x0000u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X = old MSB = 1
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
}

TEST_F(InstrFlagTest, RoxrWord_XShiftsInAtMSB) {
  sim_.State().sr = kSrSupervisor | kSrExtend;  // X=1
  sim_.State().d[0] = 0x0000;
  RunOpcode(0xE270);  // ROXR: X=1 shifts in at bit15; LSB=0→C=0,X=0
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x8000u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_FALSE(sim_.State().GetFlag(kSrExtend));  // X = old LSB = 0
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(InstrFlagTest, RoxlCount0_CEqualsX) {
  sim_.State().sr = kSrSupervisor | kSrExtend;  // X=1
  sim_.State().d[1] = 0;                        // count=0
  sim_.State().d[0] = 0x1234;
  // ROXL.W D1,D0: 0xE350 (bits11-9=001(D1),bit8=1,bits7-6=01(word),bit5=0,bits4-3=10(ROX))
  // = 1110 001 1 01 0 10 000 = 0xE350
  sim_.GetMemory().WriteWord(0x1000, 0xE350);
  sim_.Step();
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x1234u);  // unchanged
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));     // C = X = 1
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));    // X unchanged
}

// ---------------------------------------------------------------------------
// MULU / MULS — N/Z from long result; V=0, C=0; X unchanged
// MULU.W D1,D0: 0xC0C1; MULS.W D1,D0: 0xC1C1
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, Mulu_Simple) {
  sim_.State().sr = kSrSupervisor | kSrExtend;
  sim_.State().d[0] = 0x0002;
  sim_.State().d[1] = 0x0003;
  RunOpcode(0xC0C1);
  EXPECT_EQ(sim_.State().d[0], 0x00000006u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unchanged
}

TEST_F(InstrFlagTest, Muls_NegativeResult) {
  sim_.State().d[0] = 0xFFFF;  // -1 as int16
  sim_.State().d[1] = 0x0002;
  RunOpcode(0xC1C1);  // MULS: -1 * 2 = -2 = $FFFFFFFE
  EXPECT_EQ(sim_.State().d[0], 0xFFFFFFFEu);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

// ---------------------------------------------------------------------------
// EXT — sign-extend byte→word or word→long; N/Z from result; V=0, C=0
// EXT.W D0: 0x4880; EXT.L D0: 0x48C0
// ---------------------------------------------------------------------------

TEST_F(InstrFlagTest, ExtWord_SignExtendNegByte) {
  sim_.State().sr = kSrSupervisor | kSrExtend;
  sim_.State().d[0] = 0x0000008F;  // byte $8F → word $FF8F
  RunOpcode(0x4880);
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0xFF8Fu);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unchanged
}

TEST_F(InstrFlagTest, ExtLong_SignExtendPosWord) {
  sim_.State().d[0] = 0x00007FFF;  // word $7FFF → long $00007FFF
  RunOpcode(0x48C0);
  EXPECT_EQ(sim_.State().d[0], 0x00007FFFu);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
}

}  // namespace
}  // namespace easym68k::sim
