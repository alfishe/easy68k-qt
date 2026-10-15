// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Quality gate 13.1 (part b): peripheral (mouse/keyboard IRQ), simulator
// environment (time, delay, addresses, automatic IRQs, window), task 58
// file dialog and printing back ends.

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QPdfWriter>
#include <QTemporaryDir>
#include <thread>
#include <vector>

#include "easym68k/sim/types.h"
#include "qt_file_io.h"
#include "qt_peripheral_io.h"
#include "qt_print_io.h"
#include "qt_simulator_env.h"
#include "sim_io_screen.h"

namespace easym68k::gui {
namespace {

// Runs the event loop until `done` or a timeout.
template <typename Pred>
bool ProcessEventsUntil(Pred done, int timeout_ms = 3000) {
  QElapsedTimer timer;
  timer.start();
  while (!done()) {
    if (timer.elapsed() > timeout_ms)
      return false;
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  }
  return true;
}

// ---- Peripheral -------------------------------------------------------------

TEST(QtPeripheralIOTest, MouseStates) {
  QtPeripheralIO p(nullptr);
  QtPeripheralIO::Buttons down;
  down.left = true;
  down.shift = true;
  p.MouseDown(10, 20, down);
  long state = -1, pos = -1;
  p.ReadMouse(0, &state, &pos);
  EXPECT_EQ(state, 0x11);
  EXPECT_EQ(pos, (20L << 16) | 10);
  p.MouseMove(30, 40, false, false, true);  // buttons kept, modifiers updated
  p.ReadMouse(0, &state, &pos);
  EXPECT_EQ(state, 0x41);
  EXPECT_EQ(pos, (40L << 16) | 30);
  QtPeripheralIO::Buttons up;
  up.right = true;
  up.double_click = true;
  p.MouseUp(50, 60, up);
  p.ReadMouse(1, &state, &pos);  // at the last button up
  EXPECT_EQ(state, 0x0A);
  EXPECT_EQ(pos, (60L << 16) | 50);
  p.ReadMouse(2, &state, &pos);  // at the last button down
  EXPECT_EQ(state, 0x11);
  EXPECT_EQ(pos, (20L << 16) | 10);
  pos = 1234;
  p.ReadMouse(3, &state, &pos);  // reserved
  EXPECT_EQ(state, 0);
  EXPECT_EQ(pos, 1234);
  p.Reset();
  p.ReadMouse(0, &state, &pos);
  EXPECT_EQ(state, 0);
  EXPECT_EQ(pos, 0);
}

TEST(QtPeripheralIOTest, MouseAndKeyInterrupts) {
  std::vector<int> irqs;
  QtPeripheralIO p([&](int level) { irqs.push_back(level); });
  QtPeripheralIO::Buttons b;
  p.MouseDown(0, 0, b);  // no IRQ configured
  EXPECT_TRUE(irqs.empty());
  p.SetMouseIRQ(3, 0x07);
  p.SetMouseIRQ(9, 0x07);  // invalid level: ignored
  p.MouseDown(0, 0, b);
  p.MouseUp(0, 0, b);
  p.MouseMove(1, 1, false, false, false);
  EXPECT_EQ(irqs, (std::vector<int>{3, 3, 3}));
  p.SetMouseIRQ(0, 0x04);  // move off
  p.MouseMove(2, 2, false, false, false);
  EXPECT_EQ(irqs.size(), 3u);
  // Keyboard IRQs come from the I/O window's key hook.
  SimIoScreen screen;
  screen.SetKeyEventHook([&](bool down) { p.KeyEvent(down); });
  p.SetKeyIRQ(5, 0x01);
  screen.KeyDown(0x41);
  screen.KeyUp(0x41);
  EXPECT_EQ(irqs.back(), 5);
  EXPECT_EQ(irqs.size(), 4u);
  p.SetKeyIRQ(6, 0x02);
  screen.KeyUp(0x41);
  EXPECT_EQ(irqs.back(), 6);
  p.Reset();
  screen.KeyDown(0x41);
  EXPECT_EQ(irqs.size(), 5u);
}

// ---- Simulator environment -----------------------------------------------------

TEST(QtSimulatorEnvTest, TimeVersionAddressesWindow) {
  SimIoScreen screen;
  QtSimulatorEnv env(&screen, nullptr);
  const uint32_t t = env.GetTimeHundredths();
  EXPECT_LT(t, 24u * 60 * 60 * 100);
  EXPECT_EQ(env.GetVersion(), 0x00051001u);
  EXPECT_EQ(env.GetSeg7Address(), 0xE00000u);
  EXPECT_EQ(env.GetLEDAddress(), 0xE00010u);
  EXPECT_EQ(env.GetSwitchAddress(), 0xE00012u);
  EXPECT_EQ(env.GetPushButtonAddress(), 0xE00014u);
  QtSimulatorEnv::HardwareAddresses a;
  a.seg7 = 0xD00000;
  env.SetHardwareAddresses(a);
  EXPECT_EQ(env.GetSeg7Address(), 0xD00000u);
  env.SetWindowSize(1024, 768);
  unsigned short w = 0, h = 0;
  env.GetWindowSize(&w, &h);
  EXPECT_EQ(w, 1024);
  EXPECT_EQ(h, 768);
}

TEST(QtSimulatorEnvTest, RequestsAreSignalled) {
  SimIoScreen screen;
  QtSimulatorEnv env(&screen, nullptr);
  int hardware = 0;
  bool fullscreen = false, exceptions = false;
  QObject::connect(&env, &QtSimulatorEnv::ShowHardwareRequested, [&] { hardware++; });
  QObject::connect(&env, &QtSimulatorEnv::FullScreenRequested, [&](bool f) { fullscreen = f; });
  QObject::connect(&env, &QtSimulatorEnv::ExceptionsEnabledChanged,
                   [&](bool e) { exceptions = e; });
  env.ShowHardware();
  env.SetupWindow(true);
  env.SetExceptionsEnabled(true);
  EXPECT_EQ(hardware, 1);
  EXPECT_TRUE(fullscreen);
  EXPECT_TRUE(exceptions);
}

TEST(QtSimulatorEnvTest, DelayWaitsAndCanBeCancelled) {
  SimIoScreen screen;
  QtSimulatorEnv env(&screen, nullptr);
  QElapsedTimer timer;
  timer.start();
  env.DelayHundredths(5);
  EXPECT_GE(timer.elapsed(), 45);
  timer.restart();
  std::thread sim([&] { env.DelayHundredths(100000); });  // 1000 s
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  env.CancelDelay();
  sim.join();
  EXPECT_LT(timer.elapsed(), 2000);
  env.DelayHundredths(100000);  // still cancelled: returns at once
  env.ResumeDelay();
  timer.restart();
  env.DelayHundredths(2);
  EXPECT_GE(timer.elapsed(), 15);
}

TEST(QtSimulatorEnvTest, AutomaticInterrupts) {
  SimIoScreen screen;
  std::vector<int> irqs;
  QtSimulatorEnv env(&screen, [&](int level) { irqs.push_back(level); });
  env.SetAutoIRQ(0x82, 5);  // IRQ 2 every 5 ms, but the hardware is disabled
  QCoreApplication::processEvents();
  EXPECT_TRUE(env.GetAutoIrq(2).checked);
  EXPECT_EQ(env.GetAutoIrq(2).interval_ms, 5);
  EXPECT_FALSE(env.AutoIrqRunning(2));
  env.SetHardwareEnabled(true);
  EXPECT_TRUE(env.AutoIrqRunning(2));
  EXPECT_TRUE(ProcessEventsUntil([&] { return irqs.size() >= 2; }));
  EXPECT_EQ(irqs.front(), 2);
  env.SetAutoIRQ(0x02, 0);  // IRQ 2 off
  EXPECT_FALSE(env.AutoIrqRunning(2));
  EXPECT_FALSE(env.GetAutoIrq(2).checked);
  // From the simulator thread: queued to the GUI thread.
  std::thread sim([&] { env.SetAutoIRQ(0x87, 1000); });
  sim.join();
  EXPECT_TRUE(ProcessEventsUntil([&] { return env.AutoIrqRunning(7); }));
  env.SetAutoIRQ(0x83, 1000);
  env.SetAutoIRQ(0x00, 0);  // all off; automatic mode stays on
  EXPECT_FALSE(env.AutoIrqRunning(3));
  EXPECT_FALSE(env.AutoIrqRunning(7));
  env.SetAutoIRQ(0x84, 1000);
  EXPECT_TRUE(env.AutoIrqRunning(4));
  env.AutoIrqOff();
  EXPECT_FALSE(env.AutoIrqRunning(4));
  EXPECT_TRUE(env.GetAutoIrq(4).checked);  // check box kept
}

// ---- File dialog -----------------------------------------------------------------

class QtFileIOTest : public ::testing::Test {
 protected:
  QtFileIOTest() : memory_(sim::kMemorySize, 0) {}

