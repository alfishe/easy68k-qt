// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {
namespace {

class ShiftTest : public ::testing::Test {
 protected:
  static M68kSimulator sim_;

  void SetUp() override {
    sim_.GetMemory().Clear();
    sim_.GetMemory().WriteLong(0x0, 0x3FFE);
    sim_.GetMemory().WriteLong(0x4, 0x1000);
    sim_.Reset();
    sim_.State().sr = kSrSupervisor;
  }
};

M68kSimulator ShiftTest::sim_{M68kSimulator::Config{}};

// =============================================================================
// ASL — Arithmetic Shift Left
// =============================================================================

// ASL.B #1,D0: 1110 0001 00 100 000 = 0xE100 + (1<<9) | (1<<5) | D0
// Register form: bits11-9=001(count=1), bit8=1(left), bits7-6=00(byte),
//                bit5=1(imm), bits4-3=00(AS), bits2-0=000(D0)
// 0xE100 | (1<<9) | (1<<5) | 0 = 0xE100 | 0x0200 | 0x0020 = 0xE320... wait

// Let me recalculate:
// bits15-12 = 1110, bits11-9 = 001(count=1), bit8=1(left), bits7-6=00(byte),
// bit5=1(imm), bits4-3=00(AS), bits2-0=000(D0)
// = 1110 001 1 00 1 00 000 = 0xE320

TEST_F(ShiftTest, AslByte_SignChanges_SetsV) {
  sim_.State().d[0] = 0x40;                    // 0100 0000 → ASL by 1 → 0x80, sign changes
  sim_.GetMemory().WriteWord(0x1000, 0xE320);  // ASL.B #1,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x80u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  // 0x40 = 0100 0000: bit7=0, so carry out (old MSB) = 0; X=C=0.
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_FALSE(sim_.State().GetFlag(kSrExtend));
}

TEST_F(ShiftTest, AslByte_NoSignChange_ClearsV) {
  sim_.State().d[0] = 0x20;  // 0010 0000 → ASL by 1 → 0x40, sign unchanged (both 0)
  sim_.GetMemory().WriteWord(0x1000, 0xE320);  // ASL.B #1,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x40u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(ShiftTest, AslByte_CarryFromMSB) {
  sim_.State().d[0] = 0x81;                    // 1000 0001: MSB=1 → C=1, result=0x02
  sim_.GetMemory().WriteWord(0x1000, 0xE320);  // ASL.B #1,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x02u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));
}

// ASL.W #2,D0: bits11-9=010(count=2), bit8=1, bits7-6=01(word),
//              bit5=1(imm), bits4-3=00(AS), bits2-0=000
// = 1110 010 1 01 1 00 000 = 0xE540 | 0x20 = 0xE560
TEST_F(ShiftTest, AslWord_Count2) {
  sim_.State().d[0] = 0x0001;
  sim_.GetMemory().WriteWord(0x1000, 0xE560);  // ASL.W #2,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x0004u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
}

// ASL count=0 via register: use D1=0 as count
// ASL.B D1,D0: bits11-9=001(D1), bit8=1, bits7-6=00(byte),
//              bit5=0(reg), bits4-3=00(AS), bits2-0=000(D0)
// = 1110 001 1 00 0 00 000 = 0xE300
TEST_F(ShiftTest, AslCount0_NoCFlagChange) {
  sim_.State().sr = kSrSupervisor | kSrExtend;  // X=1 before
  sim_.State().d[0] = 0x42;
  sim_.State().d[1] = 0;                       // count = 0
  sim_.GetMemory().WriteWord(0x1000, 0xE300);  // ASL.B D1,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x42u);  // unchanged
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unchanged when count=0
}

// =============================================================================
// ASR — Arithmetic Shift Right
// =============================================================================

// ASR.B #1,D0: bits11-9=001, bit8=0(right), bits7-6=00(byte),
//              bit5=1(imm), bits4-3=00(AS), bits2-0=000
// = 1110 001 0 00 1 00 000 = 0xE220
TEST_F(ShiftTest, AsrByte_FillsWithSign) {
  sim_.State().d[0] = 0x80;  // 1000 0000 → ASR by 1 → 0xC0 (fills with sign bit 1)
  sim_.GetMemory().WriteWord(0x1000, 0xE220);  // ASR.B #1,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0xC0u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));  // LSB of 0x80 = 0
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(ShiftTest, AsrByte_CarryFromLSB) {
  sim_.State().d[0] = 0x01;  // LSB=1 → C=1, result=0x00
  sim_.GetMemory().WriteWord(0x1000, 0xE220);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x00u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
}

// =============================================================================
// LSL / LSR — Logical Shift
// =============================================================================

// LSL.W #1,D0: bits11-9=001, bit8=1(left), bits7-6=01(word),
//              bit5=1(imm), bits4-3=01(LS), bits2-0=000
// = 1110 001 1 01 1 01 000 = 0xE368
TEST_F(ShiftTest, LslWord) {
  sim_.State().d[0] = 0x8001;
  sim_.GetMemory().WriteWord(0x1000, 0xE368);  // LSL.W #1,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x0002u);  // MSB shifted out, fills with 0
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
}

// LSR.W #1,D0: bit8=0(right), type=LS(01)
// = 1110 001 0 01 1 01 000 = 0xE268
TEST_F(ShiftTest, LsrWord_FillsWithZero) {
  sim_.State().d[0] = 0x8001;
  sim_.GetMemory().WriteWord(0x1000, 0xE268);  // LSR.W #1,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x4000u);  // fills with 0, not sign bit
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));     // LSB=1
}

