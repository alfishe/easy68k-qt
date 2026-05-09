// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {
namespace {

class ArithTest : public ::testing::Test {
 protected:
  static M68kSimulator sim_;

  void SetUp() override {
    sim_.GetMemory().Clear();
    sim_.GetMemory().WriteLong(0x0, 0x3FFE);  // SSP
    sim_.GetMemory().WriteLong(0x4, 0x1000);  // PC
    sim_.Reset();
    sim_.State().sr = kSrSupervisor;
  }

  void Opcode(uint16_t op) { sim_.GetMemory().WriteWord(0x1000, op); }
  void OpcodeImm16(uint16_t op, uint16_t imm) {
    sim_.GetMemory().WriteWord(0x1000, op);
    sim_.GetMemory().WriteWord(0x1002, imm);
  }
  void OpcodeImm32(uint16_t op, uint32_t imm) {
    sim_.GetMemory().WriteWord(0x1000, op);
    sim_.GetMemory().WriteLong(0x1002, imm);
  }
};

M68kSimulator ArithTest::sim_{M68kSimulator::Config{}};

// =============================================================================
// ADD
// =============================================================================

TEST_F(ArithTest, Add_Byte_DnDn) {
  sim_.State().d[1] = 3;
  sim_.State().d[0] = 5;
  Opcode(0xD001);  // ADD.B D1,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 8u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
}

TEST_F(ArithTest, Add_Word_Carry) {
  sim_.State().d[2] = 1;
  sim_.State().d[0] = 0xFFFF;
  Opcode(0xD042);  // ADD.W D2,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));
}

TEST_F(ArithTest, Add_Long_Overflow) {
  sim_.State().d[1] = 0x80000000;
  sim_.State().d[0] = 0x80000000;
  Opcode(0xD081);  // ADD.L D1,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrOverflow));
}

TEST_F(ArithTest, Add_ToMemory) {
  sim_.GetMemory().WriteLong(0x2000, 0x10);
  sim_.State().d[0] = 5;
  sim_.GetMemory().WriteWord(0x1000, 0xD1B9);  // ADD.L D0,(abs.L)
  sim_.GetMemory().WriteLong(0x1002, 0x2000);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.GetMemory().ReadLong(0x2000).value, 0x15u);
}

// =============================================================================
// ADDA
// =============================================================================

TEST_F(ArithTest, Adda_W_SignExtend_Negative) {
  // ADDA.W: source sign-extended. D1=0xFFFF → -1 → An += -1
  sim_.State().a[0] = 0x1000;
  sim_.State().d[1] = 0xFFFF;
  Opcode(0xD0C1);  // ADDA.W D1,A0  (1101 000 0 11 000 001)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().GetAReg(0), 0x0FFFu);
}

TEST_F(ArithTest, Adda_L) {
  sim_.State().a[1] = 0x1000;
  sim_.State().d[0] = 0x2000;
  Opcode(0xD2C0);  // ADDA.L D0,A1  (1101 001 1 11 000 000)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().GetAReg(1), 0x3000u);
}

TEST_F(ArithTest, Adda_NoFlagUpdate) {
  sim_.State().sr = kSrSupervisor | kSrZero | kSrNegative;
  sim_.State().a[0] = 0;
  sim_.State().d[0] = 0;
  Opcode(0xD0C0);  // ADDA.W D0,A0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));      // unchanged
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));  // unchanged
}

// =============================================================================
// ADDI
// =============================================================================

TEST_F(ArithTest, Addi_Byte) {
  sim_.State().d[0] = 0x10;
  OpcodeImm16(0x0600, 0x0005);  // ADDI.B #5,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x15u);
}

TEST_F(ArithTest, Addi_Long) {
  sim_.State().d[0] = 1;
  OpcodeImm32(0x0680, 1);  // ADDI.L #1,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 2u);
}

// =============================================================================
// ADDQ
// =============================================================================

TEST_F(ArithTest, Addq_Imm1_Wrap) {
  sim_.State().d[0] = 0xFF;
  Opcode(0x5200);  // ADDQ.B #1,D0  (0101 001 0 00 000 000)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(ArithTest, Addq_Imm8) {
  sim_.State().d[0] = 0;
  Opcode(0x5040);  // ADDQ.W #8,D0  (imm3=000→8, ss=01=word)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 8u);
}

