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

#include "easym68k/asm/assembler.h"
#include "sim_io_screen.h"

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
    s68.open(QIODevice::WriteOnly);
    s68.write(r.srecord.c_str());
    QFile l68(base + QStringLiteral(".L68"));
    l68.open(QIODevice::WriteOnly);
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

}  // namespace
}  // namespace easym68k::gui
