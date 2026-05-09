// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {
namespace {

// ---------------------------------------------------------------------------
// Fixture — static simulator; reset to SSP=0x3FFE, PC=0x1000 each test.
// ---------------------------------------------------------------------------

class MoveTest : public ::testing::Test {
 protected:
  static M68kSimulator sim_;

  void SetUp() override {
    sim_.GetMemory().Clear();
    sim_.GetMemory().WriteLong(0x0, 0x3FFE);
    sim_.GetMemory().WriteLong(0x4, 0x1000);
    sim_.Reset();
  }

  // Write a sequence of words starting at PC (0x1000).
  void PutWords(std::initializer_list<uint16_t> words) {
    uint32_t addr = 0x1000;
    for (uint16_t w : words) {
      sim_.GetMemory().WriteWord(addr, w);
      addr += 2;
    }
  }
};

M68kSimulator MoveTest::sim_{M68kSimulator::Config{}};

// ===========================================================================
// MOVE.B — group 1
// ===========================================================================

TEST_F(MoveTest, MoveByte_DnToDn) {
  // MOVE.B D1, D0  = 0x1001
  sim_.State().d[1] = 0xAB;
  PutWords({0x1001});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFF, 0xABu);
  EXPECT_EQ(sim_.State().pc, 0x1002u);
}

TEST_F(MoveTest, MoveByte_DnToDn_FlagsNegative) {
  // MOVE.B D1, D0 with D1 = 0x80 → N flag set, Z clear
  sim_.State().d[1] = 0x80;
  PutWords({0x1001});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrOverflow));
  EXPECT_FALSE(sim_.State().GetFlag(kSrCarry));
}

TEST_F(MoveTest, MoveByte_DnToDn_FlagsZero) {
  // MOVE.B D1, D0 with D1 = 0 → Z flag set
  sim_.State().d[1] = 0;
  PutWords({0x1001});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
}

TEST_F(MoveTest, MoveByte_PreservesUpperBitsOfDest) {
  // MOVE.B D1, D0 should only change low byte of D0
  sim_.State().d[0] = 0xDEADBEEF;
  sim_.State().d[1] = 0x42;
  PutWords({0x1001});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0xDEADBE42u);
}

TEST_F(MoveTest, MoveByte_ImmToMem) {
  // MOVE.B #0x55, (A0) = 0x10C0 #0x0055 (word with byte imm in low byte)
  // opcode: 0001 0000 1100 0000 = MOVE.B #n, (A0)
  // src: 7,4 = immediate; dst: 2,0 = (A0)
  sim_.State().a[0] = 0x2000;
  PutWords({0x10BC, 0x0055});  // MOVE.B #$55, (A0)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.GetMemory().ReadByte(0x2000).value, 0x55u);
}

// ===========================================================================
// MOVE.W — group 3
// ===========================================================================

TEST_F(MoveTest, MoveWord_DnToDn) {
  // MOVE.W D2, D0 = 0x3002
  sim_.State().d[2] = 0xBEEF;
  PutWords({0x3002});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0xBEEFu);
  EXPECT_EQ(sim_.State().pc, 0x1002u);
}

TEST_F(MoveTest, MoveWord_PreservesUpper) {
  sim_.State().d[0] = 0x12345678;
  sim_.State().d[2] = 0xABCD;
  PutWords({0x3002});  // MOVE.W D2, D0
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0x1234ABCDu);
}

// ===========================================================================
// MOVE.L — group 2
// ===========================================================================

TEST_F(MoveTest, MoveLong_DnToDn) {
  // MOVE.L D3, D0 = 0x2003
  sim_.State().d[3] = 0xDEADBEEF;
  PutWords({0x2003});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0xDEADBEEFu);
}

TEST_F(MoveTest, MoveLong_ImmToDn) {
  // MOVE.L #0x12345678, D0 = 0x203C 0x1234 0x5678
  PutWords({0x203C, 0x1234, 0x5678});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0x12345678u);
  EXPECT_EQ(sim_.State().pc, 0x1006u);
}

// ===========================================================================
// MOVEA — move to address register
// ===========================================================================

