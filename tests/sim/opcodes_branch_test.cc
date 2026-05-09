// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {
namespace {

class BranchTest : public ::testing::Test {
 protected:
  static M68kSimulator sim_;

  void SetUp() override {
    sim_.GetMemory().Clear();
    sim_.GetMemory().WriteLong(0x0, 0x3FFE);  // SSP
    sim_.GetMemory().WriteLong(0x4, 0x1000);  // PC
    sim_.Reset();
    sim_.State().sr = kSrSupervisor;
  }
};

M68kSimulator BranchTest::sim_{M68kSimulator::Config{}};

// =============================================================================
// BRA — unconditional branch
// =============================================================================

TEST_F(BranchTest, BraShortForward) {
  sim_.GetMemory().WriteWord(0x1000, 0x6004);  // BRA +4 (base=0x1002)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1006u);
}

TEST_F(BranchTest, BraShortBackward) {
  sim_.GetMemory().WriteWord(0x1000, 0x60FE);  // BRA -2 (disp8=0xFE=-2)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1000u);  // 0x1002 + (-2)
}

TEST_F(BranchTest, BraWordForm) {
  sim_.GetMemory().WriteWord(0x1000, 0x6000);  // BRA (word form, disp8=0)
  sim_.GetMemory().WriteWord(0x1002, 0x0010);  // disp = +16
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1012u);  // 0x1002 + 16
}

TEST_F(BranchTest, BraWordFormNegative) {
  sim_.GetMemory().WriteWord(0x1000, 0x6000);
  sim_.GetMemory().WriteWord(0x1002, 0xFFF0);  // disp = -16
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x0FF2u);  // 0x1002 + (-16)
}

// =============================================================================
// BSR — branch to subroutine
// =============================================================================

TEST_F(BranchTest, BsrShortPushesReturnAddr) {
  // BSR +14 → target 0x1002+14=0x1010; short form: return addr is pc past opcode
  sim_.GetMemory().WriteWord(0x1000, 0x610E);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.GetMemory().ReadLong(0x3FFA).value, 0x1002u);
  EXPECT_EQ(sim_.State().GetSP(), 0x3FFAu);
  EXPECT_EQ(sim_.State().pc, 0x1010u);
}

TEST_F(BranchTest, BsrWordFormReturnAddr) {
  // Word form: return addr is pc past opcode + extension word = 0x1004
  sim_.GetMemory().WriteWord(0x1000, 0x6100);
  sim_.GetMemory().WriteWord(0x1002, 0x000E);  // disp = +14 → 0x1010
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.GetMemory().ReadLong(0x3FFA).value, 0x1004u);
  EXPECT_EQ(sim_.State().pc, 0x1010u);
}

// =============================================================================
// Bcc — conditional branch
// =============================================================================

TEST_F(BranchTest, BccEqTaken) {
  sim_.State().sr = kSrSupervisor | kSrZero;
  sim_.GetMemory().WriteWord(0x1000, 0x6704);  // BEQ +4
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1006u);
}

TEST_F(BranchTest, BccEqNotTaken) {
  // Z clear → not taken; PC advances past opcode only (short form)
  sim_.GetMemory().WriteWord(0x1000, 0x6704);  // BEQ +4
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1002u);
}

TEST_F(BranchTest, BccNeTaken) {
  // Z clear → BNE taken
  sim_.GetMemory().WriteWord(0x1000, 0x6604);  // BNE +4
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1006u);
}

TEST_F(BranchTest, BccNeNotTaken) {
  sim_.State().sr = kSrSupervisor | kSrZero;
  sim_.GetMemory().WriteWord(0x1000, 0x6604);  // BNE +4
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1002u);
}

TEST_F(BranchTest, BccWordFormTaken) {
  // BCC (carry clear), carry is clear by default
  sim_.GetMemory().WriteWord(0x1000, 0x6400);  // BCC (word form)
  sim_.GetMemory().WriteWord(0x1002, 0x0010);  // disp = +16
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1012u);
}

