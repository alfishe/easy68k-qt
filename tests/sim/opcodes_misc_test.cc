// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {
namespace {

class MiscTest : public ::testing::Test {
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

M68kSimulator MiscTest::sim_{M68kSimulator::Config{}};

// =============================================================================
// TRAP #n
// =============================================================================

TEST_F(MiscTest, TrapReturnsKTrapException) {
  sim_.GetMemory().WriteWord(0x1000, 0x4E40);  // TRAP #0
  EXPECT_EQ(sim_.Step(), SimResult::kTrapException);
}

TEST_F(MiscTest, TrapRedirectsThroughVector) {
  // TRAP #5 → vector 32+5=37 at address 37*4=0x94
  uint32_t handler = 0x5000;
  sim_.GetMemory().WriteLong(37 * 4, handler);
  sim_.GetMemory().WriteWord(0x1000, 0x4E45);  // TRAP #5
  sim_.Step();
  EXPECT_EQ(sim_.State().pc, handler);
}

TEST_F(MiscTest, TrapPushesStackFrame) {
  // HandleException pushes SR (word) + PC (long) onto SSP
  sim_.GetMemory().WriteLong(32 * 4, 0x5000);  // TRAP #0 handler
  sim_.GetMemory().WriteWord(0x1000, 0x4E40);  // TRAP #0
  uint16_t sr_before = sim_.State().sr;
  sim_.Step();
  uint32_t new_ssp = sim_.State().ssp;
  EXPECT_EQ(sim_.GetMemory().ReadWord(new_ssp).value, sr_before);
  EXPECT_EQ(sim_.GetMemory().ReadLong(new_ssp + 2).value, 0x1000u);  // inst_pc_ = TRAP address
}

TEST_F(MiscTest, TrapVectorRange) {
  // TRAP #15 → vector 47, address 47*4=0xBC
  sim_.GetMemory().WriteLong(47 * 4, 0x6000);
  sim_.GetMemory().WriteWord(0x1000, 0x4E4F);  // TRAP #15
  sim_.Step();
  EXPECT_EQ(sim_.State().pc, 0x6000u);
}

// =============================================================================
// TRAPV
// =============================================================================

TEST_F(MiscTest, TrapV_VSet_Traps) {
  sim_.State().sr = kSrSupervisor | kSrOverflow;
  sim_.GetMemory().WriteWord(0x1000, 0x4E76);  // TRAPV
  EXPECT_EQ(sim_.Step(), SimResult::kTrapVException);
}

TEST_F(MiscTest, TrapV_VClear_NoOp) {
  // V clear → TRAPV is a no-op, PC just advances
  sim_.GetMemory().WriteWord(0x1000, 0x4E76);  // TRAPV
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1002u);
}

TEST_F(MiscTest, TrapV_VSet_RedirectsThroughVector) {
  sim_.State().sr = kSrSupervisor | kSrOverflow;
  sim_.GetMemory().WriteLong(static_cast<uint32_t>(ExceptionVector::kTrapV) * 4, 0x7000);
  sim_.GetMemory().WriteWord(0x1000, 0x4E76);
  sim_.Step();
  EXPECT_EQ(sim_.State().pc, 0x7000u);
}

// =============================================================================
// ILLEGAL
// =============================================================================

TEST_F(MiscTest, IllegalReturnsBadInstruction) {
  sim_.GetMemory().WriteWord(0x1000, 0x4AFC);  // ILLEGAL
  EXPECT_EQ(sim_.Step(), SimResult::kBadInstruction);
}

TEST_F(MiscTest, IllegalRedirectsThroughVector) {
  sim_.GetMemory().WriteLong(static_cast<uint32_t>(ExceptionVector::kIllegalInst) * 4, 0x8000);
  sim_.GetMemory().WriteWord(0x1000, 0x4AFC);
  sim_.Step();
  EXPECT_EQ(sim_.State().pc, 0x8000u);
}

// =============================================================================
// RESET
// =============================================================================

TEST_F(MiscTest, Reset_Supervisor_ReturnsOk) {
  sim_.GetMemory().WriteWord(0x1000, 0x4E70);  // RESET
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1002u);
}