TEST_F(MoveTest, Movea_Word_SignExtend) {
  // MOVEA.W #0x8000, A0 = 0x3040 0x8000 → A0 = 0xFFFF8000 (sign extended)
  PutWords({0x3040, 0x8000});  // src = immediate 0x8000, dst = A0
  // Actually: MOVEA.W D0, A1 would be a more direct test.
  // Let's use MOVEA.W with an immediate source.
  // MOVEA.W #imm, A0: group 3, dst_mode=1 (An), dst_reg=0 → opcode bits: 0011 0000 01 111 100 =
  // 0x307C
  sim_.GetMemory().WriteWord(0x1000, 0x307C);
  sim_.GetMemory().WriteWord(0x1002, 0x8000);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().a[0], 0xFFFF8000u);
}

TEST_F(MoveTest, Movea_Long_NoSignExtend) {
  // MOVEA.L #0x12345678, A1 = 0x227C 0x1234 0x5678
  PutWords({0x227C, 0x1234, 0x5678});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().a[1], 0x12345678u);
  EXPECT_EQ(sim_.State().pc, 0x1006u);
}

TEST_F(MoveTest, Movea_NoFlagsUpdate) {
  // MOVEA must not change condition codes
  sim_.State().sr |= kSrZero | kSrNegative;
  PutWords({0x227C, 0x00, 0x01});  // MOVEA.L #1, A1
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));      // unchanged
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));  // unchanged
}

// ===========================================================================
// MOVEQ — move quick (sign-extended immediate to Dn)
// ===========================================================================

TEST_F(MoveTest, Moveq_Positive) {
  // MOVEQ #66, D0 = 0x7042
  PutWords({0x7042});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 66u);
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
  EXPECT_FALSE(sim_.State().GetFlag(kSrZero));
}

TEST_F(MoveTest, Moveq_Negative) {
  // MOVEQ #-1 = #0xFF, D1 = 0x72FF → D1 = 0xFFFFFFFF
  PutWords({0x72FF});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[1], 0xFFFFFFFFu);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(MoveTest, Moveq_Zero) {
  // MOVEQ #0, D0 = 0x7000
  PutWords({0x7000});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
}

// ===========================================================================
// SWAP
// ===========================================================================

TEST_F(MoveTest, Swap_Basic) {
  // SWAP D0 = 0x4840, D0 = 0x12345678 → 0x56781234
  sim_.State().d[0] = 0x12345678;
  PutWords({0x4840});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0x56781234u);
  EXPECT_EQ(sim_.State().pc, 0x1002u);
}

TEST_F(MoveTest, Swap_FlagsZero) {
  // SWAP D0 with D0=0 → Z flag set
  sim_.State().d[0] = 0;
  PutWords({0x4840});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_TRUE(sim_.State().GetFlag(kSrZero));
  EXPECT_FALSE(sim_.State().GetFlag(kSrNegative));
}

TEST_F(MoveTest, Swap_FlagsNegative) {
  // SWAP D0 with D0=0x0000_8000 → result=0x8000_0000, N set
  sim_.State().d[0] = 0x00008000;
  PutWords({0x4840});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0x80000000u);
  EXPECT_TRUE(sim_.State().GetFlag(kSrNegative));
}

// ===========================================================================
// EXG
// ===========================================================================

TEST_F(MoveTest, Exg_DnDn) {
  // EXG D0, D1 = 1100 0001 0100 0001 = 0xC141
  sim_.State().d[0] = 0xAAAA;
  sim_.State().d[1] = 0xBBBB;
  PutWords({0xC141});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0xBBBBu);
  EXPECT_EQ(sim_.State().d[1], 0xAAAAu);
}

TEST_F(MoveTest, Exg_AnAn) {
  // EXG A0, A1 = 1100 0001 0100 1001 = 0xC149
  sim_.State().a[0] = 0x1111;
  sim_.State().a[1] = 0x2222;
  PutWords({0xC149});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().a[0], 0x2222u);
  EXPECT_EQ(sim_.State().a[1], 0x1111u);
}