TEST_F(ArithTest, Addq_An_NoFlags) {
  sim_.State().sr = kSrSupervisor | kSrZero;
  sim_.State().a[0] = 0x1000;
  Opcode(0x5248);  // ADDQ.W #1,A0  (0101 001 0 01 001 000)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().GetAReg(0), 0x1001u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));  // unchanged
}

// =============================================================================
// ADDX
// =============================================================================

TEST_F(ArithTest, Addx_NoExtend) {
  sim_.State().sr = kSrSupervisor;
  sim_.State().d[0] = 1;
  sim_.State().d[1] = 2;
  Opcode(0xD101);  // ADDX.B D1,D0  (1101 000 1 00 000 001)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 3u);
}

TEST_F(ArithTest, Addx_WithExtend) {
  sim_.State().sr = kSrSupervisor | kSrExtend;
  sim_.State().d[0] = 1;
  sim_.State().d[1] = 2;
  Opcode(0xD101);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 4u);
}

TEST_F(ArithTest, Addx_ZeroFlagPreserved) {
  // Z is NOT set by ADDX; only cleared when result != 0
  sim_.State().sr = kSrSupervisor | kSrExtend | kSrZero;
  sim_.State().d[0] = 0xFF;
  sim_.State().d[1] = 0;
  Opcode(0xD101);  // ADDX.B D1,D0  → 0xFF+0+1 = 0x100, byte=0, carry=1
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0u);
  // Result is 0 → Z not cleared (stays set if it was)
  // Actually: Z is cleared only if result!=0. result==0 → Z unchanged → still set
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
}

// =============================================================================
// SUB
// =============================================================================

TEST_F(ArithTest, Sub_Byte_Basic) {
  sim_.State().d[0] = 10;
  sim_.State().d[1] = 3;
  Opcode(0x9001);  // SUB.B D1,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 7u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(ArithTest, Sub_Borrow) {
  sim_.State().d[0] = 1;
  sim_.State().d[1] = 2;
  Opcode(0x9001);  // SUB.B D1,D0 → 1-2 = -1 = 0xFF
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0xFFu);
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(ArithTest, Sub_ToMemory) {
  sim_.GetMemory().WriteWord(0x2000, 0x000A);
  sim_.State().d[0] = 3;
  sim_.GetMemory().WriteWord(0x1000, 0x9178);  // SUB.W D0,(abs.W)
  sim_.GetMemory().WriteWord(0x1002, 0x2000);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x2000).value, 7u);
}

// =============================================================================
// SUBA
// =============================================================================

TEST_F(ArithTest, Suba_W) {
  sim_.State().a[0] = 0x2000;
  sim_.State().d[0] = 0x1000;
  Opcode(0x90C0);  // SUBA.W D0,A0  (1001 000 0 11 000 000)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().GetAReg(0), 0x1000u);
}

TEST_F(ArithTest, Suba_L) {
  sim_.State().a[1] = 0x5000;
  sim_.State().d[0] = 0x3000;
  Opcode(0x92C0);  // SUBA.L D0,A1  (1001 001 1 11 000 000)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().GetAReg(1), 0x2000u);
}

// =============================================================================
// SUBI
// =============================================================================

TEST_F(ArithTest, Subi_Word) {
  sim_.State().d[0] = 0x10;
  OpcodeImm16(0x0440, 0x0005);  // SUBI.W #5,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0xBu);
}

// =============================================================================
// SUBQ
// =============================================================================

TEST_F(ArithTest, Subq_Byte) {
  sim_.State().d[0] = 10;
  Opcode(0x5300);  // SUBQ.B #1,D0  (0101 001 1 00 000 000)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 9u);
}

TEST_F(ArithTest, Subq_An_NoFlags) {
  sim_.State().sr = kSrSupervisor | kSrZero;
  sim_.State().a[2] = 0x2008;
  Opcode(0x554A);  // SUBQ.W #2,A2  (0101 010 1 01 001 010)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().GetAReg(2), 0x2006u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));  // unchanged
}

// =============================================================================
// SUBX
// =============================================================================

