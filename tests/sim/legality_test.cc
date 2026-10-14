// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Opcode legality table (src/sim/legality.cc, Task 10.3). The full table is
// verified against the original Sim68K core by the opcode sweep
// (tools/reference/compare_sim_corpus.sh --sweep); these tests pin the rules
// and the places where the port deliberately differs from the original
// (data/golden/sim/deviations.tsv).

#include <gtest/gtest.h>

#include "easym68k/sim/decode.h"
#include "easym68k/sim/simulator.h"

namespace easym68k::sim {
namespace {

TEST(LegalityTest, AddressRegisterNeverWithByteSize) {
  EXPECT_FALSE(IsLegalOpcode(0x1008, false));  // MOVE.B A0,D0
  EXPECT_FALSE(IsLegalOpcode(0x1048, false));  // MOVEA.B
  EXPECT_TRUE(IsLegalOpcode(0x3048, false));   // MOVEA.W A0,A0
  EXPECT_FALSE(IsLegalOpcode(0xD008, false));  // ADD.B A0,D0
  EXPECT_TRUE(IsLegalOpcode(0xD048, false));   // ADD.W A0,D0
  // ADDQ.B/SUBQ.B #n,An: illegal on the 68000 (the original runs it as .L).
  EXPECT_FALSE(IsLegalOpcode(0x5208, false));  // ADDQ.B #1,A0
  EXPECT_TRUE(IsLegalOpcode(0x5248, false));   // ADDQ.W #1,A0
}

TEST(LegalityTest, DestinationModes) {
  EXPECT_FALSE(IsLegalOpcode(0x39C0, false));  // MOVE.W D0,#imm
  EXPECT_FALSE(IsLegalOpcode(0x35C0, false));  // MOVE.W D0,d16(PC)
  EXPECT_TRUE(IsLegalOpcode(0x33C0, false));   // MOVE.W D0,abs.L
  EXPECT_FALSE(IsLegalOpcode(0x4AFA, false));  // TST d16(PC) (68020)
  EXPECT_FALSE(IsLegalOpcode(0x0C7A, false));  // CMPI d16(PC) (68020)
  EXPECT_FALSE(IsLegalOpcode(0xD17A, false));  // ADD D0,d16(PC)
}

TEST(LegalityTest, BitOperations) {
  EXPECT_TRUE(IsLegalOpcode(0x013A, false));   // BTST D0,d16(PC)
  EXPECT_TRUE(IsLegalOpcode(0x013C, false));   // BTST D0,#imm
  EXPECT_TRUE(IsLegalOpcode(0x083C, false));   // BTST #n,#imm (as the original)
  EXPECT_FALSE(IsLegalOpcode(0x017A, false));  // BCHG D0,d16(PC)
  EXPECT_FALSE(IsLegalOpcode(0x01FC, false));  // BSET D0,#imm
  EXPECT_FALSE(IsLegalOpcode(0x087A, false));  // BCHG #n,d16(PC)
  EXPECT_TRUE(IsLegalOpcode(0x0108, false));   // MOVEP.W d16(A0),D0
}

TEST(LegalityTest, GroupFourAndMisc) {
  EXPECT_TRUE(IsLegalOpcode(0x49F8, false));   // LEA abs.W,A4
  EXPECT_TRUE(IsLegalOpcode(0x4F85, false));   // CHK D5,D7
  EXPECT_FALSE(IsLegalOpcode(0x4F05, false));  // CHK.L (68020)
  EXPECT_FALSE(IsLegalOpcode(0x4ED8, false));  // JMP (A0)+
  EXPECT_FALSE(IsLegalOpcode(0x48D8, false));  // MOVEM to (A0)+
  EXPECT_FALSE(IsLegalOpcode(0x4CE0, false));  // MOVEM from -(A0)
  EXPECT_TRUE(IsLegalOpcode(0x4E71, false));   // NOP
  EXPECT_FALSE(IsLegalOpcode(0x4E74, false));  // RTD (68010)
  EXPECT_FALSE(IsLegalOpcode(0x4E7A, false));  // MOVEC (68010)
  EXPECT_FALSE(IsLegalOpcode(0x7100, false));  // MOVEQ with bit 8 set
  EXPECT_TRUE(IsLegalOpcode(0xA000, false));   // Line A: own exception
  EXPECT_TRUE(IsLegalOpcode(0xF000, false));   // Line F: own exception
}

TEST(LegalityTest, BitFieldGroupFollowsTheOption) {
  EXPECT_FALSE(IsLegalOpcode(0xE8C0, false));  // BFTST D0 (bit fields off)
  EXPECT_TRUE(IsLegalOpcode(0xE8C0, true));
  EXPECT_TRUE(IsLegalOpcode(0xE8FA, true));   // BFTST d16(PC)
  EXPECT_FALSE(IsLegalOpcode(0xEAFA, true));  // BFCHG d16(PC): not alterable
  EXPECT_FALSE(IsLegalOpcode(0xE8D8, true));  // BFTST (A0)+: not control
}

// An illegal encoding is rejected before its operands are touched.
TEST(LegalityTest, IllegalEncodingHasNoSideEffects) {
  M68kSimulator sim{M68kSimulator::Config{}};
  Memory& m = sim.GetMemory();
  m.Erase();
  m.WriteLong(0x10, 0x3000);    // illegal instruction vector
  m.WriteWord(0x1000, 0x105E);  // MOVEA.B (A6)+,A0
  sim.SetStartAddress(0x1000);
  sim.InitProgram();
  sim.State().a[6] = 0x2000;
  EXPECT_EQ(sim.Step(), SimResult::kBadInstruction);
  EXPECT_EQ(sim.State().a[6], 0x2000u);  // (A6)+ not applied
  EXPECT_EQ(sim.State().pc, 0x3000u);
}

}  // namespace
}  // namespace easym68k::sim
