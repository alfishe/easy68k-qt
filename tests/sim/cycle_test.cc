// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Cycle counting tests (Task 9.2). Expected values follow the original
// EASy68K simulator (RUN.CPP eff_addr() timing, CODE1-CODE9.CPP inc_cyc()
// calls), which is the parity target for this port.

#include <gtest/gtest.h>

#include <cstdint>
#include <initializer_list>

#include "easym68k/sim/cycle_data.h"
#include "easym68k/sim/simulator.h"
#include "easym68k/sim/trap15.h"

namespace easym68k::sim {
namespace {

constexpr uint32_t kCodeAddr = 0x1000;
constexpr uint32_t kDataAddr = 0x2000;
constexpr uint32_t kStackTop = 0x8000;

class CycleTest : public ::testing::Test {
 protected:
  static M68kSimulator sim_;

  // Loads program words at kCodeAddr and resets (supervisor mode, SSP=kStackTop).
  void Load(std::initializer_list<uint16_t> words) {
    sim_.GetMemory().Clear();
    sim_.GetMemory().WriteLong(0x0, kStackTop);
    sim_.GetMemory().WriteLong(0x4, kCodeAddr);
    uint32_t addr = kCodeAddr;
    for (uint16_t w : words) {
      sim_.GetMemory().WriteWord(addr, w);
      addr += 2;
    }
    sim_.ClearBreakpoints();
    sim_.ClearPendingIrqs();
    sim_.Reset();
  }

  // Executes one instruction and returns the cycles it added.
  uint64_t StepCycles() {
    uint64_t before = sim_.State().cycles;
    sim_.Step();
    return sim_.State().cycles - before;
  }