TEST_F(ArithTest, Subx_NoExtend) {
  sim_.State().sr = kSrSupervisor;
  sim_.State().d[0] = 5;
  sim_.State().d[1] = 3;
  Opcode(0x9101);  // SUBX.B D1,D0  (1001 000 1 00 000 001)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 2u);
}

TEST_F(ArithTest, Subx_WithExtend) {
  sim_.State().sr = kSrSupervisor | kSrExtend;
  sim_.State().d[0] = 5;
  sim_.State().d[1] = 3;
  Opcode(0x9101);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 1u);
}

// =============================================================================
// MULU
// =============================================================================

TEST_F(ArithTest, Mulu_Basic) {
  sim_.State().d[0] = 3;
  sim_.State().d[1] = 4;
  Opcode(0xC0C1);  // MULU.W D1,D0  (1100 000 0 11 000 001)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 12u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
}

TEST_F(ArithTest, Mulu_MaxWord) {
  sim_.State().d[0] = 0xFFFF;
  sim_.State().d[1] = 0xFFFF;
  Opcode(0xC0C1);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0xFFFE0001u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(ArithTest, Mulu_Zero) {
  sim_.State().d[0] = 0x1234;
  sim_.State().d[1] = 0;
  Opcode(0xC0C1);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
}

// =============================================================================
// MULS
// =============================================================================

TEST_F(ArithTest, Muls_Negative) {
  sim_.State().d[0] = 0xFFFF;  // -1 signed
  sim_.State().d[1] = 3;
  Opcode(0xC1C1);  // MULS.W D1,D0  (1100 000 1 11 000 001)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], static_cast<uint32_t>(-3));
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(ArithTest, Muls_NegNeg) {
  sim_.State().d[0] = 0xFFFF;  // -1
  sim_.State().d[1] = 0xFFFF;  // -1
  Opcode(0xC1C1);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 1u);
}

// =============================================================================
// DIVU
// =============================================================================

TEST_F(ArithTest, Divu_Basic) {
  sim_.State().d[0] = 7;
  sim_.State().d[1] = 2;
  Opcode(0x80C1);  // DIVU.W D1,D0  (1000 000 0 11 000 001)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  // quotient=3, remainder=1 → D0 = 0x00010003
  EXPECT_EQ(sim_.State().d[0], 0x00010003u);
}

TEST_F(ArithTest, Divu_Exact) {
  sim_.State().d[0] = 10;
  sim_.State().d[1] = 5;
  Opcode(0x80C1);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0x00000002u);  // remainder=0, quotient=2
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
}

TEST_F(ArithTest, Divu_DivByZero) {
  sim_.State().d[0] = 1;
  sim_.State().d[1] = 0;
  Opcode(0x80C1);
  EXPECT_EQ(sim_.Step(), SimResult::kDivideByZero);
}

TEST_F(ArithTest, Divu_Overflow) {
  sim_.State().d[0] = 0x00020000;  // quotient=131072 > 0xFFFF
  sim_.State().d[1] = 1;
  Opcode(0x80C1);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_EQ(sim_.State().d[0], 0x00020000u);  // Dn unchanged
}

// =============================================================================
// DIVS
// =============================================================================

TEST_F(ArithTest, Divs_Negative) {
  sim_.State().d[0] = static_cast<uint32_t>(-7);
  sim_.State().d[1] = 2;
  Opcode(0x81C1);  // DIVS.W D1,D0  (1000 000 1 11 000 001)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  // -7 / 2 = quotient=-3, remainder=-1
  uint32_t q = static_cast<uint32_t>(static_cast<uint16_t>(static_cast<int16_t>(-3)));
  uint32_t r = static_cast<uint32_t>(static_cast<uint16_t>(static_cast<int16_t>(-1)));
  EXPECT_EQ(sim_.State().d[0], (r << 16) | q);
}

TEST_F(ArithTest, Divs_DivByZero) {
  sim_.State().d[0] = 10;
  sim_.State().d[1] = 0;
  Opcode(0x81C1);
  EXPECT_EQ(sim_.Step(), SimResult::kDivideByZero);
}

TEST_F(ArithTest, Divs_Overflow) {
  sim_.State().d[0] = 0x7FFFFFFF;  // quotient won't fit in signed 16-bit
  sim_.State().d[1] = 1;
  Opcode(0x81C1);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrOverflow));
}

// =============================================================================
// NEG
// =============================================================================

