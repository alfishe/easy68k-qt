// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Execution logging tests (Task 9.4).

#include <gtest/gtest.h>

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include "easym68k/sim/iexecution_logger.h"
#include "easym68k/sim/itext_io.h"
#include "easym68k/sim/simulator.h"
#include "easym68k/sim/trap15.h"

namespace easym68k::sim {
namespace {

constexpr uint32_t kCode = 0x1000;
constexpr uint32_t kStack = 0x8000;

// Records every logger call in order.
class RecordingLogger : public ExecutionLoggerBase {
 public:
  struct Inst {
    uint32_t pc;
    uint16_t opcode;
    CpuState state;
  };
  struct Dump {
    uint32_t start;
    uint32_t length;
    uint8_t first_byte;
  };

  void LogInstruction(uint32_t pc, uint16_t opcode, const CpuState& state) override {
    insts.push_back({pc, opcode, state});
    order.push_back('I');
  }
  void LogMemoryDump(uint32_t start, uint32_t length, const Memory& memory) override {
    dumps.push_back({start, length, static_cast<uint8_t>(memory.ReadByte(start).value)});
    order.push_back('M');
  }
  void LogOutput(const char* text, int length) override { output.append(text, length); }

  std::vector<Inst> insts;
  std::vector<Dump> dumps;
  std::string order;
  std::string output;
};

// Minimal ITextIO that swallows output.
class NullTextIO : public ITextIO {
 public:
  void TextOut(std::string_view) override {}
  void TextOutCRLF(std::string_view) override {}
  void TextIn(char*, long*, long*) override {}
  void CharIn(char*) override {}
  void CharOut(char) override {}
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
};

class ExecutionLoggerTest : public ::testing::Test {
 protected:
  ExecutionLoggerTest() : sim_(MakeConfig()) {}

  M68kSimulator::Config MakeConfig() {
    M68kSimulator::Config c;
    c.execution_logger = &logger_;
    c.text_io = &text_io_;
    return c;
  }

  void Load(std::initializer_list<uint16_t> words) {
    Memory& m = sim_.GetMemory();
    m.Clear();
    m.WriteLong(0x0, kStack);
    m.WriteLong(0x4, kCode);
    uint32_t addr = kCode;
    for (uint16_t w : words) {
      m.WriteWord(addr, w);
      addr += 2;
    }
    sim_.Reset();
  }

