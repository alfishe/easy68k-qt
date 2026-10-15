// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Memory map semantics and address/bus error (class 0) exceptions (Task 9.7).
// Expected frames follow RUN.CPP exceptionHandler() and UTILS.CPP
// mem_put()/mem_req()/memoryMapCheck().

#include <gtest/gtest.h>

#include <cstdint>
#include <initializer_list>
#include <vector>

#include "easym68k/sim/isim_observer.h"
#include "easym68k/sim/simulator.h"

namespace easym68k::sim {
namespace {

constexpr uint32_t kCode = 0x1000;
constexpr uint32_t kBusHandler = 0x3000;
constexpr uint32_t kAddrHandler = 0x3100;
constexpr uint32_t kInitialSsp = 0x01000000;

struct Class0Frame {
  uint16_t info;
  uint32_t address;
  uint16_t ir;
  uint16_t sr;
  uint32_t pc;
};

class AddressBusErrorTest : public ::testing::Test {
 protected:
  AddressBusErrorTest() : sim_(M68kSimulator::Config{}) {}

  void Load(std::initializer_list<uint16_t> words) {
    Memory& m = sim_.GetMemory();
    m.Clear();
    m.WriteLong(0x08, kBusHandler);
    m.WriteLong(0x0C, kAddrHandler);
    uint32_t addr = kCode;
    for (uint16_t w : words) {
      m.WriteWord(addr, w);
      addr += 2;
    }
    m.WriteWord(kBusHandler, 0x4E71);
    m.WriteWord(kAddrHandler, 0x4E71);
    sim_.SetStartAddress(kCode);
    sim_.InitProgram();  // SR=$2000, SSP=$01000000
  }

  Class0Frame Frame() {
    uint32_t sp = sim_.State().ssp & kAddressMask;
    const Memory& m = sim_.GetMemory();
    return {static_cast<uint16_t>(m.ReadWord(sp).value), m.ReadLong(sp + 2).value,
            static_cast<uint16_t>(m.ReadWord(sp + 6).value),
            static_cast<uint16_t>(m.ReadWord(sp + 8).value), m.ReadLong(sp + 10).value};
  }

  M68kSimulator sim_;
};

TEST_F(AddressBusErrorTest, OddWordReadBuildsClass0Frame) {
  Load({0x3010});  // MOVE.W (A0),D0
  sim_.State().a[0] = 0x2001;
  sim_.State().d[0] = 0x11112222;
  EXPECT_EQ(sim_.Step(), SimResult::kAddressError);
  EXPECT_FALSE(sim_.State().halted);
  EXPECT_EQ(sim_.State().pc, kAddrHandler);
  EXPECT_EQ(sim_.State().ssp, kInitialSsp - 14);
  EXPECT_TRUE(sim_.State().IsSupervisor());
  EXPECT_EQ(sim_.State().cycles, 50u);
  EXPECT_EQ(sim_.State().d[0], 0x11112222u);  // not modified
  Class0Frame f = Frame();
  EXPECT_EQ(f.info, 0x15);  // read, supervisor data
  EXPECT_EQ(f.address, 0x2001u);
  EXPECT_EQ(f.ir, 0x3010);
  EXPECT_EQ(f.sr, 0x2000);
  EXPECT_EQ(f.pc, kCode + 2);  // PC at the time of the read
}

TEST_F(AddressBusErrorTest, OddWordWriteUsesInstructionAddressPlus2) {
  Load({0x31C0, 0x2001});  // MOVE.W D0,$2001.W
  EXPECT_EQ(sim_.Step(), SimResult::kAddressError);
  Class0Frame f = Frame();
  EXPECT_EQ(f.info, 0x05);  // write, supervisor data
  EXPECT_EQ(f.address, 0x2001u);
  EXPECT_EQ(f.pc, kCode + 2);  // OLD_PC + 2 for writes
}

TEST_F(AddressBusErrorTest, ReadAfterExtensionWordStacksCurrentPc) {
  Load({0x3038, 0x2001});  // MOVE.W $2001.W,D0
  EXPECT_EQ(sim_.Step(), SimResult::kAddressError);
  EXPECT_EQ(Frame().pc, kCode + 4);
}

TEST_F(AddressBusErrorTest, OddPcFetchIsProgramAccess) {
  Load({0x4ED0});  // JMP (A0)
  sim_.State().a[0] = 0x2001;
  EXPECT_EQ(sim_.Step(), SimResult::kOk);  // JMP itself succeeds
  EXPECT_EQ(sim_.Step(), SimResult::kAddressError);
  Class0Frame f = Frame();
  EXPECT_EQ(f.info, 0x16);  // read, supervisor program
  EXPECT_EQ(f.address, 0x2001u);
  EXPECT_EQ(f.pc, 0x2001u);
  EXPECT_EQ(f.ir, 0x4ED0);  // previous instruction word
}

TEST_F(AddressBusErrorTest, InvalidMapReadIsBusError) {
  Load({0x3038, 0x5000});  // MOVE.W $5000.W,D0
  M68kSimulator::MemoryMapConfig map;
  map.invalid = {true, 0x5000, 0x5FFF};
  sim_.ConfigureMemoryMap(map);
  EXPECT_EQ(sim_.Step(), SimResult::kBusError);
  EXPECT_EQ(sim_.State().pc, kBusHandler);
  EXPECT_EQ(Frame().info, 0x15);
}

TEST_F(AddressBusErrorTest, ProtectedMapUserModeBusError) {
  Load({0x31C0, 0x6000});  // MOVE.W D0,$6000.W
  M68kSimulator::MemoryMapConfig map;
  map.protected_ = {true, 0x6000, 0x6FFF};
  sim_.ConfigureMemoryMap(map);
  sim_.State().sr = 0x0000;  // user mode
  EXPECT_EQ(sim_.Step(), SimResult::kBusError);
  EXPECT_EQ(Frame().sr, 0x0000);  // SR before exception processing
}

TEST_F(AddressBusErrorTest, ProtectedMapSupervisorAllowed) {
  Load({0x31C0, 0x6000});  // MOVE.W D0,$6000.W
  M68kSimulator::MemoryMapConfig map;
  map.protected_ = {true, 0x6000, 0x6FFF};
  sim_.ConfigureMemoryMap(map);
  sim_.State().d[0] = 0xABCD;
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x6000).value, 0xABCDu);
}

