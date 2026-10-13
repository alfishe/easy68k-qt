// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Simulator core integration test (Task 9.6): assemble with the Phase 8
// assembler, load the S-records, start the program the way Sim68K does
// (InitProgram) and run headlessly, exercising cycles, interrupts, STOP,
// logging, observers, the SIMHALT/exception toggles, the memory map and
// Trap #15 text I/O together.

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "easym68k/asm/assembler.h"
#include "easym68k/sim/cycle_data.h"
#include "easym68k/sim/iexecution_logger.h"
#include "easym68k/sim/isim_observer.h"
#include "easym68k/sim/itext_io.h"
#include "easym68k/sim/simulator.h"

namespace easym68k {
namespace {

using sim::CpuState;
using sim::IExecutionLogger;
using sim::M68kSimulator;
using sim::Memory;
using sim::SimEvent;
using sim::SimEventType;
using sim::SimResult;

// Assembles `source`, loads it and initialises the program like Sim68K's
// File > Open: InitProgram() with the S-record start address.
::testing::AssertionResult AssembleAndLoad(const std::string& source, M68kSimulator& sim) {
  asm_::Assembler assembler;
  asm_::AssemblyResult r = assembler.Assemble(source);
  if (!r.success) {
    std::string msg = "assembly failed:";
    for (const auto& e : r.errors)
      msg += "\n  line " + std::to_string(e.line_number) + ": " + e.message;
    return ::testing::AssertionFailure() << msg;
  }
  std::vector<std::string> lines;
  std::istringstream in(r.srecord);
  for (std::string line; std::getline(in, line);) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (!line.empty())
      lines.push_back(line);
  }
  sim.GetMemory().Clear();
  if (!sim.LoadSRecordLines(lines))
    return ::testing::AssertionFailure() << "S-record load failed";
  sim.InitProgram();
  return ::testing::AssertionSuccess();
}

class CapturingTextIO : public sim::ITextIO {
 public:
  void TextOut(std::string_view t) override { out.append(t); }
  void TextOutCRLF(std::string_view t) override {
    out.append(t);
    out.append("\r\n");
  }
  void TextIn(char*, long*, long*) override {}
  void CharIn(char*) override {}
  void CharOut(char ch) override { out.push_back(ch); }
  void SetCursorPosition(int, int) override {}
  void GetCursorPosition(int*, int*) override {}
  void ClearScreen() override {}
  void SetKeyboardEcho(bool) override {}
  void SetInputPrompt(bool) override {}
  void SetInputLFDisplay(bool) override {}
  void GetCharAt(int, int, char*) override {}
  void ScrollRect(int, int, int, int, int) override {}
  void SetFontProperties(uint32_t, uint32_t) override {}
  bool IsKeyPending() override { return false; }
  void GetKeyState(long*) override {}
  void SetKeyCommandsEnabled(bool) override {}
  std::string out;
};

class PcLogger : public sim::ExecutionLoggerBase {
 public:
  void LogInstruction(uint32_t pc, uint16_t, const CpuState&) override { pcs.push_back(pc); }
  void LogMemoryDump(uint32_t, uint32_t, const Memory&) override {}
  void LogOutput(const char*, int) override {}
  std::vector<uint32_t> pcs;
};

class EventLog : public sim::ISimObserver {
 public:
  void OnEvent(const SimEvent& e) override { events.push_back(e); }
  std::vector<SimEvent> events;
};

// The Task 9.6 program (with the terminate call written correctly: the
// Trap #15 task number goes in D0).
constexpr const char* kAddProgram = R"(
        ORG     $1000
START:  MOVE.W  #$1234,D0
        MOVE.W  #$5678,D1
        ADD.W   D0,D1
        MOVE.W  D1,$2000
        MOVE.B  #9,D0
        TRAP    #15
        END     START
)";

class CoreIntegrationTest : public ::testing::Test {
 protected:
  CoreIntegrationTest() : sim_(MakeConfig()) {}
  M68kSimulator::Config MakeConfig() {
    M68kSimulator::Config c;
    c.text_io = &text_;
    c.execution_logger = &logger_;
    return c;
  }
  CapturingTextIO text_;
  PcLogger logger_;
  M68kSimulator sim_;
};

