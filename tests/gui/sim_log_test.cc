// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Logging (Task 13.12): trace lines in the message pane, the execution log
// (memory rows, registers, listing line), the output log, Log Start/Stop,
// the Log Output dialog and the file-exists question (logU.cpp, RUN.CPP
// exec_inst(), SIM68Ku.cpp lineToLog(), simIOu.cpp).

#include "sim_log.h"

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "easym68k/asm/assembler.h"
#include "log_window.h"
#include "mainwindow.h"
#include "sim_io_screen.h"
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

const char kProgram[] =
    "        ORG     $1000\n"
    "START   MOVEQ   #1,D1\n"
    "        BSR     SUB\n"
    "        LEA     MSG,A1\n"
    "        MOVE.B  #14,D0\n"
    "        TRAP    #15\n"
    "        SIMHALT\n"
    "SUB     MOVEQ   #2,D2\n"
    "        RTS\n"
    "MSG     DC.B    'Done',0\n"
    "        END     START\n";

class SimLogTest : public ::testing::Test {
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

  static QStringList TraceLines(const QSignalSpy& messages) {
    QStringList lines;
    for (const auto& m : messages) {
      if (m.at(0).toString().startsWith(QStringLiteral("PC=")))
        lines << m.at(0).toString();
    }
    return lines;
  }

  static QString Read(const QString& path) {
    QFile f(path);
    EXPECT_TRUE(f.open(QIODevice::ReadOnly));
    return QString::fromLatin1(f.readAll());
  }

  bool Step(SimSession& s, bool over) {
    over ? s.StepOver() : s.TraceInto();
    return Until([&] { return !s.IsRunning(); });
  }

  QTemporaryDir dir_;
};

TEST_F(SimLogTest, ListingLineHasLabels) {
  const std::vector<std::string> listing = {
      "00001000 Starting Address",
      "Assembler used: EASy68K Editor/Assembler v5.16.01",
      "Created On: ",
      "",
      "00001000                             1          ORG     $1000",
      "00001000  7201                       2  START   MOVEQ   #1,D1",
      "00001002  4E71                       3          NOP",
      "00001004                             4  * comment",
      "00001004  4E71                       5          NOP",
  };
  EXPECT_EQ(SimLog::ListingLine(listing, 0x1002),
            "PC=00001002  Code=4E71                   Line=    3          NOP");
  // The last line with the PC (comments share it).
  const std::string last = SimLog::ListingLine(listing, 0x1004);
  EXPECT_EQ(last.substr(last.size() - 14), "5          NOP");
  EXPECT_TRUE(SimLog::ListingLine(listing, 0x1006).empty());  // past the end: not found
  EXPECT_TRUE(SimLog::ListingLine(listing, 0x0FFE).empty());
  EXPECT_TRUE(SimLog::ListingLine({"a", "b"}, 0x1000).empty());
}

// RUN.CPP exec_inst(): one message pane line per traced instruction; Step
// Over traces only the call, Run nothing.
TEST_F(SimLogTest, TraceLinesInTheMessagePane) {
  SimSession s;
  QSignalSpy messages(&s, &SimSession::Message);
  ASSERT_TRUE(s.Open(Assemble("trace", kProgram)));
  ASSERT_TRUE(Step(s, false));
  ASSERT_TRUE(Step(s, true));  // BSR over SUB
  QCoreApplication::processEvents();
  EXPECT_EQ(TraceLines(messages),
            (QStringList{QStringLiteral("PC=00001000  Code=7201  MOVEQ       "),
                         QStringLiteral("PC=00001002  Code=6100  BSR         ")}));
  EXPECT_EQ(s.Simulator().State().d[2], 2u);
  messages.clear();
  s.Run();
  ASSERT_TRUE(Until([&] { return s.State() == RunState::kHalted; }));
  QCoreApplication::processEvents();
  EXPECT_TRUE(TraceLines(messages).isEmpty());
  // SIMHALT is named by its two words.
  s.Rewind();
  for (int i = 0; i < 5; i++)  // to the SIMHALT after TRAP #15
    ASSERT_TRUE(Step(s, true));
  messages.clear();
  ASSERT_TRUE(Step(s, false));
  QCoreApplication::processEvents();
  EXPECT_EQ(TraceLines(messages), QStringList{QStringLiteral("PC=00001012  Code=FFFF  SIMHALT")});
}

