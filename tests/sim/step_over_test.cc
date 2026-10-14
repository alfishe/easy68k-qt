// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Step Over / Run To core API (Task 9.10) — SIM68Ku.cpp StepExecute and
// RunToCursorExecute, CODE8.CPP BSR/JSR stepToAddr, RUN.CPP runprog().

#include <gtest/gtest.h>

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {
namespace {

class StepOverTest : public ::testing::Test {
 protected:
  M68kSimulator sim_{M68kSimulator::Config{}};

  // 1000: BSR.W SUB      1004: NOP  1006: NOP  1008: NOP
  // 100A: SUB  ADDQ.L #1,D0 / BSR.W SUB2 / RTS
  // 1012: SUB2 ADDQ.L #1,D1 / RTS
  void SetUp() override {
    Memory& m = sim_.GetMemory();
    m.Clear();
    m.WriteLong(0x0, 0x8000);
    m.WriteLong(0x4, 0x1000);
    const uint16_t code[] = {0x6100, 0x0008, 0x4E71, 0x4E71, 0x4E71, 0x5280,
                             0x6100, 0x0004, 0x4E75, 0x5281, 0x4E75};
    uint32_t a = 0x1000;
    for (uint16_t w : code) {
      m.WriteWord(a, w);
      a += 2;
    }
    sim_.Reset();
  }
};

TEST_F(StepOverTest, StepOverRunsWholeSubroutine) {
  EXPECT_EQ(sim_.StepOver(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1004u);
  EXPECT_EQ(sim_.State().d[0], 1u);
  EXPECT_EQ(sim_.State().d[1], 1u);  // nested call ran too
}

TEST_F(StepOverTest, StepOverNonCallIsOneInstruction) {
  sim_.State().pc = 0x1004;
  sim_.StepOver();
  EXPECT_EQ(sim_.State().pc, 0x1006u);
}

TEST_F(StepOverTest, StepOverStopsAtBreakpointInSubroutine) {
  sim_.AddBreakpoint(0x1012);
  sim_.StepOver();
  EXPECT_EQ(sim_.State().pc, 0x1012u);
  EXPECT_EQ(sim_.State().d[0], 1u);
  EXPECT_EQ(sim_.State().d[1], 0u);
}

TEST_F(StepOverTest, StepOverJsr) {
  Memory& m = sim_.GetMemory();
  m.WriteWord(0x1000, 0x4EB9);  // JSR $0000100A
  m.WriteLong(0x1002, 0x0000100A);
  sim_.StepOver();
  EXPECT_EQ(sim_.State().pc, 0x1006u);
  EXPECT_EQ(sim_.State().d[0], 1u);
}

TEST_F(StepOverTest, TraceIntoEntersSubroutine) {
  sim_.Step();
  EXPECT_EQ(sim_.State().pc, 0x100Au);
}

TEST_F(StepOverTest, RunTo) {
  sim_.State().pc = 0x1004;
  sim_.RunTo(0x1008);
  EXPECT_EQ(sim_.State().pc, 0x1008u);
  EXPECT_EQ(sim_.GetRunToAddress(), 0u);  // cleared when reached
}

TEST_F(StepOverTest, RunToBreakpointAddressIsNotArmed) {
  sim_.State().pc = 0x1004;
  sim_.AddBreakpoint(0x1008);
  sim_.RunTo(0x1008);
  EXPECT_EQ(sim_.State().pc, 0x1008u);
  EXPECT_EQ(sim_.GetRunToAddress(), 0u);
}

TEST_F(StepOverTest, RunToStaysArmedAfterAnEarlierBreakpoint) {
  sim_.State().pc = 0x1004;
  sim_.AddBreakpoint(0x1006);
  sim_.RunTo(0x1008);
  EXPECT_EQ(sim_.State().pc, 0x1006u);
  EXPECT_EQ(sim_.GetRunToAddress(), 0x1008u);
  sim_.RemoveBreakpoint(0x1006);
  sim_.Run();
  EXPECT_EQ(sim_.State().pc, 0x1008u);

  sim_.RunTo(0x2000, 1);
  EXPECT_EQ(sim_.GetRunToAddress(), 0x2000u);
  sim_.InitProgram();  // initSim() clears runToAddr
  EXPECT_EQ(sim_.GetRunToAddress(), 0u);
}

TEST_F(StepOverTest, ResetBreakPassCounts) {
  BreakCondition c;
  c.enabled = true;
  c.type = BreakCondition::kPcReg;
  c.type_id = BreakCondition::kPc;
  c.op = BreakCondition::kAnyValue;
  sim_.SetBreakCondition(0, c);
  BreakExpression e;
  e.enabled = true;
  e.postfix = {0};
  e.count = 2;
  sim_.SetBreakExpressions({e});
  sim_.State().pc = 0x1004;
  sim_.Step();  // pass 1
  EXPECT_FALSE(sim_.AdvancedBreakpointHit());
  sim_.ResetBreakPassCounts();
  sim_.Step();  // pass 1 again
  EXPECT_FALSE(sim_.AdvancedBreakpointHit());
  sim_.Step();  // pass 2
  EXPECT_TRUE(sim_.AdvancedBreakpointHit());
}

}  // namespace
}  // namespace easym68k::sim
