// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Regression tests for simulator differences found by the reference oracle
// (tools/reference/compare_sim_corpus.sh, Task 10.2). Each test names the
// corpus case that exposed it; the expected values are the oracle's (the
// original Sim68K core), except where data/golden/sim/deviations.tsv
// documents an original bug the port fixes.

#include <gtest/gtest.h>

#include <cstdint>
#include <initializer_list>

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {
namespace {

constexpr uint32_t kCode = 0x1000;
constexpr uint32_t kAddrHandler = 0x3100;
constexpr uint32_t kInitialSsp = 0x01000000;

class OracleRegressionTest : public ::testing::Test {
 protected:
  OracleRegressionTest() : sim_(M68kSimulator::Config{}) {}

  void Load(std::initializer_list<uint16_t> words) {
    Memory& m = sim_.GetMemory();
    m.Erase();
    m.WriteLong(0x0C, kAddrHandler);
    uint32_t addr = kCode;
    for (uint16_t w : words) {
      m.WriteWord(addr, w);
      addr += 2;
    }
    m.WriteWord(kAddrHandler, 0x4E71);
    sim_.SetStartAddress(kCode);
    sim_.InitProgram();  // SR=$2000, SSP=$01000000
  }

  uint16_t FrameInfo() { return sim_.GetMemory().ReadWord(sim_.State().ssp).value & 0xFFFF; }
  uint32_t FramePc() { return sim_.GetMemory().ReadLong(sim_.State().ssp + 10).value; }

  M68kSimulator sim_;
};

// NEGX: C = X = Dm | Rm. 0 - $FFFF - X(1) = 0 borrows (corpus NEGX
// $12(A0,A1.L)); the port computed C from `result != 0`.
TEST_F(OracleRegressionTest, NegxBorrowWithZeroResult) {
  Load({0x4041});  // NEGX.W D1
  sim_.State().d[1] = 0x0000FFFF;
  sim_.State().sr = 0x2010;  // X=1, Z=0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[1] & 0xFFFF, 0u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrCarry));
  EXPECT_TRUE(sim_.State().GetFlag(kSrExtend));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));  // Z unchanged when result is 0
}

// CHK: N is set when Dn < 0 (the exception case); the port left N unchanged.
TEST_F(OracleRegressionTest, ChkSetsNegativeForNegativeDn) {
  Load({0x4381});  // CHK D1,D1
  sim_.GetMemory().WriteLong(0x18, 0x3300);
  sim_.State().d[1] = 0x0000FFFE;  // -2
  sim_.Step();
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(OracleRegressionTest, ChkClearsNegativeForDnAboveBound) {
  Load({0x4380});  // CHK D0,D1
  sim_.GetMemory().WriteLong(0x18, 0x3300);
  sim_.State().d[0] = 5;
  sim_.State().d[1] = 9;
  sim_.State().sr = 0x2008;  // N set before
  sim_.Step();
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
}

// CLR, Scc and MOVE from SR read the destination before writing it (68000
// read-modify-write; the original via eff_addr()), so an odd address faults
// as a read with the PC of the read (corpus CLR (A2,D3), MOVE SR,$10(A6,D7.W)).
TEST_F(OracleRegressionTest, ClrOddAddressFaultsAsRead) {
  Load({0x4250});  // CLR.W (A0)
  sim_.State().a[0] = 0x2001;
  EXPECT_EQ(sim_.Step(), SimResult::kAddressError);
  EXPECT_EQ(FrameInfo(), 0x15);  // read, supervisor data
  EXPECT_EQ(FramePc(), kCode + 2);
}

TEST_F(OracleRegressionTest, MoveFromSrOddAddressFaultsAsRead) {
  Load({0x40E8, 0x0010});  // MOVE SR,$10(A0)
  sim_.State().a[0] = 0x2001;
  EXPECT_EQ(sim_.Step(), SimResult::kAddressError);
  EXPECT_EQ(FrameInfo(), 0x15);
  EXPECT_EQ(FramePc(), kCode + 4);
}

TEST_F(OracleRegressionTest, SccReadsDestinationFirst) {
  Load({0x50D0});  // ST (A0)
  sim_.State().a[0] = 0x2000;
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.GetMemory().GetAccessTracking().read);
  EXPECT_EQ(sim_.GetMemory().GetAccessTracking().read_address, 0x2000u);
  EXPECT_EQ(sim_.GetMemory().ReadByte(0x2000).value, 0xFFu);
}

