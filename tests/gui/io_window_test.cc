// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// IoWindow (Tasks 13.9-13.11): full screen output window (Alt+Enter, task
// 33, View > Output Window, /f) with the canvas scaled to the screen, the
// Shift+Insert and Ctrl+Tab keys, task 24, and the Fullscreen Options dialog.

#include "io_window.h"

#include <gtest/gtest.h>

#include <QClipboard>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QScreen>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <thread>

#include "easym68k/asm/assembler.h"
#include "fullscreen_options_dialog.h"
#include "mainwindow.h"
#include "qt_peripheral_io.h"
#include "qt_simulator_env.h"
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

class IoWindowTest : public ::testing::Test {
 protected:
  SimIoScreen screen_;
  QtPeripheralIO peripheral_{[](int) {}};
};

TEST_F(IoWindowTest, FullScreenScalesTheCanvas) {
  IoWindow w(&screen_, &peripheral_);
  w.show();
  ASSERT_TRUE(QTest::qWaitForWindowExposed(&w));
  EXPECT_EQ(w.size(), QSize(640, 480));
  QTest::keyClick(&w, Qt::Key_Return, Qt::AltModifier);  // Alt+Enter
  EXPECT_TRUE(w.FullScreen());
  ASSERT_TRUE(Until([&] { return w.isFullScreen() && w.isVisible(); }));
  const QRect screen = QGuiApplication::primaryScreen()->geometry();
  ASSERT_TRUE(Until([&] { return w.size() == screen.size(); }));
  EXPECT_TRUE(w.windowFlags() & Qt::FramelessWindowHint);
  // The mouse reports canvas coordinates.
  const QPoint centre = w.CanvasToWindow(QPoint(320, 240));
  EXPECT_EQ(w.WindowToCanvas(centre), QPoint(320, 240));
  QTest::mousePress(&w, Qt::LeftButton, Qt::NoModifier, centre);
  long buttons = 0, position = 0;
  peripheral_.ReadMouse(2, &buttons, &position);  // last mouse down
  EXPECT_EQ(position & 0xFFFF, 320);
  EXPECT_EQ(position >> 16, 240);
  // Task 33 size change: the canvas stays scaled to the screen.
  screen_.SetWindowSize(800, 600);
  QCoreApplication::processEvents();
  EXPECT_EQ(w.size(), screen.size());
  EXPECT_EQ(w.WindowToCanvas(w.CanvasToWindow(QPoint(799, 599))), QPoint(799, 599));
  QTest::keyClick(&w, Qt::Key_Return, Qt::AltModifier);
  EXPECT_FALSE(w.FullScreen());
  ASSERT_TRUE(Until([&] { return !w.isFullScreen() && w.size() == QSize(800, 600); }));
  EXPECT_FALSE(w.windowFlags() & Qt::FramelessWindowHint);
}

TEST_F(IoWindowTest, FullScreenAppliedWhenShown) {
  IoWindow w(&screen_, &peripheral_);
  w.SetFullScreen(true);  // /f, task 33 before the window shows
  EXPECT_FALSE(w.isVisible());
  w.show();
  ASSERT_TRUE(Until([&] { return w.isFullScreen(); }));
  w.SetFullScreen(false);
  ASSERT_TRUE(Until([&] { return !w.isFullScreen(); }));
}

TEST_F(IoWindowTest, ShortcutKeys) {
  IoWindow w(&screen_, &peripheral_);
  w.show();
  QSignalSpy next(&w, &IoWindow::NextWindowRequested);
  QTest::keyClick(&w, Qt::Key_Tab, Qt::ControlModifier);
  EXPECT_EQ(next.count(), 1);
  // Alt+key types nothing (WM_SYSKEYDOWN).
  QTest::keyClick(&w, Qt::Key_A, Qt::AltModifier);
  EXPECT_FALSE(screen_.IsKeyPending());
  // Shift+Insert types the clipboard into an input line.
  std::string line;
  bool done = false;
  std::thread reader([&] { done = screen_.ReadLine(&line); });
  ASSERT_TRUE(Until([&] { return screen_.InputActive(); }));
  QGuiApplication::clipboard()->setText(QStringLiteral("Hi 68K\r"));
  QTest::keyClick(&w, Qt::Key_Insert, Qt::ShiftModifier);
  reader.join();
  EXPECT_TRUE(done);
  EXPECT_EQ(line, "Hi 68K");
  // Task 24 disables the shortcut keys.
  screen_.SetKeyCommandsEnabled(false);
  QTest::keyClick(&w, Qt::Key_Tab, Qt::ControlModifier);
  QTest::keyClick(&w, Qt::Key_Return, Qt::AltModifier);
  EXPECT_EQ(next.count(), 1);
  EXPECT_FALSE(w.FullScreen());
}

TEST(FullscreenOptionsTest, OkOnlyForAnExistingScreen) {
  FullscreenOptionsDialog dialog(0, 2);
  EXPECT_EQ(dialog.ScreenNumber()->text(), QStringLiteral("1"));
  EXPECT_TRUE(dialog.OkButton()->isEnabled());
  dialog.ScreenNumber()->setText(QStringLiteral("3"));
  EXPECT_FALSE(dialog.OkButton()->isEnabled());
  dialog.ScreenNumber()->setText(QStringLiteral("0"));
  EXPECT_FALSE(dialog.OkButton()->isEnabled());
  dialog.ScreenNumber()->setText(QStringLiteral("x"));
  EXPECT_FALSE(dialog.OkButton()->isEnabled());
  dialog.ScreenNumber()->setText(QStringLiteral("2"));
  EXPECT_TRUE(dialog.OkButton()->isEnabled());
  EXPECT_EQ(dialog.Monitor(), 1);
}

TEST(IoWindowMainTest, Task33AndOutputWindowMenu) {
  QTemporaryDir dir;
  asm_::Assembler assembler;
  assembler.ConfigureListing("fs.X68", "");
  const auto r = assembler.Assemble(
      "        ORG     $1000\n"
      "START   MOVEQ   #2,D1\n"  // task 33, D1 = 2: full screen
      "        MOVE.B  #33,D0\n"
      "        TRAP    #15\n"
      "        SIMHALT\n"
      "        END     START\n");
  ASSERT_TRUE(r.success);
  QFile s68(dir.filePath(QStringLiteral("fs.S68")));
  ASSERT_TRUE(s68.open(QIODevice::WriteOnly));
  s68.write(r.srecord.c_str());
  s68.close();
  MainWindow w;
  w.show();
  ASSERT_TRUE(w.OpenFile(s68.fileName()));
  w.Io()->show();
  w.RunAction()->trigger();
  ASSERT_TRUE(Until([&] { return w.Io()->isFullScreen(); }));
  // View > Output Window leaves full screen (OutputWindow1Click()).
  for (QAction* a : w.findChildren<QAction*>()) {
    if (a->text() == QStringLiteral("&Output Window"))
      a->trigger();
  }
  EXPECT_FALSE(w.Io()->FullScreen());
  ASSERT_TRUE(Until([&] { return !w.Io()->isFullScreen(); }));
  // /f
  w.ApplyArguments({QString(), QStringLiteral("/F")});
  EXPECT_TRUE(w.Io()->FullScreen());
}

}  // namespace
}  // namespace easym68k::gui
