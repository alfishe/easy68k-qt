// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {
namespace {

class LogicTest : public ::testing::Test {
 protected:
  static M68kSimulator sim_;

  void SetUp() override {
    sim_.GetMemory().Clear();
    sim_.GetMemory().WriteLong(0x0, 0x3FFE);
    sim_.GetMemory().WriteLong(0x4, 0x1000);
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

M68kSimulator LogicTest::sim_{M68kSimulator::Config{}};

// =============================================================================
// OR
// =============================================================================

TEST_F(LogicTest, Or_Byte_DnDn) {
  sim_.State().d[0] = 0x0F;
  sim_.State().d[1] = 0xF0;
  Opcode(0x8001);  // OR.B D1,D0  (1000 000 0 00 000 001)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0xFFu);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(LogicTest, Or_Byte_Result_Zero) {
  sim_.State().d[0] = 0;
  sim_.State().d[1] = 0;
  Opcode(0x8001);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(LogicTest, Or_Word_ToMemory) {
  sim_.GetMemory().WriteWord(0x2000, 0x00F0);
  sim_.State().d[0] = 0x000F;
  sim_.GetMemory().WriteWord(0x1000, 0x8178);  // OR.W D0,(abs.W)
  sim_.GetMemory().WriteWord(0x1002, 0x2000);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x2000).value, 0x00FFu);
}

TEST_F(LogicTest, Or_Long_DnDn) {
  sim_.State().d[0] = 0xAAAAAAAA;
  sim_.State().d[1] = 0x55555555;
  Opcode(0x8081);  // OR.L D1,D0  (1000 000 0 10 000 001)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0xFFFFFFFFu);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(LogicTest, Or_XNotAffected) {
  sim_.State().sr = kSrSupervisor | kSrExtend;
  sim_.State().d[0] = 1;
  sim_.State().d[1] = 2;
  Opcode(0x8001);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unchanged
}

// =============================================================================
// ORI
// =============================================================================

TEST_F(LogicTest, Ori_Byte) {
  sim_.State().d[0] = 0x0F;
  OpcodeImm16(0x0000, 0x00F0);  // ORI.B #0xF0,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0xFFu);
}

TEST_F(LogicTest, Ori_Word) {
  sim_.State().d[1] = 0x0000;
  OpcodeImm16(0x0041, 0x8000);  // ORI.W #0x8000,D1
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[1] & 0xFFFF, 0x8000u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(LogicTest, Ori_Long) {
  sim_.State().d[2] = 0x00000000;
  OpcodeImm32(0x0082, 0xDEADBEEF);  // ORI.L #0xDEADBEEF,D2
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[2], 0xDEADBEEFu);
}

// =============================================================================
// ORI to CCR / SR
// =============================================================================

TEST_F(LogicTest, OriToCcr) {
  sim_.State().sr = kSrSupervisor;  // no CCR flags set
  OpcodeImm16(0x003C, 0x0005);  // ORI #5,CCR  (5 = kSrCarry|kSrZero)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_EQ(sim_.State().sr & kSrSupervisor, kSrSupervisor);  // S bit preserved
}

TEST_F(LogicTest, OriToCcr_PreservesHighByte) {
  sim_.State().sr = kSrSupervisor | (3 << 8);  // interrupt mask = 3
  OpcodeImm16(0x003C, 0x0001);                 // ORI #1,CCR
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().sr & kSrIntMask, static_cast<uint16_t>(3 << 8));  // mask unchanged
}

TEST_F(LogicTest, OriToSr_Supervisor) {
  sim_.State().sr = kSrSupervisor;
  OpcodeImm16(0x007C, 0x0001);  // ORI #1,SR  (sets carry)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(LogicTest, OriToSr_UserMode_Privilege) {
  sim_.State().sr = 0;  // user mode
  OpcodeImm16(0x007C, 0x0001);
  EXPECT_EQ(sim_.Step(), SimResult::kPrivilegeViolation);
}

// =============================================================================
// AND
// =============================================================================

TEST_F(LogicTest, And_Byte_DnDn) {
  sim_.State().d[0] = 0xFF;
  sim_.State().d[1] = 0x0F;
  Opcode(0xC001);  // AND.B D1,D0  (1100 000 0 00 000 001)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x0Fu);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
}

TEST_F(LogicTest, And_Result_Zero) {
  sim_.State().d[0] = 0xAA;
  sim_.State().d[1] = 0x55;
  Opcode(0xC001);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
}

TEST_F(LogicTest, And_Word_ToMemory) {
  sim_.GetMemory().WriteWord(0x2000, 0xFFFF);
  sim_.State().d[0] = 0x0F0F;
  sim_.GetMemory().WriteWord(0x1000, 0xC178);  // AND.W D0,(abs.W)
  sim_.GetMemory().WriteWord(0x1002, 0x2000);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x2000).value, 0x0F0Fu);
}

