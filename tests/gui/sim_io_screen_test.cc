// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// SimIoScreen (simIOu.cpp TsimIO port): text grid, cursor, scrolling,
// graphics in every pen mode, double buffering, keyboard input and the
// change notification.

#include "sim_io_screen.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

namespace easym68k::gui {
namespace {

constexpr uint32_t kBlack = 0x000000;
constexpr uint32_t kWhite = 0xFFFFFF;
constexpr uint32_t kRed = 0x0000FF;  // TColor 0x00BBGGRR
constexpr uint32_t kGreen = 0x00FF00;
constexpr uint32_t kBlue = 0xFF0000;

std::string Row(const SimIoScreen& s, int row, int cols) {
  std::string out;
  for (int c = 0; c < cols; c++)
    out += s.CharAt(row, c);
  return out;
}

// Waits (bounded) until the screen is waiting for input.
void WaitForInput(const SimIoScreen& s) {
  for (int i = 0; i < 2000 && !s.InputActive(); i++)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  ASSERT_TRUE(s.InputActive());
}

// ---- Window and text --------------------------------------------------------

TEST(SimIoScreenTest, DefaultWindowAndCursor) {
  SimIoScreen s;
  int w = 0, h = 0;
  s.GetWindowSize(&w, &h);
  EXPECT_EQ(w, 640);
  EXPECT_EQ(h, 480);
  int col = -1, row = -1;
  s.GetRC(&col, &row);
  EXPECT_EQ(col, 0);
  EXPECT_EQ(row, 0);
  EXPECT_EQ(s.CharAt(0, 0), ' ');
  EXPECT_GT(s.CellSize().width(), 0);
  EXPECT_GT(s.CellSize().height(), 0);
}

TEST(SimIoScreenTest, WindowSizeHasAMinimumAndZeroKeepsIt) {
  SimIoScreen s;
  int w = 0, h = 0;
  std::atomic<int> size_changes{0};
  QObject::connect(&s, &SimIoScreen::WindowSizeChanged, [&](int, int) { size_changes++; });
  s.SetWindowSize(800, 600);
  s.GetWindowSize(&w, &h);
  EXPECT_EQ(w, 800);
  EXPECT_EQ(h, 600);
  s.SetWindowSize(0, 0);
  s.GetWindowSize(&w, &h);
  EXPECT_EQ(w, 800);
  EXPECT_EQ(h, 600);
  s.SetWindowSize(100, 100);
  s.GetWindowSize(&w, &h);
  EXPECT_EQ(w, 640);
  EXPECT_EQ(h, 480);
  EXPECT_EQ(size_changes, 3);
}

TEST(SimIoScreenTest, TextGoesToTheGridAndMovesTheCursor) {
  SimIoScreen s;
  s.TextOut("AB");
  EXPECT_EQ(Row(s, 0, 3), "AB ");
  int col = 0, row = 0;
  s.GetRC(&col, &row);
  EXPECT_EQ(col, 2);
  EXPECT_EQ(row, 0);
  s.TextOutCRLF("C");
  s.GetRC(&col, &row);
  EXPECT_EQ(col, 0);
  EXPECT_EQ(row, 1);
  EXPECT_EQ(Row(s, 0, 3), "ABC");
}

TEST(SimIoScreenTest, ControlCharacters) {
  SimIoScreen s;
  int col = 0, row = 0;
  s.TextOut("abc\b");  // BS: back one column, nothing erased
  s.GetRC(&col, &row);
  EXPECT_EQ(col, 2);
  s.CharOut('\t');  // HT: five columns
  s.GetRC(&col, &row);
  EXPECT_EQ(col, 7);
  s.CharOut('\n');  // LF: next row, same column
  s.GetRC(&col, &row);
  EXPECT_EQ(row, 1);
  EXPECT_EQ(col, 7);
  s.CharOut('\v');  // VT: four rows
  s.GetRC(&col, &row);
  EXPECT_EQ(row, 5);
  s.CharOut('\r');
  s.GetRC(&col, &row);
  EXPECT_EQ(col, 0);
  EXPECT_EQ(s.CharAt(0, 2), 'c');
  s.CharOut('\f');  // FF: clear and home
  s.GetRC(&col, &row);
  EXPECT_EQ(col, 0);
  EXPECT_EQ(row, 0);
  EXPECT_EQ(s.CharAt(0, 0), ' ');
  EXPECT_EQ(s.GetPixel(0, 0), kBlack);
}

TEST(SimIoScreenTest, BellIsSignalled) {
  SimIoScreen s;
  int bells = 0;
  QObject::connect(&s, &SimIoScreen::Bell, [&] { bells++; });
  s.TextOut("x\ay");
  s.CharOut('\a');
  EXPECT_EQ(bells, 2);
  EXPECT_EQ(Row(s, 0, 2), "xy");
}

TEST(SimIoScreenTest, CursorPositioningAndClamping) {
  SimIoScreen s;
  s.GotoRC(3, 10);
  int col = 0, row = 0;
  s.GetRC(&col, &row);
  EXPECT_EQ(row, 3);
  EXPECT_EQ(col, 10);
  s.GotoRC(-5, -5);
  s.GetRC(&col, &row);
  EXPECT_EQ(row, 0);
  EXPECT_EQ(col, 0);
  s.GotoRC(0, 1000);  // no wrap: the column is kept (limited to the grid)
  s.GetRC(&col, &row);
  EXPECT_EQ(row, 0);
  EXPECT_EQ(col, SimIoScreen::kMaxCols - 1);
}

TEST(SimIoScreenTest, RowBelowTheWindowScrollsOnce) {
  SimIoScreen s;
  const int rows = 480 / s.CellSize().height();
  s.GotoRC(1, 0);
  s.TextOut("X");
  s.GotoRC(1000, 0);
  int col = 0, row = 0;
  s.GetRC(&col, &row);
  EXPECT_EQ(row, rows - 1);
  EXPECT_EQ(s.CharAt(0, 0), 'X');  // moved up one row
  EXPECT_EQ(s.CharAt(1, 0), ' ');
}

TEST(SimIoScreenTest, OutputPastTheLastRowScrolls) {
  SimIoScreen s;
  const int rows = 480 / s.CellSize().height();
  s.TextOutCRLF("first");
  for (int i = 1; i < rows; i++)
    s.TextOutCRLF("x");
  EXPECT_EQ(Row(s, 0, 5), "x    ");  // "first" scrolled off
}

TEST(SimIoScreenTest, TextWrapOption) {
  SimIoScreen s;
  const int cols = 640 / s.CellSize().width();
  s.SetTextWrap(true);
  s.TextOut(std::string(static_cast<size_t>(cols) + 1, 'w'));
  int col = 0, row = 0;
  s.GetRC(&col, &row);
  EXPECT_EQ(row, 1);
  EXPECT_EQ(col, 1);
  EXPECT_EQ(s.CharAt(1, 0), 'w');
}

TEST(SimIoScreenTest, ScrollRectMovesTextInEachDirection) {
  SimIoScreen s;
  s.GotoRC(0, 0);
  s.TextOut("abc");
  s.GotoRC(1, 0);
  s.TextOut("def");
  s.ScrollRect(0, 0, 2, 3, 0);  // up
  EXPECT_EQ(Row(s, 0, 3), "def");
  EXPECT_EQ(Row(s, 1, 3), "   ");
  s.ScrollRect(0, 0, 2, 3, 1);  // down
  EXPECT_EQ(Row(s, 0, 3), "   ");
  EXPECT_EQ(Row(s, 1, 3), "def");
  s.ScrollRect(1, 0, 1, 3, 2);  // left
  EXPECT_EQ(Row(s, 1, 3), "ef ");
  s.ScrollRect(1, 0, 1, 3, 3);  // right
  EXPECT_EQ(Row(s, 1, 3), " ef");
  s.ScrollRect(1, 0, 0, 3, 0);  // invalid: ignored
  s.ScrollRect(127, 0, 1, 3, 0);
  s.ScrollRect(1, 0, 1, 3, 9);  // reserved direction
  EXPECT_EQ(Row(s, 1, 3), " ef");
}

TEST(SimIoScreenTest, TextUsesTheFillColourAsBackground) {
  SimIoScreen s;
  s.SetFillColor(kRed);
  s.TextOut("W");
  const QSize cell = s.CellSize();
  bool red = false, white = false;
  for (int y = 0; y < cell.height(); y++) {
    for (int x = 0; x < cell.width(); x++) {
      red |= s.GetPixel(x, y) == kRed;
      white |= s.GetPixel(x, y) == kWhite;
    }
  }
  EXPECT_TRUE(red);
  EXPECT_TRUE(white);
  EXPECT_EQ(s.GetPixel(cell.width() + 1, 0), kBlack);
}

TEST(SimIoScreenTest, FontPropertiesKeepTheCursorCell) {
  SimIoScreen s;
  s.GotoRC(2, 3);
  s.SetFontProperties(kGreen, 0x0003000F | 0x00100000);  // all styles, Courier New 16
  int col = 0, row = 0;
  s.GetRC(&col, &row);
  EXPECT_EQ(row, 2);
  EXPECT_EQ(col, 3);
  s.TextOut("Q");
  EXPECT_EQ(s.CharAt(2, 3), 'Q');
}

// ---- Graphics ---------------------------------------------------------------

TEST(SimIoScreenTest, PixelsAndOutOfRange) {
  SimIoScreen s;
  s.SetPenColor(kBlue);
  s.DrawPixel(5, 6);
  EXPECT_EQ(s.GetPixel(5, 6), kBlue);
  s.DrawPixel(-1, 0);
  s.DrawPixel(640, 0);
  EXPECT_EQ(s.GetPixel(-1, 0), 0xFFFFFFFFu);
  EXPECT_EQ(s.GetPixel(0, 480), 0xFFFFFFFFu);
}

TEST(SimIoScreenTest, LinesIncludeBothEndPointsAndMoveThePen) {
  SimIoScreen s;
  s.SetPenColor(kWhite);
  s.Line(10, 10, 20, 10);
  EXPECT_EQ(s.GetPixel(10, 10), kWhite);
  EXPECT_EQ(s.GetPixel(20, 10), kWhite);
  EXPECT_EQ(s.GetPixel(21, 10), kBlack);
  EXPECT_EQ(s.GetPixel(9, 10), kBlack);
  int x = 0, y = 0;
  s.GetXY(&x, &y);
  EXPECT_EQ(x, 20);
  EXPECT_EQ(y, 10);
  s.LineTo(20, 30);
  EXPECT_EQ(s.GetPixel(20, 30), kWhite);
  EXPECT_EQ(s.GetPixel(20, 31), kBlack);
  s.MoveTo(100, 101);
  s.GetXY(&x, &y);
  EXPECT_EQ(x, 100);
  EXPECT_EQ(y, 101);
}

TEST(SimIoScreenTest, RectanglesAreInclusive) {
  SimIoScreen s;
  s.SetPenColor(kRed);
  s.SetFillColor(kGreen);
  s.Rectangle(20, 20, 10, 10);  // reversed corners
  EXPECT_EQ(s.GetPixel(10, 10), kRed);
  EXPECT_EQ(s.GetPixel(20, 20), kRed);
  EXPECT_EQ(s.GetPixel(20, 10), kRed);
  EXPECT_EQ(s.GetPixel(15, 15), kGreen);
  EXPECT_EQ(s.GetPixel(21, 21), kBlack);
  EXPECT_EQ(s.GetPixel(9, 9), kBlack);
  s.UnfilledRectangle(30, 30, 40, 40);
  EXPECT_EQ(s.GetPixel(30, 30), kRed);
  EXPECT_EQ(s.GetPixel(40, 40), kRed);
  EXPECT_EQ(s.GetPixel(35, 35), kBlack);
}

TEST(SimIoScreenTest, Ellipses) {
  SimIoScreen s;
  s.SetPenColor(kRed);
  s.SetFillColor(kGreen);
  s.Ellipse(10, 10, 50, 30);
  EXPECT_EQ(s.GetPixel(30, 20), kGreen);
  EXPECT_EQ(s.GetPixel(10, 10), kBlack);  // corner outside
  EXPECT_EQ(s.GetPixel(30, 10), kRed);    // top of the outline
  EXPECT_EQ(s.GetPixel(30, 30), kRed);    // bottom
  s.UnfilledEllipse(100, 100, 140, 120);
  EXPECT_EQ(s.GetPixel(120, 110), kBlack);
  EXPECT_EQ(s.GetPixel(120, 100), kRed);
  // Large coordinates are drawn (no Windows 95 limit).
  s.Ellipse(600, 400, 639, 479);
  EXPECT_EQ(s.GetPixel(620, 440), kGreen);
}

TEST(SimIoScreenTest, XorDrawsEveryPixelOnceAndUndoes) {
  SimIoScreen s;
  s.SetDrawingMode(SimIoScreen::kPenXor);
  s.SetPenColor(kWhite);
  s.SetFillColor(kWhite);
  s.Rectangle(10, 10, 20, 20);
  for (int y = 10; y <= 20; y++)
    for (int x = 10; x <= 20; x++)
      ASSERT_EQ(s.GetPixel(x, y), kWhite) << x << "," << y;
  s.Rectangle(10, 10, 20, 20);
  for (int y = 10; y <= 20; y++)
    for (int x = 10; x <= 20; x++)
      ASSERT_EQ(s.GetPixel(x, y), kBlack) << x << "," << y;
  s.Line(0, 50, 30, 50);
  EXPECT_EQ(s.GetPixel(30, 50), kWhite);  // end point XORed as well
  s.Line(0, 50, 30, 50);
  EXPECT_EQ(s.GetPixel(30, 50), kBlack);
  EXPECT_EQ(s.GetPixel(0, 50), kBlack);
}

TEST(SimIoScreenTest, PenModes) {
  SimIoScreen s;
  s.SetPenColor(kRed);
  s.SetDrawingMode(SimIoScreen::kPenWhite);
  s.Line(0, 0, 5, 0);
  EXPECT_EQ(s.GetPixel(3, 0), kWhite);
  s.SetDrawingMode(SimIoScreen::kPenBlack);
  s.Line(0, 0, 5, 0);
  EXPECT_EQ(s.GetPixel(3, 0), kBlack);
  s.SetDrawingMode(SimIoScreen::kPenNot);
  s.Line(0, 0, 5, 0);
  EXPECT_EQ(s.GetPixel(3, 0), kWhite);
  s.SetDrawingMode(SimIoScreen::kPenNop);
  s.SetPenColor(kGreen);
  s.Line(0, 0, 5, 0);
  EXPECT_EQ(s.GetPixel(3, 0), kWhite);
  s.SetDrawingMode(SimIoScreen::kPenMask);  // P & D
  s.Line(0, 0, 5, 0);
  EXPECT_EQ(s.GetPixel(3, 0), kGreen);
  s.SetDrawingMode(SimIoScreen::kPenNotCopy);  // ~P
  s.Line(0, 0, 5, 0);
  EXPECT_EQ(s.GetPixel(3, 0), 0xFF00FFu);
  s.SetDrawingMode(99);  // invalid: ignored, still NOTCOPYPEN
  s.Line(0, 0, 5, 0);
  EXPECT_EQ(s.GetPixel(3, 0), 0xFF00FFu);
  // DrawPixel is SetPixel: never combined.
  s.DrawPixel(3, 0);
  EXPECT_EQ(s.GetPixel(3, 0), kGreen);
}

TEST(SimIoScreenTest, WidePen) {
  SimIoScreen s;
  s.SetPenColor(kWhite);
  s.SetPenWidth(5);
  s.Line(50, 50, 100, 50);
  EXPECT_EQ(s.GetPixel(75, 48), kWhite);
  EXPECT_EQ(s.GetPixel(75, 52), kWhite);
  EXPECT_EQ(s.GetPixel(75, 56), kBlack);
}

TEST(SimIoScreenTest, FloodFillFillsTheConnectedArea) {
  SimIoScreen s;
  s.SetPenColor(kWhite);
  s.UnfilledRectangle(10, 10, 30, 30);
  s.SetFillColor(kBlue);
  s.FloodFill(20, 20);
  EXPECT_EQ(s.GetPixel(11, 11), kBlue);
  EXPECT_EQ(s.GetPixel(29, 29), kBlue);
  EXPECT_EQ(s.GetPixel(10, 10), kWhite);
  EXPECT_EQ(s.GetPixel(5, 5), kBlack);
  s.FloodFill(20, 20);  // same colour: nothing to do
  s.FloodFill(-1, 0);
  EXPECT_EQ(s.GetPixel(20, 20), kBlue);
}

TEST(SimIoScreenTest, DrawTextIsNotInTheGridAndMovesThePen) {
  SimIoScreen s;
  s.DrawText("Hi", 100, 200);
  EXPECT_EQ(s.CharAt(0, 0), ' ');
  int x = 0, y = 0;
  s.GetXY(&x, &y);
  EXPECT_GT(x, 100);
  EXPECT_EQ(y, 200);
}

TEST(SimIoScreenTest, DoubleBuffering) {
  SimIoScreen s;
  s.SetPenColor(kWhite);
  s.SetDrawingMode(SimIoScreen::kDoubleBufferOn);
  s.DrawPixel(1, 1);
  EXPECT_EQ(s.GetPixel(1, 1), kWhite);  // the back buffer
  EXPECT_EQ(s.Snapshot().pixel(1, 1) & 0xFFFFFF, 0u);
  s.Repaint();
  EXPECT_EQ(s.Snapshot().pixel(1, 1) & 0xFFFFFF, 0xFFFFFFu);
  s.DrawPixel(2, 2);
  EXPECT_EQ(s.Snapshot().pixel(2, 2) & 0xFFFFFF, 0u);
  s.SetDrawingMode(SimIoScreen::kDoubleBufferOff);
  EXPECT_EQ(s.Snapshot().pixel(2, 2) & 0xFFFFFF, 0xFFFFFFu);
}

TEST(SimIoScreenTest, ResetForRunRestoresDefaults) {
  SimIoScreen s;
  s.SetWindowSize(800, 600);
  s.TextOut("abc");
  s.SetPenColor(kRed);
  s.SetPenWidth(9);
  s.SetDrawingMode(SimIoScreen::kPenXor);
  s.SetKeyboardEcho(false);
  s.ResetForRun();
  int w = 0, h = 0;
  s.GetWindowSize(&w, &h);
  EXPECT_EQ(w, 800);  // size kept
  EXPECT_EQ(s.CharAt(0, 0), ' ');
  s.Line(0, 5, 10, 5);
  EXPECT_EQ(s.GetPixel(5, 5), kWhite);  // white pen, copy mode
  EXPECT_EQ(s.GetPixel(5, 7), kBlack);  // width 1
}

// ---- Change notification ------------------------------------------------------

TEST(SimIoScreenTest, ChangedIsCoalescedUntilSnapshot) {
  SimIoScreen s;
  int changes = 0;
  QObject::connect(&s, &SimIoScreen::Changed, [&] { changes++; });
  s.Snapshot();
  s.TextOut("a");
  s.TextOut("b");
  s.DrawPixel(1, 1);
  EXPECT_EQ(changes, 1);
  s.Snapshot();
  s.TextOut("c");
  EXPECT_EQ(changes, 2);
  s.Snapshot();
  s.SetDrawingMode(SimIoScreen::kDoubleBufferOn);
  s.TextOut("d");  // back buffer only
  EXPECT_EQ(changes, 2);
  s.Repaint();
  EXPECT_EQ(changes, 3);
}

TEST(SimIoScreenTest, ShowIsRequestedOnceWhileHidden) {
  SimIoScreen s;
  int shows = 0;
  QObject::connect(&s, &SimIoScreen::ShowRequested, [&] { shows++; });
  s.TextOut("a");
  s.TextOut("b");
  EXPECT_EQ(shows, 1);
  s.SetWindowVisible(true);
  s.TextOut("c");
  EXPECT_EQ(shows, 1);
  s.SetWindowVisible(false);
  s.CharOut('d');
  EXPECT_EQ(shows, 2);
}

// ---- Keyboard input -----------------------------------------------------------

TEST(SimIoScreenTest, ReadLineEchoesAndEndsWithCrLf) {
  SimIoScreen s;
  std::string line;
  bool ok = false;
  std::thread sim([&] { ok = s.ReadLine(&line); });
  WaitForInput(s);
  EXPECT_TRUE(s.PromptInfo().active);
  for (char c : std::string("hi\r"))
    s.KeyPress(c);
  sim.join();
  EXPECT_TRUE(ok);
  EXPECT_EQ(line, "hi");
  EXPECT_EQ(Row(s, 0, 3), "hi ");  // echo stored in the grid
  int col = 0, row = 0;
  s.GetRC(&col, &row);
  EXPECT_EQ(row, 1);
  EXPECT_EQ(col, 0);
  EXPECT_FALSE(s.PromptInfo().active);
}

TEST(SimIoScreenTest, BackspaceEditsTheInput) {
  SimIoScreen s;
  std::string line;
  std::thread sim([&] { s.ReadLine(&line); });
  WaitForInput(s);
  for (char c : std::string("abX\bc\r"))
    s.KeyPress(c);
  sim.join();
  EXPECT_EQ(line, "abc");
  EXPECT_EQ(Row(s, 0, 4), "abc ");
}

TEST(SimIoScreenTest, InputEndsAfter79Keys) {
  SimIoScreen s;
  s.SetKeyboardEcho(false);
  std::string line;
  std::thread sim([&] { s.ReadLine(&line); });
  WaitForInput(s);
  for (int i = 0; i < 79; i++)
    s.KeyPress('k');
  s.KeyPress('\b');  // still edits at the limit
  s.KeyPress('k');
  EXPECT_TRUE(s.InputActive());
  s.KeyPress('z');  // the 80th key ends the input and is not stored
  sim.join();
  EXPECT_EQ(line, std::string(79, 'k'));
  EXPECT_EQ(s.CharAt(0, 0), ' ');  // echo off
}

TEST(SimIoScreenTest, KeyTypedBeforeTheRequestIsPending) {
  SimIoScreen s;
  EXPECT_FALSE(s.IsKeyPending());
  s.KeyPress('q');
  EXPECT_TRUE(s.IsKeyPending());
  char ch = 0;
  EXPECT_TRUE(s.ReadChar(&ch));
  EXPECT_EQ(ch, 'q');
  EXPECT_FALSE(s.IsKeyPending());
  EXPECT_EQ(s.CharAt(0, 0), 'q');  // echoed
  // A pending key starts a line input.
  s.KeyPress('a');
  std::string line;
  std::thread sim([&] { s.ReadLine(&line); });
  WaitForInput(s);
  s.KeyPress('\r');
  sim.join();
  EXPECT_EQ(line, "a");
}

TEST(SimIoScreenTest, ReadCharEchoAndLineFeedDisplay) {
  SimIoScreen s;
  char ch = 0;
  std::thread sim([&] { s.ReadChar(&ch); });
  WaitForInput(s);
  s.KeyPress('\r');
  sim.join();
  EXPECT_EQ(ch, '\r');
  int col = 0, row = 0;
  s.GetRC(&col, &row);
  EXPECT_EQ(row, 1);  // CR echoed with LF
  s.SetInputLFDisplay(false);
  s.KeyPress('\r');  // pending key: same echo rules
  ASSERT_TRUE(s.ReadChar(&ch));
  s.GetRC(&col, &row);
  EXPECT_EQ(row, 1);
  EXPECT_EQ(col, 0);
}

TEST(SimIoScreenTest, CancelInputWakesTheSimulator) {
  SimIoScreen s;
  std::string line = "unchanged";
  bool ok = true;
  std::thread sim([&] { ok = s.ReadLine(&line); });
  WaitForInput(s);
  s.CancelInput();
  sim.join();
  EXPECT_FALSE(ok);
  EXPECT_EQ(line, "unchanged");
  char ch = 0;
  EXPECT_FALSE(s.ReadChar(&ch));  // stays cancelled
  s.ResumeInput();
  s.KeyPress('x');
  EXPECT_TRUE(s.ReadChar(&ch));
  EXPECT_EQ(ch, 'x');
}

TEST(SimIoScreenTest, KeyStateAndHook) {
  SimIoScreen s;
  int downs = 0, ups = 0;
  s.SetKeyEventHook([&](bool down) { (down ? downs : ups)++; });
  s.KeyDown(0x41);
  s.KeyDown(0x25);
  long codes = 0x00002541;
  s.GetKeyState(&codes);
  EXPECT_EQ(static_cast<uint32_t>(codes), 0x0000FFFFu);
  s.KeyUp(0x41);
  codes = 0x00002541;
  s.GetKeyState(&codes);
  EXPECT_EQ(static_cast<uint32_t>(codes), 0x0000FF00u);
  codes = 0;  // last codes: up << 16 | down
  s.GetKeyState(&codes);
  EXPECT_EQ(static_cast<uint32_t>(codes), 0x00410025u);
  EXPECT_EQ(downs, 2);
  EXPECT_EQ(ups, 1);
  s.ClearKeys();
  codes = 0x25;
  s.GetKeyState(&codes);
  EXPECT_EQ(codes, 0);
}

TEST(SimIoScreenTest, KeyCommandsFlag) {
  SimIoScreen s;
  EXPECT_TRUE(s.KeyCommandsEnabled());
  s.SetKeyCommandsEnabled(false);
  EXPECT_FALSE(s.KeyCommandsEnabled());
}

}  // namespace
}  // namespace easym68k::gui
