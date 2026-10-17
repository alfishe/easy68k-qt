// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Quality gate 13.2: the Sim68K-Qt main window opens a program, sets the
// PC, runs and steps with the actions, shows registers and messages, keeps
// the menu states of the original and accepts dropped .S68 files.

#include "mainwindow.h"

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QKeyEvent>
#include <QMimeData>
#include <QTemporaryDir>
#include <QUrl>

#include "auto_trace_options_dialog.h"
#include "easym68k/asm/assembler.h"
#include "hardware_window.h"
#include "io_window.h"
#include "memory_window.h"
#include "qt_print_io.h"
#include "register_widget.h"
#include "sim_io_screen.h"
#include "source_view.h"
#include "stack_window.h"

namespace easym68k::gui {
namespace {

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

class MainWindowTest : public ::testing::Test {
 protected:
  QString Assemble(const QString& name, const std::string& source) {
    asm_::Assembler assembler;
    assembler.ConfigureListing(name.toStdString() + ".X68", "");
    auto r = assembler.Assemble(source);
    EXPECT_TRUE(r.success);
    const QString base = dir_.filePath(name);
    QFile s68(base + QStringLiteral(".S68"));
    EXPECT_TRUE(s68.open(QIODevice::WriteOnly));
    s68.write(r.srecord.c_str());
    QFile l68(base + QStringLiteral(".L68"));
    EXPECT_TRUE(l68.open(QIODevice::WriteOnly));
    l68.write(assembler.GetListing().c_str());
    return base + QStringLiteral(".S68");
  }

