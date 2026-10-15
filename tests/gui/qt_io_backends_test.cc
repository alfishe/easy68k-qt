// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Quality gate 13.1: the Qt back ends plugged into the simulator. The
// program runs on a worker thread (as in Sim68K-Qt); output, graphics and
// the blocking input path are exercised across threads.

#include <gtest/gtest.h>

#include <chrono>
#include <cstring>
#include <thread>
#include <vector>

#include "easym68k/sim/simulator.h"
#include "qt_graphics_io.h"
#include "qt_text_io.h"
#include "sim_io_screen.h"

namespace easym68k::gui {
namespace {

using sim::M68kSimulator;
using sim::SimResult;

constexpr uint32_t kOrigin = 0x1000;

class QtIoBackendsTest : public ::testing::Test {
 protected:
  QtIoBackendsTest() : text_io_(&screen_), graphics_io_(&screen_), sim_(MakeConfig()) {}

  M68kSimulator::Config MakeConfig() {
    M68kSimulator::Config config;
    config.text_io = &text_io_;
    config.graphics_io = &graphics_io_;
    return config;
  }

  void Load(const std::vector<uint8_t>& code) {
    sim_.GetMemory().LoadData(kOrigin, code);
    sim_.SetStartAddress(kOrigin);
    sim_.InitProgram();
  }

  SimResult RunToHalt() { return sim_.Run(1000); }