TEST_F(MiscTest, Reset_UserMode_PrivilegeViolation) {
  sim_.State().sr = 0;  // user mode
  sim_.GetMemory().WriteWord(0x1000, 0x4E70);
  EXPECT_EQ(sim_.Step(), SimResult::kPrivilegeViolation);
}

// =============================================================================
// STOP
// =============================================================================

TEST_F(MiscTest, Stop_LoadsSR) {
  sim_.GetMemory().WriteWord(0x1000, 0x4E72);  // STOP
  sim_.GetMemory().WriteWord(0x1002, 0x2004);  // new SR: supervisor + Z
  EXPECT_EQ(sim_.Step(), SimResult::kStopInstruction);
  EXPECT_EQ(sim_.State().sr, 0x2004u);
}

TEST_F(MiscTest, Stop_SetsHaltedFlag) {
  sim_.GetMemory().WriteWord(0x1000, 0x4E72);
  sim_.GetMemory().WriteWord(0x1002, kSrSupervisor);
  sim_.Step();
  EXPECT_TRUE(sim_.State().halted);
  EXPECT_TRUE(sim_.State().stopped);
}

TEST_F(MiscTest, Stop_AdvancesPastExtWord) {
  sim_.GetMemory().WriteWord(0x1000, 0x4E72);
  sim_.GetMemory().WriteWord(0x1002, kSrSupervisor);
  sim_.Step();
  EXPECT_EQ(sim_.State().pc, 0x1004u);  // past the immediate word
}

TEST_F(MiscTest, Stop_UserMode_PrivilegeViolation) {
  sim_.State().sr = 0;  // user mode
  sim_.GetMemory().WriteWord(0x1000, 0x4E72);
  sim_.GetMemory().WriteWord(0x1002, 0x0000);
  EXPECT_EQ(sim_.Step(), SimResult::kPrivilegeViolation);
}

TEST_F(MiscTest, Stop_UserMode_DoesNotSetStoppedFlag) {
  // Extension word is fetched before privilege check (matches original order),
  // but STOP never completes so stopped is not set.
  sim_.State().sr = 0;  // user mode
  sim_.GetMemory().WriteWord(0x1000, 0x4E72);
  sim_.GetMemory().WriteWord(0x1002, 0x0000);
  sim_.Step();
  EXPECT_FALSE(sim_.State().stopped);
}

TEST_F(MiscTest, Stop_PostLoadPrivilegeViolation) {
  // New SR clears supervisor bit → post-load privilege violation.
  // 0x0000 has no supervisor bit.
  sim_.GetMemory().WriteWord(0x1000, 0x4E72);
  sim_.GetMemory().WriteWord(0x1002, 0x0000);  // new SR: no supervisor
  EXPECT_EQ(sim_.Step(), SimResult::kPrivilegeViolation);
}

TEST_F(MiscTest, Stop_TraceActiveGeneratesTraceException) {
  sim_.State().sr = kSrSupervisor | kSrTrace;
  sim_.GetMemory().WriteWord(0x1000, 0x4E72);
  sim_.GetMemory().WriteWord(0x1002, kSrSupervisor);  // new SR: supervisor, no trace
  // Trace was active → STOP returns trace exception instead of halting.
  EXPECT_EQ(sim_.Step(), SimResult::kTraceException);
  // HandleException clears trace; CPU was not halted.
  EXPECT_FALSE(sim_.State().GetFlag(kSrTrace));
  EXPECT_FALSE(sim_.State().halted);
}

TEST_F(MiscTest, Stop_HaltedPreventsFurtherStepping) {
  sim_.GetMemory().WriteWord(0x1000, 0x4E72);
  sim_.GetMemory().WriteWord(0x1002, kSrSupervisor);
  sim_.Step();  // STOP → halted
  // Subsequent Step() returns kHalted without executing anything
  EXPECT_EQ(sim_.Step(), SimResult::kHalted);
  EXPECT_EQ(sim_.State().pc, 0x1004u);  // PC unchanged after halt
}

}  // namespace
}  // namespace easym68k::sim