  void Put(uint32_t address, const char* text) {
    std::memcpy(memory_.data() + address, text, std::strlen(text) + 1);
  }
  std::string Get(uint32_t address) const {
    return reinterpret_cast<const char*>(memory_.data() + address);
  }

  std::vector<uint8_t> memory_;
  QObject gui_;
};

TEST_F(QtFileIOTest, OpenDialogWritesTheChosenName) {
  QtFileIO::DialogRequest seen;
  int cleared = 0;
  QtFileIO io(
      memory_.data(), &gui_,
      [&](const QtFileIO::DialogRequest& r) {
        seen = r;
        return std::optional<QString>(QStringLiteral("/tmp/new.txt"));
      },
      [&] { cleared++; });
  Put(0x1000, "Pick a file");
  Put(0x1100, "*.txt;*.dat");
  Put(0x1200, "old_name.txt");
  memory_[0x1200 + 14] = 0xAA;  // past the new name's terminator
  long mode = 0;
  short result = -1;
  io.DisplayFileDialog(&mode, 0x1000, 0x1100, 0x1200, &result);
  EXPECT_EQ(result, sim::kFileSuccess);
  EXPECT_EQ(mode, 1);
  EXPECT_FALSE(seen.save);
  EXPECT_EQ(seen.title, QStringLiteral("Pick a file"));
  EXPECT_EQ(seen.filter, QStringLiteral("*.txt;*.dat (*.txt *.dat)"));
  EXPECT_EQ(seen.path, QStringLiteral("old_name.txt"));
  EXPECT_EQ(Get(0x1200), "/tmp/new.txt");
  EXPECT_EQ(memory_[0x1200 + 14], 0xAA);  // no zero fill beyond the name
  EXPECT_EQ(cleared, 1);
}

TEST_F(QtFileIOTest, SaveCancelAndOtherModes) {
  QtFileIO::DialogRequest seen;
  bool cancel = true;
  QtFileIO io(memory_.data(), &gui_, [&](const QtFileIO::DialogRequest& r) {
    seen = r;
    return cancel ? std::nullopt : std::optional<QString>(QStringLiteral("out.bin"));
  });
  long mode = 1;
  short result = -1;
  io.DisplayFileDialog(&mode, 0, 0, 0x2000, &result);  // no title, no filter
  EXPECT_TRUE(seen.save);
  EXPECT_TRUE(seen.title.isEmpty());
  EXPECT_TRUE(seen.filter.isEmpty());
  EXPECT_EQ(mode, 0);  // cancelled
  EXPECT_EQ(result, sim::kFileSuccess);
  cancel = false;
  mode = 1;
  io.DisplayFileDialog(&mode, 0, 0, 0x2000, &result);
  EXPECT_EQ(mode, 1);
  EXPECT_EQ(Get(0x2000), "out.bin");
  mode = 7;  // not a dialog mode: nothing happens
  io.DisplayFileDialog(&mode, 0, 0, 0x2000, &result);
  EXPECT_EQ(mode, 7);
  EXPECT_EQ(result, sim::kFileSuccess);
}

TEST_F(QtFileIOTest, NameThatDoesNotFitIsAnError) {
  QtFileIO io(memory_.data(), &gui_, [](const QtFileIO::DialogRequest&) {
    return std::optional<QString>(QStringLiteral("a_long_file_name.txt"));
  });
  long mode = 0;
  short result = 0;
  io.DisplayFileDialog(&mode, 0, 0, sim::kMemorySize - 4, &result);
  EXPECT_EQ(result, sim::kFileError);
}

TEST_F(QtFileIOTest, DialogFromTheSimulatorThreadRunsOnTheGuiThread) {
  std::thread::id dialog_thread;
  QtFileIO io(memory_.data(), &gui_, [&](const QtFileIO::DialogRequest&) {
    dialog_thread = std::this_thread::get_id();
    return std::optional<QString>(QStringLiteral("x"));
  });
  long mode = 0;
  short result = -1;
  std::atomic<bool> done{false};
  std::thread sim([&] {
    io.DisplayFileDialog(&mode, 0, 0, 0x3000, &result);
    done = true;
  });
  EXPECT_TRUE(ProcessEventsUntil([&] { return done.load(); }));
  sim.join();
  EXPECT_EQ(dialog_thread, std::this_thread::get_id());
  EXPECT_EQ(mode, 1);
}

TEST_F(QtFileIOTest, FilesUseTheBaseDirectory) {
  QTemporaryDir dir;
  QtFileIO io(memory_.data(), &gui_, nullptr);
  io.SetBaseDir(QDir::toNativeSeparators(dir.path()).toStdString());
  long fid = -1;
  short result = -1;
  io.NewFile(&fid, "data.bin", &result);
  EXPECT_EQ(result, sim::kFileSuccess);
  io.WriteFile(fid, "abc", 3, &result);
  io.CloseFile(fid, &result);
  EXPECT_TRUE(QFile::exists(dir.filePath(QStringLiteral("data.bin"))));
}

// ---- Printing -----------------------------------------------------------------------

class QtPrintIOTest : public ::testing::Test {
 protected:
  QtPrintIO::DeviceFactory Factory() {
    return [this]() -> std::unique_ptr<QPagedPaintDevice> {
      const QString path = dir_.filePath(QStringLiteral("job%1.pdf").arg(jobs_++));
      files_.push_back(path);
      return std::make_unique<QPdfWriter>(path);
    };
  }
  void Print(QtPrintIO& io, const std::string& text) {
    for (char c : text)
      io.PrintChar(c);
  }
  bool Written(int job) const { return QFile(files_.at(job)).size() > 0; }

