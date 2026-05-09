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
  SetupOpcode(0x4E75);  // RTS (group 4) — not yet implemented in Phase 5
  EXPECT_EQ(sim_.Step(), SimResult::kBadInstruction);
}

// ---------------------------------------------------------------------------
// 3. MoveqDispatches
// ---------------------------------------------------------------------------

TEST_F(DecodeTest, MoveqDispatches) {
  SetupOpcode(0x7042);  // MOVEQ #66,D0 (group 7) — not yet implemented in Phase 5
  EXPECT_EQ(sim_.Step(), SimResult::kBadInstruction);
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
  SetupOpcode(0x003C);  // ORI to CCR (group 0) — not yet implemented in Phase 5
  EXPECT_EQ(sim_.Step(), SimResult::kBadInstruction);
}

// ---------------------------------------------------------------------------
// 9. ExgDnDn — group C, Dn/Dn exchange
// ---------------------------------------------------------------------------

TEST_F(DecodeTest, ExgDnDn) {
  SetupOpcode(0xC140);  // EXG D0,D0 (group C) — not yet implemented in Phase 5
  EXPECT_EQ(sim_.Step(), SimResult::kBadInstruction);
}

// ---------------------------------------------------------------------------
// 10. ExgAnAn — group C, An/An exchange
// ---------------------------------------------------------------------------

TEST_F(DecodeTest, ExgAnAn) {
  SetupOpcode(0xC148);  // EXG A0,A0 (group C) — not yet implemented in Phase 5
  EXPECT_EQ(sim_.Step(), SimResult::kBadInstruction);
}

// ---------------------------------------------------------------------------
// 11. MovemRegister — group 4
// ---------------------------------------------------------------------------

TEST_F(DecodeTest, MovemRegister) {
  SetupOpcode(0x4880);  // MOVEM (group 4) — not yet implemented in Phase 5
  EXPECT_EQ(sim_.Step(), SimResult::kBadInstruction);
}

// ---------------------------------------------------------------------------
// 12. BtstDynamic — group 0, not confused with ORI (same group, different bits)
// ---------------------------------------------------------------------------

TEST_F(DecodeTest, BtstDynamic) {
  SetupOpcode(0x0100);  // BTST Dn (group 0) — not yet implemented in Phase 5
  EXPECT_EQ(sim_.Step(), SimResult::kBadInstruction);
}

}  // namespace
}  // namespace easym68k::sim
