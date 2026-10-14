// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// 68020 bit field instructions (Task 9.8) — Sim68K CODE7.CPP BIT_FIELD().
// Opcodes are taken from the reference assembler's listing.

#include <gtest/gtest.h>

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {
namespace {

class BitFieldTest : public ::testing::Test {
 protected:
  M68kSimulator sim_{MakeConfig()};

  static M68kSimulator::Config MakeConfig() {
    M68kSimulator::Config c;
    c.bitfield_enabled = true;
    return c;
  }

  void SetUp() override {
    sim_.GetMemory().Clear();
    sim_.GetMemory().WriteLong(0x0, 0x3FFE);
    sim_.GetMemory().WriteLong(0x4, 0x1000);
    sim_.Reset();
    sim_.State().sr = kSrSupervisor;
  }

  void Code(std::initializer_list<uint16_t> words) {
    uint32_t a = 0x1000;
    for (uint16_t w : words) {
      sim_.GetMemory().WriteWord(a, w);
      a += 2;
    }
  }
  void Bytes(uint32_t addr, std::initializer_list<uint8_t> bytes) {
    for (uint8_t b : bytes)
      sim_.GetMemory().WriteByte(addr++, b);
  }
  uint8_t Byte(uint32_t addr) { return sim_.GetMemory().ReadByte(addr).value & 0xFF; }
  bool N() { return sim_.State().GetFlag(kSrNegative); }
  bool Z() { return sim_.State().GetFlag(kSrZero); }
};

TEST_F(BitFieldTest, DisabledIsIllegalInstruction) {
  // Sim68K default: bit field instructions off (SIM68Ku.cpp).
  M68kSimulator::Config c;
  EXPECT_FALSE(c.bitfield_enabled);
  sim_.SetBitFieldEnabled(false);
  Code({0xE8C0, 0x0108});  // BFTST D0{4:8}
  EXPECT_EQ(sim_.Step(), SimResult::kBadInstruction);
  sim_.SetBitFieldEnabled(true);
  EXPECT_TRUE(sim_.IsBitFieldEnabled());
}

TEST_F(BitFieldTest, BftstDataRegisterSetsFlagsOnly) {
  sim_.State().d[0] = 0x0F000000;
  sim_.State().SetFlag(kSrExtend, true);
  sim_.State().SetFlag(kSrOverflow, true);
  sim_.State().SetFlag(kSrCarry, true);
  Code({0xE8C0, 0x0108});  // BFTST D0{4:8} -> field $F0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0x0F000000u);
  EXPECT_TRUE(N());
  EXPECT_FALSE(Z());
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));  // X unchanged
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
  EXPECT_EQ(sim_.State().pc, 0x1004u);
}

TEST_F(BitFieldTest, BfextuAndBfextsDataRegister) {
  sim_.State().d[1] = 0x0AB00000;
  Code({0xE9C1, 0x2108});  // BFEXTU D1{4:8},D2
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[2], 0xABu);
  EXPECT_TRUE(N());

  SetUp();
  sim_.State().d[1] = 0x0AB00000;
  Code({0xEBC1, 0x2108});  // BFEXTS D1{4:8},D2
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[2], 0xFFFFFFABu);
}

TEST_F(BitFieldTest, BfchgDataRegister) {
  sim_.State().d[3] = 0x12345678;
  Code({0xEAC3, 0x0108});  // BFCHG D3{4:8}: field $23 -> $DC
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[3], 0x1DC45678u);
  EXPECT_FALSE(N());
  EXPECT_FALSE(Z());
}

TEST_F(BitFieldTest, DataRegisterFieldDoesNotWrapAround) {
  // CODE7.CPP shifts the register left by the offset and ignores the bits
  // that would wrap around to bit 31 on a 68020 (kept for parity).
  sim_.State().d[3] = 0x0000000F;
  Code({0xEAC3, 0x0708});  // BFCHG D3{28:8}: field read as $F0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(N());
  EXPECT_EQ(sim_.State().d[3], 0x00000000u);
}