  QTemporaryDir dir_;
  int jobs_ = 0;
  std::vector<QString> files_;
  QObject gui_;
};

TEST_F(QtPrintIOTest, FormFeedEndsTheJob) {
  QtPrintIO io(&gui_, Factory(), QtPrintIO::DefaultFont());
  Print(io, "Hello\r\nworld\f");
  EXPECT_EQ(jobs_, 1);
  EXPECT_TRUE(Written(0));
  Print(io, "second\f");
  EXPECT_EQ(jobs_, 2);
  EXPECT_TRUE(Written(1));
}

TEST_F(QtPrintIOTest, FormFeedWithoutTextPrintsABlankPage) {
  QtPrintIO io(&gui_, Factory(), QtPrintIO::DefaultFont());
  io.FormFeed();
  EXPECT_EQ(jobs_, 1);
  EXPECT_TRUE(Written(0));
}

TEST_F(QtPrintIOTest, EndJobPrintsAnOpenJob) {
  QtPrintIO io(&gui_, Factory(), QtPrintIO::DefaultFont());
  Print(io, "no form feed");
  EXPECT_EQ(jobs_, 1);
  io.EndJob();
  EXPECT_TRUE(Written(0));
  io.EndJob();  // nothing open
  EXPECT_EQ(jobs_, 1);
}

TEST_F(QtPrintIOTest, BottomOfThePageStartsANewJob) {
  QtPrintIO io(&gui_, Factory(), QtPrintIO::DefaultFont());
  Print(io, "top");
  Print(io, std::string(200, '\n'));
  Print(io, "next page\t\b\v\x80");
  EXPECT_GE(jobs_, 2);
  io.EndJob();
  for (int i = 0; i < jobs_; i++)
    EXPECT_TRUE(Written(i)) << i;
}

TEST_F(QtPrintIOTest, NoPrinterDropsCharacters) {
  QtPrintIO io(
      &gui_, [] { return std::unique_ptr<QPagedPaintDevice>(); }, QtPrintIO::DefaultFont());
  Print(io, "text\f");
  io.FormFeed();
  io.EndJob();
  SUCCEED();
}

TEST_F(QtPrintIOTest, CharactersFromTheSimulatorThreadAreQueued) {
  QtPrintIO io(&gui_, Factory(), QtPrintIO::DefaultFont());
  std::thread sim([&] {
    for (char c : std::string("queued\f"))
      io.PrintChar(c);
  });
  sim.join();
  EXPECT_EQ(jobs_, 0);  // not yet run on the GUI thread
  EXPECT_TRUE(ProcessEventsUntil([&] { return jobs_ == 1 && Written(0); }));
}

}  // namespace
}  // namespace easym68k::gui