  RecordingLogger logger_;
  NullTextIO text_io_;
  M68kSimulator sim_;
};

TEST(ExecutionLoggerNullTest, NullLoggerIsNoOp) {
  M68kSimulator sim{M68kSimulator::Config{}};
  sim.GetMemory().WriteLong(0x0, kStack);
  sim.GetMemory().WriteLong(0x4, kCode);
  sim.GetMemory().WriteWord(kCode, 0x7005);  // MOVEQ #5,D0
  sim.Reset();
  EXPECT_EQ(sim.Step(), SimResult::kOk);
  EXPECT_EQ(sim.State().d[0], 5u);
}

TEST_F(ExecutionLoggerTest, LogInstructionCalledPerStep) {
  logger_.SetMode(IExecutionLogger::Mode::kInstructions);
  Load({0x4E71, 0x4E71, 0x4E71});
  sim_.Step();
  sim_.Step();
  sim_.Step();
  EXPECT_EQ(logger_.insts.size(), 3u);
}

TEST_F(ExecutionLoggerTest, LogInstructionReceivesCorrectPc) {
  logger_.SetMode(IExecutionLogger::Mode::kInstructions);
  Load({0x7005, 0x4E71});
  sim_.Step();
  sim_.Step();
  ASSERT_EQ(logger_.insts.size(), 2u);
  EXPECT_EQ(logger_.insts[0].pc, kCode);
  EXPECT_EQ(logger_.insts[1].pc, kCode + 2);
}

TEST_F(ExecutionLoggerTest, LogInstructionReceivesCorrectOpcode) {
  logger_.SetMode(IExecutionLogger::Mode::kInstructions);
  Load({0x7005, 0x4E71});
  sim_.Step();
  sim_.Step();
  ASSERT_EQ(logger_.insts.size(), 2u);
  EXPECT_EQ(logger_.insts[0].opcode, 0x7005);
  EXPECT_EQ(logger_.insts[1].opcode, 0x4E71);
}

// The original logs registers before the instruction executes (RUN.CPP).
TEST_F(ExecutionLoggerTest, LogInstructionReceivesPreExecutionState) {
  logger_.SetMode(IExecutionLogger::Mode::kInstructionsAndRegisters);
  Load({0x7005});  // MOVEQ #5,D0
  sim_.State().d[0] = 0x11111111;
  sim_.Step();
  ASSERT_EQ(logger_.insts.size(), 1u);
  EXPECT_EQ(logger_.insts[0].state.d[0], 0x11111111u);  // before
  EXPECT_EQ(sim_.State().d[0], 5u);                     // after
}

TEST_F(ExecutionLoggerTest, ModeOffNoLogging) {
  logger_.SetMode(IExecutionLogger::Mode::kOff);
  Load({0x4E71});
  sim_.Step();
  EXPECT_TRUE(logger_.insts.empty());
  EXPECT_TRUE(logger_.dumps.empty());
}

TEST_F(ExecutionLoggerTest, ModeInstructionsOnly) {
  logger_.SetMode(IExecutionLogger::Mode::kInstructions);
  logger_.SetDumpRange(0x2000, 32);
  Load({0x4E71});
  sim_.Step();
  EXPECT_EQ(logger_.insts.size(), 1u);
  EXPECT_TRUE(logger_.dumps.empty());
}

TEST_F(ExecutionLoggerTest, ModeInstructionsAndRegistersNoDump) {
  logger_.SetMode(IExecutionLogger::Mode::kInstructionsAndRegisters);
  logger_.SetDumpRange(0x2000, 32);
  Load({0x4E71});
  sim_.Step();
  EXPECT_EQ(logger_.insts.size(), 1u);
  EXPECT_TRUE(logger_.dumps.empty());
}

TEST_F(ExecutionLoggerTest, ModeInstructionsRegistersAndMemory) {
  logger_.SetMode(IExecutionLogger::Mode::kInstructionsRegistersAndMemory);
  logger_.SetDumpRange(0x2000, 32);
  Load({0x4E71});
  sim_.Step();
  EXPECT_EQ(logger_.insts.size(), 1u);
  EXPECT_EQ(logger_.dumps.size(), 1u);
  EXPECT_EQ(logger_.order, "MI");  // memory rows precede the instruction line
}

TEST_F(ExecutionLoggerTest, DumpRangeConfigurable) {
  logger_.SetMode(IExecutionLogger::Mode::kInstructionsRegistersAndMemory);
  logger_.SetDumpRange(0x2010, 48);
  Load({0x4E71});
  sim_.GetMemory().WriteByte(0x2010, 0xAB);
  sim_.Step();
  ASSERT_EQ(logger_.dumps.size(), 1u);
  EXPECT_EQ(logger_.dumps[0].start, 0x2010u);
  EXPECT_EQ(logger_.dumps[0].length, 48u);
  EXPECT_EQ(logger_.dumps[0].first_byte, 0xAB);
}

TEST_F(ExecutionLoggerTest, ModeSwitchAtRuntime) {
  Load({0x4E71, 0x4E71, 0x4E71});
  logger_.SetMode(IExecutionLogger::Mode::kOff);
  sim_.Step();
  logger_.SetMode(IExecutionLogger::Mode::kInstructions);
  sim_.Step();
  logger_.SetMode(IExecutionLogger::Mode::kOff);
  sim_.Step();
  ASSERT_EQ(logger_.insts.size(), 1u);
  EXPECT_EQ(logger_.insts[0].pc, kCode + 2);
}

TEST_F(ExecutionLoggerTest, LogOutputCapturesText) {
  logger_.SetOutputLogging(true);
  Load({0x4E4F});  // TRAP #15
  const char kMsg[] = "HELLO";
  for (int i = 0; i < 5; ++i)
    sim_.GetMemory().WriteByte(0x2000 + i, static_cast<uint8_t>(kMsg[i]));
  sim_.State().a[1] = 0x2000;
  sim_.State().d[1] = 5;
  sim_.State().d[0] = kTrap15DisplayStringCRLF;  // task 0
  sim_.Step();
  EXPECT_EQ(logger_.output, "HELLO\r\n");
}

TEST_F(ExecutionLoggerTest, LogOutputCapturesChar) {
  logger_.SetOutputLogging(true);
  Load({0x4E4F});  // TRAP #15
  sim_.State().d[1] = 'Z';
  sim_.State().d[0] = kTrap15DisplayChar;  // task 6
  sim_.Step();
  EXPECT_EQ(logger_.output, "Z");
}

TEST_F(ExecutionLoggerTest, LogOutputCapturesNumber) {
  logger_.SetOutputLogging(true);
  Load({0x4E4F});  // TRAP #15
  sim_.State().d[1] = static_cast<uint32_t>(-42);
  sim_.State().d[0] = kTrap15DisplayNumber;  // task 3
  sim_.Step();
  EXPECT_EQ(logger_.output, "-42");
}

TEST_F(ExecutionLoggerTest, LogOutputDisabledCapturesNothing) {
  logger_.SetOutputLogging(false);
  logger_.SetMode(IExecutionLogger::Mode::kInstructions);
  Load({0x4E4F});
  sim_.State().d[1] = 'Z';
  sim_.State().d[0] = kTrap15DisplayChar;
  sim_.Step();
  EXPECT_TRUE(logger_.output.empty());
}

TEST_F(ExecutionLoggerTest, OutputLoggingIndependentOfExecutionMode) {
  logger_.SetMode(IExecutionLogger::Mode::kOff);
  logger_.SetOutputLogging(true);
  Load({0x4E4F});
  sim_.State().d[1] = 'Q';
  sim_.State().d[0] = kTrap15DisplayChar;
  sim_.Step();
  EXPECT_EQ(logger_.output, "Q");
  EXPECT_TRUE(logger_.insts.empty());
}

TEST_F(ExecutionLoggerTest, MultipleStepsLogMultiple) {
  logger_.SetMode(IExecutionLogger::Mode::kInstructions);
  Load({0x4E71, 0x4E71, 0x4E71, 0x4E71, 0x4E71, 0x4E71, 0x4E71, 0x4E71, 0x4E71, 0x4E71});
  for (int i = 0; i < 10; ++i)
    sim_.Step();
  EXPECT_EQ(logger_.insts.size(), 10u);
}

TEST_F(ExecutionLoggerTest, InterruptEntryIsNotLogged) {
  logger_.SetMode(IExecutionLogger::Mode::kInstructions);
  Load({0x4E71});
  sim_.GetMemory().WriteLong((24 + 7) * 4, 0x3000);
  sim_.Interrupt(7);
  sim_.Step();  // services the IRQ; no instruction executes
  EXPECT_TRUE(logger_.insts.empty());
}

}  // namespace
}  // namespace easym68k::sim
