// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Advanced breakpoint engine (Task 9.9) — Sim68K BPoint/BPointExpr and the
// runprog() breakpoint checks (RUN.CPP).

#include <gtest/gtest.h>

#include <vector>

#include "easym68k/sim/isim_observer.h"
#include "easym68k/sim/simulator.h"

namespace easym68k::sim {
namespace {

class BreakConditionTest : public ::testing::Test {
 protected:
  M68kSimulator sim_{M68kSimulator::Config{}};

  void Load(std::initializer_list<uint16_t> words) {
    sim_.GetMemory().Clear();
    sim_.GetMemory().WriteLong(0x0, 0x3FFE);
    sim_.GetMemory().WriteLong(0x4, 0x1000);
    uint32_t a = 0x1000;
    for (uint16_t w : words) {
      sim_.GetMemory().WriteWord(a, w);
      a += 2;
    }
    for (; a < 0x1100; a += 2)
      sim_.GetMemory().WriteWord(a, 0x4E71);  // NOP padding
    sim_.Reset();
  }

  static BreakCondition Reg(uint32_t id, int op, int32_t value, int size = BreakCondition::kLong) {
    BreakCondition c;
    c.enabled = true;
    c.type = BreakCondition::kPcReg;
    c.type_id = id;
    c.op = op;
    c.value = value;
    c.size = size;
    return c;
  }
  static BreakCondition Mem(uint32_t addr, int access, int op = BreakCondition::kAnyValue,
                            int32_t value = 0, int size = BreakCondition::kWord) {
    BreakCondition c;
    c.enabled = true;
    c.type = BreakCondition::kAddress;
    c.type_id = addr;
    c.access = access;
    c.op = op;
    c.value = value;
    c.size = size;
    return c;
  }
  void Expr(std::vector<int> postfix, int count = 1) {
    BreakExpression e;
    e.enabled = true;
    e.postfix = std::move(postfix);
    e.count = count;
    sim_.SetBreakExpressions({e});
  }
};

TEST_F(BreakConditionTest, RunStartedOnPcBreakpointExecutesIt) {
  // runprog() executes an instruction before checking the breakpoints.
  Load({0x4E71, 0x4E71, 0x4E71});
  sim_.AddBreakpoint(0x1000);
  sim_.AddBreakpoint(0x1004);
  sim_.Run();
  EXPECT_EQ(sim_.State().pc, 0x1004u);
}

TEST_F(BreakConditionTest, RegisterEquals) {
  Load({0x5280, 0x5280, 0x5280, 0x5280, 0x5280, 0x5280, 0x5280});  // ADDQ.L #1,D0
  sim_.SetBreakCondition(0, Reg(BreakCondition::kD0, BreakCondition::kEqual, 5));
  Expr({0});
  sim_.Run();
  EXPECT_EQ(sim_.State().d[0], 5u);
  EXPECT_EQ(sim_.State().pc, 0x100Au);
  EXPECT_TRUE(sim_.AdvancedBreakpointHit());
}

TEST_F(BreakConditionTest, PassCount) {
  // D0 > 2 is true at D0 = 3, 4, 5: the third pass breaks.
  Load({0x5280, 0x5280, 0x5280, 0x5280, 0x5280, 0x5280, 0x5280, 0x5280});
  sim_.SetBreakCondition(0, Reg(BreakCondition::kD0, BreakCondition::kGreater, 2));
  Expr({0}, 3);
  sim_.Run();
  EXPECT_EQ(sim_.State().d[0], 5u);
}

TEST_F(BreakConditionTest, AndOrExpressions) {
  Load({0x5280, 0x5280, 0x5280, 0x5280, 0x5280, 0x5280});
  sim_.SetBreakCondition(0, Reg(BreakCondition::kD0, BreakCondition::kEqual, 4));
  sim_.SetBreakCondition(1, Reg(BreakCondition::kD0 + 1, BreakCondition::kEqual, 0));
  Expr({0, 1, BreakExpression::kAnd});
  sim_.Run();
  EXPECT_EQ(sim_.State().d[0], 4u);

  Load({0x5280, 0x5280, 0x5280, 0x5280, 0x5280, 0x5280});
  sim_.SetBreakCondition(1, Reg(BreakCondition::kD0 + 1, BreakCondition::kEqual, 99));
  Expr({0, 1, BreakExpression::kAnd});
  sim_.Run(6);
  EXPECT_FALSE(sim_.AdvancedBreakpointHit());  // D1 is never 99

  Load({0x5280, 0x5280, 0x5280, 0x5280, 0x5280, 0x5280});
  Expr({1, 0, BreakExpression::kOr});
  sim_.Run();
  EXPECT_EQ(sim_.State().d[0], 4u);
}

TEST_F(BreakConditionTest, SignedLongComparison) {
  Load({0x4E71, 0x70FF, 0x4E71});  // NOP; MOVEQ #-1,D0
  sim_.SetBreakCondition(0, Reg(BreakCondition::kD0, BreakCondition::kLess, 0));
  Expr({0});
  sim_.Run();
  EXPECT_EQ(sim_.State().pc, 0x1004u);
}

TEST_F(BreakConditionTest, ByteSizeMasksValue) {
  sim_.State().d[0] = 0;
  Load({0x5280, 0x5280});
  sim_.State().d[0] = 0x12345600;
  sim_.SetBreakCondition(
      0, Reg(BreakCondition::kD0, BreakCondition::kEqual, 2, BreakCondition::kByte));
  Expr({0});
  sim_.Run();
  EXPECT_EQ(sim_.State().d[0], 0x12345602u);
}

TEST_F(BreakConditionTest, PcCondition) {
  Load({0x4E71, 0x4E71, 0x4E71, 0x4E71, 0x4E71});
  sim_.SetBreakCondition(0, Reg(BreakCondition::kPc, BreakCondition::kEqual, 0x1006));
  Expr({0});
  sim_.Run();
  EXPECT_EQ(sim_.State().pc, 0x1006u);
}

TEST_F(BreakConditionTest, MemoryWrite) {
  Load({0x4E71, 0x4E71, 0x31C0, 0x2000, 0x4E71});  // MOVE.W D0,$2000.W
  sim_.SetBreakCondition(0, Mem(0x2000, BreakCondition::kWrite));
  Expr({0});
  sim_.Run();
  EXPECT_EQ(sim_.State().pc, 0x1008u);
}

TEST_F(BreakConditionTest, MemoryReadAndAccessKind) {
  Load({0x4E71, 0x3238, 0x2000, 0x4E71, 0x31C0, 0x2000, 0x4E71});  // MOVE.W $2000.W,D1; ...
  sim_.SetBreakCondition(0, Mem(0x2000, BreakCondition::kRead));
  Expr({0});
  sim_.Run();
  EXPECT_EQ(sim_.State().pc, 0x1006u);

  // A write-only condition ignores the read and breaks at the write.
  Load({0x4E71, 0x3238, 0x2000, 0x4E71, 0x31C0, 0x2000, 0x4E71});
  sim_.SetBreakCondition(0, Mem(0x2000, BreakCondition::kWrite));
  Expr({0});
  sim_.Run();
  EXPECT_EQ(sim_.State().pc, 0x100Cu);
}

TEST_F(BreakConditionTest, MemoryValue) {
  Load({0x4E71, 0x31FC, 0x1234, 0x2000, 0x4E71});  // MOVE.W #$1234,$2000.W
  sim_.SetBreakCondition(0,
                         Mem(0x2000, BreakCondition::kAnyAccess, BreakCondition::kEqual, 0x1234));
  Expr({0});
  sim_.Run();
  EXPECT_EQ(sim_.State().pc, 0x1008u);
}

TEST_F(BreakConditionTest, EvaluatingAMemoryConditionCountsAsARead) {
  // BPoint::isBreak() reads the location through mem_req(), which records it
  // as the last read: a later read condition on the same address is then met
  // even though the instruction did not read it (original behaviour).
  Load({0x4E71, 0x4E71});
  sim_.SetBreakCondition(0, Mem(0x2000, BreakCondition::kAnyAccess));
  sim_.SetBreakCondition(1, Mem(0x2000, BreakCondition::kRead));
  Expr({0, 1, BreakExpression::kAnd});
  sim_.Step();
  EXPECT_TRUE(sim_.AdvancedBreakpointHit());

  Load({0x4E71, 0x4E71});
  Expr({1, 0, BreakExpression::kAnd});  // read condition evaluated first
  sim_.Step();
  EXPECT_FALSE(sim_.AdvancedBreakpointHit());
}

TEST_F(BreakConditionTest, DisabledConditionsAndExpressions) {
  Load({0x5280, 0x5280, 0x5280});
  BreakCondition c = Reg(BreakCondition::kD0, BreakCondition::kEqual, 1);
  c.enabled = false;
  sim_.SetBreakCondition(0, c);
  Expr({0});
  sim_.Run(3);
  EXPECT_FALSE(sim_.AdvancedBreakpointHit());
  EXPECT_EQ(sim_.State().d[0], 3u);

  Load({0x5280, 0x5280, 0x5280});
  sim_.SetBreakCondition(0, Reg(BreakCondition::kD0, BreakCondition::kEqual, 1));
  BreakExpression e;
  e.postfix = {0};
  sim_.SetBreakExpressions({e});  // not enabled
  sim_.Run(3);
  EXPECT_EQ(sim_.State().d[0], 3u);

  sim_.ClearAdvancedBreakpoints();
  EXPECT_TRUE(sim_.GetBreakExpressions().empty());
  EXPECT_FALSE(sim_.GetBreakCondition(0).enabled);
}

struct HitCounter : ISimObserver {
  int hits = 0;
  void OnEvent(const SimEvent& e) override {
    if (e.type == SimEventType::kBreakpointHit)
      ++hits;
  }
};

TEST_F(BreakConditionTest, StepReportsHitAndEvent) {
  Load({0x5280, 0x5280});
  HitCounter obs;
  sim_.AddObserver(&obs);
  sim_.SetBreakCondition(0, Reg(BreakCondition::kD0, BreakCondition::kEqual, 1));
  Expr({0});
  sim_.Step();
  EXPECT_TRUE(sim_.AdvancedBreakpointHit());
  EXPECT_EQ(obs.hits, 1);
  sim_.Step();
  EXPECT_FALSE(sim_.AdvancedBreakpointHit());
  sim_.RemoveObserver(&obs);
}

}  // namespace
}  // namespace easym68k::sim
