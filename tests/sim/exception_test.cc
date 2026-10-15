// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Exception processing (Task 11.1). Frames, stacked PCs and cycle counts
// follow RUN.CPP exec_inst()/exceptionHandler(); the common cases are also
// compared with the original Sim68K core by the golden trace `exceptions`
// (data/golden/sim/src/exceptions.x68).

#include <gtest/gtest.h>

#include <initializer_list>

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {
namespace {

constexpr uint32_t kCode = 0x1000;
constexpr uint32_t kHandler = 0x4000;  // every vector points here
constexpr uint32_t kSsp = 0x8000;

class ExceptionTest : public ::testing::Test {
 protected:
  ExceptionTest() : sim_(Config()) {}

  static M68kSimulator::Config Config() {
    M68kSimulator::Config c;
    c.exceptions_enabled = true;
    return c;
  }

  void Load(std::initializer_list<uint16_t> words, uint16_t sr = 0x2000) {
    Memory& m = sim_.GetMemory();
    m.Erase();
    for (uint32_t v = 2; v < 64; v++)
      m.WriteLong(v * 4, kHandler);
    m.WriteWord(kHandler, 0x4E71);  // NOP
    uint32_t a = kCode;
    for (uint16_t w : words) {
      m.WriteWord(a, w);
      a += 2;
    }
    sim_.SetStartAddress(kCode);
    sim_.InitProgram();
    sim_.State().ssp = kSsp;
    sim_.State().sr = sr;
  }

  // Group 1/2 frame: SR at SSP, PC at SSP+2.
  uint16_t FrameSr() { return Word(sim_.State().ssp); }
  uint32_t FramePc() { return Long(sim_.State().ssp + 2); }
  uint16_t Word(uint32_t a) { return static_cast<uint16_t>(sim_.GetMemory().ReadWord(a).value); }
  uint32_t Long(uint32_t a) { return sim_.GetMemory().ReadLong(a).value; }

  M68kSimulator sim_;
};

// 1. Bus error: access to an Invalid memory-map range (the original has no
// unmapped memory; bus errors come from the memory map).
TEST_F(ExceptionTest, BusErrorOnInvalidMapRead) {
  Load({0x3010});  // MOVE.W (A0),D0
  M68kSimulator::MemoryMapConfig map;
  map.invalid = {true, 0x5000, 0x5FFF};
  sim_.ConfigureMemoryMap(map);
  sim_.State().a[0] = 0x5000;
  EXPECT_EQ(sim_.Step(), SimResult::kBusError);
  EXPECT_EQ(sim_.State().pc, kHandler);
  EXPECT_EQ(sim_.State().ssp, kSsp - 14);          // class 0 frame
  EXPECT_EQ(Long(sim_.State().ssp + 2), 0x5000u);  // fault address
  EXPECT_EQ(sim_.State().cycles, 50u);
}

// 2. Address error: word read at an odd address.
TEST_F(ExceptionTest, AddressErrorOnOddWordRead) {
  Load({0x3010});
  sim_.State().a[0] = 0x3001;
  EXPECT_EQ(sim_.Step(), SimResult::kAddressError);
  EXPECT_EQ(sim_.State().ssp, kSsp - 14);
  EXPECT_EQ(Word(sim_.State().ssp), 0x15);            // read, supervisor data
  EXPECT_EQ(Long(sim_.State().ssp + 2), 0x3001u);     // fault address
  EXPECT_EQ(Word(sim_.State().ssp + 6), 0x3010);      // instruction register
  EXPECT_EQ(Long(sim_.State().ssp + 10), kCode + 2);  // PC at the read
}

// 3. Privilege violation: MOVE to SR in user mode; stacks the address of the
// instruction (the original stacked the PC after its first word; bug, fixed).
TEST_F(ExceptionTest, PrivilegeViolation) {
  Load({0x46FC, 0x2700}, 0x0000);  // MOVE #$2700,SR in user mode
  EXPECT_EQ(sim_.Step(), SimResult::kPrivilegeViolation);
  EXPECT_EQ(sim_.State().pc, kHandler);
  EXPECT_TRUE(sim_.State().IsSupervisor());
  EXPECT_EQ(FrameSr(), 0x0000);
  EXPECT_EQ(FramePc(), kCode);
  EXPECT_EQ(sim_.State().cycles, 34u);
}

// 4. Illegal instruction: stacks the address of the instruction.
TEST_F(ExceptionTest, IllegalInstruction) {
  Load({0x4AFC});
  EXPECT_EQ(sim_.Step(), SimResult::kBadInstruction);
  EXPECT_EQ(FramePc(), kCode);
  EXPECT_EQ(sim_.State().cycles, 34u);
}

// 5. Divide by zero.
TEST_F(ExceptionTest, DivideByZero) {
  Load({0x80C1});  // DIVU D1,D0
  sim_.State().d[0] = 100;
  sim_.State().d[1] = 0;
  EXPECT_EQ(sim_.Step(), SimResult::kDivideByZero);
  EXPECT_EQ(FramePc(), kCode + 2);
  EXPECT_EQ(sim_.State().d[0], 100u);
}

// 6. CHK: Dn < 0 (N set) and Dn > bound (N clear).
TEST_F(ExceptionTest, ChkBelowZeroAndAboveBound) {
  Load({0x4581});  // CHK D1,D2
  sim_.State().d[1] = 10;
  sim_.State().d[2] = 0xFFFF;  // -1
  EXPECT_EQ(sim_.Step(), SimResult::kChkException);
  EXPECT_EQ(FramePc(), kCode + 2);
  EXPECT_TRUE(FrameSr() & kSrNegative);
  Load({0x4581});
  sim_.State().d[1] = 10;
  sim_.State().d[2] = 11;
  EXPECT_EQ(sim_.Step(), SimResult::kChkException);
  EXPECT_FALSE(FrameSr() & kSrNegative);
  Load({0x4581});
  sim_.State().d[1] = 10;
  sim_.State().d[2] = 10;  // in bounds
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
}

// 7. TRAPV traps only with V set.
TEST_F(ExceptionTest, TrapvWithOverflow) {
  Load({0x4E76}, 0x2000);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  Load({0x4E76}, 0x2002);
  EXPECT_EQ(sim_.Step(), SimResult::kTrapVException);
  EXPECT_EQ(FramePc(), kCode + 2);
}

// 8. Trace: with T set, the instruction completes and then traps; the frame
// holds the next PC and the SR with T, and T is cleared in the handler.
TEST_F(ExceptionTest, TraceAfterInstruction) {
  Load({0x4E71, 0x4E71}, 0xA000);  // NOP, T=1 S=1
  EXPECT_EQ(sim_.Step(), SimResult::kTraceException);
  EXPECT_EQ(sim_.State().pc, kHandler);
  EXPECT_EQ(FramePc(), kCode + 2);
  EXPECT_EQ(FrameSr(), 0xA000);
  EXPECT_FALSE(sim_.State().sr & kSrTrace);
}

// 9-10. Line 1010 / Line 1111 emulator exceptions.
TEST_F(ExceptionTest, LineAAndLineF) {
  Load({0xA123});
  EXPECT_EQ(sim_.Step(), SimResult::kLine1010);
  EXPECT_EQ(sim_.State().pc, kHandler);
  Load({0xF123});
  EXPECT_EQ(sim_.Step(), SimResult::kLine1111);
  EXPECT_EQ(sim_.State().pc, kHandler);
}

// 11. Frame order: PC pushed first (higher address), then SR.
TEST_F(ExceptionTest, FrameOrderPcThenSr) {
  Load({0x4E43}, 0x2715);  // TRAP #3
  EXPECT_EQ(sim_.Step(), SimResult::kTrapException);
  EXPECT_EQ(sim_.State().ssp, kSsp - 6);
  EXPECT_EQ(Word(kSsp - 6), 0x2715);
  EXPECT_EQ(Long(kSsp - 4), kCode + 2);
  EXPECT_EQ(sim_.State().cycles, 38u);
}

// 12. RTE restores SR and PC.
TEST_F(ExceptionTest, RteRestoresSrAndPc) {
  Load({0x4E43}, 0x2715);
  sim_.GetMemory().WriteWord(kHandler, 0x4E73);  // RTE
  sim_.Step();
  sim_.State().sr = 0x2700;
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().sr, 0x2715);
  EXPECT_EQ(sim_.State().pc, kCode + 2);
  EXPECT_EQ(sim_.State().ssp, kSsp);
}

// 13. A fault while the previous instruction's fault is being handled (here:
// the handler itself faults) halts the simulator.
TEST_F(ExceptionTest, DoubleFaultHalts) {
  Load({0x3010});
  sim_.State().a[0] = 0x3001;
  sim_.GetMemory().WriteWord(kHandler, 0x3010);  // handler faults too
  EXPECT_EQ(sim_.Step(), SimResult::kAddressError);
  EXPECT_FALSE(sim_.State().halted);
  sim_.Step();
  EXPECT_TRUE(sim_.State().halted);
}

// With exceptions disabled every exception condition halts without a frame.
TEST_F(ExceptionTest, ExceptionsDisabledHalt) {
  M68kSimulator::Config config;
  config.exceptions_enabled = false;
  M68kSimulator off{config};
  off.GetMemory().Erase();
  off.GetMemory().WriteWord(kCode, 0x4AFC);
  off.SetStartAddress(kCode);
  off.InitProgram();
  off.Step();
  EXPECT_TRUE(off.State().halted);
  EXPECT_EQ(off.State().ssp, 0x01000000u);
}

}  // namespace
}  // namespace easym68k::sim
