// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "easym68k/sim/simulator.h"

namespace easym68k::sim {
namespace {

class InterruptTest : public ::testing::Test {
 protected:
  static M68kSimulator sim_;

  void ResetWithVectors(uint32_t ssp_val, uint32_t pc_val) {
    sim_.GetMemory().Clear();
    sim_.GetMemory().WriteLong(0x0, ssp_val);
    sim_.GetMemory().WriteLong(0x4, pc_val);
    sim_.ClearBreakpoints();
    sim_.ClearPendingIrqs();
    sim_.Reset();
  }
};

M68kSimulator InterruptTest::sim_{M68kSimulator::Config{}};

// 1. InterruptSetsPendingBit
TEST_F(InterruptTest, InterruptSetsPendingBit) {
  ResetWithVectors(0x3FFE, 0x1000);
  sim_.Interrupt(5);
  EXPECT_EQ(sim_.GetPendingIrqs(), 1u << 4);
}

// 2. InterruptLevel1Through7
TEST_F(InterruptTest, InterruptLevel1Through7) {
  ResetWithVectors(0x3FFE, 0x1000);
  for (int lvl = 1; lvl <= 7; ++lvl) {
    sim_.ClearPendingIrqs();
    sim_.Interrupt(lvl);
    EXPECT_EQ(sim_.GetPendingIrqs(), 1u << (lvl - 1));
  }
}

// 3. IrqBelowMaskNoException
TEST_F(InterruptTest, IrqBelowMaskNoException) {
  ResetWithVectors(0x3FFE, 0x1000);
  sim_.State().SetInterruptMask(7);
  sim_.Interrupt(3);

  // Fill memory with a NOP so the simulator executes one step
  sim_.GetMemory().WriteWord(0x1000, 0x4E71);

  auto res = sim_.Step();
  EXPECT_EQ(res, SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x1002u);        // ran NOP, didn't handle interrupt
  EXPECT_EQ(sim_.GetPendingIrqs(), 1u << 2);  // still pending
}

// 4. IrqAboveMaskGeneratesException
TEST_F(InterruptTest, IrqAboveMaskGeneratesException) {
  ResetWithVectors(0x3FFE, 0x1000);
  sim_.State().SetInterruptMask(3);
  sim_.Interrupt(5);

  // Set up autovector handler address
  uint32_t autovec_addr = (24 + 5) * 4;
  sim_.GetMemory().WriteLong(autovec_addr, 0x5000);

  auto res = sim_.Step();
  EXPECT_EQ(res, SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x5000u);   // jumped to ISR
  EXPECT_EQ(sim_.GetPendingIrqs(), 0u);  // cleared
}

// 5. Irq7NmiCannotBeMasked
TEST_F(InterruptTest, Irq7NmiCannotBeMasked) {
  ResetWithVectors(0x3FFE, 0x1000);
  sim_.State().SetInterruptMask(7);  // maximum mask
  sim_.Interrupt(7);                 // NMI

  // Set up NMI autovector address
  uint32_t autovec_addr = (24 + 7) * 4;
  sim_.GetMemory().WriteLong(autovec_addr, 0x7000);

  auto res = sim_.Step();
  EXPECT_EQ(res, SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x7000u);   // jumped to ISR anyway
  EXPECT_EQ(sim_.GetPendingIrqs(), 0u);  // cleared
}

// 6. AutovectorDispatchCorrectAddress
TEST_F(InterruptTest, AutovectorDispatchCorrectAddress) {
  ResetWithVectors(0x3FFE, 0x1000);
  sim_.State().SetInterruptMask(0);

  // Autovector for IRQ level 3 is at vector 27 ($6C)
  sim_.GetMemory().WriteLong(27 * 4, 0x6C00);
  sim_.Interrupt(3);

  auto res = sim_.Step();
  EXPECT_EQ(res, SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x6C00u);
}

// 7. IrqSetsNewInterruptMask
TEST_F(InterruptTest, IrqSetsNewInterruptMask) {
  ResetWithVectors(0x3FFE, 0x1000);
  sim_.State().SetInterruptMask(1);
  sim_.Interrupt(5);

  uint32_t autovec_addr = (24 + 5) * 4;
  sim_.GetMemory().WriteLong(autovec_addr, 0x5000);

  sim_.Step();
  EXPECT_EQ(sim_.State().GetInterruptMask(), 5);  // mask elevated to serviced level
}

// 8. IrqClearsPendingBit
TEST_F(InterruptTest, IrqClearsPendingBit) {
  ResetWithVectors(0x3FFE, 0x1000);
  sim_.State().SetInterruptMask(0);
  sim_.Interrupt(4);

  uint32_t autovec_addr = (24 + 4) * 4;
  sim_.GetMemory().WriteLong(autovec_addr, 0x4000);

  EXPECT_EQ(sim_.GetPendingIrqs(), 1u << 3);
  sim_.Step();
  EXPECT_EQ(sim_.GetPendingIrqs(), 0u);
}

// 9. IrqStackFrameCorrect
TEST_F(InterruptTest, IrqStackFrameCorrect) {
  ResetWithVectors(0x3FFE, 0x1000);
  sim_.State().SetInterruptMask(0);

  // Set PC to 0x1002 so we push 0x1002 onto stack as return address
  sim_.State().pc = 0x1002;
  sim_.State().sr = kSrSupervisor | kSrZero;  // supervisor with Z flag set
  uint16_t orig_sr = sim_.State().sr;

  sim_.Interrupt(3);
  uint32_t autovec_addr = (24 + 3) * 4;
  sim_.GetMemory().WriteLong(autovec_addr, 0x3000);

  sim_.Step();

  uint32_t new_ssp = 0x3FFE - 6;
  EXPECT_EQ(sim_.State().ssp, new_ssp);
  // SR is pushed at top of stack (word)
  EXPECT_EQ(sim_.GetMemory().ReadWord(new_ssp).value, orig_sr);
  // PC is pushed above SR (longword)
  EXPECT_EQ(sim_.GetMemory().ReadLong(new_ssp + 2).value, 0x1002u);
  // CPU is in supervisor mode
  EXPECT_TRUE(sim_.State().IsSupervisor());
}

// 10. IrqClearsTraceFlag
TEST_F(InterruptTest, IrqClearsTraceFlag) {
  ResetWithVectors(0x3FFE, 0x1000);
  sim_.State().SetInterruptMask(0);
  sim_.State().sr |= kSrTrace;  // enable trace

  sim_.Interrupt(2);
  sim_.GetMemory().WriteLong((24 + 2) * 4, 0x2000);

  sim_.Step();
  // Trace flag should be cleared inside exception handler to prevent nested trace exceptions
  EXPECT_FALSE(sim_.State().GetFlag(kSrTrace));
}

// 11. MultiplePendingIrqsHighestFirst
TEST_F(InterruptTest, MultiplePendingIrqsHighestFirst) {
  ResetWithVectors(0x3FFE, 0x1000);
  sim_.State().SetInterruptMask(0);

  // Both IRQ3 and IRQ5 are pending
  sim_.Interrupt(3);
  sim_.Interrupt(5);

  sim_.GetMemory().WriteLong((24 + 3) * 4, 0x3000);
  sim_.GetMemory().WriteLong((24 + 5) * 4, 0x5000);

  // Should service IRQ5 first
  sim_.Step();
  EXPECT_EQ(sim_.State().pc, 0x5000u);
  EXPECT_EQ(sim_.State().GetInterruptMask(), 5);

  // IRQ3 should still be pending
  EXPECT_EQ(sim_.GetPendingIrqs(), 1u << 2);
}

// 12. StopWakesOnInterrupt
TEST_F(InterruptTest, StopWakesOnInterrupt) {
  ResetWithVectors(0x3FFE, 0x1000);
  sim_.State().SetInterruptMask(0);

  // Write STOP instruction at $1000
  // STOP #$2000 (loads SR with supervisor and clears other flags)
  sim_.GetMemory().WriteWord(0x1000, 0x4E72);
  sim_.GetMemory().WriteWord(0x1002, 0x2000);

  // Step 1: execute STOP
  auto res = sim_.Step();
  EXPECT_EQ(res, SimResult::kStopInstruction);
  EXPECT_TRUE(sim_.State().stopped);
  EXPECT_TRUE(sim_.State().halted);
  EXPECT_EQ(sim_.State().pc, 0x1004u);  // PC is advanced past the STOP immediate word

  // Step 2: call Step again while stopped; should return Halted and do nothing
  res = sim_.Step();
  EXPECT_EQ(res, SimResult::kHalted);

  // Step 3: Inject non-masked interrupt
  sim_.GetMemory().WriteLong((24 + 4) * 4, 0x4000);
  sim_.Interrupt(4);

  // Interrupt should clear stopped and halted
  EXPECT_FALSE(sim_.State().stopped);
  EXPECT_FALSE(sim_.State().halted);

  // Step 4: execute next step, which services the interrupt!
  res = sim_.Step();
  EXPECT_EQ(res, SimResult::kOk);
  EXPECT_EQ(sim_.State().pc, 0x4000u);  // PC redirected to autovector 4
}

// 13. StopDoesNotWakeOnMaskedIrq
TEST_F(InterruptTest, StopDoesNotWakeOnMaskedIrq) {
  ResetWithVectors(0x3FFE, 0x1000);
  sim_.State().SetInterruptMask(4);

  // Execute STOP instruction
  // STOP #$2400 (loads SR with supervisor and sets interrupt mask to 4)
  sim_.GetMemory().WriteWord(0x1000, 0x4E72);
  sim_.GetMemory().WriteWord(0x1002, 0x2400);
  sim_.Step();

  EXPECT_TRUE(sim_.State().stopped);
  EXPECT_TRUE(sim_.State().halted);

  // Inject level 3 interrupt (masked by 4)
  sim_.Interrupt(3);

  // Should stay stopped and halted
  EXPECT_TRUE(sim_.State().stopped);
  EXPECT_TRUE(sim_.State().halted);

  // Calling Step() should still return Halted
  auto res = sim_.Step();
  EXPECT_EQ(res, SimResult::kHalted);
}

// 14. ThreadSafeInterrupt
TEST_F(InterruptTest, ThreadSafeInterrupt) {
  ResetWithVectors(0x3FFE, 0x1000);
  sim_.State().SetInterruptMask(0);

  // Set up a self-loop so the simulator can run indefinitely
  // $1000: BRA $1000 (opcode 0x60FE)
  sim_.GetMemory().WriteWord(0x1000, 0x60FE);

  // Set up NMI autovector address with SIMHALT (0xFFFF 0xFFFF)
  sim_.GetMemory().WriteLong((24 + 5) * 4, 0x5000);
  sim_.GetMemory().WriteWord(0x5000, 0xFFFF);
  sim_.GetMemory().WriteWord(0x5002, 0xFFFF);

  std::thread t([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    sim_.Interrupt(5);
  });

  // Run the loop, but limit maximum execution just in case
  sim_.Run(20000000);

  if (t.joinable()) {
    t.join();
  }

  // Simulator should have received the interrupt, exited the self-loop,
  // jumped to the ISR at 0x5000, and immediately halted there due to SIMHALT.
  // The PC will point past the 4-byte double-word SIMHALT instruction ($5004).
  EXPECT_EQ(sim_.State().pc, 0x5004u);
}

// 15. ClearPendingIrqs
TEST_F(InterruptTest, ClearPendingIrqs) {
  ResetWithVectors(0x3FFE, 0x1000);
  sim_.Interrupt(3);
  sim_.Interrupt(6);

  EXPECT_EQ(sim_.GetPendingIrqs(), (1u << 2) | (1u << 5));
  sim_.ClearPendingIrqs();
  EXPECT_EQ(sim_.GetPendingIrqs(), 0u);
}

}  // namespace
}  // namespace easym68k::sim