TEST_F(BitFieldTest, BfclrMemoryAndCycles) {
  sim_.State().a[0] = 0x2000;
  Bytes(0x2000, {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC});
  Code({0xECD0, 0x0310});  // BFCLR (A0){12:16}: field $4567
  uint64_t before = sim_.State().cycles;
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_FALSE(N());
  EXPECT_FALSE(Z());
  EXPECT_EQ(Byte(0x2000), 0x12);
  EXPECT_EQ(Byte(0x2001), 0x30);
  EXPECT_EQ(Byte(0x2002), 0x00);
  EXPECT_EQ(Byte(0x2003), 0x08);
  EXPECT_EQ(Byte(0x2004), 0x9A);
  EXPECT_EQ(Byte(0x2005), 0xBC);
  // (A0) byte EA time 4 + BFCLR memory 16
  EXPECT_EQ(sim_.State().cycles - before, 20u);
}

TEST_F(BitFieldTest, BfsetMemory) {
  sim_.State().a[0] = 0x2000;
  Code({0xEED0, 0x0104});  // BFSET (A0){4:4}
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(Z());  // field was 0
  EXPECT_EQ(Byte(0x2000), 0x0F);
  EXPECT_EQ(Byte(0x2001), 0x00);
}

TEST_F(BitFieldTest, BfinsMemory) {
  sim_.State().a[0] = 0x2000;
  sim_.State().d[2] = 0xABC;
  Bytes(0x2000, {0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
  Code({0xEFD0, 0x200C});  // BFINS D2,(A0){0:12}
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(N());  // bit 11 of the inserted value
  EXPECT_EQ(Byte(0x2000), 0xAB);
  EXPECT_EQ(Byte(0x2001), 0xCF);
  EXPECT_EQ(Byte(0x2002), 0xFF);
}

TEST_F(BitFieldTest, BfffoMemory) {
  sim_.State().a[0] = 0x2000;
  Bytes(0x2000, {0x00, 0x00, 0x10, 0x00, 0x00});
  Code({0xEDD0, 0x3000});  // BFFFO (A0){0:32},D3
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[3], 19u);
  EXPECT_FALSE(Z());
}

TEST_F(BitFieldTest, RegisterOffsetAndWidth) {
  sim_.State().a[0] = 0x2000;
  sim_.State().d[1] = 8;
  sim_.State().d[2] = 8;
  Bytes(0x2000, {0x11, 0x22, 0x33, 0x44, 0x55});
  Code({0xE9D0, 0x3862});  // BFEXTU (A0){D1:D2},D3
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[3], 0x22u);
}

TEST_F(BitFieldTest, NegativeRegisterOffset) {
  sim_.State().a[0] = 0x2000;
  sim_.State().d[1] = 0xFFFFFFFF;  // -1
  Bytes(0x2001, {0x01, 0xFE, 0x00, 0x00, 0x00});
  Code({0xE8E8, 0x0848, 0x0002});  // BFTST 2(A0){D1:8}: last bit of $2001 + 7 bits
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(N());
  EXPECT_FALSE(Z());
  EXPECT_EQ(sim_.State().pc, 0x1006u);
}

TEST_F(BitFieldTest, FiveByteFieldWritesOldSecondByte) {
  // CODE7.CPP computes the new fifth byte but writes bf1 (the old second
  // byte) to it; kept for parity.
  sim_.State().a[0] = 0x2000;
  Bytes(0x2000, {0x00, 0x5A, 0x00, 0x00, 0x00});
  Code({0xEED0, 0x011E});  // BFSET (A0){4:30}
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(Byte(0x2000), 0x0F);
  EXPECT_EQ(Byte(0x2001), 0xFF);
  EXPECT_EQ(Byte(0x2003), 0xFF);
  EXPECT_EQ(Byte(0x2004), 0x5A);
}

TEST_F(BitFieldTest, NonControlAddressingModeIsIllegal) {
  sim_.State().a[0] = 0x2000;
  Code({0xE8D8, 0x0008});  // BFTST (A0)+{0:8}: not a control mode
  EXPECT_EQ(sim_.Step(), SimResult::kBadInstruction);
}

TEST_F(BitFieldTest, MemoryShiftsStillDecode) {
  // $E0C0-$E7FF memory shifts are unaffected (bit 11 clear).
  sim_.State().a[0] = 0x2000;
  sim_.GetMemory().WriteWord(0x2000, 0x8001);
  Code({0xE3D0});  // LSL.W (A0)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x2000).value, 0x0002u);
}

}  // namespace
}  // namespace easym68k::sim