  CpuState& S() { return sim_.State(); }
  Memory& M() { return sim_.GetMemory(); }
};

M68kSimulator CycleTest::sim_{M68kSimulator::Config{}};

// ---------------------------------------------------------------------------
// Basic instructions
// ---------------------------------------------------------------------------

TEST_F(CycleTest, NopIs4Cycles) {
  Load({0x4E71});
  EXPECT_EQ(StepCycles(), 4u);
}

TEST_F(CycleTest, ResetStateClearsCycles) {
  Load({0x4E71});
  StepCycles();
  sim_.Reset();
  EXPECT_EQ(S().cycles, 0u);
}

TEST_F(CycleTest, MultipleInstructionsAccumulate) {
  Load({0x4E71, 0x4E71, 0x4E71, 0x4E71, 0x4E71, 0x4E71, 0x4E71, 0x4E71, 0x4E71, 0x4E71});
  for (int i = 0; i < 10; ++i)
    sim_.Step();
  EXPECT_EQ(S().cycles, 40u);
}

TEST_F(CycleTest, MoveqIs4Cycles) {
  Load({0x7005});  // MOVEQ #5,D0
  EXPECT_EQ(StepCycles(), 4u);
}

// ---------------------------------------------------------------------------
// MOVE (CODE1.CPP move_bw_times / move_l_times; no separate EA time)
// ---------------------------------------------------------------------------

TEST_F(CycleTest, MoveRegRegWord) {
  Load({0x3200});  // MOVE.W D0,D1
  EXPECT_EQ(StepCycles(), 4u);
}

TEST_F(CycleTest, MoveRegMemWord) {
  Load({0x3080});  // MOVE.W D0,(A0)
  S().a[0] = kDataAddr;
  EXPECT_EQ(StepCycles(), 8u);  // move_bw_times[Dn][(An)]
}

TEST_F(CycleTest, MoveMemRegLong) {
  Load({0x2010});  // MOVE.L (A0),D0
  S().a[0] = kDataAddr;
  EXPECT_EQ(StepCycles(), 12u);  // move_l_times[(An)][Dn]
}

TEST_F(CycleTest, MoveLongAbsToAbs) {
  Load({0x23F9, 0x0000, 0x2000, 0x0000, 0x3000});  // MOVE.L $2000,$3000
  EXPECT_EQ(StepCycles(), 36u);                    // move_l_times[xxx.L][xxx.L]
}

TEST_F(CycleTest, MoveaWordFromImmediate) {
  Load({0x307C, 0x1234});  // MOVEA.W #$1234,A0
  EXPECT_EQ(StepCycles(), 8u);
}

TEST_F(CycleTest, MovemLongRegsToPreDec) {
  Load({0x48E7, 0xC000});  // MOVEM.L D0-D1,-(A7)
  EXPECT_EQ(StepCycles(), 8u + 2 * 8u);
}

TEST_F(CycleTest, MovemWordPostIncToRegs) {
  Load({0x4C98, 0x0007});  // MOVEM.W (A0)+,D0-D2
  S().a[0] = kDataAddr;
  EXPECT_EQ(StepCycles(), 12u + 3 * 4u);
}

TEST_F(CycleTest, MoveToSrImmediate) {
  Load({0x46FC, 0x2700});  // MOVE #$2700,SR
  EXPECT_EQ(StepCycles(), 4u + 12u);
}

TEST_F(CycleTest, ExgIs6Cycles) {
  Load({0xC141});  // EXG D0,D1
  EXPECT_EQ(StepCycles(), 6u);
}

TEST_F(CycleTest, LinkAndUnlk) {
  Load({0x4E56, 0xFFFC, 0x4E5E});  // LINK A6,#-4 ; UNLK A6
  EXPECT_EQ(StepCycles(), 16u);
  EXPECT_EQ(StepCycles(), 12u);
}

TEST_F(CycleTest, LeaAnIndirectIs4) {
  Load({0x43D0});  // LEA (A0),A1
  EXPECT_EQ(StepCycles(), 4u);
}

TEST_F(CycleTest, LeaDisplacementIs8) {
  Load({0x43E8, 0x0010});  // LEA 16(A0),A1
  EXPECT_EQ(StepCycles(), 8u);
}

TEST_F(CycleTest, PeaAnIndirectIs12) {
  Load({0x4850});  // PEA (A0)
  EXPECT_EQ(StepCycles(), 12u);
}

// ---------------------------------------------------------------------------
// Arithmetic
// ---------------------------------------------------------------------------

TEST_F(CycleTest, AddRegRegByte) {
  Load({0xD200});  // ADD.B D0,D1
  EXPECT_EQ(StepCycles(), 4u);
}

TEST_F(CycleTest, AddRegRegLong) {
  Load({0xD280});  // ADD.L D0,D1
  EXPECT_EQ(StepCycles(), 8u);
}

TEST_F(CycleTest, AddMemToRegLong) {
  Load({0xD290});  // ADD.L (A0),D1
  S().a[0] = kDataAddr;
  EXPECT_EQ(StepCycles(), 8u + 6u);
}

TEST_F(CycleTest, AddRegToMemWord) {
  Load({0xD150});  // ADD.W D0,(A0)
  S().a[0] = kDataAddr;
  EXPECT_EQ(StepCycles(), 4u + 8u);
}

TEST_F(CycleTest, AddImmReg) {
  Load({0x0640, 0x0001});  // ADDI.W #1,D0
  EXPECT_EQ(StepCycles(), 8u);
}

TEST_F(CycleTest, SubImmReg) {
  Load({0x0440, 0x0001});  // SUBI.W #1,D0
  EXPECT_EQ(StepCycles(), 8u);
}

TEST_F(CycleTest, AddqWordDataReg) {
  Load({0x5240});  // ADDQ.W #1,D0
  EXPECT_EQ(StepCycles(), 4u);
}

TEST_F(CycleTest, AddqAddressReg) {
  Load({0x5288});  // ADDQ.L #1,A0
  EXPECT_EQ(StepCycles(), 8u);
}

TEST_F(CycleTest, AddxLongPreDec) {
  Load({0xD388});  // ADDX.L -(A0),-(A1)
  S().a[0] = kDataAddr + 8;
  S().a[1] = kDataAddr + 16;
  EXPECT_EQ(StepCycles(), 30u);
}

TEST_F(CycleTest, AbcdRegIs6) {
  Load({0xC300});  // ABCD D0,D1
  EXPECT_EQ(StepCycles(), 6u);
}

TEST_F(CycleTest, CmpLongImmediate) {
  Load({0xB0BC, 0x0000, 0x0001});  // CMP.L #1,D0
  EXPECT_EQ(StepCycles(), 8u + 6u);
}

TEST_F(CycleTest, ExtIs4) {
  Load({0x4880});  // EXT.W D0
  EXPECT_EQ(StepCycles(), 4u);
}

// MULU/MULS: 38 + 2n (n = ones / bit transitions in source), plus EA time.

TEST_F(CycleTest, MuluZeroSourceIs38) {
  Load({0xC2C0});  // MULU D0,D1
  S().d[0] = 0;
  EXPECT_EQ(StepCycles(), 38u);
}

TEST_F(CycleTest, MuluAllOnesIs70) {
  Load({0xC2C0});  // MULU D0,D1
  S().d[0] = 0xFFFF;
  EXPECT_EQ(StepCycles(), 70u);
}

TEST_F(CycleTest, MuluMemoryAddsEa) {
  Load({0xC2D0});  // MULU (A0),D1
  S().a[0] = kDataAddr;
  M().WriteWord(kDataAddr, 0xFFFF);
  EXPECT_EQ(StepCycles(), 4u + 70u);
}

TEST_F(CycleTest, MulsAlternatingBitsIs70) {
  Load({0xC3C0});  // MULS D0,D1
  S().d[0] = 0x5555;
  EXPECT_EQ(StepCycles(), 70u);
}

TEST_F(CycleTest, MulsZeroSourceIs38) {
  Load({0xC3C0});  // MULS D0,D1
  S().d[0] = 0;
  EXPECT_EQ(StepCycles(), 38u);
}

// DIVU/DIVS: exact data-dependent timing (getDivu68kCycles/getDivs68kCycles).

TEST_F(CycleTest, DivuWorstCaseIs136) {
  Load({0x82C0});  // DIVU D0,D1
  S().d[0] = 1;
  S().d[1] = 0;
  EXPECT_EQ(StepCycles(), 136u);
}

TEST_F(CycleTest, DivuOverflowIs10) {
  Load({0x82C0});  // DIVU D0,D1
  S().d[0] = 1;
  S().d[1] = 0x00010000;
  EXPECT_EQ(StepCycles(), 10u);
}

TEST_F(CycleTest, DivsZeroDividendIs150) {
  Load({0x83C0});  // DIVS D0,D1
  S().d[0] = 1;
  S().d[1] = 0;
  EXPECT_EQ(StepCycles(), 150u);
}

TEST_F(CycleTest, DivsNegativeDividendWorstCaseIs156) {
  Load({0x83C0});  // DIVS D0,D1
  S().d[0] = 1;
  S().d[1] = 0xFFFFFFFF;  // -1
  EXPECT_EQ(StepCycles(), 156u);
}

TEST_F(CycleTest, DivsAbsoluteOverflowIs16) {
  Load({0x83C0});  // DIVS D0,D1
  S().d[0] = 1;
  S().d[1] = 0x00010000;
  EXPECT_EQ(StepCycles(), 16u);
}

TEST_F(CycleTest, DivByZeroAddsExceptionCycles) {
  Load({0x82C0});  // DIVU D0,D1
  M().WriteLong(0x14, 0x3000);
  S().d[0] = 0;
  EXPECT_EQ(StepCycles(), 42u);
}

TEST_F(CycleTest, ChkInBoundsIs10) {
  Load({0x4181});  // CHK D1,D0
  S().d[0] = 5;
  S().d[1] = 10;
  EXPECT_EQ(StepCycles(), 10u);
}

TEST_F(CycleTest, ChkOutOfBoundsIs44) {
  Load({0x4181});  // CHK D1,D0
  M().WriteLong(0x18, 0x3000);
  S().d[0] = 20;
  S().d[1] = 10;
  EXPECT_EQ(StepCycles(), 44u);  // exception only; no +10
}

// ---------------------------------------------------------------------------
// Logic, shifts, bit operations
// ---------------------------------------------------------------------------

TEST_F(CycleTest, AndiToCcrIs20) {
  Load({0x023C, 0x00FF});  // ANDI #$FF,CCR
  EXPECT_EQ(StepCycles(), 20u);
}

TEST_F(CycleTest, NotMemoryLong) {
  Load({0x4690});  // NOT.L (A0)
  S().a[0] = kDataAddr;
  EXPECT_EQ(StepCycles(), 8u + 12u);
}

TEST_F(CycleTest, LslWordImmediateCount) {
  Load({0xE748});  // LSL.W #3,D0
  EXPECT_EQ(StepCycles(), 6u + 2 * 3u);
}

TEST_F(CycleTest, LslLongRegisterCount) {
  Load({0xE3A8});  // LSL.L D1,D0
  S().d[1] = 5;
  EXPECT_EQ(StepCycles(), 8u + 2 * 5u);
}

TEST_F(CycleTest, LslMemory) {
  Load({0xE3D0});  // LSL.W (A0)
  S().a[0] = kDataAddr;
  EXPECT_EQ(StepCycles(), 4u + 8u);
}

TEST_F(CycleTest, BtstStaticDataReg) {
  Load({0x0800, 0x0003});  // BTST #3,D0
  EXPECT_EQ(StepCycles(), 6u);
}

TEST_F(CycleTest, BclrDynamicDataReg) {
  Load({0x0380});  // BCLR D1,D0
  EXPECT_EQ(StepCycles(), 10u);
}

TEST_F(CycleTest, BsetDynamicMemory) {
  Load({0x03D0});  // BSET D1,(A0)
  S().a[0] = kDataAddr;
  EXPECT_EQ(StepCycles(), 4u + 8u + 4u);
}

// ---------------------------------------------------------------------------
// Branches and flow control
// ---------------------------------------------------------------------------

TEST_F(CycleTest, BraIs10) {
  Load({0x6002});  // BRA.S *+4
  EXPECT_EQ(StepCycles(), 10u);
}

TEST_F(CycleTest, BccTakenIs10) {
  Load({0x6702});  // BEQ.S *+4
  S().SetFlag(kSrZero, true);
  EXPECT_EQ(StepCycles(), 10u);
}

TEST_F(CycleTest, BccNotTakenByte) {
  Load({0x6702});  // BEQ.S *+4
  S().SetFlag(kSrZero, false);
  EXPECT_EQ(StepCycles(), 8u);
}

TEST_F(CycleTest, BccNotTakenWord) {
  Load({0x6700, 0x0004});  // BEQ.W *+6
  S().SetFlag(kSrZero, false);
  EXPECT_EQ(StepCycles(), 12u);
}

TEST_F(CycleTest, BsrIs18) {
  Load({0x6102});  // BSR.S *+4
  EXPECT_EQ(StepCycles(), 18u);
}

TEST_F(CycleTest, DbccBranchTakenIs10) {
  Load({0x51C8, 0xFFFE});  // DBRA D0,*
  S().d[0] = 2;
  EXPECT_EQ(StepCycles(), 10u);
}

TEST_F(CycleTest, DbccCounterExpiredIs14) {
  Load({0x51C8, 0xFFFE});  // DBRA D0,*
  S().d[0] = 0;
  EXPECT_EQ(StepCycles(), 14u);
}

TEST_F(CycleTest, DbccConditionTrueIs12) {
  Load({0x57C8, 0xFFFE});  // DBEQ D0,*
  S().SetFlag(kSrZero, true);
  EXPECT_EQ(StepCycles(), 12u);
}

TEST_F(CycleTest, SccDataRegTrueAndFalse) {
  Load({0x50C0, 0x51C0});  // ST D0 ; SF D0
  EXPECT_EQ(StepCycles(), 6u);
  EXPECT_EQ(StepCycles(), 4u);
}

TEST_F(CycleTest, SccMemory) {
  Load({0x50D0});  // ST (A0)
  S().a[0] = kDataAddr;
  EXPECT_EQ(StepCycles(), 4u + 8u);
}

TEST_F(CycleTest, JmpAndJsrAnIndirect) {
  Load({0x4ED0});  // JMP (A0)
  S().a[0] = kCodeAddr;
  EXPECT_EQ(StepCycles(), 8u);
  Load({0x4E90});  // JSR (A0)
  S().a[0] = kCodeAddr;
  EXPECT_EQ(StepCycles(), 16u);
}

TEST_F(CycleTest, RtsIs16Cycles) {
  Load({0x4E75});  // RTS
  S().ssp = kStackTop - 4;
  M().WriteLong(kStackTop - 4, kCodeAddr);
  EXPECT_EQ(StepCycles(), 16u);
}

TEST_F(CycleTest, RteIs20Cycles) {
  Load({0x4E73});  // RTE
  S().ssp = kStackTop - 6;
  M().WriteWord(kStackTop - 6, 0x2700);
  M().WriteLong(kStackTop - 4, kCodeAddr);
  EXPECT_EQ(StepCycles(), 20u);
}

// ---------------------------------------------------------------------------
// Miscellaneous and exceptions
// ---------------------------------------------------------------------------

TEST_F(CycleTest, TrapIs38Cycles) {
  Load({0x4E40});  // TRAP #0
  M().WriteLong(0x80, 0x3000);
  EXPECT_EQ(StepCycles(), 38u);
}

TEST_F(CycleTest, Trap15AddsNoCycles) {
  Load({0x4E4F});               // TRAP #15
  S().d[0] = kTrap15GetCycles;  // task 31: harmless, no I/O interface needed
  EXPECT_EQ(StepCycles(), 0u);
}

TEST_F(CycleTest, TrapvNoOverflowIs4) {
  Load({0x4E76});  // TRAPV
  S().SetFlag(kSrOverflow, false);
  EXPECT_EQ(StepCycles(), 4u);
}

TEST_F(CycleTest, TrapvOverflowIs34) {
  Load({0x4E76});  // TRAPV
  M().WriteLong(0x1C, 0x3000);
  S().SetFlag(kSrOverflow, true);
  EXPECT_EQ(StepCycles(), 34u);
}

TEST_F(CycleTest, ResetIs132Cycles) {
  Load({0x4E70});  // RESET
  EXPECT_EQ(StepCycles(), 132u);
}

TEST_F(CycleTest, StopIs4Cycles) {
  Load({0x4E72, 0x2700});  // STOP #$2700
  EXPECT_EQ(StepCycles(), 4u);
}

TEST_F(CycleTest, IllegalIs34Cycles) {
  Load({0x4AFC});  // ILLEGAL
  M().WriteLong(0x10, 0x3000);
  EXPECT_EQ(StepCycles(), 34u);
}

TEST_F(CycleTest, PrivilegeViolationIs34Cycles) {
  Load({0x4E70});  // RESET in user mode
  S().sr = 0x0000;
  M().WriteLong(0x20, 0x3000);
  EXPECT_EQ(StepCycles(), 34u);
}

TEST_F(CycleTest, InterruptIs34Cycles) {
  Load({0x4E71});
  M().WriteLong((24 + 5) * 4, 0x3000);
  S().SetInterruptMask(0);
  sim_.Interrupt(5);
  EXPECT_EQ(StepCycles(), 34u);
}

// ---------------------------------------------------------------------------
// Trap #15 tasks 30/31
// ---------------------------------------------------------------------------

TEST_F(CycleTest, ResetClearsCycles) {
  Load({0x4E71, 0x4E4F});  // NOP ; TRAP #15
  sim_.Step();
  ASSERT_EQ(S().cycles, 4u);
  S().d[0] = kTrap15ClearCycles;
  sim_.Step();
  EXPECT_EQ(S().cycles, 0u);
}

TEST_F(CycleTest, GetCyclesReturnsCurrent) {
  Load({0x4E71, 0x4E71, 0x4E4F});  // NOP ; NOP ; TRAP #15
  sim_.Step();
  sim_.Step();
  S().d[0] = kTrap15GetCycles;
  sim_.Step();
  EXPECT_EQ(S().d[1], 8u);
}

// ---------------------------------------------------------------------------
// Effective address penalties (measured with TST: EA time + 4)
// ---------------------------------------------------------------------------

TEST_F(CycleTest, EaPenaltyAnIndirect) {
  Load({0x4A50});  // TST.W (A0)
  S().a[0] = kDataAddr;
  EXPECT_EQ(StepCycles(), 4u + 4u);
}

TEST_F(CycleTest, EaPenaltyAnPostInc) {
  Load({0x4A58});  // TST.W (A0)+
  S().a[0] = kDataAddr;
  EXPECT_EQ(StepCycles(), 4u + 4u);
}

TEST_F(CycleTest, EaPenaltyAnPreDec) {
  Load({0x4A60});  // TST.W -(A0)
  S().a[0] = kDataAddr + 2;
  EXPECT_EQ(StepCycles(), 6u + 4u);
}

TEST_F(CycleTest, EaPenaltyAnDisplacement) {
  Load({0x4A68, 0x0010});  // TST.W 16(A0)
  S().a[0] = kDataAddr;
  EXPECT_EQ(StepCycles(), 8u + 4u);
}

TEST_F(CycleTest, EaPenaltyAnIndex) {
  Load({0x4A70, 0x0000});  // TST.W 0(A0,D0.W)
  S().a[0] = kDataAddr;
  EXPECT_EQ(StepCycles(), 10u + 4u);
}

TEST_F(CycleTest, EaPenaltyAbsolute) {
  Load({0x4A78, 0x2000, 0x4A79, 0x0000, 0x2000});  // TST.W $2000.W ; TST.W $2000.L
  EXPECT_EQ(StepCycles(), 8u + 4u);
  EXPECT_EQ(StepCycles(), 12u + 4u);
}

TEST_F(CycleTest, EaPenaltyLongIsLarger) {
  Load({0x4A90});  // TST.L (A0)
  S().a[0] = kDataAddr;
  EXPECT_EQ(StepCycles(), 8u + 4u);
}

TEST_F(CycleTest, EaPenaltyImmediate) {
  Load({0xB07C, 0x0001, 0xB0BC, 0x0000, 0x0001});  // CMP.W #1,D0 ; CMP.L #1,D0
  EXPECT_EQ(StepCycles(), 4u + 4u);
  EXPECT_EQ(StepCycles(), 8u + 6u);
}

// ---------------------------------------------------------------------------
// cycle_data helpers
// ---------------------------------------------------------------------------

TEST(CycleDataTest, EaCyclePenaltyTable) {
  EXPECT_EQ(EaCyclePenalty(0, 0, DataSize::kLong), 0);
  EXPECT_EQ(EaCyclePenalty(1, 0, DataSize::kWord), 0);
  EXPECT_EQ(EaCyclePenalty(2, 0, DataSize::kByte), 4);
  EXPECT_EQ(EaCyclePenalty(2, 0, DataSize::kLong), 8);
  EXPECT_EQ(EaCyclePenalty(4, 0, DataSize::kWord), 6);
  EXPECT_EQ(EaCyclePenalty(4, 0, DataSize::kLong), 10);
  EXPECT_EQ(EaCyclePenalty(7, 1, DataSize::kLong), 16);
  EXPECT_EQ(EaCyclePenalty(7, 3, DataSize::kWord), 10);
  EXPECT_EQ(EaCyclePenalty(7, 4, DataSize::kLong), 8);
}

TEST(CycleDataTest, EaCodeMapping) {
  EXPECT_EQ(EaCode(0, 3), 0);
  EXPECT_EQ(EaCode(6, 1), 6);
  EXPECT_EQ(EaCode(7, 0), 7);
  EXPECT_EQ(EaCode(7, 4), 11);
  EXPECT_EQ(EaCode(7, 5), 12);
}

TEST(CycleDataTest, OutOfRangeCodesReturnZero) {
  EXPECT_EQ(MoveCycles(12, 0, DataSize::kWord), 0);
  EXPECT_EQ(MoveCycles(0, 9, DataSize::kLong), 0);
  EXPECT_EQ(MovemCycles(11, true), 0);
}

TEST(CycleDataTest, DivideByZeroReturnsZero) {
  EXPECT_EQ(DivuCycles(1234, 0), 0);
  EXPECT_EQ(DivsCycles(1234, 0), 0);
}

TEST(CycleDataTest, DivsExtremeOperandsDoNotOverflow) {
  // INT32_MIN / INT16_MIN: absolute overflow path, dividend negative.
  EXPECT_EQ(DivsCycles(INT32_MIN, INT16_MIN), (6 + 1 + 2) * 2);
}

}  // namespace
}  // namespace easym68k::sim