TEST_F(LogicTest, And_Long) {
  sim_.State().d[0] = 0xFFFF0000;
  sim_.State().d[1] = 0x0000FFFF;
  Opcode(0xC081);  // AND.L D1,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
}

TEST_F(LogicTest, And_XNotAffected) {
  sim_.State().sr = kSrSupervisor | kSrExtend;
  sim_.State().d[0] = 0xFF;
  sim_.State().d[1] = 0xFF;
  Opcode(0xC001);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));
}

// =============================================================================
// ANDI
// =============================================================================

TEST_F(LogicTest, Andi_Byte_ClearBits) {
  sim_.State().d[0] = 0xFF;
  OpcodeImm16(0x0200, 0x000F);  // ANDI.B #0x0F,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x0Fu);
}

TEST_F(LogicTest, Andi_Long) {
  sim_.State().d[0] = 0xFFFFFFFF;
  OpcodeImm32(0x0280, 0x0F0F0F0F);  // ANDI.L #0x0F0F0F0F,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0x0F0F0F0Fu);
}

// =============================================================================
// ANDI to CCR / SR
// =============================================================================

TEST_F(LogicTest, AndiToCcr_ClearFlags) {
  sim_.State().sr = kSrSupervisor | kSrCarry | kSrZero | kSrNegative;
  OpcodeImm16(0x023C, 0x00FE);  // ANDI #0xFE,CCR  (0xFE = ~kSrCarry & 0xFF)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_EQ(sim_.State().sr & kSrSupervisor, kSrSupervisor);  // S bit preserved
}

TEST_F(LogicTest, AndiToSr_Supervisor) {
  sim_.State().sr = kSrSupervisor | kSrCarry | kSrZero;
  OpcodeImm16(0x027C, 0xFFFE);  // ANDI #0xFFFE,SR  (~kSrCarry)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
}

TEST_F(LogicTest, AndiToSr_UserMode_Privilege) {
  sim_.State().sr = 0;
  OpcodeImm16(0x027C, 0xFFFF);
  EXPECT_EQ(sim_.Step(), SimResult::kPrivilegeViolation);
}

// =============================================================================
// EOR
// =============================================================================

TEST_F(LogicTest, Eor_Byte_DnDn) {
  sim_.State().d[0] = 0xFF;
  sim_.State().d[1] = 0x0F;
  // EOR.B D0,D1: 1011 000 1 00 000 001 = 0xB101
  Opcode(0xB101);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[1] & 0xFF, 0xF0u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(LogicTest, Eor_Long_Toggle) {
  sim_.State().d[0] = 0xFFFFFFFF;
  sim_.State().d[1] = 0xAAAAAAAA;
  // EOR.L D0,D1: 1011 000 1 10 000 001 = 0xB181
  Opcode(0xB181);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[1], 0x55555555u);
}

TEST_F(LogicTest, Eor_Self_Zero) {
  sim_.State().d[2] = 0xDEADBEEF;
  // EOR.L D2,D2: 1011 010 1 10 000 010 = 0xB582... let me calculate
  // EOR.L D2,D2: 1011 DDD 1 ss MMMRRR where DDD=010(D2), ss=10(long), mode=000, reg=010
  // = 1011 010 1 10 000 010 = 0xB582
  Opcode(0xB582);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[2], 0u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
}