// A faulting source must not let the instruction store its result (corpus
// MOVE (A3),-(A5) with odd A3: the port wrote 0 to -(A5)).
TEST_F(OracleRegressionTest, FaultingSourceDoesNotWriteDestination) {
  Load({0x3B13});  // MOVE.W (A3),-(A5)
  sim_.State().a[3] = 0x3031;
  sim_.State().a[5] = 0x3050;
  sim_.GetMemory().WriteWord(0x304E, 0x1234);
  EXPECT_EQ(sim_.Step(), SimResult::kAddressError);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x304E).value, 0x1234u);
  EXPECT_EQ(sim_.State().ssp, kInitialSsp - 14);
}

// NBCD: digit-by-digit as the original (identical for valid BCD; the
// original's results for non-BCD digits, corpus NBCD $10(A6) / $2F00).
TEST_F(OracleRegressionTest, NbcdNonBcdDigits) {
  struct Case {
    uint8_t in;
    bool x;
    uint8_t out;
    bool carry;
  } cases[] = {{0x45, false, 0x55, true}, {0x00, false, 0x00, false}, {0x00, true, 0x99, true},
               {0x4A, true, 0x4F, true},  {0xFF, true, 0x9A, true},   {0x0A, false, 0x90, true}};
  for (const Case& c : cases) {
    Load({0x4800});  // NBCD D0
    sim_.State().d[0] = c.in;
    sim_.State().sr = c.x ? 0x2014 : 0x2004;  // Z set
    EXPECT_EQ(sim_.Step(), SimResult::kOk);
    EXPECT_EQ(sim_.State().d[0] & 0xFF, c.out) << std::hex << int(c.in);
    EXPECT_EQ(sim_.State().GetFlag(kSrCarry), c.carry) << std::hex << int(c.in);
    EXPECT_EQ(sim_.State().GetFlag(kSrExtend), c.carry) << std::hex << int(c.in);
  }
}

// The PC is a 32-bit register (Task 10.5): RTS/RTR/RTE, branches and
// exception vectors keep the upper byte, instruction fetches use the low 24
// bits; JMP/JSR mask their target as the original's eff_addr() does.
TEST_F(OracleRegressionTest, RtsKeepsUpperPcByte) {
  Load({0x4E75});  // RTS
  sim_.State().ssp = 0x8000;
  sim_.GetMemory().WriteLong(0x8000, 0xFF002000);
  sim_.GetMemory().WriteWord(0x2000, 0x7005);  // MOVEQ #5,D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0xFF002000u);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);  // fetched from $002000
  EXPECT_EQ(sim_.State().d[0], 5u);
  EXPECT_EQ(sim_.State().pc, 0xFF002002u);
}

TEST_F(OracleRegressionTest, JmpMasksTarget) {
  Load({0x4EF9, 0xFF00, 0x2000});  // JMP $FF002000
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x002000u);
}

TEST_F(OracleRegressionTest, BreakpointMatchesLow24Bits) {
  Load({0x4E75});  // RTS to $FF002000
  sim_.State().ssp = 0x8000;
  sim_.GetMemory().WriteLong(0x8000, 0xFF002000);
  sim_.GetMemory().WriteWord(0x2000, 0x4E71);
  sim_.AddBreakpoint(0x2000);
  sim_.Run(10);
  EXPECT_EQ(sim_.State().pc, 0xFF002000u);
}

TEST_F(OracleRegressionTest, OddUpperPcFaultsAtFetch) {
  Load({0x4E75});  // RTS to $FFFFFFFF: address error on the next fetch
  sim_.State().ssp = 0x8000;
  sim_.GetMemory().WriteLong(0x8000, 0xFFFFFFFF);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0xFFFFFFFFu);
  EXPECT_EQ(sim_.Step(), SimResult::kAddressError);
  EXPECT_EQ(sim_.State().pc, kAddrHandler);
  // fault address in the frame keeps all 32 bits, as in the original
  EXPECT_EQ(sim_.GetMemory().ReadLong(sim_.State().ssp + 2).value, 0xFFFFFFFFu);
}

}  // namespace
}  // namespace easym68k::sim
