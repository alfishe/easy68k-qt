// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// StackWindow (Task 13.5): rows of four bytes centred on the selected stack
// register, aqua/yellow/red marking, scrolling, register selector, and the
// image kept while the program runs.

#include "stack_window.h"

#include <gtest/gtest.h>

#include <QTest>

namespace easym68k::gui {
namespace {

class StackWindowTest : public ::testing::Test {
 protected:
  void SetUp() override {
    for (uint32_t i = 0; i < 0x100; i++)
      memory_.RawData()[0x2000 + i] = static_cast<uint8_t>(i);
    state_.a[0] = 0x2041;  // odd: the view centres on $2040
    state_.a[7] = 0x2080;  // US
    state_.ssp = 0x2010;   // SS
    state_.sr = 0x2000;    // supervisor: SP = SS
  }
  int RowOf(const StackWindow& w, uint32_t address) {
    for (int r = 0; r < w.Rows(); r++)
      if (w.RowText(r).startsWith(
              QStringLiteral("%1:").arg(address, 8, 16, QLatin1Char('0')).toUpper()))
        return r;
    return -1;
  }

  sim::Memory memory_;
  sim::CpuState state_{};
  bool readable_ = true;
};

TEST_F(StackWindowTest, CentresOnSsAndMarksTheStackPointer) {
  StackWindow w(&memory_, [this] { return readable_; });
  w.UpdateState(state_);
  const int rows = w.Rows();
  ASSERT_GT(rows, 4);
  const int mid = RowOf(w, 0x2010);
  EXPECT_EQ(mid, rows / 2);
  EXPECT_EQ(w.RowText(mid), QStringLiteral("00002010: 10 11 12 13 "));
  EXPECT_EQ(w.Background(mid, 0), QColor(0, 255, 255));  // SS selected and SP: aqua first
  EXPECT_EQ(w.Background(mid, 1), QColor(Qt::white));
  EXPECT_EQ(w.Foreground(mid, 0), QColor(Qt::black));
}

TEST_F(StackWindowTest, SelectorAndColours) {
  StackWindow w(&memory_, [this] { return readable_; });
  w.UpdateState(state_);
  w.Selector()->setCurrentIndex(0);  // A0 = $2041
  const int mid = RowOf(w, 0x2040);
  EXPECT_EQ(mid, w.Rows() / 2);
  EXPECT_EQ(w.Background(mid, 1), QColor(0, 255, 255));  // the odd byte itself
  const int sp = RowOf(w, 0x2010);
  if (sp >= 0) {
    EXPECT_EQ(w.Background(sp, 0), QColor(255, 255, 0));  // active SP: yellow
  }
  w.Selector()->setCurrentIndex(7);  // US
  EXPECT_EQ(RowOf(w, 0x2080), w.Rows() / 2);
  memory_.SetFlags(0x2080, 0x2081, sim::kMemInvalid);
  w.update();
  EXPECT_EQ(w.Foreground(RowOf(w, 0x2080), 0), QColor(Qt::red));
  EXPECT_EQ(w.Foreground(RowOf(w, 0x2080), 2), QColor(Qt::black));
}

TEST_F(StackWindowTest, ScrollsByRowsOnlyWhenStopped) {
  StackWindow w(&memory_, [this] { return readable_; });
  w.UpdateState(state_);
  const int mid = w.Rows() / 2;
  w.ScrollRows(1);
  EXPECT_TRUE(w.RowText(mid).startsWith(QStringLiteral("00002014:")));
  w.ScrollRows(-2);
  EXPECT_TRUE(w.RowText(mid).startsWith(QStringLiteral("0000200C:")));
  readable_ = false;  // running
  w.ScrollRows(5);
  EXPECT_TRUE(w.RowText(mid).startsWith(QStringLiteral("0000200C:")));
}

TEST_F(StackWindowTest, CtrlTabAsksForTheNextWindow) {
  StackWindow w(&memory_, [this] { return readable_; });
  int next = 0;
  QObject::connect(&w, &StackWindow::NextWindowRequested, [&] { next++; });
  QTest::keyClick(&w, Qt::Key_Tab, Qt::ControlModifier);
  EXPECT_EQ(next, 1);
}

}  // namespace
}  // namespace easym68k::gui