TEST_F(MoveTest, Exg_DnAn) {
  // EXG D0, A1 = 0xC189
  sim_.State().d[0] = 0xDEAD;
  sim_.State().a[1] = 0xBEEF;
  PutWords({0xC189});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0xBEEFu);
  EXPECT_EQ(sim_.State().a[1], 0xDEADu);
}

// ===========================================================================
// LEA
// ===========================================================================

TEST_F(MoveTest, Lea_AbsoluteWord) {
  // LEA $2000.W, A0 = 0x41F8 0x2000
  PutWords({0x41F8, 0x2000});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().a[0], 0x2000u);
  EXPECT_EQ(sim_.State().pc, 0x1004u);
}

TEST_F(MoveTest, Lea_AnDisp) {
  // LEA 4(A0), A1 = 0x43E8 0x0004
  sim_.State().a[0] = 0x2000;
  PutWords({0x43E8, 0x0004});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().a[1], 0x2004u);
}

TEST_F(MoveTest, Lea_AbsoluteLong) {
  // LEA $123456.L, A2 = 0x45F9 0x0012 0x3456
  PutWords({0x45F9, 0x0012, 0x3456});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().a[2], 0x123456u);
  EXPECT_EQ(sim_.State().pc, 0x1006u);
}

// ===========================================================================
// PEA
// ===========================================================================

TEST_F(MoveTest, Pea_AbsoluteWord) {
  // PEA $2000.W = 0x4878 0x2000
  // SSP starts at 0x3FFE; PEA pushes 4 bytes → SSP = 0x3FFA
  PutWords({0x4878, 0x2000});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().ssp, 0x3FFAu);
  EXPECT_EQ(sim_.GetMemory().ReadLong(0x3FFA).value, 0x2000u);
}

// ===========================================================================
// LINK / UNLK
// ===========================================================================

TEST_F(MoveTest, Link_Basic) {
  // LINK A6, #-8 = 0x4E56 0xFFF8
  sim_.State().a[6] = 0xDEAD;
  PutWords({0x4E56, 0xFFF8});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  // SSP was 0x3FFE, push A6 → SSP = 0x3FFA, mem[0x3FFA] = 0xDEAD
  EXPECT_EQ(sim_.GetMemory().ReadLong(0x3FFA).value, 0xDEADu);
  // A6 = SSP after push = 0x3FFA
  EXPECT_EQ(sim_.State().a[6], 0x3FFAu);
  // SSP = 0x3FFA + (-8) = 0x3FF2
  EXPECT_EQ(sim_.State().ssp, 0x3FF2u);
}

TEST_F(MoveTest, Unlk_Basic) {
  // Set up: manually place a saved A6 value on the stack and prepare A6.
  sim_.State().a[6] = 0x3FFA;
  sim_.State().ssp = 0x3FF2;
  sim_.GetMemory().WriteLong(0x3FFA, 0xDEAD);

  // UNLK A6 = 0x4E5E
  PutWords({0x4E5E});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  // SSP = A6 = 0x3FFA; then pop into A6 → A6 = 0xDEAD, SSP = 0x3FFE
  EXPECT_EQ(sim_.State().a[6], 0xDEADu);
  EXPECT_EQ(sim_.State().ssp, 0x3FFEu);
}

// ===========================================================================
// MOVE from SR
// ===========================================================================

TEST_F(MoveTest, MoveFrSr_ToDn) {
  // MOVE SR, D0 = 0x40C0
  sim_.State().sr = 0x2704;  // supervisor + some flags
  PutWords({0x40C0});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0] & 0xFFFF, 0x2704u);
}

// ===========================================================================
// MOVE to CCR
// ===========================================================================

TEST_F(MoveTest, MoveToCcr_FromDn) {
  // MOVE D1, CCR = 0x44C1 (src=D1)
  sim_.State().d[1] = 0x1F;  // set all CCR bits
  PutWords({0x44C1});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().sr & 0x00FF, 0x1Fu);
  // Upper byte of SR unchanged (supervisor bit preserved)
  EXPECT_TRUE(sim_.State().IsSupervisor());
}

// ===========================================================================
// MOVE to SR (privileged)
// ===========================================================================