TEST_F(SimLogTest, ExecutionAndOutputLogs) {
  SimSession s;
  ASSERT_TRUE(s.Open(Assemble("logs", kProgram)));
  SimLog& log = s.Log();
  log.SetFileNames(dir_.filePath(QStringLiteral("logs.S68")));
  SimLog::Settings settings = log.GetSettings();
  EXPECT_EQ(settings.exec_path, dir_.filePath(QStringLiteral("logs_RunLog.txt")));
  EXPECT_EQ(settings.output_path, dir_.filePath(QStringLiteral("logs_OutLog.txt")));
  EXPECT_FALSE(log.CanStart());
  settings.exec_type = SimLog::kExecMemory;
  settings.output_type = SimLog::kOutputText;
  settings.mem_from = QStringLiteral("1003");  // to $1000
  settings.mem_bytes = QStringLiteral("11");   // to 2 rows
  log.SetSettings(settings);
  ASSERT_TRUE(log.CanStart());
  ASSERT_EQ(log.Start([](const QString&) { return SimLog::Exists::kReplace; }),
            SimLog::StartResult::kStarted);
  EXPECT_TRUE(log.Logging());
  ASSERT_TRUE(Step(s, false));  // MOVEQ #1,D1
  s.Run();                      // not traced; the output is logged
  ASSERT_TRUE(Until([&] { return s.State() == RunState::kHalted; }));
  s.Rewind();
  log.Stop();
  const QString exec = Read(settings.exec_path);
  EXPECT_TRUE(exec.startsWith(QStringLiteral("EASy68K execution log file: ")))
      << exec.toStdString();
  EXPECT_TRUE(exec.contains(QStringLiteral(
      "\n\n00001000: 72 01 61 00 00 12 43 F9 00 00 10 1A 10 3C 00 0E r-a---C------<--\n"
      "00001010: 4E 4F FF FF FF FF 74 02 4E 75 44 6F 6E 65 00 FF NO----t-NuDone--\n"
      "\nD0=00000000 D4=00000000 A0=00000000 A4=00000000    T_S__INT___XNZVC\n"
      "D1=00000000 D5=00000000 A1=00000000 A5=00000000 SR=0010000000000000\n"
      "D2=00000000 D6=00000000 A2=00000000 A6=00000000 US=00FF0000\n"
      "D3=00000000 D7=00000000 A3=00000000 A7=01000000 SS=01000000\n"
      "PC=00001000  Code=7201")))
      << exec.toStdString();
  EXPECT_EQ(exec.count(QStringLiteral("PC=")), 1);  // Run is not logged
  EXPECT_TRUE(exec.endsWith(QStringLiteral("\n***** Reset Program Pressed *****\n")));
  const QString output = Read(settings.output_path);
  EXPECT_TRUE(output.startsWith(QStringLiteral("EASy68K output log file: ")));
  EXPECT_TRUE(output.contains(QStringLiteral("\r\n\r\nDone\n***** Reset Program Pressed *****\n")))
      << output.toStdString();
  // An existing file: Append keeps it, Cancel starts nothing.
  QStringList asked;
  ASSERT_EQ(log.Start([&](const QString& m) {
    asked << m;
    return SimLog::Exists::kAppend;
  }),
            SimLog::StartResult::kStarted);
  log.Stop();
  EXPECT_EQ(asked, (QStringList{QStringLiteral("Execution Log File exists!"),
                                QStringLiteral("Output Log File exists!")}));
  EXPECT_TRUE(Read(settings.exec_path).startsWith(exec));
  EXPECT_EQ(log.Start([](const QString&) { return SimLog::Exists::kCancel; }),
            SimLog::StartResult::kCancelled);
  EXPECT_FALSE(log.Logging());
}

