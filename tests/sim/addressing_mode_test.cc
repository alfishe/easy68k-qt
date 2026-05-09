// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/sim/addressing_mode.h"

#include <gtest/gtest.h>

namespace easym68k::sim {
namespace {

// ---------------------------------------------------------------------------
// Test fixture — fresh Memory + zeroed CpuState for each test
// ---------------------------------------------------------------------------

class EATest : public ::testing::Test {
 protected:
  static Memory memory;
  CpuState state{};  // zero-init; user mode (S-bit clear)

  void SetUp() override { memory.Clear(); }
};

Memory EATest::memory;

// ---------------------------------------------------------------------------
// Helper: write a big-endian word/long into memory at address
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// CalculateEA — all 12 modes
// ---------------------------------------------------------------------------

// 1. Mode 0: Dn
TEST_F(EATest, CalcEA_Dn) {
  uint32_t pc = 0x1000;
  auto ea = CalculateEA(memory, state, pc, 0, 3, DataSize::kWord);
  EXPECT_EQ(ea.target, EaTarget::kDataReg);
  EXPECT_EQ(ea.index, 3u);
  EXPECT_EQ(pc, 0x1000u);  // no PC advance
}

// 2. Mode 1: An
TEST_F(EATest, CalcEA_An) {
  uint32_t pc = 0x1000;
  auto ea = CalculateEA(memory, state, pc, 1, 5, DataSize::kWord);
  EXPECT_EQ(ea.target, EaTarget::kAddrReg);
  EXPECT_EQ(ea.index, 5u);
  EXPECT_EQ(pc, 0x1000u);
}

// 3. Mode 2: (An)
TEST_F(EATest, CalcEA_AnInd) {
  state.a[2] = 0x3000;
  uint32_t pc = 0x1000;
  auto ea = CalculateEA(memory, state, pc, 2, 2, DataSize::kLong);
  EXPECT_EQ(ea.target, EaTarget::kMemory);
  EXPECT_EQ(ea.address, 0x3000u);
  EXPECT_EQ(state.a[2], 0x3000u);  // register not modified
  EXPECT_EQ(pc, 0x1000u);
}

// 4. Mode 3: (An)+ — byte/word/long; SP byte → 2
TEST_F(EATest, CalcEA_AnPost_Byte) {
  state.a[0] = 0x2000;
  uint32_t pc = 0x1000;
  auto ea = CalculateEA(memory, state, pc, 3, 0, DataSize::kByte);
  EXPECT_EQ(ea.target, EaTarget::kMemory);
  EXPECT_EQ(ea.address, 0x2000u);
  EXPECT_EQ(state.a[0], 0x2001u);  // incremented by 1
}

TEST_F(EATest, CalcEA_AnPost_Word) {
  state.a[1] = 0x2000;
  uint32_t pc = 0x1000;
  auto ea = CalculateEA(memory, state, pc, 3, 1, DataSize::kWord);
  EXPECT_EQ(ea.address, 0x2000u);
  EXPECT_EQ(state.a[1], 0x2002u);
}

TEST_F(EATest, CalcEA_AnPost_Long) {
  state.a[2] = 0x2000;
  uint32_t pc = 0x1000;
  auto ea = CalculateEA(memory, state, pc, 3, 2, DataSize::kLong);
  EXPECT_EQ(ea.address, 0x2000u);
  EXPECT_EQ(state.a[2], 0x2004u);
}

TEST_F(EATest, CalcEA_AnPost_SpByteAligned) {
  // SP byte postincrement forces step of 2
  state.a[7] = 0x2000;
  uint32_t pc = 0x1000;
  auto ea = CalculateEA(memory, state, pc, 3, 7, DataSize::kByte);
  EXPECT_EQ(ea.address, 0x2000u);
  EXPECT_EQ(state.a[7], 0x2002u);  // 2, not 1
}

// 5. Mode 4: -(An)
TEST_F(EATest, CalcEA_AnPre_Word) {
  state.a[3] = 0x3004;
  uint32_t pc = 0x1000;
  auto ea = CalculateEA(memory, state, pc, 4, 3, DataSize::kWord);
  EXPECT_EQ(ea.target, EaTarget::kMemory);
  EXPECT_EQ(ea.address, 0x3002u);  // decremented before access
  EXPECT_EQ(state.a[3], 0x3002u);
}

TEST_F(EATest, CalcEA_AnPre_SpByteAligned) {
  state.a[7] = 0x3002;
  uint32_t pc = 0x1000;
  auto ea = CalculateEA(memory, state, pc, 4, 7, DataSize::kByte);
  EXPECT_EQ(ea.address, 0x3000u);
  EXPECT_EQ(state.a[7], 0x3000u);
}

// 6. Mode 5: d(An)
TEST_F(EATest, CalcEA_AnDisp) {
  state.a[4] = 0x5000;
  uint32_t pc = 0x1000;
  // Write displacement word +0x0100 into memory at PC
  memory.WriteWord(pc, 0x0100);
  auto ea = CalculateEA(memory, state, pc, 5, 4, DataSize::kWord);
  EXPECT_EQ(ea.target, EaTarget::kMemory);
  EXPECT_EQ(ea.address, 0x5100u);
  EXPECT_EQ(pc, 0x1002u);  // PC advanced past displacement word
}

TEST_F(EATest, CalcEA_AnDisp_Negative) {
  state.a[0] = 0x5000;
  uint32_t pc = 0x1000;
  memory.WriteWord(pc, 0xFFFE);  // displacement = -2
  auto ea = CalculateEA(memory, state, pc, 5, 0, DataSize::kWord);
  EXPECT_EQ(ea.address, 0x4FFEu);
}

// 7. Mode 6: d(An,Xi) — data register word index
TEST_F(EATest, CalcEA_AnIndex_DataWordIndex) {
  state.a[0] = 0x3000;
  state.d[1] = 0x0010;  // word index = 16
  uint32_t pc = 0x1000;
  // Extension word: bit15=0 (Dn), bits14-12=001 (D1), bit11=0 (word), bits7-0=0x04 (disp=4)
  memory.WriteWord(pc, 0x1004);
  auto ea = CalculateEA(memory, state, pc, 6, 0, DataSize::kByte);
  EXPECT_EQ(ea.target, EaTarget::kMemory);
  EXPECT_EQ(ea.address, 0x3014u);  // 0x3000 + 4 + 16
  EXPECT_EQ(pc, 0x1002u);
}

// 8. Mode 7.0: xxx.W absolute word (sign-extended)
TEST_F(EATest, CalcEA_AbsW_Positive) {
  uint32_t pc = 0x1000;
  memory.WriteWord(pc, 0x4000);
  auto ea = CalculateEA(memory, state, pc, 7, 0, DataSize::kWord);
  EXPECT_EQ(ea.target, EaTarget::kMemory);
  EXPECT_EQ(ea.address, 0x4000u);
  EXPECT_EQ(pc, 0x1002u);
}

TEST_F(EATest, CalcEA_AbsW_SignExtended) {
  uint32_t pc = 0x1000;
  memory.WriteWord(pc, 0x8000);  // negative → sign-extend then mask to 24 bits
  auto ea = CalculateEA(memory, state, pc, 7, 0, DataSize::kWord);
  // 0xFFFF8000 & 0x00FFFFFF = 0x00FF8000
  EXPECT_EQ(ea.address, 0xFF8000u);
}

// 9. Mode 7.1: xxx.L absolute long
TEST_F(EATest, CalcEA_AbsL) {
  uint32_t pc = 0x1000;
  memory.WriteLong(pc, 0x00123456);
  auto ea = CalculateEA(memory, state, pc, 7, 1, DataSize::kWord);
  EXPECT_EQ(ea.target, EaTarget::kMemory);
  EXPECT_EQ(ea.address, 0x123456u);
  EXPECT_EQ(pc, 0x1004u);
}

// 10. Mode 7.2: d(PC)
TEST_F(EATest, CalcEA_PcDisp) {
  uint32_t pc = 0x1000;
  memory.WriteWord(pc, 0x0040);  // displacement = +64
  auto ea = CalculateEA(memory, state, pc, 7, 2, DataSize::kWord);
  EXPECT_EQ(ea.target, EaTarget::kMemory);
  // EA = ext_pc (0x1000) + 64 = 0x1040
  EXPECT_EQ(ea.address, 0x1040u);
  EXPECT_EQ(pc, 0x1002u);
}

TEST_F(EATest, CalcEA_PcDisp_Negative) {
  uint32_t pc = 0x1010;
  memory.WriteWord(pc, 0xFFF0);  // displacement = -16
  auto ea = CalculateEA(memory, state, pc, 7, 2, DataSize::kWord);
  EXPECT_EQ(ea.address, 0x1000u);  // 0x1010 - 16 = 0x1000
}

// 11. Mode 7.3: d(PC,Xi)
TEST_F(EATest, CalcEA_PcIndex) {
  state.d[2] = 0x0008;  // word index = 8
  uint32_t pc = 0x1000;
  // Extension word: bit15=0 (Dn), bits14-12=010 (D2), bit11=0 (word), bits7-0=0x02 (disp=2)
  memory.WriteWord(pc, 0x2002);
  auto ea = CalculateEA(memory, state, pc, 7, 3, DataSize::kWord);
  // EA = ext_pc (0x1000) + 2 + 8 = 0x100A
  EXPECT_EQ(ea.target, EaTarget::kMemory);
  EXPECT_EQ(ea.address, 0x100Au);
  EXPECT_EQ(pc, 0x1002u);
}

// 12. Mode 7.4: #imm
TEST_F(EATest, CalcEA_Imm_Byte) {
  uint32_t pc = 0x1000;
  memory.WriteWord(pc, 0x00AB);
  auto ea = CalculateEA(memory, state, pc, 7, 4, DataSize::kByte);
  EXPECT_EQ(ea.target, EaTarget::kImmediate);
  EXPECT_EQ(ea.address, 0xABu);
  EXPECT_EQ(pc, 0x1002u);
}

TEST_F(EATest, CalcEA_Imm_Word) {
  uint32_t pc = 0x1000;
  memory.WriteWord(pc, 0x1234);
  auto ea = CalculateEA(memory, state, pc, 7, 4, DataSize::kWord);
  EXPECT_EQ(ea.target, EaTarget::kImmediate);
  EXPECT_EQ(ea.address, 0x1234u);
  EXPECT_EQ(pc, 0x1002u);
}

TEST_F(EATest, CalcEA_Imm_Long) {
  uint32_t pc = 0x1000;
  memory.WriteLong(pc, 0x12345678);
  auto ea = CalculateEA(memory, state, pc, 7, 4, DataSize::kLong);
  EXPECT_EQ(ea.target, EaTarget::kImmediate);
  EXPECT_EQ(ea.address, 0x12345678u);
  EXPECT_EQ(pc, 0x1004u);
}

// ---------------------------------------------------------------------------
// ReadEA tests
// ---------------------------------------------------------------------------

// 13. ReadEA from data register — byte/word/long masking
TEST_F(EATest, ReadEA_DataReg_Byte) {
  state.d[3] = 0x12345678;
  auto ea = MakeDataRegEa(3);
  EXPECT_EQ(ReadEA(memory, state, ea, DataSize::kByte), 0x78u);
}

TEST_F(EATest, ReadEA_DataReg_Word) {
  state.d[3] = 0x12345678;
  auto ea = MakeDataRegEa(3);
  EXPECT_EQ(ReadEA(memory, state, ea, DataSize::kWord), 0x5678u);
}

TEST_F(EATest, ReadEA_DataReg_Long) {
  state.d[3] = 0x12345678;
  auto ea = MakeDataRegEa(3);
  EXPECT_EQ(ReadEA(memory, state, ea, DataSize::kLong), 0x12345678u);
}

// 14. ReadEA from address register
TEST_F(EATest, ReadEA_AddrReg_Long) {
  state.a[5] = 0xABCDEF01;
  auto ea = MakeAddrRegEa(5);
  EXPECT_EQ(ReadEA(memory, state, ea, DataSize::kLong), 0xABCDEF01u);
}

TEST_F(EATest, ReadEA_AddrReg_Word) {
  state.a[5] = 0xABCDEF01;
  auto ea = MakeAddrRegEa(5);
  EXPECT_EQ(ReadEA(memory, state, ea, DataSize::kWord), 0xEF01u);
}

// 15. ReadEA from memory — big-endian
TEST_F(EATest, ReadEA_Memory_Word) {
  memory.WriteWord(0x3000, 0xBEEF);
  auto ea = MakeMemoryEa(0x3000, 2, 0);
  EXPECT_EQ(ReadEA(memory, state, ea, DataSize::kWord), 0xBEEFu);
}

TEST_F(EATest, ReadEA_Memory_Long) {
  memory.WriteLong(0x3000, 0xDEADBEEF);
  auto ea = MakeMemoryEa(0x3000, 2, 0);
  EXPECT_EQ(ReadEA(memory, state, ea, DataSize::kLong), 0xDEADBEEFu);
}

// 16. ReadEA from immediate — returns stored value
TEST_F(EATest, ReadEA_Immediate) {
  auto ea = MakeImmediateEa(0x4200);
  EXPECT_EQ(ReadEA(memory, state, ea, DataSize::kWord), 0x4200u);
}

// ---------------------------------------------------------------------------
// WriteEA tests
// ---------------------------------------------------------------------------

// 17. WriteEA to data register — preserves upper bits for byte/word
TEST_F(EATest, WriteEA_DataReg_Byte_PreservesUpper) {
  state.d[0] = 0xFFFFFFFF;
  auto ea = MakeDataRegEa(0);
  EXPECT_EQ(WriteEA(memory, state, ea, 0xAB, DataSize::kByte), SimResult::kOk);
  EXPECT_EQ(state.d[0], 0xFFFFFFABu);
}

TEST_F(EATest, WriteEA_DataReg_Word_PreservesUpper) {
  state.d[1] = 0xFFFFFFFF;
  auto ea = MakeDataRegEa(1);
  EXPECT_EQ(WriteEA(memory, state, ea, 0x1234, DataSize::kWord), SimResult::kOk);
  EXPECT_EQ(state.d[1], 0xFFFF1234u);
}

TEST_F(EATest, WriteEA_DataReg_Long) {
  auto ea = MakeDataRegEa(2);
  EXPECT_EQ(WriteEA(memory, state, ea, 0x12345678, DataSize::kLong), SimResult::kOk);
  EXPECT_EQ(state.d[2], 0x12345678u);
}

// 18. WriteEA to address register — word sign-extends, long stores full
TEST_F(EATest, WriteEA_AddrReg_WordSignExtend) {
  auto ea = MakeAddrRegEa(3);
  // Write negative word value — should sign-extend to 32 bits
  EXPECT_EQ(WriteEA(memory, state, ea, 0x8000, DataSize::kWord), SimResult::kOk);
  EXPECT_EQ(state.a[3], 0xFFFF8000u);
}

TEST_F(EATest, WriteEA_AddrReg_WordPositive) {
  auto ea = MakeAddrRegEa(3);
  EXPECT_EQ(WriteEA(memory, state, ea, 0x0100, DataSize::kWord), SimResult::kOk);
  EXPECT_EQ(state.a[3], 0x00000100u);
}

TEST_F(EATest, WriteEA_AddrReg_Long) {
  auto ea = MakeAddrRegEa(4);
  EXPECT_EQ(WriteEA(memory, state, ea, 0xDEADBEEF, DataSize::kLong), SimResult::kOk);
  EXPECT_EQ(state.a[4], 0xDEADBEEFu);
}

// 19. WriteEA to memory — big-endian
TEST_F(EATest, WriteEA_Memory_Word) {
  auto ea = MakeMemoryEa(0x4000, 2, 0);
  EXPECT_EQ(WriteEA(memory, state, ea, 0xCAFE, DataSize::kWord), SimResult::kOk);
  EXPECT_EQ(memory.ReadByte(0x4000).value, 0xCAu);
  EXPECT_EQ(memory.ReadByte(0x4001).value, 0xFEu);
}

// 20. WriteEA to immediate — returns error
TEST_F(EATest, WriteEA_Immediate_Error) {
  auto ea = MakeImmediateEa(0x1234);
  EXPECT_EQ(WriteEA(memory, state, ea, 0, DataSize::kWord), SimResult::kBadInstruction);
}

}  // namespace
}  // namespace easym68k::sim