TEST_F(AddressBusErrorTest, RomMapWritesIgnored) {
  Load({0x31C0, 0x7000});  // MOVE.W D0,$7000.W
  sim_.GetMemory().WriteWord(0x7000, 0x5555);
  M68kSimulator::MemoryMapConfig map;
  map.rom = {true, 0x7000, 0x7FFF};
  sim_.ConfigureMemoryMap(map);
  sim_.State().d[0] = 0xABCD;
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x7000).value, 0x5555u);
}

TEST_F(AddressBusErrorTest, ReadMapWriteIsBusError) {
  Load({0x31C0, 0x7000});  // MOVE.W D0,$7000.W
  M68kSimulator::MemoryMapConfig map;
  map.read = {true, 0x7000, 0x7FFF};
  sim_.ConfigureMemoryMap(map);
  EXPECT_EQ(sim_.Step(), SimResult::kBusError);
  EXPECT_EQ(Frame().info, 0x05);
}

TEST_F(AddressBusErrorTest, HardwareAddressesExempt) {
  Load({0x11C0, 0x7010});  // MOVE.B D0,$7010.W
  M68kSimulator::MemoryMapConfig map;
  map.invalid = {true, 0x7000, 0x7FFF};
  sim_.ConfigureMemoryMap(map);
  sim_.SetHardwareAddresses(0x7010, 0x7030, 0x7032, 0x7034);
  sim_.State().d[0] = 0x42;
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
  EXPECT_EQ(sim_.GetMemory().RawData()[0x7010], 0x42);
}

TEST_F(AddressBusErrorTest, RegistersRestoredOnFault) {
  Load({0xD050});  // ADD.W (A0),D0
  sim_.State().a[0] = 0x2001;
  sim_.State().d[0] = 0x00000005;
  sim_.Step();
  EXPECT_EQ(sim_.State().d[0], 5u);
}

TEST_F(AddressBusErrorTest, DoubleFaultHalts) {
  Load({0x3010});                            // MOVE.W (A0),D0
  sim_.GetMemory().WriteLong(0x0C, 0x3101);  // address error handler at odd address
  sim_.State().a[0] = 0x2001;
  EXPECT_EQ(sim_.Step(), SimResult::kAddressError);
  EXPECT_FALSE(sim_.State().halted);
  sim_.Step();  // handler fetch faults again
  EXPECT_TRUE(sim_.State().halted);
}

TEST_F(AddressBusErrorTest, FaultThenGoodInstructionClearsDoubleFaultState) {
  Load({0x3010});
  sim_.State().a[0] = 0x2001;
  sim_.Step();                             // fault -> handler (NOP)
  EXPECT_EQ(sim_.Step(), SimResult::kOk);  // handler NOP
  sim_.State().pc = kCode;                 // fault again later
  EXPECT_EQ(sim_.Step(), SimResult::kAddressError);
  EXPECT_FALSE(sim_.State().halted);
}