TEST_F(MoveTest, MoveToSr_FromDn_Supervisor) {
  // MOVE D1, SR = 0x46C1; must be in supervisor mode
  sim_.State().d[1] = 0x2700;  // supervisor + interrupt level 7
  PutWords({0x46C1});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().sr, 0x2700u);
}

TEST_F(MoveTest, MoveToSr_Privilege_Violation) {
  // Drop to user mode, then attempt MOVE D1, SR
  sim_.State().sr &= ~kSrSupervisor;
  sim_.State().d[1] = 0x0000;
  PutWords({0x46C1});
  EXPECT_EQ(sim_.Step(), SimResult::kPrivilegeViolation);
}

// ===========================================================================
// MOVEM — registers to memory, normal order
// ===========================================================================

TEST_F(MoveTest, Movem_RegsToMem_Word) {
  // MOVEM.W D0-D1, (A0) = 0x4890 0x0003 (mask 0x0003 = D0|D1)
  sim_.State().a[0] = 0x2000;
  sim_.State().d[0] = 0x1111;
  sim_.State().d[1] = 0x2222;
  PutWords({0x4890, 0x0003});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x2000).value, 0x1111u);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x2002).value, 0x2222u);
  EXPECT_EQ(sim_.State().a[0], 0x2000u);  // (An) mode: no An update
}

TEST_F(MoveTest, Movem_RegsToMem_Predecrement) {
  // MOVEM.W D0-D1, -(A0) = 0x48A0 0x0003
  // -(A0) mode means decrements first; mask for -(An) is reversed: bit0=A7...
  // mask 0x0003 in predecrement mode = D7 (bit 15) and D6 (bit 14)?
  // Actually: for -(An), bit 0 = A7, bit 7 = A0, bit 8 = D7, bit 15 = D0.
  // So mask 0xC000 = D0 and D1 in predecrement notation.
  sim_.State().a[0] = 0x2010;
  sim_.State().d[0] = 0x1111;
  sim_.State().d[1] = 0x2222;
  PutWords({0x48A0, 0xC000});  // MOVEM.W D0/D1, -(A0): mask bits 15-14 = D0,D1 in reverse map
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  // Counter iterates 0..15; counter=14 hits first (D1), counter=15 hits second (D0).
  // Decrement then write: 0x200E←D1, then 0x200C←D0.
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x200E).value, 0x2222u);  // D1
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x200C).value, 0x1111u);  // D0
  EXPECT_EQ(sim_.State().a[0], 0x200Cu);                        // updated to final address
}

TEST_F(MoveTest, Movem_MemToRegs_Word_Postincrement) {
  // MOVEM.W (A0)+, D0-D1 = 0x4C98 0x0003 (mask bit0=D0, bit1=D1)
  sim_.State().a[0] = 0x2000;
  sim_.GetMemory().WriteWord(0x2000, 0xABCD);
  sim_.GetMemory().WriteWord(0x2002, 0xEF01);
  PutWords({0x4C98, 0x0003});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().d[0], 0xFFFFABCDu);  // sign extended from 0xABCD
  EXPECT_EQ(sim_.State().d[1], 0xFFFFEF01u);  // sign extended
  EXPECT_EQ(sim_.State().a[0], 0x2004u);      // incremented by 2 per register
}

// ===========================================================================
// MOVE USP (privileged)
// ===========================================================================

TEST_F(MoveTest, MoveUsp_AnToUsp) {
  // MOVE A0, USP = 0x4E60 (direction=0: An→USP)
  sim_.State().a[0] = 0x1234;
  PutWords({0x4E60});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().a[7], 0x1234u);  // USP stored in a[7]
}

TEST_F(MoveTest, MoveUsp_UspToAn) {
  // MOVE USP, A1 = 0x4E69 (direction=1: USP→An)
  sim_.State().a[7] = 0x5678;  // set USP
  PutWords({0x4E69});
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.State().a[1], 0x5678u);
}

TEST_F(MoveTest, MoveUsp_PrivilegeViolation) {
  sim_.State().sr &= ~kSrSupervisor;
  PutWords({0x4E60});
  EXPECT_EQ(sim_.Step(), SimResult::kPrivilegeViolation);
}

}  // namespace
}  // namespace easym68k::sim