TEST_F(ArithTest, Neg_Byte) {
  sim_.State().d[0] = 5;
  Opcode(0x4400);  // NEG.B D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0xFBu);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(ArithTest, Neg_Zero) {
  sim_.State().d[0] = 0;
  Opcode(0x4440);  // NEG.W D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
}

TEST_F(ArithTest, Neg_Long) {
  sim_.State().d[0] = 0x00000001;
  Opcode(0x4480);  // NEG.L D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0xFFFFFFFFu);
}

// =============================================================================
// NEGX
// =============================================================================

TEST_F(ArithTest, Negx_WithExtend) {
  sim_.State().sr = kSrSupervisor | kSrExtend;
  sim_.State().d[0] = 5;
  Opcode(0x4000);  // NEGX.B D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0xFAu);  // 0-5-1=-6
}

TEST_F(ArithTest, Negx_ZeroPreserved) {
  sim_.State().sr = kSrSupervisor | kSrZero;  // X=0, Z=1
  sim_.State().d[0] = 0;
  Opcode(0x4000);  // NEGX.B D0 → 0-0-0=0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));  // result=0 → Z unchanged (stays 1)
}

// =============================================================================
// CLR
// =============================================================================

TEST_F(ArithTest, Clr_Byte) {
  sim_.State().d[0] = 0xDEADBEEF;
  Opcode(0x4200);  // CLR.B D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(ArithTest, Clr_Long) {
  sim_.State().d[3] = 0xFFFFFFFF;
  Opcode(0x4283);  // CLR.L D3
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[3], 0u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
}

// =============================================================================
// CMP
// =============================================================================