TEST_F(AddressBusErrorTest, ExceptionsDisabledHaltsWithoutFrame) {
  Load({0x3010});
  sim_.SetExceptionsEnabled(false);
  sim_.State().a[0] = 0x2001;
  EXPECT_EQ(sim_.Step(), SimResult::kAddressError);
  EXPECT_TRUE(sim_.State().halted);
  EXPECT_EQ(sim_.State().ssp, kInitialSsp);
}

TEST_F(AddressBusErrorTest, OddSspHaltsWithoutFrame) {
  Load({0x3010});
  sim_.State().a[0] = 0x2001;
  sim_.State().ssp = 0x00008001;
  EXPECT_EQ(sim_.Step(), SimResult::kAddressError);
  EXPECT_TRUE(sim_.State().halted);
  EXPECT_EQ(sim_.State().ssp, 0x00008001u);
  EXPECT_EQ(sim_.State().pc, kAddrHandler);  // vector fetched before the check
}

class VectorRecorder : public ISimObserver {
 public:
  void OnEvent(const SimEvent& e) override {
    if (e.type == SimEventType::kExceptionRaised)
      vectors.push_back(e.vector);
  }
  std::vector<int> vectors;
};

TEST_F(AddressBusErrorTest, ObserverSeesAddressErrorVector) {
  Load({0x3010});
  VectorRecorder rec;
  sim_.AddObserver(&rec);
  sim_.State().a[0] = 0x2001;
  sim_.Step();
  ASSERT_EQ(rec.vectors.size(), 1u);
  EXPECT_EQ(rec.vectors[0], static_cast<int>(ExceptionVector::kAddressError));
}

// The original run loop keeps running through exceptions; Run() must too.
TEST_F(AddressBusErrorTest, RunContinuesThroughExceptions) {
  Load({0x4E40,          // TRAP #0
        0x7201,          // MOVEQ #1,D1
        0x103C, 0x0009,  // MOVE.B #9,D0
        0x4E4F});        // TRAP #15 (task 9: terminate)
  sim_.GetMemory().WriteLong(0x80, 0x3200);
  sim_.GetMemory().WriteWord(0x3200, 0x7407);  // MOVEQ #7,D2
  sim_.GetMemory().WriteWord(0x3202, 0x4E73);  // RTE
  EXPECT_EQ(sim_.Run(100), SimResult::kHalted);
  EXPECT_EQ(sim_.State().d[2], 7u);
  EXPECT_EQ(sim_.State().d[1], 1u);
  EXPECT_TRUE(sim_.State().halted);
}

// RUN.CPP: class 1/2 exceptions stack OLD_PC = PC after the instruction,
// except illegal instruction (OLD_PC -= 2 → the instruction itself).
TEST_F(AddressBusErrorTest, IllegalStacksInstructionAddress) {
  Load({0x4AFC});  // ILLEGAL
  sim_.GetMemory().WriteLong(0x10, 0x3300);
  sim_.Step();
  EXPECT_EQ(sim_.GetMemory().ReadLong((sim_.State().ssp + 2) & kAddressMask).value, kCode);
}

TEST_F(AddressBusErrorTest, LineAStacksNextPc) {
  Load({0xA123});
  sim_.GetMemory().WriteLong(0x28, 0x3300);
  sim_.Step();
  EXPECT_EQ(sim_.GetMemory().ReadLong((sim_.State().ssp + 2) & kAddressMask).value, kCode + 2);
}

TEST_F(AddressBusErrorTest, PrivilegeViolationStacksInstructionAddress) {
  // 68000: the address of the privileged instruction (the original stacked
  // the PC after its first word; bug, fixed in 11.1).
  Load({0x4E70});  // RESET in user mode
  sim_.State().sr = 0x0000;
  sim_.GetMemory().WriteLong(0x20, 0x3300);
  sim_.Step();
  EXPECT_EQ(sim_.GetMemory().ReadLong((sim_.State().ssp + 2) & kAddressMask).value, kCode);
}

TEST_F(AddressBusErrorTest, ChkStacksNextPc) {
  Load({0x4181});  // CHK D1,D0
  sim_.State().d[0] = 20;
  sim_.State().d[1] = 10;
  sim_.GetMemory().WriteLong(0x18, 0x3300);
  sim_.Step();
  EXPECT_EQ(sim_.GetMemory().ReadLong((sim_.State().ssp + 2) & kAddressMask).value, kCode + 2);
}

}  // namespace
}  // namespace easym68k::sim
