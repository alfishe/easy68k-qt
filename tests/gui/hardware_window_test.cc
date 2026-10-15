// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// HardwareWindow (Task 13.8): the 7-segment digits and LEDs show memory,
// switches and push buttons change it, the device addresses, IRQ and Reset
// buttons, automatic interrupts and the memory map as hardwareu.cpp.

#include "hardware_window.h"

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "easym68k/asm/assembler.h"
#include "qt_simulator_env.h"
#include "sim_session.h"

namespace easym68k::gui {
namespace {

using RunState = SimSession::RunState;

template <typename Pred>
bool Until(Pred done, int timeout_ms = 5000) {
  QElapsedTimer t;
  t.start();
  while (!done()) {
    if (t.elapsed() > timeout_ms)
      return false;
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
  }
  return true;
}

// Reset vectors: SSP = $8000, PC = START.
const char kProgram[] =
    "        ORG     0\n"
    "        DC.L    $8000,START\n"
    "        ORG     $1000\n"
    "START   MOVEQ   #1,D0\n"
    "        MOVEQ   #2,D1\n"
    "        SIMHALT\n"
    "        END     START\n";

class HardwareWindowTest : public ::testing::Test {
 protected:
  void SetUp() override {
    window_ = std::make_unique<HardwareWindow>(&session_);
    window_->show();
  }

  QString Assemble(const QString& name, const std::string& source) {
    asm_::Assembler assembler;
    assembler.ConfigureListing(name.toStdString() + ".X68", "");
    auto r = assembler.Assemble(source);
    EXPECT_TRUE(r.success) << (r.errors.empty() ? "" : r.errors[0].message);
    const QString base = dir_.filePath(name);
    QFile s68(base + QStringLiteral(".S68"));
    s68.open(QIODevice::WriteOnly);
    s68.write(r.srecord.c_str());
    QFile l68(base + QStringLiteral(".L68"));
    l68.open(QIODevice::WriteOnly);
    l68.write(assembler.GetListing().c_str());
    return base + QStringLiteral(".S68");
  }

  uint8_t* Memory() { return session_.Simulator().GetMemory().RawData(); }

