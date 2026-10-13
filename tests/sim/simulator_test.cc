// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/sim/simulator.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <initializer_list>
#include <string>

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

// ===========================================================================
// Task 9.11 — Program init/rewind and hardware reset
// ===========================================================================

// Builds an S1/S9 record with a correct checksum.
std::string SRec(char type, uint16_t addr, std::initializer_list<uint8_t> data) {
  uint8_t count = static_cast<uint8_t>(2 + data.size() + 1);
  uint32_t sum = count + (addr >> 8) + (addr & 0xFF);
  char buf[8];
  std::string s = "S";
  s += type;
  std::snprintf(buf, sizeof(buf), "%02X%04X", count, addr);
  s += buf;
  for (uint8_t b : data) {
    std::snprintf(buf, sizeof(buf), "%02X", b);
    s += buf;
    sum += b;
  }
  std::snprintf(buf, sizeof(buf), "%02X", static_cast<uint8_t>(~sum & 0xFF));
  s += buf;
  return s;
}

class ProgramInitTest : public ::testing::Test {
 protected:
  ProgramInitTest() : sim_(M68kSimulator::Config{}) {}
  M68kSimulator sim_;
};

TEST_F(ProgramInitTest, InitProgramMatchesOriginalInitSim) {
  sim_.SetStartAddress(0x1000);
  sim_.State().d[3] = 0xDEADBEEF;
  sim_.State().a[2] = 0x1234;
  sim_.State().cycles = 999;
  sim_.InitProgram();
  const CpuState& s = sim_.State();
  for (int i = 0; i < 8; ++i)
    EXPECT_EQ(s.d[i], 0u) << "D" << i;
  for (int i = 0; i < 7; ++i)
    EXPECT_EQ(s.a[i], 0u) << "A" << i;
  EXPECT_EQ(s.a[7], 0x00FF0000u);  // USP
  EXPECT_EQ(s.ssp, 0x01000000u);   // SSP
  EXPECT_EQ(s.sr, 0x2000);
  EXPECT_EQ(s.pc, 0x1000u);
  EXPECT_EQ(s.cycles, 0u);
  EXPECT_FALSE(s.halted);
  EXPECT_FALSE(s.stopped);
}

TEST_F(ProgramInitTest, InitProgramDoesNotReadVectors) {
  sim_.GetMemory().WriteLong(0x0, 0x00004000);
  sim_.GetMemory().WriteLong(0x4, 0x00005000);
  sim_.SetStartAddress(0x1000);
  sim_.InitProgram();
  EXPECT_EQ(sim_.State().pc, 0x1000u);
  EXPECT_EQ(sim_.State().ssp, 0x01000000u);
}

TEST_F(ProgramInitTest, InitProgramUsesSRecordStartAddress) {
  // MOVEQ #5,D0 at $1200, start address $1200 in the S9 record.
  ASSERT_TRUE(sim_.LoadSRecordLines({SRec('1', 0x1200, {0x70, 0x05}), SRec('9', 0x1200, {})}));
  EXPECT_EQ(sim_.GetStartAddress(), 0x1200u);
  sim_.InitProgram();
  EXPECT_EQ(sim_.State().pc, 0x1200u);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 5u);
}

TEST_F(ProgramInitTest, InitProgramClearsHaltAndPendingInterrupts) {
  sim_.State().halted = true;
  sim_.State().stopped = true;
  sim_.Interrupt(3);
  sim_.InitProgram();
  EXPECT_FALSE(sim_.State().halted);
  EXPECT_FALSE(sim_.State().stopped);
  EXPECT_EQ(sim_.GetPendingIrqs(), 0u);
}

TEST_F(ProgramInitTest, PushOnInitialSspWrapsInto24BitSpace) {
  // SSP = $01000000; the first push lands at $00FFFFFC (24-bit address bus).
  sim_.SetStartAddress(0x1000);
  sim_.GetMemory().WriteWord(0x1000, 0x4E40);  // TRAP #0
  sim_.GetMemory().WriteLong(0x80, 0x3000);
  sim_.InitProgram();
  sim_.Step();
  EXPECT_EQ(sim_.State().pc, 0x3000u);
  EXPECT_EQ(sim_.GetMemory().ReadLong(0x00FFFFFC).value, 0x1002u);  // stacked return PC
}

TEST_F(ProgramInitTest, HardwareResetLoadsVectorsKeepsRegisters) {
  sim_.GetMemory().WriteLong(0x0, 0x00004000);
  sim_.GetMemory().WriteLong(0x4, 0x00005000);
  sim_.InitProgram();
  sim_.State().sr = 0x8005;  // user mode, trace on, mask 0, C/Z flags
  sim_.State().d[1] = 0x77;
  sim_.HardwareReset();
  const CpuState& s = sim_.State();
  EXPECT_TRUE(s.IsSupervisor());
  EXPECT_FALSE(s.GetFlag(kSrTrace));
  EXPECT_EQ(s.GetInterruptMask(), 7);
  EXPECT_EQ(s.sr & 0x1F, 0x05);  // CCR kept
  EXPECT_EQ(s.ssp, 0x4000u);
  EXPECT_EQ(s.pc, 0x5000u);
  EXPECT_EQ(s.d[1], 0x77u);
  EXPECT_EQ(s.cycles, 40u);
}

TEST_F(ProgramInitTest, HardwareResetWakesFromStop) {
  sim_.SetStartAddress(0x1000);
  sim_.GetMemory().WriteWord(0x1000, 0x4E72);  // STOP #$2000
  sim_.GetMemory().WriteWord(0x1002, 0x2000);
  sim_.GetMemory().WriteLong(0x4, 0x2000);
  sim_.GetMemory().WriteWord(0x2000, 0x4E71);
  sim_.InitProgram();
  sim_.Step();
  ASSERT_TRUE(sim_.State().stopped);
  sim_.HardwareReset();
  EXPECT_FALSE(sim_.State().stopped);
  EXPECT_FALSE(sim_.State().halted);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x2002u);
}

}  // namespace
}  // namespace easym68k::sim