// simIOu.cpp: the output log gets what the window shows; doCRLF() a line
// feed; typed input when echoed; not BEL.
TEST_F(SimLogTest, OutputLogFollowsTheWindow) {
  SimIoScreen screen;
  std::string logged;
  screen.SetOutputLogHook([&](char c) { logged += c; });
  screen.TextOut("A\aB\x01");
  screen.TextOutCRLF("C");
  screen.KeyPress('x');  // no input pending: not shown
  EXPECT_EQ(logged, "ABC\n");
}

TEST_F(SimLogTest, LogOutputDialog) {
  SimLog::Settings settings;
  settings.exec_path = QStringLiteral("run.txt");
  settings.exec_type = SimLog::kExecRegisters;
  LogOutputDialog dialog(settings);
  dialog.show();
  EXPECT_TRUE(dialog.ExecType(SimLog::kExecRegisters)->isChecked());
  EXPECT_FALSE(dialog.MemoryRange()->isVisible());
  dialog.ExecType(SimLog::kExecMemory)->setChecked(true);
  EXPECT_TRUE(dialog.MemoryRange()->isVisible());
  QStringList informed;
  dialog.SetInform([&](const QString& text) { informed << text; });
  dialog.MemFrom()->setText(QStringLiteral("1000000"));
  emit dialog.MemFrom()->editingFinished();
  EXPECT_EQ(informed, QStringList{QStringLiteral("Invalid Address")});
  dialog.OutputType(SimLog::kOutputText)->setChecked(true);
  const SimLog::Settings edited = dialog.Settings();
  EXPECT_EQ(edited.exec_type, SimLog::kExecMemory);
  EXPECT_EQ(edited.output_type, SimLog::kOutputText);
  EXPECT_EQ(edited.exec_path, QStringLiteral("run.txt"));
  EXPECT_EQ(edited.mem_from, QStringLiteral("1000000"));
}

TEST_F(SimLogTest, MainWindowLogStartStop) {
  MainWindow w;
  QStringList informed;
  w.SetInform([&](const QString&, const QString& text) { informed << text; });
  w.SetLogFileQuestion([](const QString&) { return SimLog::Exists::kReplace; });
  EXPECT_FALSE(w.LogStartAction()->isEnabled());
  EXPECT_FALSE(w.LogStopAction()->isEnabled());
  ASSERT_TRUE(w.OpenFile(Assemble("main", kProgram)));
  SimLog::Settings settings = w.Session().Log().GetSettings();
  EXPECT_EQ(settings.exec_path, dir_.filePath(QStringLiteral("main_RunLog.txt")));
  settings.exec_type = SimLog::kExecInstruction;
  w.Session().Log().SetSettings(settings);
  w.OpenFile(dir_.filePath(QStringLiteral("main.S68")));  // updates the actions
  ASSERT_TRUE(w.LogStartAction()->isEnabled());
  w.LogStartAction()->trigger();
  EXPECT_TRUE(w.Session().Log().Logging());
  EXPECT_TRUE(w.LogStopAction()->isEnabled());
  EXPECT_FALSE(w.LogStartAction()->isEnabled());
  // SIMHALT: "Current Log Stopped." (stopLogWithAnnounce()).
  w.RunAction()->trigger();
  ASSERT_TRUE(Until([&] { return w.Session().State() == RunState::kHalted; }));
  ASSERT_TRUE(Until([&] { return !w.Session().Log().Logging(); }));
  EXPECT_EQ(informed, QStringList{QStringLiteral("Current Log Stopped.")});
  EXPECT_TRUE(w.LogStartAction()->isEnabled());
  // Opening a program stops a running log.
  w.LogStartAction()->trigger();
  ASSERT_TRUE(w.Session().Log().Logging());
  w.OpenFile(dir_.filePath(QStringLiteral("main.S68")));
  EXPECT_FALSE(w.Session().Log().Logging());
  EXPECT_EQ(informed.size(), 2);
}

}  // namespace
}  // namespace easym68k::gui