  QTemporaryDir dir_;
  SimSession session_;
  std::unique_ptr<HardwareWindow> window_;
};

TEST_F(HardwareWindowTest, DigitsAndLedsShowMemory) {
  ASSERT_TRUE(session_.Open(Assemble("show", kProgram)));
  for (int i = 0; i < 16; i++)
    Memory()[0xE00000 + i] = static_cast<uint8_t>(0x10 + i);
  Memory()[0xE00010] = 0xA5;
  window_->Refresh();
  EXPECT_EQ(window_->SegmentByte(7), 0x10);  // left digit: the 7-segment address
  EXPECT_EQ(window_->SegmentByte(6), 0x12);  // every other byte
  EXPECT_EQ(window_->SegmentByte(0), 0x1E);
  EXPECT_EQ(window_->LedByte(), 0xA5);
  // updateIfNeeded(): writes near the devices refresh the display.
  Memory()[0xE00010] = 0x3C;
  window_->RefreshIfNeeded(0xE00000 - 8);
  EXPECT_EQ(window_->LedByte(), 0xA5);
  window_->RefreshIfNeeded(0xE0000C);
  EXPECT_EQ(window_->LedByte(), 0x3C);
}

TEST_F(HardwareWindowTest, SwitchesAndButtonsChangeMemory) {
  ASSERT_TRUE(session_.Open(Assemble("switches", kProgram)));
  window_->Initialize();
  EXPECT_EQ(Memory()[0xE00012], 0x00);  // OpenFile(): switches off
  QSignalSpy changed(window_.get(), &HardwareWindow::MemoryChanged);
  QTest::mouseClick(window_->Switch(0), Qt::LeftButton);
  QTest::mouseClick(window_->Switch(7), Qt::LeftButton);
  EXPECT_EQ(Memory()[0xE00012], 0x81);
  EXPECT_TRUE(window_->Switch(7)->isChecked());
  QTest::mouseClick(window_->Switch(0), Qt::LeftButton);
  EXPECT_EQ(Memory()[0xE00012], 0x80);
  EXPECT_EQ(changed.count(), 3);
  // A pressed button reads 0 until released (mouse or any key).
  EXPECT_EQ(Memory()[0xE00014], 0xFF);
  QTest::mousePress(window_->PushButton(2), Qt::LeftButton);
  EXPECT_EQ(Memory()[0xE00014], 0xFB);
  QTest::mouseRelease(window_->PushButton(2), Qt::LeftButton);
  EXPECT_EQ(Memory()[0xE00014], 0xFF);
  QTest::keyPress(window_->PushButton(5), Qt::Key_A);
  EXPECT_EQ(Memory()[0xE00014], 0xDF);
  QTest::keyRelease(window_->PushButton(5), Qt::Key_A);
  EXPECT_EQ(Memory()[0xE00014], 0xFF);
  // Initialize(): a newly opened program sees the switches off.
  window_->Initialize();
  EXPECT_FALSE(window_->Switch(7)->isChecked());
}

TEST_F(HardwareWindowTest, AddressesConflictsAndTask32) {
  EXPECT_EQ(window_->AddressField(HardwareWindow::kSeg7)->text(), QStringLiteral("E00000"));
  EXPECT_FALSE(window_->AddressConflict(HardwareWindow::kLeds));
  window_->AddressField(HardwareWindow::kLeds)->setText(QStringLiteral("E00004"));
  EXPECT_EQ(window_->Address(HardwareWindow::kLeds), 0xE00004u);
  EXPECT_EQ(session_.Env().GetLEDAddress(), 0xE00004u);  // Trap #15 task 32.2
  EXPECT_TRUE(window_->AddressConflict(HardwareWindow::kLeds));
  EXPECT_TRUE(window_->AddressConflict(HardwareWindow::kSeg7));
  window_->AddressField(HardwareWindow::kLeds)->setText(QStringLiteral("E00010"));
  EXPECT_FALSE(window_->AddressConflict(HardwareWindow::kSeg7));
  // Sixteen bytes are needed for the digits.
  window_->AddressField(HardwareWindow::kSeg7)->setText(QStringLiteral("FFFFF1"));
  EXPECT_TRUE(window_->AddressConflict(HardwareWindow::kSeg7));
  window_->AddressField(HardwareWindow::kSeg7)->setText(QStringLiteral("FFFFF0"));
  EXPECT_FALSE(window_->AddressConflict(HardwareWindow::kSeg7));
  // Hex digits only, upper case.
  window_->AddressField(HardwareWindow::kButtons)->clear();
  QTest::keyClicks(window_->AddressField(HardwareWindow::kButtons), QStringLiteral("e0g02a"));
  EXPECT_EQ(window_->AddressField(HardwareWindow::kButtons)->text(), QStringLiteral("E002A"));
  EXPECT_EQ(session_.Env().GetPushButtonAddress(), 0xE002Au);
}

TEST_F(HardwareWindowTest, IrqButtonsAddCyclesAndRunTheProgram) {
  EXPECT_FALSE(window_->IrqButton(1)->isEnabled());  // no program
  EXPECT_FALSE(window_->ResetButton()->isEnabled());
  ASSERT_TRUE(session_.Open(Assemble("irq", kProgram)));
  EXPECT_TRUE(window_->IrqButton(7)->isEnabled());
  session_.TraceInto();  // tracing: an interrupt does not run the program
  ASSERT_TRUE(Until([&] { return !session_.IsRunning(); }));
  const uint64_t cycles = session_.Simulator().State().cycles;
  QTest::mouseClick(window_->IrqButton(1), Qt::LeftButton);
  EXPECT_EQ(session_.Simulator().State().cycles, cycles + 44);  // IRQprocess()
  EXPECT_EQ(session_.State(), RunState::kReady);
  // Reset: SSP and PC from the vectors, supervisor, mask 7, 40 + 44 cycles.
  QTest::mouseClick(window_->ResetButton(), Qt::LeftButton);
  const auto& cpu = session_.Simulator().State();
  EXPECT_EQ(cpu.pc, 0x1000u);
  EXPECT_EQ(cpu.ssp, 0x8000u);
  EXPECT_EQ(cpu.GetInterruptMask(), 7);
  EXPECT_EQ(cpu.cycles, cycles + 44 + 84);
  EXPECT_EQ(session_.State(), RunState::kReady);
  // Not tracing: Reset runs the program.
  session_.Rewind();
  QTest::mouseClick(window_->ResetButton(), Qt::LeftButton);
  ASSERT_TRUE(Until([&] { return session_.State() == RunState::kHalted; }));
  EXPECT_EQ(session_.Simulator().State().d[1], 2u);
  EXPECT_FALSE(window_->ResetButton()->isEnabled());  // halted: Hardware->disable()
}

TEST_F(HardwareWindowTest, MaskedInterruptAfterStopWaits) {
  const std::string source =
      "        ORG     $1000\n"
      "START   MOVE.L  #HANDLER,$74\n"  // level 5 autovector
      "        STOP    #$2400\n"        // mask 4
      "        SIMHALT\n"
      "HANDLER MOVEQ   #5,D5\n"
      "        RTE\n"
      "        END     START\n";
  session_.SetExceptionsEnabled(true);
  ASSERT_TRUE(session_.Open(Assemble("stop", source)));
  session_.Run();
  ASSERT_TRUE(Until([&] { return session_.State() == RunState::kStopped; }));
  const uint64_t cycles = session_.Simulator().State().cycles;
  QTest::mouseClick(window_->IrqButton(3), Qt::LeftButton);  // masked: nothing
  EXPECT_EQ(session_.State(), RunState::kStopped);
  EXPECT_EQ(session_.Simulator().State().cycles, cycles);
  QTest::mouseClick(window_->IrqButton(5), Qt::LeftButton);
  ASSERT_TRUE(Until([&] { return session_.State() == RunState::kHalted; }));
  EXPECT_EQ(session_.Simulator().State().d[5], 5u);
}

TEST_F(HardwareWindowTest, AutomaticInterrupts) {
  QtSimulatorEnv& env = session_.Env();
  QTest::mouseClick(window_->AutoIrqBox(3), Qt::LeftButton);
  EXPECT_TRUE(env.GetAutoIrq(3).checked);
  window_->AutoIrqLevel()->setCurrentIndex(2);  // IRQ 3
  window_->IntervalField()->clear();
  QTest::keyClicks(window_->IntervalField(), QStringLiteral("25x0"));
  EXPECT_EQ(window_->IntervalField()->text(), QStringLiteral("250"));
  EXPECT_EQ(env.GetAutoIrq(3).interval_ms, 250);
  // Trap #15 task 32.6 (setAutoIRQ()): the check box and the interval.
  env.SetAutoIRQ(0x85, 500);
  ASSERT_TRUE(Until([&] { return window_->AutoIrqBox(5)->isChecked(); }));
  window_->AutoIrqLevel()->setCurrentIndex(4);
  EXPECT_EQ(window_->IntervalField()->text(), QStringLiteral("500"));
  env.SetHardwareEnabled(true);
  ASSERT_TRUE(Until([&] { return env.AutoIrqRunning(5); }));
  EXPECT_EQ(window_->AutoLabel()->text(), QStringLiteral("Automatic Enabled"));
  // Opening a program clears the levels (initialize()).
  ASSERT_TRUE(session_.Open(Assemble("auto", kProgram)));
  ASSERT_TRUE(Until([&] { return !window_->AutoIrqBox(5)->isChecked(); }));
  EXPECT_FALSE(window_->AutoIrqBox(3)->isChecked());
  EXPECT_EQ(window_->AutoLabel()->text(), QStringLiteral("Automatic Disabled"));
}

TEST_F(HardwareWindowTest, MemoryMapFieldsAndS0Records) {
  ASSERT_TRUE(session_.Open(Assemble("map", kProgram)));
  window_->MapStart(HardwareWindow::kRom)->setText(QStringLiteral("002000"));
  window_->MapEnd(HardwareWindow::kRom)->setText(QStringLiteral("002FFF"));
  EXPECT_FALSE(session_.Simulator().GetMemory().IsRom(0x2000));
  window_->MapCheck(HardwareWindow::kRom)->setChecked(true);
  EXPECT_TRUE(session_.MemoryMap().rom.enabled);
  EXPECT_EQ(session_.MemoryMap().rom.end, 0x2FFFu);
  EXPECT_TRUE(session_.Simulator().GetMemory().IsRom(0x2000));
  EXPECT_FALSE(session_.Simulator().GetMemory().IsRom(0x3000));
  // EASy68K S0 records set the ranges they name; the others stay.
  const std::string source =
      "        MEMORY  INVALID $5000,$5FFF\n"
      "        ORG     $1000\n"
      "START   SIMHALT\n"
      "        END     START\n";
  QSignalSpy messages(&session_, &SimSession::Message);
  ASSERT_TRUE(session_.Open(Assemble("s0map", source)));
  ASSERT_GE(messages.count(), 2);  // loadSrec() shows each S0 record
  EXPECT_EQ(messages.at(0).at(0).toString(), QStringLiteral("S0 = 68KPROG   20CREATED BY EASY68K"));
  EXPECT_EQ(messages.at(1).at(0).toString(), QStringLiteral("S0 =   INVALID 005000,005FFF"));
  EXPECT_TRUE(window_->MapCheck(HardwareWindow::kInvalid)->isChecked());
  EXPECT_EQ(window_->MapStart(HardwareWindow::kInvalid)->text(), QStringLiteral("005000"));
  EXPECT_EQ(window_->MapEnd(HardwareWindow::kInvalid)->text(), QStringLiteral("005FFF"));
  EXPECT_TRUE(window_->MapCheck(HardwareWindow::kRom)->isChecked());
  EXPECT_EQ(session_.Simulator().GetMemory().GetFlags(0x5000) & sim::kMemInvalid, sim::kMemInvalid);
  // Hardware addresses stay accessible inside a map range.
  window_->MapStart(HardwareWindow::kInvalid)->setText(QStringLiteral("E00000"));
  window_->MapEnd(HardwareWindow::kInvalid)->setText(QStringLiteral("E000FF"));
  EXPECT_TRUE(session_.Simulator().GetMemory().IsValidWrite(0xE00012));
  EXPECT_FALSE(session_.Simulator().GetMemory().IsValidWrite(0xE00020));
}

TEST_F(HardwareWindowTest, LiveDisplayWhileRunning) {
  const std::string source =
      "        ORG     $1000\n"
      "START   MOVE.B  #$5A,$E00010\n"
      "LOOP    MOVE.B  #8,D0\n"  // task 8 in a loop
      "        TRAP    #15\n"
      "        BRA     LOOP\n"
      "        END     START\n";
  ASSERT_TRUE(session_.Open(Assemble("live", source)));
  session_.Run();
  ASSERT_TRUE(Until([&] { return window_->LedByte() == 0x5A; }));
  // A switch changed while the program runs reaches memory through the
  // simulator thread.
  QTest::mouseClick(window_->Switch(3), Qt::LeftButton);
  session_.Pause();
  ASSERT_TRUE(Until([&] { return !session_.IsRunning(); }));
  EXPECT_EQ(Memory()[0xE00012], 0x08);
}

TEST_F(HardwareWindowTest, SnapshotsServedDuringADelay) {
  const std::string source =
      "        ORG     $1000\n"
      "START   MOVE.B  #$66,$E00010\n"
      "        MOVE.L  #500,D1\n"  // 5 s
      "        MOVE.B  #23,D0\n"
      "        TRAP    #15\n"
      "        SIMHALT\n"
      "        END     START\n";
  ASSERT_TRUE(session_.Open(Assemble("delay", source)));
  session_.Run();
  ASSERT_TRUE(Until([&] { return window_->LedByte() == 0x66; }, 2000));
  EXPECT_TRUE(session_.IsRunning());  // still in the delay
  session_.Pause();
  ASSERT_TRUE(Until([&] { return !session_.IsRunning(); }));
}

}  // namespace
}  // namespace easym68k::gui
