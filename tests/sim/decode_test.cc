// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {
namespace {

// ---------------------------------------------------------------------------
// Fixture — minimal simulator reset with SSP=0x3FFE, PC=0x1000
// ---------------------------------------------------------------------------

class DecodeTest : public ::testing::Test {
 protected:
  static M68kSimulator sim_;

  void SetUp() override {
    sim_.GetMemory().Clear();
    sim_.GetMemory().WriteLong(0x0, 0x3FFE);
    sim_.GetMemory().WriteLong(0x4, 0x1000);
    sim_.Reset();
  }

  void SetupOpcode(uint16_t opcode) { sim_.GetMemory().WriteWord(0x1000, opcode); }
};

M68kSimulator DecodeTest::sim_{M68kSimulator::Config{}};

// ---------------------------------------------------------------------------
// 1. NopDispatches
// ---------------------------------------------------------------------------

TEST_F(DecodeTest, NopDispatches) {
  SetupOpcode(0x4E71);  // NOP (group 4)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1002u);
}

// ---------------------------------------------------------------------------
// 2. RtsDispatches
// ---------------------------------------------------------------------------

TEST_F(DecodeTest, RtsDispatches) {
  sim_.GetMemory().WriteLong(0x3FFA, 0x2000);  // return address on stack
  sim_.State().ssp = 0x3FFA;
  SetupOpcode(0x4E75);  // RTS (group 4)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x2000u);
}

// ---------------------------------------------------------------------------
// 3. MoveqDispatches
// ---------------------------------------------------------------------------

TEST_F(DecodeTest, MoveqDispatches) {
  SetupOpcode(0x7042);  // MOVEQ #66,D0 (group 7)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 66u);
  EXPECT_EQ(sim_.State().pc, 0x1002u);
}

// ---------------------------------------------------------------------------
// 4. SimhaltDispatches
// ---------------------------------------------------------------------------

TEST_F(DecodeTest, SimhaltDispatches) {
  sim_.GetMemory().WriteLong(0x1000, 0xFFFFFFFF);  // SIMHALT — simulator-specific halt (2 words)
  EXPECT_EQ(sim_.Step(), SimResult::kHalted);
  EXPECT_TRUE(sim_.State().halted);
}

TEST_F(DecodeTest, SingleFfffIsLineF) {
  sim_.GetMemory().WriteLong(0x1000, 0xFFFF0000);  // Only one FFFF -> Line 1111 exception
  EXPECT_EQ(sim_.Step(), SimResult::kLine1111);
}

// ---------------------------------------------------------------------------
// 5. IllegalDispatches
// ---------------------------------------------------------------------------

TEST_F(DecodeTest, IllegalDispatches) {
  SetupOpcode(0x4AFC);  // ILLEGAL — always triggers illegal instruction exception
  EXPECT_EQ(sim_.Step(), SimResult::kBadInstruction);
}

// ---------------------------------------------------------------------------
// 6. LineADispatches
// ---------------------------------------------------------------------------

TEST_F(DecodeTest, LineADispatches) {
  SetupOpcode(0xA000);  // Line 1010
  EXPECT_EQ(sim_.Step(), SimResult::kLine1010);
}

// ---------------------------------------------------------------------------
// 7. LineFDispatches
// ---------------------------------------------------------------------------

TEST_F(DecodeTest, LineFDispatches) {
  SetupOpcode(0xF000);  // Line 1111 (not SIMHALT)
  EXPECT_EQ(sim_.Step(), SimResult::kLine1111);
}

// ---------------------------------------------------------------------------
// 8. OriToCcr — group 0, not confused with generic data ORI
// ---------------------------------------------------------------------------

TEST_F(DecodeTest, OriToCcr) {
  sim_.GetMemory().WriteWord(0x1000, 0x003C);  // ORI to CCR
  sim_.GetMemory().WriteWord(0x1002, 0x0001);  // immediate = 1 (sets carry)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
}

// ---------------------------------------------------------------------------
// 9. ExgDnDn — group C, Dn/Dn exchange
// ---------------------------------------------------------------------------

TEST_F(DecodeTest, ExgDnDn) {
  sim_.State().d[0] = 0x11111111;
  sim_.State().d[0] = 0x22222222;  // self-exchange leaves D0 unchanged
  SetupOpcode(0xC140);             // EXG D0,D0 (group C)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1002u);
}

// ---------------------------------------------------------------------------
// 10. ExgAnAn — group C, An/An exchange
// ---------------------------------------------------------------------------

TEST_F(DecodeTest, ExgAnAn) {
  sim_.State().a[0] = 0xAABBCCDD;  // self-exchange leaves A0 unchanged
  SetupOpcode(0xC148);             // EXG A0,A0 (group C)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1002u);
}

// ---------------------------------------------------------------------------
// 11. MovemRegister — group 4
// ---------------------------------------------------------------------------

TEST_F(DecodeTest, MovemRegister) {
  // MOVEM.W (A0)+,regs with mask=0x0000 (no registers loaded): dispatch succeeds
  sim_.State().a[0] = 0x2000;
  sim_.GetMemory().WriteWord(0x1000, 0x4C98);  // MOVEM.W (A0)+,regs (group 4, case 0xC)
  sim_.GetMemory().WriteWord(0x1002, 0x0000);  // register mask: no regs
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
}

// ---------------------------------------------------------------------------
// 12. BtstDynamic — group 0, not confused with ORI (same group, different bits)
// ---------------------------------------------------------------------------

TEST_F(DecodeTest, BtstDynamic) {
  SetupOpcode(0x0100);  // BTST D0,D0 (group 0) — dispatches to OpBtst
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
}

}  // namespace
}  // namespace easym68k::sim