// =============================================================================
// ROL / ROR — Rotate (X not affected)
// =============================================================================

// ROL.L #1,D0: bits11-9=001, bit8=1, bits7-6=10(long),
//              bit5=1(imm), bits4-3=11(RO), bits2-0=000
// = 1110 001 1 10 1 11 000 = 0xE3B8
TEST_F(ShiftTest, RolLong_WrapsMSB) {
  sim_.State().d[0] = 0x80000001;
  sim_.GetMemory().WriteWord(0x1000, 0xE3B8);  // ROL.L #1,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0x00000003u);    // MSB rotated to LSB
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));  // MSB=1 was shifted out
}

// ROR.B #1,D0: bit8=0(right), type=RO(11)
// = 1110 001 0 00 1 11 000 = 0xE238
TEST_F(ShiftTest, RorByte_WrapsLSB) {
  sim_.State().d[0] = 0x01;                    // LSB=1 → wraps to MSB
  sim_.GetMemory().WriteWord(0x1000, 0xE238);  // ROR.B #1,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x80u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(ShiftTest, RolRor_XNotAffected) {
  sim_.State().sr = kSrSupervisor | kSrExtend;
  sim_.State().d[0] = 0x00;
  sim_.GetMemory().WriteWord(0x1000, 0xE3B8);  // ROL.L #1,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unchanged
}

// =============================================================================
// ROXL / ROXR — Rotate through Extend
// =============================================================================

// ROXL.B #1,D0: bits11-9=001, bit8=1, bits7-6=00(byte),
//               bit5=1(imm), bits4-3=10(ROX), bits2-0=000
// = 1110 001 1 00 1 10 000 = 0xE330
TEST_F(ShiftTest, RoxlByte_ShiftsInExtend) {
  sim_.State().sr = kSrSupervisor | kSrExtend;  // X=1
  sim_.State().d[0] = 0x00;
  sim_.GetMemory().WriteWord(0x1000, 0xE330);  // ROXL.B #1,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x01u);    // X=1 shifted in at LSB
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));  // old MSB=0 → C=0, X=0
}

// ROXR.B #1,D0: bit8=0(right), type=ROX(10)
// = 1110 001 0 00 1 10 000 = 0xE230
TEST_F(ShiftTest, RoxrByte_ShiftsInExtend) {
  sim_.State().sr = kSrSupervisor | kSrExtend;  // X=1
  sim_.State().d[0] = 0x00;
  sim_.GetMemory().WriteWord(0x1000, 0xE230);  // ROXR.B #1,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x80u);    // X=1 shifted in at MSB
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));  // old LSB=0
}

TEST_F(ShiftTest, RoxlCount0_CEqualsX) {
  sim_.State().sr = kSrSupervisor | kSrExtend;  // X=1
  sim_.State().d[1] = 0;                        // count = 0
  sim_.State().d[0] = 0x42;
  // ROXL.B D1,D0: bits11-9=001(D1), bit8=1, bits7-6=00(byte),
  //               bit5=0(reg), bits4-3=10(ROX), bits2-0=000
  // = 1110 001 1 00 0 10 000 = 0xE310
  sim_.GetMemory().WriteWord(0x1000, 0xE310);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x42u);    // unchanged
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));   // C = X = 1
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unchanged when count=0
}

// =============================================================================
// Memory form — ASL (word, count=1)
// ASL (A0): 0xE1D0 (type=AS, left, mode=010, reg=000)
// =============================================================================

