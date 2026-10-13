// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/sim/simulator.h"

#include <gtest/gtest.h>

#include <initializer_list>

namespace easym68k::sim {
namespace {

// ---------------------------------------------------------------------------
// Fixture: simulator with empty Config (no I/O interfaces needed for unit tests)
// ---------------------------------------------------------------------------

class SimTest : public ::testing::Test {
 protected:
  static M68kSimulator sim_;

  // Convenience: reset with SSP=ssp, PC pointing to a NOP or user-supplied opcode.
  void ResetWithVectors(uint32_t ssp_val, uint32_t pc_val) {
    sim_.GetMemory().Clear();
    sim_.GetMemory().WriteLong(0x0, ssp_val);
    sim_.GetMemory().WriteLong(0x4, pc_val);
    sim_.ClearBreakpoints();
    sim_.Reset();
  }
};

M68kSimulator SimTest::sim_{M68kSimulator::Config{}};

// ---------------------------------------------------------------------------
// 1. ResetInitializesFromVectors
// ---------------------------------------------------------------------------

TEST_F(SimTest, ResetInitializesFromVectors) {
  ResetWithVectors(0x00003FFE, 0x00001000);
  EXPECT_EQ(sim_.State().ssp, 0x3FFEu);
  EXPECT_EQ(sim_.State().pc, 0x1000u);
  EXPECT_TRUE(sim_.State().IsSupervisor());
  EXPECT_FALSE(sim_.State().halted);
}

// ---------------------------------------------------------------------------
// 2. SetGetRegisters
// ---------------------------------------------------------------------------

TEST_F(SimTest, SetGetRegisters) {
  sim_.State().d[0] = 0x12345678;
  sim_.State().a[1] = 0xABCDEF;
  sim_.State().pc = 0x2000;
  sim_.State().sr = kSrSupervisor | kSrZero;

  EXPECT_EQ(sim_.State().d[0], 0x12345678u);
  EXPECT_EQ(sim_.State().a[1], 0x00ABCDEFu);
  EXPECT_EQ(sim_.State().pc, 0x2000u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_TRUE(sim_.State().IsSupervisor());
}

// ---------------------------------------------------------------------------
// 3. SupervisorMode
// ---------------------------------------------------------------------------

TEST_F(SimTest, SupervisorMode) {
  ResetWithVectors(0x3FFE, 0x1000);
  EXPECT_TRUE(sim_.State().IsSupervisor());

  sim_.State().SetSupervisor(false);
  EXPECT_FALSE(sim_.State().IsSupervisor());

  sim_.State().SetSupervisor(true);
  EXPECT_TRUE(sim_.State().IsSupervisor());
}

// ---------------------------------------------------------------------------
// 4. InterruptMask
// ---------------------------------------------------------------------------

TEST_F(SimTest, InterruptMask) {
  sim_.State().sr = kSrSupervisor;
  sim_.State().SetInterruptMask(7);
  EXPECT_EQ(sim_.State().GetInterruptMask(), 7);

  sim_.State().SetInterruptMask(3);
  EXPECT_EQ(sim_.State().GetInterruptMask(), 3);

  sim_.State().SetInterruptMask(0);
  EXPECT_EQ(sim_.State().GetInterruptMask(), 0);
}

// ---------------------------------------------------------------------------
// 5. SPSwitching
// ---------------------------------------------------------------------------

TEST_F(SimTest, SPSwitching) {
  sim_.State().sr = kSrSupervisor;  // supervisor mode
  sim_.State().ssp = 0x3000;
  sim_.State().a[7] = 0x5000;  // USP
  EXPECT_EQ(sim_.State().GetSP(), 0x3000u);

  sim_.State().SetSupervisor(false);
  EXPECT_EQ(sim_.State().GetSP(), 0x5000u);

  sim_.State().SetSP(0x4800);
  EXPECT_EQ(sim_.State().a[7], 0x4800u);

  sim_.State().SetSupervisor(true);
  sim_.State().SetSP(0x2FFE);
  EXPECT_EQ(sim_.State().ssp, 0x2FFEu);
}

// ---------------------------------------------------------------------------
// 6. StepNop
// ---------------------------------------------------------------------------

TEST_F(SimTest, StepNop) {
  ResetWithVectors(0x3FFE, 0x1000);
  sim_.GetMemory().WriteWord(0x1000, 0x4E71);  // NOP

  auto result = sim_.Step();
  EXPECT_EQ(result, SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1002u);  // advanced by 2
}

// ---------------------------------------------------------------------------
// 7. StepIllegal
// ---------------------------------------------------------------------------

TEST_F(SimTest, StepIllegal) {
  ResetWithVectors(0x3FFE, 0x1000);
  sim_.GetMemory().WriteWord(0x1000, 0x4AFC);  // unimplemented in Phase 4
  // Provide a valid illegal-instruction vector so HandleException doesn't crash
  sim_.GetMemory().WriteLong(static_cast<uint32_t>(ExceptionVector::kIllegalInst) * 4, 0x5000);

  auto result = sim_.Step();
  EXPECT_EQ(result, SimResult::kBadInstruction);
  // Exception handler redirected PC
  EXPECT_EQ(sim_.State().pc, 0x5000u);
}

// ---------------------------------------------------------------------------
// 8. RunStopRequested
// ---------------------------------------------------------------------------

TEST_F(SimTest, RunStopRequested) {
  ResetWithVectors(0x3FFE, 0x1000);
  // Fill memory with NOPs
  for (uint32_t addr = 0x1000; addr < 0x1100; addr += 2) {
    sim_.GetMemory().WriteWord(addr, 0x4E71);
  }
  // Request stop before Run — should execute zero instructions
  sim_.RequestStop();
  sim_.Run();
  EXPECT_EQ(sim_.State().pc, 0x1000u);
}

// ---------------------------------------------------------------------------
// 9. RunBreakpoint
// ---------------------------------------------------------------------------

TEST_F(SimTest, RunBreakpoint) {
  ResetWithVectors(0x3FFE, 0x1000);
  for (uint32_t addr = 0x1000; addr < 0x1010; addr += 2) {
    sim_.GetMemory().WriteWord(addr, 0x4E71);
  }
  sim_.AddBreakpoint(0x1006);

  sim_.Run();
  EXPECT_EQ(sim_.State().pc, 0x1006u);  // stopped before executing at breakpoint
}

// ---------------------------------------------------------------------------
// 10. RunMaxInstructions
// ---------------------------------------------------------------------------

TEST_F(SimTest, RunMaxInstructions) {
  ResetWithVectors(0x3FFE, 0x1000);
  for (uint32_t addr = 0x1000; addr < 0x1020; addr += 2) {
    sim_.GetMemory().WriteWord(addr, 0x4E71);
  }
  sim_.Run(4);
  EXPECT_EQ(sim_.State().pc, 0x1008u);  // 4 NOPs × 2 bytes each
}

// ---------------------------------------------------------------------------
// 11. CheckConditionAll16
// ---------------------------------------------------------------------------

TEST_F(SimTest, CheckConditionAll16) {
  auto& sr = sim_.State().sr;

  // Helper to set flags
  auto set_flags = [&](bool n, bool z, bool v, bool c) {
    sr = kSrSupervisor;
    if (n)
      sr |= kSrNegative;
    if (z)
      sr |= kSrZero;
    if (v)
      sr |= kSrOverflow;
    if (c)
      sr |= kSrCarry;
  };

  // T / F
  set_flags(false, false, false, false);
  EXPECT_TRUE(sim_.CheckCondition(Condition::kTrue));
  EXPECT_FALSE(sim_.CheckCondition(Condition::kFalse));

  // HI: !C & !Z
  set_flags(false, false, false, false);
  EXPECT_TRUE(sim_.CheckCondition(Condition::kHigh));
  set_flags(false, false, false, true);
  EXPECT_FALSE(sim_.CheckCondition(Condition::kHigh));

  // LS: C | Z
  set_flags(false, true, false, false);
  EXPECT_TRUE(sim_.CheckCondition(Condition::kLowOrSame));
  set_flags(false, false, false, false);
  EXPECT_FALSE(sim_.CheckCondition(Condition::kLowOrSame));

  // CC / CS
  set_flags(false, false, false, false);
  EXPECT_TRUE(sim_.CheckCondition(Condition::kCarryClear));
  set_flags(false, false, false, true);
  EXPECT_TRUE(sim_.CheckCondition(Condition::kCarrySet));

  // NE / EQ
  set_flags(false, false, false, false);
  EXPECT_TRUE(sim_.CheckCondition(Condition::kNotEqual));
  set_flags(false, true, false, false);
  EXPECT_TRUE(sim_.CheckCondition(Condition::kEqual));

  // VC / VS
  set_flags(false, false, false, false);
  EXPECT_TRUE(sim_.CheckCondition(Condition::kOverflowClear));
  set_flags(false, false, true, false);
  EXPECT_TRUE(sim_.CheckCondition(Condition::kOverflowSet));

  // PL / MI
  set_flags(false, false, false, false);
  EXPECT_TRUE(sim_.CheckCondition(Condition::kPlus));
  set_flags(true, false, false, false);
  EXPECT_TRUE(sim_.CheckCondition(Condition::kMinus));

  // GE: N==V  (both clear or both set)
  set_flags(false, false, false, false);
  EXPECT_TRUE(sim_.CheckCondition(Condition::kGreaterOrEqual));
  set_flags(true, false, true, false);
  EXPECT_TRUE(sim_.CheckCondition(Condition::kGreaterOrEqual));
  set_flags(true, false, false, false);
  EXPECT_FALSE(sim_.CheckCondition(Condition::kGreaterOrEqual));

  // LT: N!=V
  set_flags(true, false, false, false);
  EXPECT_TRUE(sim_.CheckCondition(Condition::kLessThan));
  set_flags(false, false, true, false);
  EXPECT_TRUE(sim_.CheckCondition(Condition::kLessThan));
  set_flags(false, false, false, false);
  EXPECT_FALSE(sim_.CheckCondition(Condition::kLessThan));

  // GT: N==V && !Z
  set_flags(false, false, false, false);
  EXPECT_TRUE(sim_.CheckCondition(Condition::kGreaterThan));
  set_flags(false, true, false, false);
  EXPECT_FALSE(sim_.CheckCondition(Condition::kGreaterThan));

  // LE: Z || N!=V
  set_flags(false, true, false, false);
  EXPECT_TRUE(sim_.CheckCondition(Condition::kLessOrEqual));
  set_flags(true, false, false, false);
  EXPECT_TRUE(sim_.CheckCondition(Condition::kLessOrEqual));
  set_flags(false, false, false, false);
  EXPECT_FALSE(sim_.CheckCondition(Condition::kLessOrEqual));
}

// ---------------------------------------------------------------------------
// 12. HandleExceptionStackFrame
// ---------------------------------------------------------------------------

TEST_F(SimTest, HandleExceptionStackFrame) {
  ResetWithVectors(0x3FFE, 0x1000);
  // Set up: SSP and handler address in vector table
  uint32_t initial_ssp = 0x3FFE;
  uint32_t handler_pc = 0x5000;
  sim_.State().ssp = initial_ssp;
  // Illegal-instruction vector is at address 4 * 4 = 0x10
  sim_.GetMemory().WriteLong(0x10, handler_pc);
  // Unimplemented opcode at PC
  uint16_t saved_sr = sim_.State().sr;
  sim_.GetMemory().WriteWord(0x1000, 0x4AFC);

  sim_.Step();

  // SSP decreased by 6 (4 for PC + 2 for SR)
  uint32_t new_ssp = initial_ssp - 6;
  EXPECT_EQ(sim_.State().ssp, new_ssp);
  // SR pushed at new_ssp (word)
  EXPECT_EQ(sim_.GetMemory().ReadWord(new_ssp).value, saved_sr);
  // inst_pc (0x1000) pushed above SR
  EXPECT_EQ(sim_.GetMemory().ReadLong(new_ssp + 2).value, 0x1000u);
  // PC redirected to handler
  EXPECT_EQ(sim_.State().pc, handler_pc);
}

// ---------------------------------------------------------------------------
// 13. HandleExceptionSupervisorSwitch
// ---------------------------------------------------------------------------

TEST_F(SimTest, HandleExceptionSupervisorSwitch) {
  ResetWithVectors(0x3FFE, 0x1000);
  // Start in user mode
  sim_.State().SetSupervisor(false);
  sim_.State().sr |= kSrTrace;                 // also set trace bit
  sim_.State().ssp = 0x3FFE;                   // set SSP directly even in user mode
  sim_.GetMemory().WriteLong(0x10, 0x5000);    // illegal inst vector
  sim_.GetMemory().WriteWord(0x1000, 0x4AFC);  // unimplemented opcode

  sim_.Step();

  EXPECT_TRUE(sim_.State().IsSupervisor());
  EXPECT_FALSE(sim_.State().GetFlag(kSrTrace));
}

// ===========================================================================
// Task 9.3 — SIMHALT configuration and exception toggle
// ===========================================================================

class SimConfigTest : public ::testing::Test {
 protected:
  static constexpr uint32_t kStack = 0x8000;
  static constexpr uint32_t kCode = 0x1000;
  static constexpr uint32_t kHandler = 0x3000;

  // Clears memory, installs reset vectors and the given program, and resets.
  static void Load(M68kSimulator& sim, std::initializer_list<uint16_t> words) {
    Memory& m = sim.GetMemory();
    m.Clear();
    m.WriteLong(0x0, kStack);
    m.WriteLong(0x4, kCode);
    uint32_t addr = kCode;
    for (uint16_t w : words) {
      m.WriteWord(addr, w);
      addr += 2;
    }
    sim.ClearPendingIrqs();
    sim.Reset();
  }

  static M68kSimulator::Config Cfg(bool simhalt, bool exceptions) {
    M68kSimulator::Config c;
    c.simhalt_enabled = simhalt;
    c.exceptions_enabled = exceptions;
    return c;
  }
};

TEST_F(SimConfigTest, ConfigDefaultsBothEnabled) {
  M68kSimulator::Config c;
  EXPECT_TRUE(c.simhalt_enabled);
  EXPECT_TRUE(c.exceptions_enabled);
  M68kSimulator sim{c};
  EXPECT_TRUE(sim.IsSimhaltEnabled());
  EXPECT_TRUE(sim.IsExceptionsEnabled());
}

TEST_F(SimConfigTest, SimhaltEnabledHalts) {
  M68kSimulator sim{Cfg(true, true)};
  Load(sim, {0xFFFF, 0xFFFF});
  EXPECT_EQ(sim.Step(), SimResult::kHalted);
  EXPECT_TRUE(sim.State().halted);
}

TEST_F(SimConfigTest, SimhaltDisabledReturnsLineF) {
  M68kSimulator sim{Cfg(false, true)};
  Load(sim, {0xFFFF, 0xFFFF});
  sim.GetMemory().WriteLong(0x2C, kHandler);
  EXPECT_EQ(sim.Step(), SimResult::kLine1111);
}

TEST_F(SimConfigTest, SimhaltDisabledTriggersException) {
  M68kSimulator sim{Cfg(false, true)};
  Load(sim, {0xFFFF, 0xFFFF});
  sim.GetMemory().WriteLong(0x2C, kHandler);
  sim.Step();
  EXPECT_FALSE(sim.State().halted);
  EXPECT_EQ(sim.State().pc, kHandler);
  EXPECT_EQ(sim.State().ssp, kStack - 6);  // class 2 frame: SR + PC
  EXPECT_EQ(sim.State().cycles, 34u);
}

TEST_F(SimConfigTest, SimhaltToggleAtRuntime) {
  M68kSimulator sim{Cfg(true, true)};
  Load(sim, {0xFFFF, 0xFFFF});
  sim.GetMemory().WriteLong(0x2C, kHandler);
  sim.SetSimhaltEnabled(false);
  EXPECT_FALSE(sim.IsSimhaltEnabled());
  EXPECT_EQ(sim.Step(), SimResult::kLine1111);

  Load(sim, {0xFFFF, 0xFFFF});
  sim.SetSimhaltEnabled(true);
  EXPECT_EQ(sim.Step(), SimResult::kHalted);
}

TEST_F(SimConfigTest, ExceptionsEnabledProcessesIllegal) {
  M68kSimulator sim{Cfg(true, true)};
  Load(sim, {0x4AFC});  // ILLEGAL
  sim.GetMemory().WriteLong(0x10, kHandler);
  EXPECT_EQ(sim.Step(), SimResult::kBadInstruction);
  EXPECT_EQ(sim.State().pc, kHandler);
  EXPECT_FALSE(sim.State().halted);
}

TEST_F(SimConfigTest, ExceptionsDisabledHaltsOnIllegal) {
  M68kSimulator sim{Cfg(true, false)};
  Load(sim, {0x4AFC});  // ILLEGAL
  sim.GetMemory().WriteLong(0x10, kHandler);
  EXPECT_EQ(sim.Step(), SimResult::kBadInstruction);
  EXPECT_TRUE(sim.State().halted);      // RUN.CPP: "Execution halted"
  EXPECT_NE(sim.State().pc, kHandler);  // not vectored
  EXPECT_EQ(sim.State().ssp, kStack);   // no stack frame
  EXPECT_EQ(sim.State().cycles, 0u);    // no exception cycles
}

TEST_F(SimConfigTest, ExceptionsDisabledSkipPrivilege) {
  M68kSimulator sim{Cfg(true, false)};
  Load(sim, {0x46FC, 0x2700});  // MOVE #$2700,SR
  sim.State().sr = 0x0000;      // user mode
  sim.GetMemory().WriteLong(0x20, kHandler);
  EXPECT_EQ(sim.Step(), SimResult::kPrivilegeViolation);
  EXPECT_TRUE(sim.State().halted);
  EXPECT_NE(sim.State().pc, kHandler);
  EXPECT_FALSE(sim.State().IsSupervisor());
}

TEST_F(SimConfigTest, ExceptionsDisabledIgnoresTrace) {
  M68kSimulator sim{Cfg(true, false)};
  Load(sim, {0x4E71});  // NOP
  sim.State().SetFlag(kSrTrace, true);
  sim.GetMemory().WriteLong(0x24, kHandler);
  EXPECT_EQ(sim.Step(), SimResult::kOk);
  EXPECT_EQ(sim.State().pc, kCode + 2);
  EXPECT_FALSE(sim.State().halted);
}

TEST_F(SimConfigTest, ExceptionsDisabledIgnoresInterrupts) {
  M68kSimulator sim{Cfg(true, false)};
  Load(sim, {0x4E71});
  sim.GetMemory().WriteLong((24 + 7) * 4, kHandler);
  sim.Interrupt(7);
  EXPECT_EQ(sim.Step(), SimResult::kOk);
  EXPECT_EQ(sim.State().pc, kCode + 2);      // NOP executed, IRQ not serviced
  EXPECT_EQ(sim.GetPendingIrqs(), 1u << 6);  // still pending
}

TEST_F(SimConfigTest, Trap15Task32Sub5ReenablesExceptionsHeadless) {
  M68kSimulator sim{Cfg(true, false)};  // no sim_env configured
  Load(sim, {0x4E4F});                  // TRAP #15
  sim.State().d[0] = 32;
  sim.State().d[1] = 5;
  EXPECT_EQ(sim.Step(), SimResult::kOk);
  EXPECT_TRUE(sim.IsExceptionsEnabled());
}

}  // namespace
}  // namespace easym68k::sim
