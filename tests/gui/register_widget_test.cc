// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// RegisterWidget (Task 13.3): display as scrshow(), editing as loadRegs()
// and the A7/US/SS key handlers, Clear Cycles, double clicks; and in the
// main window, an edited register takes effect on the next step.

#include "register_widget.h"

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "easym68k/asm/assembler.h"
#include "mainwindow.h"

namespace easym68k::gui {
namespace {

sim::CpuState SampleState() {
  sim::CpuState s{};
  for (int i = 0; i < 8; i++) {
    s.d[i] = 0x11111111u * static_cast<uint32_t>(i);
    s.a[i] = 0x00001000u + static_cast<uint32_t>(i) * 0x10;
  }
  s.a[7] = 0x00FF0000;  // US
  s.ssp = 0x01000000;
  s.pc = 0x00001234;
  s.sr = 0x2704;  // S, mask 7, Z
  s.cycles = 1234;
  return s;
}

// Types into a field as the user does (textEdited).
void Type(QLineEdit* field, const QString& text) {
  field->selectAll();
  QTest::keyClicks(field, text);
}

TEST(RegisterWidgetTest, ShowsTheState) {
  RegisterWidget w;
  w.Show(SampleState());
  EXPECT_EQ(w.DataField(3)->text(), QStringLiteral("33333333"));
  EXPECT_EQ(w.AddressField(2)->text(), QStringLiteral("00001020"));
  EXPECT_EQ(w.AddressField(7)->text(), QStringLiteral("01000000"));  // supervisor: SS
  EXPECT_EQ(w.UsField()->text(), QStringLiteral("00FF0000"));
  EXPECT_EQ(w.SsField()->text(), QStringLiteral("01000000"));
  EXPECT_EQ(w.PcField()->text(), QStringLiteral("00001234"));
  EXPECT_EQ(w.SrField()->text(), QStringLiteral("0010011100000100"));
  EXPECT_EQ(w.CyclesText(), QStringLiteral("1234"));
  sim::CpuState big = SampleState();
  big.cycles = 0x1000000000000ull;
  w.Show(big);
  EXPECT_EQ(w.CyclesText(), QStringLiteral("Overflow"));
}

TEST(RegisterWidgetTest, EditsUpdateTheState) {
  RegisterWidget w;
  w.Show(SampleState());
  QSignalSpy edited(&w, &RegisterWidget::Edited);
  Type(w.DataField(0), QStringLiteral("abcd"));
  ASSERT_GE(edited.count(), 1);
  EXPECT_EQ(w.DataField(0)->text(), QStringLiteral("ABCD"));  // upper case
  auto last = [&] { return edited.last().at(0).value<sim::CpuState>(); };
  EXPECT_EQ(last().d[0], 0xABCDu);
  Type(w.DataField(1), QStringLiteral("xyz12"));  // only hex digits
  EXPECT_EQ(w.DataField(1)->text(), QStringLiteral("12"));
  // A7 is SS in supervisor mode: both change.
  Type(w.AddressField(7), QStringLiteral("00008000"));
  EXPECT_EQ(w.SsField()->text(), QStringLiteral("00008000"));
  EXPECT_EQ(last().ssp, 0x8000u);
  // US changes A7 only in user mode.
  Type(w.UsField(), QStringLiteral("00007000"));
  EXPECT_EQ(w.AddressField(7)->text(), QStringLiteral("00008000"));
  EXPECT_EQ(last().a[7], 0x7000u);
  // Clearing S shows US in A7.
  w.SrField()->setCursorPosition(2);
  QTest::keyClicks(w.SrField(), QStringLiteral("0"));
  EXPECT_EQ(w.SrField()->text(), QStringLiteral("0000011100000100"));
  EXPECT_EQ(last().sr, 0x0704u);
  EXPECT_EQ(w.AddressField(7)->text(), QStringLiteral("00007000"));
  Type(w.PcField(), QStringLiteral("2000"));
  EXPECT_EQ(last().pc, 0x2000u);
}

TEST(RegisterWidgetTest, ReadOnlyWhileRunningAndClearCycles) {
  RegisterWidget w;
  w.Show(SampleState());
  QSignalSpy clear(&w, &RegisterWidget::ClearCycles);
  w.ClearCyclesButton()->click();
  EXPECT_EQ(clear.count(), 1);
  w.SetEditable(false);
  EXPECT_TRUE(w.DataField(0)->isReadOnly());
  EXPECT_FALSE(w.ClearCyclesButton()->isEnabled());
  w.SetEditable(true);
  EXPECT_FALSE(w.SrField()->isReadOnly());
}

TEST(RegisterWidgetTest, DoubleClicks) {
  RegisterWidget w;
  w.Show(SampleState());
  QSignalSpy address(&w, &RegisterWidget::AddressDoubleClicked);
  QSignalSpy pc(&w, &RegisterWidget::PcDoubleClicked);
  QTest::mouseDClick(w.AddressField(1), Qt::LeftButton);
  ASSERT_EQ(address.count(), 1);
  EXPECT_EQ(address.last().at(0).toUInt(), 0x1010u);
  QTest::mouseDClick(w.UsField(), Qt::LeftButton);
  EXPECT_EQ(address.last().at(0).toUInt(), 0x00FF0000u);
  QTest::mouseDClick(w.DataField(1), Qt::LeftButton);  // data registers: nothing
  EXPECT_EQ(address.count(), 2);
  QTest::mouseDClick(w.PcField(), Qt::LeftButton);
  EXPECT_EQ(pc.count(), 1);
}

TEST(RegisterWidgetTest, EditedRegisterIsUsedByTheNextStep) {
  QTemporaryDir dir;
  asm_::Assembler assembler;
  assembler.ConfigureListing("reg.X68", "");
  auto r = assembler.Assemble(
      "        ORG     $1000\n"
      "START   ADD.L   D1,D0\n"
      "        SIMHALT\n"
      "        END     START\n");
  ASSERT_TRUE(r.success);
  const QString path = dir.filePath(QStringLiteral("reg.S68"));
  QFile f(path);
  ASSERT_TRUE(f.open(QIODevice::WriteOnly));
  f.write(r.srecord.c_str());
  f.close();

  MainWindow w;
  ASSERT_TRUE(w.OpenFile(path));
  Type(w.Registers()->DataField(0), QStringLiteral("5"));
  Type(w.Registers()->DataField(1), QStringLiteral("7"));
  w.TraceAction()->trigger();
  QElapsedTimer t;
  t.start();
  while (w.Session().IsRunning() && t.elapsed() < 3000)
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
  EXPECT_EQ(w.Session().Simulator().State().d[0], 12u);
  EXPECT_EQ(w.Registers()->DataField(0)->text(), QStringLiteral("0000000C"));
  w.Registers()->ClearCyclesButton()->click();
  EXPECT_EQ(w.Session().Simulator().State().cycles, 0u);
  EXPECT_EQ(w.Registers()->CyclesText(), QStringLiteral("0"));
}

}  // namespace
}  // namespace easym68k::gui