  QTemporaryDir dir_;
};

const char kProgram[] =
    "        ORG     $1000\n"
    "START   MOVEQ   #1,D1\n"
    "        MOVEQ   #2,D2\n"
    "        LEA     MSG,A1\n"
    "        MOVE.B  #14,D0\n"
    "        TRAP    #15\n"
    "        SIMHALT\n"
    "MSG     DC.B    'Done',0\n"
    "        END     START\n";

TEST_F(MainWindowTest, OpenStepRunAndMenuStates) {
  MainWindow w;
  // No program: only Open.
  EXPECT_TRUE(w.OpenAction()->isEnabled());
  EXPECT_FALSE(w.RunAction()->isEnabled());
  EXPECT_FALSE(w.ReloadAction()->isEnabled());

  ASSERT_TRUE(w.OpenFile(Assemble("prog", kProgram)));
  EXPECT_EQ(w.Session().Simulator().State().pc, 0x1000u);
  EXPECT_TRUE(w.windowTitle().endsWith(QStringLiteral("prog.S68")));
  EXPECT_TRUE(w.RunAction()->isEnabled());
  EXPECT_TRUE(w.ReloadAction()->isEnabled());

  w.TraceAction()->trigger();
  ASSERT_TRUE(Until([&] { return !w.Session().IsRunning(); }));
  EXPECT_EQ(w.Session().Simulator().State().d[1], 1u);
  w.StepAction()->trigger();
  ASSERT_TRUE(Until([&] { return !w.Session().IsRunning(); }));
  EXPECT_EQ(w.Session().Simulator().State().d[2], 2u);

  w.RunAction()->trigger();
  ASSERT_TRUE(Until([&] { return w.Session().State() == SimSession::RunState::kHalted; }));
  // Halted (setMenuTrace): Run/Step disabled; Rewind, Close, Reload enabled.
  EXPECT_FALSE(w.RunAction()->isEnabled());
  EXPECT_FALSE(w.StepAction()->isEnabled());
  EXPECT_TRUE(w.RewindAction()->isEnabled());
  EXPECT_TRUE(w.CloseAction()->isEnabled());
  EXPECT_TRUE(w.ReloadAction()->isEnabled());
  EXPECT_EQ(w.Session().Screen().CharAt(0, 0), 'D');

  w.RewindAction()->trigger();
  EXPECT_TRUE(w.RunAction()->isEnabled());
  EXPECT_EQ(w.Session().Simulator().State().pc, 0x1000u);
}

TEST_F(MainWindowTest, MessagesAndPause) {
  const std::string loop =
      "        ORG     $1000\n"
      "START   NOP\n"
      "*[sim68k]break\n"
      "LOOP    BRA     LOOP\n"
      "        END     START\n";
  MainWindow w;
  ASSERT_TRUE(w.OpenFile(Assemble("loop", loop)));
  w.RunAction()->trigger();
  ASSERT_TRUE(Until([&] { return !w.Session().IsRunning(); }));
  EXPECT_TRUE(w.MessageText().contains(QStringLiteral("break point at 1002")));
  w.RunAction()->trigger();  // a run started on a breakpoint executes it
  ASSERT_TRUE(Until([&] { return w.Session().IsRunning(); }));
  // Running (setMenuTrace): only Pause, Rewind and Close.
  EXPECT_FALSE(w.RunAction()->isEnabled());
  EXPECT_FALSE(w.OpenAction()->isEnabled());
  EXPECT_TRUE(w.PauseAction()->isEnabled());
  w.PauseAction()->trigger();
  ASSERT_TRUE(Until([&] { return !w.Session().IsRunning(); }));
  EXPECT_TRUE(w.RunAction()->isEnabled());
}

// Delivers drop events directly (no drag in progress on the offscreen platform).
class DropWindow : public MainWindow {
 public:
  using MainWindow::dragEnterEvent;
  using MainWindow::dropEvent;
};

TEST_F(MainWindowTest, DropAnS68File) {
  DropWindow w;
  int confirmations = 0;
  w.SetConfirm([&](const QString&) {
    confirmations++;
    return true;
  });
  const QString first = Assemble("first", kProgram);
  auto drop = [&](const QString& path) {
    QMimeData mime;
    mime.setUrls({QUrl::fromLocalFile(path)});
    QDragEnterEvent enter(QPoint(10, 10), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    w.dragEnterEvent(&enter);
    QDropEvent event(QPointF(10, 10), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    w.dropEvent(&event);
  };
  drop(first);
  EXPECT_EQ(confirmations, 0);  // nothing loaded yet
  EXPECT_EQ(w.Session().ProgramPath(), first);
  const QString second = Assemble("second", kProgram);
  drop(second);
  EXPECT_EQ(confirmations, 1);
  EXPECT_EQ(w.Session().ProgramPath(), second);
  drop(dir_.filePath(QStringLiteral("second.L68")));  // not an .S68 file
  EXPECT_EQ(w.Session().ProgramPath(), second);
  w.SetConfirm([](const QString&) { return false; });  // Cancel
  drop(first);
  EXPECT_EQ(w.Session().ProgramPath(), second);
}

TEST_F(MainWindowTest, CommandLineOptions) {
  MainWindow w;
  w.ApplyArguments(
      {Assemble("cl", kProgram), QStringLiteral("/E"), QStringLiteral("/b"), QStringLiteral("/r")});
  EXPECT_TRUE(w.Session().ExceptionsEnabled());
  EXPECT_TRUE(w.Session().BitFieldEnabled());
  w.show();  // /r runs when the window shows
  ASSERT_TRUE(Until([&] { return w.Session().State() == SimSession::RunState::kHalted; }));
}

TEST_F(MainWindowTest, ListingGutterAndDoubleClick) {
  MainWindow w;
  w.resize(900, 700);
  w.show();
  ASSERT_TRUE(w.OpenFile(Assemble("lst", kProgram)));
  SourceView* source = w.Source();
  const int row = source->List()->currentRow();  // the PC line
  ASSERT_TRUE(source->CodeLineAddress(row).has_value());
  EXPECT_EQ(*source->CodeLineAddress(row), 0x1000u);
  // Toggle a breakpoint on the next code line through the gutter.
  int next = row + 1;
  while (!source->CodeLineAddress(next))
    next++;
  const uint32_t address = *source->CodeLineAddress(next);
  emit source->BreakpointToggled(address);
  EXPECT_TRUE(w.Session().HasBreakpoint(address));
  w.RunAction()->trigger();
  ASSERT_TRUE(Until([&] { return !w.Session().IsRunning(); }));
  EXPECT_EQ(w.Session().Simulator().State().pc, address);
  EXPECT_EQ(source->List()->currentRow(), next);
  emit source->ClearAllBreakpoints();
  EXPECT_TRUE(w.Session().Breakpoints().empty());
  // Double click sets the PC (ListBox1DblClick()).
  emit source->PcRequested(0x1000);
  EXPECT_EQ(w.Session().Simulator().State().pc, 0x1000u);
  EXPECT_EQ(w.Registers()->PcField()->text(), QStringLiteral("00001000"));
  EXPECT_EQ(source->List()->currentRow(), row);
}

TEST_F(MainWindowTest, KeyCommandsCanBeDisabledByTheProgram) {
  MainWindow w;
  ASSERT_FALSE(w.RunAction()->shortcut().isEmpty());
  w.Session().Screen().SetKeyCommandsEnabled(false);  // task 24, D1 = 1
  QCoreApplication::processEvents();
  EXPECT_TRUE(w.RunAction()->shortcut().isEmpty());
  w.Session().Screen().SetKeyCommandsEnabled(true);
  QCoreApplication::processEvents();
  EXPECT_EQ(w.RunAction()->shortcut(), QKeySequence(Qt::Key_F9));
}

TEST_F(MainWindowTest, FontActionsUpdateSourceOutputAndPrinterFonts) {
  MainWindow w;
  EXPECT_EQ(w.SourceFont().family(), QStringLiteral("Courier New"));
  EXPECT_EQ(w.SourceFont().pointSize(), 10);
  EXPECT_EQ(w.OutputFont().family(), QStringLiteral("Fixedsys"));
  EXPECT_EQ(w.OutputFont().pointSize(), 9);
  EXPECT_EQ(w.PrinterFont().family(), QStringLiteral("Courier New"));
  EXPECT_EQ(w.PrinterFont().pointSize(), 10);

  bool last_fixed = false;
  w.SetFontDialog([&](const QFont& current, bool fixed) -> std::optional<QFont> {
    last_fixed = fixed;
    QFont next = current;
    next.setPointSize(current.pointSize() + 4);
    next.setBold(true);
    return next;
  });

  w.SourceFontAction()->trigger();
  EXPECT_TRUE(last_fixed);
  EXPECT_EQ(w.SourceFont().pointSize(), 14);
  EXPECT_TRUE(w.SourceFont().bold());
  EXPECT_EQ(w.Source()->List()->font().pointSize(), 14);

  w.OutputFontAction()->trigger();
  EXPECT_TRUE(last_fixed);
  EXPECT_EQ(w.OutputFont().pointSize(), 13);
  EXPECT_EQ(w.Session().Screen().BaseFont().pointSize(), 13);

  w.PrinterFontAction()->trigger();
  EXPECT_FALSE(last_fixed);
  EXPECT_EQ(w.PrinterFont().pointSize(), 14);
  EXPECT_EQ(w.Session().Print().Font().pointSize(), 14);

  // Cancel leaves fonts unchanged.
  w.SetFontDialog([](const QFont&, bool) { return std::nullopt; });
  w.SourceFontAction()->trigger();
  EXPECT_EQ(w.SourceFont().pointSize(), 14);
}

TEST_F(MainWindowTest, SettingsPersistenceRoundTripAndOffscreenClamp) {
  const QString settings_file = dir_.filePath(QStringLiteral("sim68k.ini"));
  {
    MainWindow w1(settings_file);
    w1.show();
    SimLog::Settings log = w1.Session().Log().GetSettings();
    log.exec_type = SimLog::kExecRegisters;
    log.output_type = SimLog::kOutputText;
    w1.Session().Log().SetSettings(log);
    w1.ExceptionsAction()->setChecked(true);
    w1.BitFieldAction()->setChecked(true);
    w1.move(140, 120);
    w1.resize(680, 560);
    w1.Io()->move(210, 260);
    w1.Io()->show();
    w1.Io()->SetFullScreenMonitor(2);
    w1.MemoryView()->move(290, 90);
    w1.MemoryView()->ShowAddress(0x002000);
    w1.MemoryView()->show();
    w1.Stack()->move(50, 180);
    w1.Stack()->resize(w1.Stack()->width(), 440);
    w1.Stack()->Selector()->setCurrentIndex(3);  // A3
    w1.Stack()->show();
    w1.Hardware()->move(250, 110);
    w1.Hardware()->AddressField(HardwareWindow::kSeg7)->setText(QStringLiteral("E10000"));
    w1.Hardware()->AddressField(HardwareWindow::kLeds)->setText(QStringLiteral("E10010"));
    w1.Hardware()->AddressField(HardwareWindow::kSwitches)->setText(QStringLiteral("E10012"));
    w1.Hardware()->AddressField(HardwareWindow::kButtons)->setText(QStringLiteral("E10014"));
    w1.Hardware()->show();
    QFont src(QStringLiteral("Monospace"), 12);
    src.setItalic(true);
    w1.SetSourceFont(src);
    QFont out(QStringLiteral("Monospace"), 11);
    out.setBold(true);
    w1.SetOutputFont(out);
    QFont prn(QStringLiteral("Serif"), 15);
    w1.SetPrinterFont(prn);
    w1.SetAutoTraceInterval(350);
    w1.SetAutoTraceDisableDisplay(true);
    w1.close();
  }

  MainWindow w2(settings_file);
  EXPECT_EQ(w2.Session().Log().GetSettings().exec_type, SimLog::kExecRegisters);
  EXPECT_EQ(w2.Session().Log().GetSettings().output_type, SimLog::kOutputText);
  EXPECT_TRUE(w2.LogStartAction()->isEnabled());
  EXPECT_TRUE(w2.Session().ExceptionsEnabled());
  EXPECT_TRUE(w2.ExceptionsAction()->isChecked());
  EXPECT_TRUE(w2.Session().BitFieldEnabled());
  EXPECT_TRUE(w2.BitFieldAction()->isChecked());
  EXPECT_EQ(w2.x(), 140);
  EXPECT_EQ(w2.y(), 120);
  EXPECT_EQ(w2.width(), 680);
  EXPECT_EQ(w2.height(), 560);
  EXPECT_EQ(w2.Io()->x(), 210);
  EXPECT_EQ(w2.Io()->y(), 260);
  EXPECT_TRUE(w2.Io()->isVisible());
  EXPECT_EQ(w2.Io()->FullScreenMonitor(), 2);
  EXPECT_EQ(w2.MemoryView()->x(), 290);
  EXPECT_EQ(w2.MemoryView()->y(), 90);
  EXPECT_EQ(w2.MemoryView()->TopAddress(), 0x002000u);
  EXPECT_EQ(w2.MemoryView()->AddressField()->text(), QStringLiteral("002000"));
  EXPECT_TRUE(w2.MemoryView()->isVisible());
  EXPECT_EQ(w2.Stack()->x(), 50);
  EXPECT_EQ(w2.Stack()->y(), 180);
  EXPECT_EQ(w2.Stack()->height(), 440);
  EXPECT_EQ(w2.Stack()->Selector()->currentIndex(), 3);
  EXPECT_TRUE(w2.Stack()->isVisible());
  EXPECT_EQ(w2.Hardware()->x(), 250);
  EXPECT_EQ(w2.Hardware()->y(), 110);
  EXPECT_EQ(w2.Hardware()->Address(HardwareWindow::kSeg7), 0xE10000u);
  EXPECT_EQ(w2.Hardware()->Address(HardwareWindow::kLeds), 0xE10010u);
  EXPECT_EQ(w2.Hardware()->Address(HardwareWindow::kSwitches), 0xE10012u);
  EXPECT_EQ(w2.Hardware()->Address(HardwareWindow::kButtons), 0xE10014u);
  EXPECT_TRUE(w2.Hardware()->isVisible());
  EXPECT_EQ(w2.SourceFont().pointSize(), 12);
  EXPECT_TRUE(w2.SourceFont().italic());
  EXPECT_EQ(w2.OutputFont().pointSize(), 11);
  EXPECT_TRUE(w2.OutputFont().bold());
  EXPECT_EQ(w2.PrinterFont().pointSize(), 15);
  EXPECT_EQ(w2.AutoTraceInterval(), 350);
  EXPECT_TRUE(w2.AutoTraceDisableDisplay());
  w2.close();

  // Off-screen coordinates fall back to def.h defaults.
  QSettings bad(settings_file, QSettings::IniFormat);
  bad.setValue(QStringLiteral("main_left"), -32000);
  bad.setValue(QStringLiteral("main_top"), 99999);
  bad.setValue(QStringLiteral("simio_left"), 99999);
  bad.setValue(QStringLiteral("simio_top"), -10);
  bad.sync();
  MainWindow w3(settings_file);
  EXPECT_EQ(w3.x(), 100);
  EXPECT_EQ(w3.y(), 100);
  EXPECT_EQ(w3.Io()->x(), 200);
  EXPECT_EQ(w3.Io()->y(), 300);
  w3.close();
}

TEST_F(MainWindowTest, Task32Subtask5SavesExceptionsSetting) {
  const QString settings_file = dir_.filePath(QStringLiteral("task32.ini"));
  const std::string prog =
      "        ORG     $1000\n"
      "START   MOVEQ   #5,D1\n"
      "        MOVEQ   #32,D0\n"
      "        TRAP    #15\n"
      "        SIMHALT\n"
      "        END     START\n";
  MainWindow w(settings_file);
  EXPECT_FALSE(w.Session().ExceptionsEnabled());
  ASSERT_TRUE(w.OpenFile(Assemble("t32", prog)));
  w.RunAction()->trigger();
  ASSERT_TRUE(Until([&] { return w.Session().State() == SimSession::RunState::kHalted; }));
  EXPECT_TRUE(w.Session().ExceptionsEnabled());
  QSettings saved(settings_file, QSettings::IniFormat);
  EXPECT_TRUE(saved.value(QStringLiteral("exceptions")).toBool());
}

TEST_F(MainWindowTest, AutoTraceOptionsDialogClampsAndAccepts) {
  AutoTraceOptionsDialog dialog(5, true);
  EXPECT_EQ(dialog.Interval(), 10);  // clamped to [10, 1000]
  EXPECT_TRUE(dialog.DisableDisplay());

  dialog.IntervalSpinBox()->setValue(120);
  dialog.DisableDisplayCheckBox()->setChecked(false);
  dialog.OkButton()->click();
  EXPECT_EQ(dialog.result(), QDialog::Accepted);
  EXPECT_EQ(dialog.Interval(), 120);
  EXPECT_FALSE(dialog.DisableDisplay());
}

TEST_F(MainWindowTest, AutoTraceStepsAndStopsOnBreakpointAndHalt) {
  const std::string prog =
      "        ORG     $1000\n"
      "START   MOVEQ   #1,D0\n"
      "        MOVEQ   #2,D0\n"
      "*[sim68k]break\n"
      "        MOVEQ   #3,D0\n"
      "        SIMHALT\n"
      "        END     START\n";
  MainWindow w{QString()};
  ASSERT_TRUE(w.OpenFile(Assemble("autotrace", prog)));
  w.SetAutoTraceInterval(10);
  w.SetAutoTraceDisableDisplay(true);

  const int initial_row = w.Source()->List()->currentRow();
  w.AutoTraceAction()->trigger();
  EXPECT_TRUE(w.IsAutoTracing());
  EXPECT_FALSE(w.RunAction()->isEnabled());
  EXPECT_FALSE(w.AutoTraceAction()->isEnabled());

  // Auto-trace steps through the first two instructions and stops at the
  // breakpoint at $1004; stopping refreshes the PC highlight even when
  // DisableDisplay was checked during stepping.
  ASSERT_TRUE(Until([&] { return !w.IsAutoTracing() && !w.Session().IsRunning(); }));
  EXPECT_EQ(w.Session().Simulator().State().d[0], 2u);
  EXPECT_EQ(w.Session().Simulator().State().pc, 0x1004u);
  EXPECT_GT(w.Source()->List()->currentRow(), initial_row);
  EXPECT_TRUE(w.MessageText().contains(QStringLiteral("PC=00001000")));

  // Resuming auto-trace runs until SIMHALT and stops automatically.
  w.AutoTraceAction()->trigger();
  ASSERT_TRUE(Until([&] { return w.Session().State() == SimSession::RunState::kHalted; }));
  EXPECT_FALSE(w.IsAutoTracing());
  EXPECT_EQ(w.Session().Simulator().State().d[0], 3u);
}

}  // namespace
}  // namespace easym68k::gui