TEST_F(CoreIntegrationTest, FullProgramExecution) {
  ASSERT_TRUE(AssembleAndLoad(kAddProgram, sim_));
  EXPECT_EQ(sim_.State().pc, 0x1000u);
  EXPECT_EQ(sim_.Run(), SimResult::kHalted);
  EXPECT_EQ(sim_.State().d[1] & 0xFFFF, 0x68ACu);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x2000).value, 0x68ACu);
}

TEST_F(CoreIntegrationTest, CycleCountAccumulated) {
  ASSERT_TRUE(AssembleAndLoad(kAddProgram, sim_));
  bool abs_w = sim_.GetMemory().ReadWord(0x100A).value == 0x31C1;  // MOVE.W D1,$2000.W
  sim_.Run();
  // MOVE.W #,D0 (8) + MOVE.W #,D1 (8) + ADD.W (4) + MOVE.W D1,abs (12/16)
  // + MOVE.B #,D0 (8) + TRAP #15 (0), per the original's tables.
  uint64_t expected = 8 + 8 + 4 + (abs_w ? 12 : 16) + 8;
  EXPECT_EQ(sim_.State().cycles, expected);
}

TEST_F(CoreIntegrationTest, InterruptDuringRun) {
  ASSERT_TRUE(AssembleAndLoad(R"(
        ORG     $1000
START:  MOVE.L  #ISR,$6C        ; autovector level 3
LOOP:   BRA     LOOP
ISR:    MOVE.W  #$BEEF,$3000
        MOVE.B  #9,D0
        TRAP    #15
        END     START
)",
                              sim_));
  std::atomic<bool> done{false};
  std::thread runner([&] {
    sim_.Run(50000000);
    done = true;
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  sim_.Interrupt(3);
  runner.join();
  ASSERT_TRUE(done);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x3000).value, 0xBEEFu);
  EXPECT_TRUE(sim_.State().halted);
}

TEST_F(CoreIntegrationTest, StopWakeCycle) {
  ASSERT_TRUE(AssembleAndLoad(R"(
        ORG     $1000
START:  MOVE.L  #ISR,$70        ; autovector level 4
        STOP    #$2000
        MOVE.W  #2,$3004
        MOVE.B  #9,D0
        TRAP    #15
ISR:    MOVE.W  #1,$3002
        RTE
        END     START
)",
                              sim_));
  EXPECT_EQ(sim_.Run(), SimResult::kStopInstruction);
  ASSERT_TRUE(sim_.State().stopped);
  sim_.Interrupt(4);
  EXPECT_EQ(sim_.Run(), SimResult::kHalted);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x3002).value, 1u);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x3004).value, 2u);
}

TEST_F(CoreIntegrationTest, LoggerCapturesFullTrace) {
  ASSERT_TRUE(AssembleAndLoad(kAddProgram, sim_));
  logger_.SetMode(IExecutionLogger::Mode::kInstructions);
  sim_.Run();
  ASSERT_EQ(logger_.pcs.size(), 6u);
  EXPECT_EQ(logger_.pcs.front(), 0x1000u);
  for (size_t i = 1; i < logger_.pcs.size(); ++i)
    EXPECT_GT(logger_.pcs[i], logger_.pcs[i - 1]);
}

TEST_F(CoreIntegrationTest, ObserverReceivesAllEvents) {
  ASSERT_TRUE(AssembleAndLoad(kAddProgram, sim_));
  EventLog log;
  sim_.AddObserver(&log);
  sim_.Run();
  ASSERT_EQ(log.events.size(), 7u);
  for (int i = 0; i < 6; ++i)
    EXPECT_EQ(log.events[i].type, SimEventType::kInstructionExecuted) << i;
  EXPECT_EQ(log.events[6].type, SimEventType::kHalted);
}

