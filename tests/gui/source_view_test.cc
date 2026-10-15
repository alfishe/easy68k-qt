// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// SourceView (Task 13.4): listing display with expanded tabs, PC highlight
// as highlight(), breakpoint gutter clicks, double click to set the PC.

#include "source_view.h"

#include <gtest/gtest.h>

#include <QSignalSpy>
#include <QTest>
#include <set>

#include "find_dialog.h"
#include "mainwindow.h"

namespace easym68k::gui {
namespace {

QStringList Sample() {
  return {
      QStringLiteral("00001000 Starting Address"),
      QStringLiteral("Assembler used: EASy68K Editor/Assembler v5.16.01"),
      QStringLiteral("Created On: "),
      QStringLiteral("00001000                             1  START:"),
      QStringLiteral("00001000  4E71                       2  \tNOP"),
      QStringLiteral("00001002  4E71                       3          NOP"),
      QStringLiteral("00001004                             4  * comment"),
      QStringLiteral("00001004  FFFF FFFF                  5          SIMHALT"),
      QStringLiteral("00001008                             6  * after"),
  };
}

class SourceViewTest : public ::testing::Test {
 protected:
  void SetUp() override {
    view_.resize(700, 400);
    view_.SetListing(Sample(), true);
    view_.show();
  }
  // A point on the gutter beside `row`.
  QPoint GutterPoint(int row) {
    QListWidget* list = view_.List();
    const QRect r = list->visualItemRect(list->item(row));
    const QPoint in_view = list->viewport()->mapTo(&view_, r.center());
    return QPoint(5, in_view.y() - view_.Gutter()->mapTo(&view_, QPoint(0, 0)).y());
  }