TEST_F(BranchTest, BccWordFormNotTaken) {
  // BCC not taken (carry set); PC advances past extension word
  sim_.State().sr = kSrSupervisor | kSrCarry;
  sim_.GetMemory().WriteWord(0x1000, 0x6400);  // BCC (word form)
  sim_.GetMemory().WriteWord(0x1002, 0x0010);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1004u);  // extension word was consumed
}

TEST_F(BranchTest, BccCarrySetTaken) {
  sim_.State().sr = kSrSupervisor | kSrCarry;
  sim_.GetMemory().WriteWord(0x1000, 0x6506);  // BCS +6
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1008u);
}

// =============================================================================
// DBcc — decrement and branch
// =============================================================================

TEST_F(BranchTest, DbcfBranchTaken) {
  // DBRA D0 (DBF): condition always false → always decrements and branches
  sim_.State().d[0] = 1;
  sim_.GetMemory().WriteWord(0x1000, 0x51C8);  // DBRA D0
  sim_.GetMemory().WriteWord(0x1002, 0xFFFE);  // disp = -2 → target 0x1000
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1000u);
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0u);
}

TEST_F(BranchTest, DbcfFallThroughWhenMinus1) {
  // Counter starts at 0, decrements to -1 → fall through
  sim_.State().d[0] = 0;
  sim_.GetMemory().WriteWord(0x1000, 0x51C8);  // DBRA D0
  sim_.GetMemory().WriteWord(0x1002, 0xFFFE);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1004u);             // past displacement word
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0xFFFFu);  // counter = -1
}

TEST_F(BranchTest, DbccConditionTrue_FallThrough) {
  // DBT D0: condition T is always true → fall through immediately, no decrement
  sim_.State().d[0] = 5;
  sim_.GetMemory().WriteWord(0x1000, 0x50C8);  // DBT D0
  sim_.GetMemory().WriteWord(0x1002, 0xFFFE);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1004u);
  EXPECT_EQ(sim_.State().d[0], 5u);  // unchanged
}

TEST_F(BranchTest, DbccPreservesHighWord) {
  // Only the low word of Dn is used as the counter
  sim_.State().d[0] = 0x00050002;              // high=5, low=2
  sim_.GetMemory().WriteWord(0x1000, 0x51C8);  // DBRA D0
  sim_.GetMemory().WriteWord(0x1002, 0xFFFE);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0x00050001u);  // high word intact, low word decremented
}

// =============================================================================
// Scc — set byte according to condition
// =============================================================================

TEST_F(BranchTest, SccTrue) {
  sim_.State().d[0] = 0;
  sim_.GetMemory().WriteWord(0x1000, 0x50C0);  // ST D0 (condition always true)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0xFFu);
}

TEST_F(BranchTest, SccFalse) {
  sim_.State().d[0] = 0xFF;
  sim_.GetMemory().WriteWord(0x1000, 0x51C0);  // SF D0 (condition always false)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x00u);
}

TEST_F(BranchTest, SccEqWhenZeroSet) {
  sim_.State().sr = kSrSupervisor | kSrZero;
  sim_.State().d[0] = 0;
  sim_.GetMemory().WriteWord(0x1000, 0x57C0);  // SEQ D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0xFFu);
}

TEST_F(BranchTest, SccEqWhenZeroClear) {
  // Z clear → SEQ writes 0x00
  sim_.State().d[0] = 0xFF;
  sim_.GetMemory().WriteWord(0x1000, 0x57C0);  // SEQ D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0x00u);
}

TEST_F(BranchTest, SccToMemory) {
  sim_.State().a[0] = 0x2000;
  sim_.GetMemory().WriteByte(0x2000, 0x42);
  sim_.GetMemory().WriteWord(0x1000, 0x51D0);  // SF (A0): always writes 0x00
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.GetMemory().ReadByte(0x2000).value, 0x00u);
}

// =============================================================================
// JMP — jump to effective address
// =============================================================================

TEST_F(BranchTest, JmpIndirect) {
  sim_.State().a[0] = 0x2000;
  sim_.GetMemory().WriteWord(0x1000, 0x4ED0);  // JMP (A0)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x2000u);
}

TEST_F(BranchTest, JmpAbsoluteWord) {
  sim_.GetMemory().WriteWord(0x1000, 0x4EF8);  // JMP (abs.W)
  sim_.GetMemory().WriteWord(0x1002, 0x2000);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x2000u);
}