TEST_F(ShiftTest, AslMemory_WordByOne) {
  sim_.State().a[0] = 0x2000;
  sim_.GetMemory().WriteWord(0x2000, 0x0001);
  sim_.GetMemory().WriteWord(0x1000, 0xE1D0);  // ASL.W (A0)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x2000).value, 0x0002u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

// LSR (A0): 0xE2D0
TEST_F(ShiftTest, LsrMemory_WordByOne) {
  sim_.State().a[0] = 0x2000;
  sim_.GetMemory().WriteWord(0x2000, 0x8002);
  sim_.GetMemory().WriteWord(0x1000, 0xE2D0);  // LSR.W (A0)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x2000).value, 0x4001u);  // fills with 0
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));                 // LSB of 0x8002 = 0
}

// =============================================================================
// BTST — Bit Test
// =============================================================================

TEST_F(ShiftTest, BtstStatic_ZeroSet) {
  sim_.State().d[0] = 0xFFFFFFFE;              // bit 0 = 0
  sim_.GetMemory().WriteWord(0x1000, 0x0800);  // BTST #n,D0 (static)
  sim_.GetMemory().WriteWord(0x1002, 0x0000);  // bit number = 0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
}

TEST_F(ShiftTest, BtstStatic_ZeroClear) {
  sim_.State().d[0] = 0x00000001;  // bit 0 = 1
  sim_.GetMemory().WriteWord(0x1000, 0x0800);
  sim_.GetMemory().WriteWord(0x1002, 0x0000);  // bit 0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
}

TEST_F(ShiftTest, BtstDynamic_HighBit) {
  sim_.State().d[0] = 0x80000000;  // bit 31 = 1
  sim_.State().d[1] = 31;          // bit number in D1
  // BTST D1,D0: 0000 001 100 000 000 = 0x0300
  sim_.GetMemory().WriteWord(0x1000, 0x0300);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
}

TEST_F(ShiftTest, BtstMemory_ModEight) {
  sim_.State().a[0] = 0x2000;
  sim_.GetMemory().WriteByte(0x2000, 0x0F);    // bits 0-3 set
  sim_.GetMemory().WriteWord(0x1000, 0x0810);  // BTST #n,(A0) static
  sim_.GetMemory().WriteWord(0x1002, 0x0003);  // bit 3
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
}

// =============================================================================
// BCHG / BCLR / BSET
// =============================================================================

TEST_F(ShiftTest, BchgToggles) {
  sim_.State().d[0] = 0x00000001;              // bit 0 = 1
  sim_.GetMemory().WriteWord(0x1000, 0x0840);  // BCHG #n,D0 (static, bits7-6=01)
  sim_.GetMemory().WriteWord(0x1002, 0x0000);  // bit 0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0x00000000u);    // bit 0 toggled off
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));  // was 1, so Z=0
}

TEST_F(ShiftTest, BclrClearsBit) {
  sim_.State().d[0] = 0xFFFFFFFF;
  sim_.GetMemory().WriteWord(0x1000, 0x0880);  // BCLR #n,D0 (static, bits7-6=10)
  sim_.GetMemory().WriteWord(0x1002, 0x0010);  // bit 16
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0xFFFEFFFFu);
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));  // was 1
}

TEST_F(ShiftTest, BsetSetsBit) {
  sim_.State().d[0] = 0x00000000;
  sim_.GetMemory().WriteWord(0x1000, 0x08C0);  // BSET #n,D0 (static, bits7-6=11)
  sim_.GetMemory().WriteWord(0x1002, 0x0004);  // bit 4
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0x00000010u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));  // was 0
}

TEST_F(ShiftTest, BsetMemory) {
  sim_.State().a[0] = 0x2000;
  sim_.GetMemory().WriteByte(0x2000, 0x00);
  // BSET #n,(A0): 0x08D0 + bits7-6=11 → 0x08C0 | 0x10 = 0x08D0?
  // Static BSET: 0x08C0 base, (A0) mode=010,reg=000 → EA bits = 010 000 = 0x10
  // 0x08C0 | 0x10 = 0x08D0
  sim_.GetMemory().WriteWord(0x1000, 0x08D0);  // BSET #n,(A0)
  sim_.GetMemory().WriteWord(0x1002, 0x0002);  // bit 2
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.GetMemory().ReadByte(0x2000).value, 0x04u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));  // was 0
}

}  // namespace
}  // namespace easym68k::sim