TEST_F(ArithTest, Cmp_Equal) {
  sim_.State().d[0] = 5;
  sim_.State().d[1] = 5;
  Opcode(0xB001);  // CMP.B D1,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 5u);  // Dn unchanged
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(ArithTest, Cmp_Less) {
  sim_.State().d[0] = 3;
  sim_.State().d[1] = 5;
  Opcode(0xB001);  // CMP.B D1,D0 → 3-5
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
}

TEST_F(ArithTest, Cmp_XNotAffected) {
  sim_.State().sr = kSrSupervisor | kSrExtend;
  sim_.State().d[0] = 5;
  sim_.State().d[1] = 3;
  Opcode(0xB001);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unchanged by CMP
}

// =============================================================================
// CMPA
// =============================================================================

TEST_F(ArithTest, Cmpa_W_Equal) {
  sim_.State().a[0] = 0x1000;
  sim_.State().d[1] = 0x1000;
  Opcode(0xB0C1);  // CMPA.W D1,A0  (1011 000 0 11 000 001)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
}

TEST_F(ArithTest, Cmpa_L_Greater) {
  sim_.State().a[1] = 0x3000;
  sim_.State().d[0] = 0x2000;
  Opcode(0xB2C0);  // CMPA.L D0,A1  (1011 001 1 11 000 000)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

// =============================================================================
// CMPI
// =============================================================================

TEST_F(ArithTest, Cmpi_Byte_Equal) {
  sim_.State().d[0] = 0x42;
  OpcodeImm16(0x0C00, 0x0042);  // CMPI.B #0x42,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
}

TEST_F(ArithTest, Cmpi_Word_Less) {
  sim_.State().d[1] = 0x0003;
  OpcodeImm16(0x0C41, 0x0005);  // CMPI.W #5,D1
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
}

// =============================================================================
// CMPM
// =============================================================================

TEST_F(ArithTest, Cmpm_Word_Equal) {
  sim_.GetMemory().WriteWord(0x2000, 0xABCD);
  sim_.GetMemory().WriteWord(0x3000, 0xABCD);
  sim_.State().a[0] = 0x2000;
  sim_.State().a[1] = 0x3000;
  // CMPM.W (A0)+,(A1)+: 1011 001 1 01 001 000 = 0xB348
  Opcode(0xB348);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_EQ(sim_.State().GetAReg(0), 0x2002u);
  EXPECT_EQ(sim_.State().GetAReg(1), 0x3002u);
}

TEST_F(ArithTest, Cmpm_Byte_NotEqual) {
  sim_.GetMemory().WriteByte(0x2000, 0x01);
  sim_.GetMemory().WriteByte(0x2010, 0x02);
  sim_.State().a[0] = 0x2000;
  sim_.State().a[2] = 0x2010;
  // CMPM.B (A0)+,(A2)+: 1011 010 1 00 001 000 = 0xB508
  Opcode(0xB508);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
}

// =============================================================================
// TST
// =============================================================================

TEST_F(ArithTest, Tst_Negative) {
  sim_.State().d[0] = 0x80;
  Opcode(0x4A00);  // TST.B D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
}

TEST_F(ArithTest, Tst_Zero) {
  sim_.State().d[1] = 0;
  Opcode(0x4A41);  // TST.W D1
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(ArithTest, Tst_Long_Positive) {
  sim_.State().d[2] = 0x7FFFFFFF;
  Opcode(0x4A82);  // TST.L D2
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
}

// =============================================================================
// EXT
// =============================================================================

TEST_F(ArithTest, Ext_ByteToWord_Positive) {
  sim_.State().d[0] = 0x7F;
  Opcode(0x4880);  // EXT.W D0  (0100 1000 10 000 000)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x007Fu);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(ArithTest, Ext_ByteToWord_Negative) {
  sim_.State().d[0] = 0xFF;
  Opcode(0x4880);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0xFFFFu);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(ArithTest, Ext_WordToLong_Negative) {
  sim_.State().d[0] = 0x8000;
  Opcode(0x48C0);  // EXT.L D0  (0100 1000 11 000 000)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0xFFFF8000u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(ArithTest, Ext_WordToLong_Positive) {
  sim_.State().d[1] = 0x00007FFF;
  Opcode(0x48C1);  // EXT.L D1
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[1], 0x7FFFu);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
}

// =============================================================================
// ABCD / SBCD
// =============================================================================

TEST_F(ArithTest, Abcd_Dn_NoCarry) {
  sim_.State().sr = kSrSupervisor | kSrZero;  // X=0
  sim_.State().d[0] = 0x09;
  sim_.State().d[1] = 0x01;
  Opcode(0xC101);  // ABCD D1,D0  (1100 000 1 0000 0 001)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x10u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(ArithTest, Abcd_Dn_Carry) {
  sim_.State().sr = kSrSupervisor | kSrZero;
  sim_.State().d[0] = 0x99;
  sim_.State().d[1] = 0x01;
  Opcode(0xC101);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x00u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(ArithTest, Sbcd_Dn) {
  sim_.State().sr = kSrSupervisor | kSrZero;  // X=0
  sim_.State().d[0] = 0x15;
  sim_.State().d[1] = 0x05;
  Opcode(0x8101);  // SBCD D1,D0  (1000 000 1 0000 0 001)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x10u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

// =============================================================================
// NBCD
// =============================================================================

TEST_F(ArithTest, Nbcd_Basic) {
  sim_.State().sr = kSrSupervisor;
  sim_.State().d[0] = 0x05;
  Opcode(0x4800);  // NBCD D0  (0100 1000 00 000 000)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  // 0 - 5 BCD = 95 with borrow
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x95u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(ArithTest, Nbcd_Zero) {
  sim_.State().sr = kSrSupervisor | kSrZero;
  sim_.State().d[0] = 0x00;
  Opcode(0x4800);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

// =============================================================================
// CHK
// =============================================================================

TEST_F(ArithTest, Chk_InBounds) {
  sim_.State().d[0] = 5;
  sim_.State().d[1] = 10;
  Opcode(0x4181);  // CHK D1,D0  (0100 000 1 10 000 001)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
}

TEST_F(ArithTest, Chk_Negative) {
  sim_.State().d[0] = static_cast<uint32_t>(-1);
  sim_.State().d[1] = 10;
  Opcode(0x4181);
  EXPECT_EQ(sim_.Step(), SimResult::kChkException);
}

TEST_F(ArithTest, Chk_TooLarge) {
  sim_.State().d[0] = 11;
  sim_.State().d[1] = 10;
  Opcode(0x4181);
  EXPECT_EQ(sim_.Step(), SimResult::kChkException);
}

TEST_F(ArithTest, Chk_AtUpperBound) {
  sim_.State().d[0] = 10;
  sim_.State().d[1] = 10;
  Opcode(0x4181);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
}

}  // namespace
}  // namespace easym68k::sim