  SimIoScreen screen_;
  QtTextIO text_io_;
  QtGraphicsIO graphics_io_;
  M68kSimulator sim_;
};

TEST_F(QtIoBackendsTest, TextOutputTasks) {
  Load({
      0x70, 0x06,              // MOVEQ #6,D0     display char
      0x72, 0x41,              // MOVEQ #'A',D1
      0x4E, 0x4F,              // TRAP #15
      0x70, 0x03,              // MOVEQ #3,D0     display number
      0x72, 0xF9,              // MOVEQ #-7,D1
      0x4E, 0x4F,              // TRAP #15
      0x70, 0x0B,              // MOVEQ #11,D0    cursor to col 5, row 2
      0x32, 0x3C, 0x05, 0x02,  // MOVE.W #$0502,D1
      0x4E, 0x4F,              // TRAP #15
      0x70, 0x06,              // MOVEQ #6,D0
      0x72, 0x5A,              // MOVEQ #'Z',D1
      0x4E, 0x4F,              // TRAP #15
      0x70, 0x0B,              // MOVEQ #11,D0    get cursor
      0x32, 0x3C, 0x00, 0xFF,  // MOVE.W #$00FF,D1
      0x4E, 0x4F,              // TRAP #15
      0x70, 0x09,              // MOVEQ #9,D0
      0x4E, 0x4F,              // TRAP #15
  });
  EXPECT_EQ(RunToHalt(), SimResult::kHalted);
  EXPECT_EQ(screen_.CharAt(0, 0), 'A');
  EXPECT_EQ(screen_.CharAt(0, 1), '-');
  EXPECT_EQ(screen_.CharAt(0, 2), '7');
  EXPECT_EQ(screen_.CharAt(2, 5), 'Z');
  EXPECT_EQ(sim_.State().d[1] & 0xFFFF, 0x0602u);  // col 6, row 2
}

TEST_F(QtIoBackendsTest, GraphicsTasks) {
  Load({
      0x70, 0x50,  // MOVEQ #80,D0    pen colour
      0x72, 0xFF,  // MOVEQ #-1,D1    (white; high byte ignored)
      0x4E, 0x4F,  // TRAP #15
      0x70, 0x54,  // MOVEQ #84,D0    line 10,20 -> 30,20
      0x72, 0x0A,  // MOVEQ #10,D1
      0x74, 0x14,  // MOVEQ #20,D2
      0x76, 0x1E,  // MOVEQ #30,D3
      0x78, 0x14,  // MOVEQ #20,D4
      0x4E, 0x4F,  // TRAP #15
      0x70, 0x60,  // MOVEQ #96,D0    get pen position
      0x4E, 0x4F,  // TRAP #15
      0x70, 0x53,  // MOVEQ #83,D0    get pixel 30,20
      0x72, 0x1E,  // MOVEQ #30,D1
      0x74, 0x14,  // MOVEQ #20,D2
      0x4E, 0x4F,  // TRAP #15
      0x2A, 0x00,  // MOVE.L D0,D5
      0x70, 0x09,  // MOVEQ #9,D0
      0x4E, 0x4F,  // TRAP #15
  });
  EXPECT_EQ(RunToHalt(), SimResult::kHalted);
  EXPECT_EQ(sim_.State().d[5], 0x00FFFFFFu);
  EXPECT_EQ(screen_.GetPixel(10, 20), 0x00FFFFFFu);
  EXPECT_EQ(sim_.State().d[1] & 0xFFFF, 30u);  // pen X
  EXPECT_EQ(sim_.State().d[2] & 0xFFFF, 20u);  // pen Y
}

TEST_F(QtIoBackendsTest, StringAndNumberInputAcrossThreads) {
  Load({
      0x70, 0x02,                          // MOVEQ #2,D0     read string
      0x43, 0xF9, 0x00, 0x00, 0x20, 0x00,  // LEA $2000,A1
      0x4E, 0x4F,                          // TRAP #15
      0x26, 0x01,                          // MOVE.L D1,D3    length
      0x70, 0x04,                          // MOVEQ #4,D0     read number
      0x4E, 0x4F,                          // TRAP #15
      0x70, 0x09,                          // MOVEQ #9,D0
      0x4E, 0x4F,                          // TRAP #15
  });
  SimResult result = SimResult::kOk;
  std::thread worker([&] { result = RunToHalt(); });
  auto type = [&](const char* keys) {
    for (int i = 0; i < 2000 && !screen_.InputActive(); i++)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    ASSERT_TRUE(screen_.InputActive());
    for (const char* k = keys; *k; k++)
      screen_.KeyPress(*k);
  };
  type("hello\r");
  type("-123\r");
  worker.join();
  EXPECT_EQ(result, SimResult::kHalted);
  const uint8_t* mem = sim_.GetMemory().RawData();
  EXPECT_EQ(std::memcmp(mem + 0x2000, "hello", 6), 0);  // with terminator
  EXPECT_EQ(sim_.State().d[3], 5u);
  EXPECT_EQ(static_cast<int32_t>(sim_.State().d[1]), -123);
  EXPECT_EQ(screen_.CharAt(0, 0), 'h');
  EXPECT_EQ(screen_.CharAt(1, 0), '-');
}

TEST_F(QtIoBackendsTest, InputAtTheTopOfMemoryWraps) {
  Load({
      0x70,
      0x02,  // MOVEQ #2,D0
      0x43,
      0xF9,
      0x00,
      0xFF,
      0xFF,
      0xFE,  // LEA $FFFFFE,A1
      0x4E,
      0x4F,  // TRAP #15
      0x70,
      0x09,  // MOVEQ #9,D0
      0x4E,
      0x4F,  // TRAP #15
  });
  std::thread worker([&] { RunToHalt(); });
  for (int i = 0; i < 2000 && !screen_.InputActive(); i++)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  for (char k : std::string("abc\r"))
    screen_.KeyPress(k);
  worker.join();
  const uint8_t* mem = sim_.GetMemory().RawData();
  EXPECT_EQ(mem[0xFFFFFE], 'a');
  EXPECT_EQ(mem[0xFFFFFF], 'b');
  EXPECT_EQ(mem[0x000000], 'c');
  EXPECT_EQ(mem[0x000001], 0);
}

TEST_F(QtIoBackendsTest, CancelledInputLeavesRegistersAndMemory) {
  Load({
      0x72,
      0x07,  // MOVEQ #7,D1
      0x70,
      0x02,  // MOVEQ #2,D0
      0x43,
      0xF9,
      0x00,
      0x00,
      0x20,
      0x00,  // LEA $2000,A1
      0x4E,
      0x4F,  // TRAP #15
      0x70,
      0x09,  // MOVEQ #9,D0
      0x4E,
      0x4F,  // TRAP #15
  });
  std::thread worker([&] { RunToHalt(); });
  for (int i = 0; i < 2000 && !screen_.InputActive(); i++)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  screen_.CancelInput();
  worker.join();
  EXPECT_EQ(sim_.State().d[1], 7u);
  EXPECT_EQ(sim_.GetMemory().RawData()[0x2000], 0);
}

}  // namespace
}  // namespace easym68k::gui