  SourceView view_;
};

TEST_F(SourceViewTest, TabsAreExpandedToEightColumns) {
  const QString shown = view_.List()->item(4)->text();
  EXPECT_FALSE(shown.contains(QLatin1Char('\t')));
  const int tab = Sample()[4].indexOf(QLatin1Char('\t'));
  EXPECT_EQ(shown.indexOf(QStringLiteral("NOP")), (tab / 8 + 1) * 8);
}

TEST_F(SourceViewTest, CodeLines) {
  EXPECT_FALSE(view_.CodeLineAddress(0).has_value());  // header
  EXPECT_FALSE(view_.CodeLineAddress(3).has_value());  // label only
  ASSERT_TRUE(view_.CodeLineAddress(4).has_value());
  EXPECT_EQ(*view_.CodeLineAddress(4), 0x1000u);
  EXPECT_FALSE(view_.CodeLineAddress(6).has_value());  // comment
}

TEST_F(SourceViewTest, HighlightThePcLine) {
  view_.HighlightPc(0x1002, false);
  EXPECT_EQ(view_.List()->currentRow(), 5);
  view_.HighlightPc(0x1004, false);
  EXPECT_EQ(view_.List()->currentRow(), 7);  // the code line, not the comment
  view_.HighlightPc(0x1008, false);          // no code line: selection kept
  EXPECT_EQ(view_.List()->currentRow(), 7);
  view_.HighlightPc(0x1008, true);  // halted: any line with the address
  EXPECT_EQ(view_.List()->currentRow(), 8);
  view_.List()->setCurrentRow(5);
  ASSERT_TRUE(view_.SelectedAddress().has_value());
  EXPECT_EQ(*view_.SelectedAddress(), 0x1002u);
  view_.List()->setCurrentRow(6);
  EXPECT_FALSE(view_.SelectedAddress().has_value());
}

TEST_F(SourceViewTest, GutterClickTogglesBreakpoints) {
  std::set<uint32_t> bps;
  view_.SetBreakpointQuery([&](uint32_t a) { return bps.count(a) != 0; });
  QObject::connect(&view_, &SourceView::BreakpointToggled, [&](uint32_t a) {
    if (!bps.erase(a))
      bps.insert(a);
  });
  QTest::mouseClick(view_.Gutter(), Qt::LeftButton, Qt::NoModifier, GutterPoint(5));
  EXPECT_EQ(bps, std::set<uint32_t>{0x1002});
  QTest::mouseClick(view_.Gutter(), Qt::LeftButton, Qt::NoModifier, GutterPoint(6));  // comment
  EXPECT_EQ(bps, std::set<uint32_t>{0x1002});
  QTest::mouseClick(view_.Gutter(), Qt::LeftButton, Qt::NoModifier, GutterPoint(5));
  EXPECT_TRUE(bps.empty());
  QTest::mouseClick(view_.Gutter(), Qt::RightButton, Qt::NoModifier, GutterPoint(4));
  EXPECT_TRUE(bps.empty());  // right button: the popup menu, not a toggle
}

TEST_F(SourceViewTest, DoubleClickRequestsThePc) {
  QSignalSpy pc(&view_, &SourceView::PcRequested);
  QListWidget* list = view_.List();
  auto dclick = [&](int row) {  // a double click: press/release, then double-click
    const QPoint p = list->visualItemRect(list->item(row)).center();
    QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::NoModifier, p);
    QTest::mouseDClick(list->viewport(), Qt::LeftButton, Qt::NoModifier, p);
  };
  dclick(7);
  ASSERT_EQ(pc.count(), 1);
  EXPECT_EQ(pc.last().at(0).toUInt(), 0x1004u);
  dclick(6);
  EXPECT_EQ(pc.count(), 1);  // not a code line
}

TEST_F(SourceViewTest, NoListingTextHasNoCodeLines) {
  view_.SetListing({QStringLiteral("00001000  4E71   no listing")}, false);
  EXPECT_FALSE(view_.CodeLineAddress(0).has_value());
  view_.HighlightPc(0x1000, false);
  EXPECT_EQ(view_.List()->currentRow(), -1);
}

TEST_F(SourceViewTest, FindAndFindNextCaseInsensitive) {
  // Search starts at row 3 (skips header rows 0..2).
  EXPECT_FALSE(view_.Find(QStringLiteral("Starting Address"), false));

  // First match for "nop" (case-insensitive) is row 4.
  EXPECT_TRUE(view_.Find(QStringLiteral("nop"), false));
  EXPECT_EQ(view_.List()->currentRow(), 4);

  // Next match is row 5.
  EXPECT_TRUE(view_.Find(QStringLiteral("nop"), true));
  EXPECT_EQ(view_.List()->currentRow(), 5);

  // No third match; selection stays at row 5.
  EXPECT_FALSE(view_.Find(QStringLiteral("nop"), true));
  EXPECT_EQ(view_.List()->currentRow(), 5);

  // next = false restarts from top (row 3).
  EXPECT_TRUE(view_.Find(QStringLiteral("nop"), false));
  EXPECT_EQ(view_.List()->currentRow(), 4);
}

TEST_F(SourceViewTest, FindDialogAndGotoPcInMainWindow) {
  MainWindow w{QString()};
  w.Source()->SetListing(Sample(), true);
  w.Session().Simulator().State().pc = 0x1004;

  ASSERT_NE(w.FindDialogView(), nullptr);
  EXPECT_FALSE(w.FindDialogView()->isVisible());
  EXPECT_FALSE(w.FindDialogView()->FindNextButton()->isEnabled());

  w.FindAction()->trigger();
  EXPECT_TRUE(w.FindDialogView()->isVisible());

  w.FindDialogView()->SetText(QStringLiteral("nop"));
  ASSERT_TRUE(w.FindDialogView()->FindNextButton()->isEnabled());

  // First click searches from top (row 4); second click searches next (row 5).
  w.FindDialogView()->FindNextButton()->click();
  EXPECT_EQ(w.Source()->List()->currentRow(), 4);
  w.FindDialogView()->FindNextButton()->click();
  EXPECT_EQ(w.Source()->List()->currentRow(), 5);

  // Changing text resets to search from top; F3 in dialog finds next.
  w.FindDialogView()->SetText(QStringLiteral("NOP"));
  w.FindDialogView()->FindNextButton()->click();
  EXPECT_EQ(w.Source()->List()->currentRow(), 4);
  QTest::keyClick(w.FindDialogView(), Qt::Key_F3);
  EXPECT_EQ(w.Source()->List()->currentRow(), 5);

  // Go to PC (Ctrl+G) highlights the PC line (row 7 for $1004).
  w.GotoPcAction()->trigger();
  EXPECT_EQ(w.Source()->List()->currentRow(), 7);
}

}  // namespace
}  // namespace easym68k::gui