TEST_F(LogicTest, Eor_XNotAffected) {
  sim_.State().sr = kSrSupervisor | kSrExtend;
  sim_.State().d[0] = 0xAA;
  sim_.State().d[1] = 0x55;
  Opcode(0xB101);  // EOR.B D0,D1
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));
}

// =============================================================================
// EORI
// =============================================================================

TEST_F(LogicTest, Eori_Byte) {
  sim_.State().d[0] = 0xFF;
  OpcodeImm16(0x0A00, 0x000F);  // EORI.B #0x0F,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0xF0u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(LogicTest, Eori_Word_Zero) {
  sim_.State().d[1] = 0xABCD;
  OpcodeImm16(0x0A41, 0xABCD);  // EORI.W #0xABCD,D1
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[1] & 0xFFFF, 0u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
}

// =============================================================================
// EORI to CCR / SR
// =============================================================================

TEST_F(LogicTest, EoriToCcr_Toggle) {
  sim_.State().sr = kSrSupervisor | kSrCarry;
  OpcodeImm16(0x0A3C, 0x0003);  // EORI #3,CCR  (toggles C and V)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));    // was 1, XOR 1 → 0
  EXPECT_TRUE(sim_.State().GetFlag(kSrOverflow));  // was 0, XOR 1 → 1
}

TEST_F(LogicTest, EoriToSr_Supervisor) {
  sim_.State().sr = kSrSupervisor | kSrZero;
  OpcodeImm16(0x0A7C, 0x0004);  // EORI #4,SR  (toggles Z)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
}

TEST_F(LogicTest, EoriToSr_UserMode_Privilege) {
  sim_.State().sr = 0;
  OpcodeImm16(0x0A7C, 0x0001);
  EXPECT_EQ(sim_.Step(), SimResult::kPrivilegeViolation);
}

// =============================================================================
// NOT
// =============================================================================

TEST_F(LogicTest, Not_Byte) {
  sim_.State().d[0] = 0x00;
  Opcode(0x4600);  // NOT.B D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0xFFu);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
}

TEST_F(LogicTest, Not_Word_AllOnes) {
  sim_.State().d[1] = 0xFFFF;
  Opcode(0x4641);  // NOT.W D1
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[1] & 0xFFFF, 0u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(LogicTest, Not_Long) {
  sim_.State().d[2] = 0xAAAAAAAA;
  Opcode(0x4682);  // NOT.L D2
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[2], 0x55555555u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(LogicTest, Not_ClearsVC) {
  sim_.State().sr = kSrSupervisor | kSrOverflow | kSrCarry;
  sim_.State().d[0] = 0x01;
  Opcode(0x4600);  // NOT.B D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(LogicTest, Not_XNotAffected) {
  sim_.State().sr = kSrSupervisor | kSrExtend;
  sim_.State().d[0] = 0x01;
  Opcode(0x4600);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));
}

// =============================================================================
// TAS
// =============================================================================

TEST_F(LogicTest, Tas_ClearByte_SetsBit7) {
  sim_.State().d[0] = 0x00;
  Opcode(0x4AC0);  // TAS D0  (0100 1010 11 000 000)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x80u);  // bit 7 set
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));  // original value was 0
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(LogicTest, Tas_NonZero_SetsBit7) {
  sim_.State().d[0] = 0x3F;
  Opcode(0x4AC0);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0xBFu);  // 0x3F | 0x80
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));  // original 0x3F was positive
}

TEST_F(LogicTest, Tas_AlreadySet_StaysSet) {
  sim_.State().d[0] = 0x80;
  Opcode(0x4AC0);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x80u);      // bit 7 already set
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));  // original 0x80 is negative
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
}

TEST_F(LogicTest, Tas_Memory) {
  sim_.GetMemory().WriteByte(0x2000, 0x00);
  sim_.State().a[0] = 0x2000;
  Opcode(0x4AD0);  // TAS (A0)  (0100 1010 11 010 000)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.GetMemory().ReadByte(0x2000).value, 0x80u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
}

}  // namespace
}  // namespace easym68k::sim