TEST_F(CoreIntegrationTest, SimhaltDisabledStillRuns) {
  const char* program = R"(
        ORG     $1000
START:  MOVE.L  #LINEF,$2C
        SIMHALT
        MOVE.W  #$55,$3000      ; reached only after the Line-F handler
        MOVE.B  #9,D0
        TRAP    #15
LINEF:  MOVE.W  #$AA,$3002
        ADDQ.L  #2,2(A7)        ; skip the second SIMHALT word
        RTE
        END     START
)";
  // Enabled (default): SIMHALT halts immediately.
  ASSERT_TRUE(AssembleAndLoad(program, sim_));
  EXPECT_EQ(sim_.Run(), SimResult::kHalted);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x3000).value, 0u);

  // Disabled: $FFFF is a Line-F exception; execution continues.
  sim_.SetSimhaltEnabled(false);
  ASSERT_TRUE(AssembleAndLoad(program, sim_));
  EXPECT_EQ(sim_.Run(), SimResult::kHalted);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x3002).value, 0xAAu);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x3000).value, 0x55u);
  sim_.SetSimhaltEnabled(true);
}

TEST_F(CoreIntegrationTest, ExceptionsDisabledNoTrap) {
  ASSERT_TRUE(AssembleAndLoad(R"(
        ORG     $1000
START:  MOVE.L  #HANDLER,$10
        ILLEGAL
        MOVE.W  #1,$3000
HANDLER MOVE.W  #2,$3002
        MOVE.B  #9,D0
        TRAP    #15
        END     START
)",
                              sim_));
  sim_.SetExceptionsEnabled(false);
  EXPECT_EQ(sim_.Run(), SimResult::kBadInstruction);
  EXPECT_TRUE(sim_.State().halted);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x3002).value, 0u);  // handler not entered
  sim_.SetExceptionsEnabled(true);
}

TEST_F(CoreIntegrationTest, MemoryProtectionWithInterrupt) {
  ASSERT_TRUE(AssembleAndLoad(R"(
        ORG     $1000
START:  MOVE.L  #ISR,$74        ; autovector level 5
        MOVE.L  #BUSERR,$08
        ANDI.W  #$D8FF,SR       ; user mode, interrupt mask 0
LOOP:   TST.W   PROTECT         ; supervisor-only area: bus error in user mode
        BRA     LOOP
BUSERR: MOVE.W  #$BAD,$3010     ; reached if the user-mode access faults
        BRA     LOOP2
LOOP2:  BRA     LOOP2
ISR:    MOVE.W  #$600D,PROTECT  ; supervisor write succeeds
        MOVE.B  #9,D0
        TRAP    #15
        ORG     $4000
PROTECT DS.W    1
        END     START
)",
                              sim_));
  M68kSimulator::MemoryMapConfig map;
  map.protected_ = {true, 0x4000, 0x4FFF};
  sim_.ConfigureMemoryMap(map);

  // User-mode access to the protected area raises a bus error...
  sim_.Run(10);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x3010).value, 0x0BADu);
  // ...while the supervisor-mode interrupt handler may write it.
  sim_.Interrupt(5);
  EXPECT_EQ(sim_.Run(1000), SimResult::kHalted);
  sim_.GetMemory().SetSupervisor(true);
  EXPECT_EQ(sim_.GetMemory().ReadWord(0x4000).value, 0x600Du);
}

TEST_F(CoreIntegrationTest, HeadlessTrap15IO) {
  ASSERT_TRUE(AssembleAndLoad(R"(
        ORG     $1000
START:  LEA     MSG,A1
        MOVE.B  #14,D0
        TRAP    #15
        MOVE.L  #42,D1
        MOVE.B  #3,D0
        TRAP    #15
        MOVE.B  #9,D0
        TRAP    #15
MSG:    DC.B    'Hello, 68000! ',0
        END     START
)",
                              sim_));
  EXPECT_EQ(sim_.Run(), SimResult::kHalted);
  EXPECT_EQ(text_.out, "Hello, 68000! 42");
}

}  // namespace
}  // namespace easym68k