// =============================================================================
// JSR — jump to subroutine
// =============================================================================

TEST_F(BranchTest, JsrPushesReturnAddress) {
  sim_.State().a[0] = 0x2000;
  sim_.GetMemory().WriteWord(0x1000, 0x4E90);  // JSR (A0)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.GetMemory().ReadLong(0x3FFA).value, 0x1002u);  // return addr
  EXPECT_EQ(sim_.State().GetSP(), 0x3FFAu);
}

TEST_F(BranchTest, JsrBranches) {
  sim_.State().a[1] = 0x3000;
  sim_.GetMemory().WriteWord(0x1000, 0x4E91);  // JSR (A1)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x3000u);
}

// =============================================================================
// RTS — return from subroutine
// =============================================================================

TEST_F(BranchTest, RtsRestoresPC) {
  sim_.State().ssp = 0x3FFA;
  sim_.GetMemory().WriteLong(0x3FFA, 0x0000BEEF);
  sim_.GetMemory().WriteWord(0x1000, 0x4E75);  // RTS
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0xBEEFu);
  EXPECT_EQ(sim_.State().GetSP(), 0x3FFEu);
}

TEST_F(BranchTest, BsrRtsRoundTrip) {
  sim_.GetMemory().WriteWord(0x1000, 0x610E);  // BSR +14 → 0x1010
  sim_.GetMemory().WriteWord(0x1010, 0x4E75);  // RTS

  EXPECT_EQ(sim_.Step(), SimResult::kOk);  // BSR
  EXPECT_EQ(sim_.State().pc, 0x1010u);

  EXPECT_EQ(sim_.Step(), SimResult::kOk);  // RTS
  EXPECT_EQ(sim_.State().pc, 0x1002u);     // returns to after BSR
}

// =============================================================================
// RTE — return from exception (supervisor only)
// =============================================================================

TEST_F(BranchTest, RteSupervisor) {
  sim_.State().ssp = 0x3FF8;
  sim_.GetMemory().WriteWord(0x3FF8, kSrSupervisor);  // SR on stack
  sim_.GetMemory().WriteLong(0x3FFA, 0x5000);         // PC on stack
  sim_.GetMemory().WriteWord(0x1000, 0x4E73);         // RTE
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().sr, kSrSupervisor);
  EXPECT_EQ(sim_.State().pc, 0x5000u);
  EXPECT_EQ(sim_.State().ssp, 0x3FFEu);
}

TEST_F(BranchTest, RteUserModePrivilege) {
  sim_.State().sr = 0;                         // user mode
  sim_.GetMemory().WriteWord(0x1000, 0x4E73);  // RTE
  EXPECT_EQ(sim_.Step(), SimResult::kPrivilegeViolation);
}

// =============================================================================
// RTR — return and restore CCR
// =============================================================================

TEST_F(BranchTest, RtrRestoresCcrOnly) {
  // RTR restores only the low byte of SR (CCR), not the supervisor half
  sim_.State().sr = kSrSupervisor;
  sim_.State().ssp = 0x3FF8;
  sim_.GetMemory().WriteWord(0x3FF8, 0x001F);  // CCR: all flags (X,N,Z,V,C)
  sim_.GetMemory().WriteLong(0x3FFA, 0x5000);
  sim_.GetMemory().WriteWord(0x1000, 0x4E77);  // RTR
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().sr & 0xFF00u, static_cast<uint16_t>(kSrSupervisor & 0xFF00u));
  EXPECT_EQ(sim_.State().sr & 0xFFu, 0x1Fu);
}

TEST_F(BranchTest, RtrRestoresPC) {
  sim_.State().ssp = 0x3FF8;
  sim_.GetMemory().WriteWord(0x3FF8, 0x0000);  // CCR: no flags
  sim_.GetMemory().WriteLong(0x3FFA, 0x7000);
  sim_.GetMemory().WriteWord(0x1000, 0x4E77);  // RTR
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x7000u);
  EXPECT_EQ(sim_.State().GetSP(), 0x3FFEu);
}

}  // namespace
}  // namespace easym68k::sim
