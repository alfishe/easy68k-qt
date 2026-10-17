// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// BreakpointsWindow (Task 13.22): Register milestones, Memory milestones,
// Expression Builder (infix to postfix, grammar button states, Backspace,
// decimal pass count), and live simulator breaks.

#include "breakpoints_window.h"

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "easym68k/asm/assembler.h"
#include "mainwindow.h"
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

const char kLoopProgram[] =
    "        ORG     0\n"
    "        DC.L    $8000,START\n"
    "        ORG     $1000\n"
    "START   MOVEQ   #0,D0\n"
    "LOOP    ADDQ.L  #1,D0\n"
    "        MOVE.W  D0,$2000\n"
    "        CMP.L   #10,D0\n"
    "        BLT     LOOP\n"
    "        SIMHALT\n"
    "        END     START\n";

class BreakpointsWindowTest : public ::testing::Test {
 protected:
  void SetUp() override {
    window_ = std::make_unique<BreakpointsWindow>(&session_);
    window_->SetNotify([this](const QString& msg) { notices_.push_back(msg); });
    window_->show();
  }

  QString Assemble(const QString& name, const std::string& source) {
    asm_::Assembler assembler;
    assembler.ConfigureListing(name.toStdString() + ".X68", "");
    auto r = assembler.Assemble(source);
    EXPECT_TRUE(r.success) << (r.errors.empty() ? "" : r.errors[0].message);
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
  SimSession session_;
  std::unique_ptr<BreakpointsWindow> window_;
  QStringList notices_;
};

TEST_F(BreakpointsWindowTest, RegMilestoneSetEditClearAndClearAll) {
  // Skipping ahead beyond reg_count_ + 1 warns.
  window_->OpenRegRow(2);
  EXPECT_EQ(notices_.size(), 1);
  EXPECT_FALSE(window_->RegSetButton()->isEnabled());

  // Add R1: D0 == 00000003 (L).
  window_->OpenRegRow(0);
  ASSERT_TRUE(window_->RegSetButton()->isEnabled());
  window_->RegSelectCombo()->setCurrentIndex(0);  // D0
  window_->RegOpCombo()->setCurrentIndex(0);      // ==
  window_->RegSizeCombo()->setCurrentIndex(2);    // L
  window_->RegValueEdit()->setText(QStringLiteral("3"));
  window_->RegSetButton()->click();

  EXPECT_EQ(window_->RegCount(), 1);
  EXPECT_EQ(window_->CellText(window_->RegGrid(), 0, 0), QStringLiteral("D0"));
  EXPECT_EQ(window_->CellText(window_->RegGrid(), 0, 1), QStringLiteral("=="));
  EXPECT_EQ(window_->CellText(window_->RegGrid(), 0, 2),
            QStringLiteral("00000003"));
  EXPECT_EQ(window_->CellText(window_->RegGrid(), 0, 3), QStringLiteral("L"));

  // Add R2: A0 != 1234 (W).
  window_->OpenRegRow(1);
  window_->RegSelectCombo()->setCurrentIndex(8);  // A0
  window_->RegOpCombo()->setCurrentIndex(1);      // !=
  window_->RegSizeCombo()->setCurrentIndex(1);    // W
  window_->RegValueEdit()->setText(QStringLiteral("1234"));
  window_->RegSetButton()->click();
  EXPECT_EQ(window_->RegCount(), 2);

  // Reopen R2 and verify fields populate from stored condition.
  window_->OpenRegRow(1);
  EXPECT_EQ(window_->RegSelectCombo()->currentIndex(), 8);
  EXPECT_EQ(window_->RegOpCombo()->currentIndex(), 1);
  EXPECT_EQ(window_->RegSizeCombo()->currentIndex(), 1);
  EXPECT_EQ(window_->RegValueEdit()->text(), QStringLiteral("1234"));

  // Clear R1: R2 shifts into R1.
  window_->SelectRegRow(0);
  ASSERT_TRUE(window_->RegClearButton()->isEnabled());
  window_->RegClearButton()->click();
  EXPECT_EQ(window_->RegCount(), 1);
  EXPECT_EQ(window_->CellText(window_->RegGrid(), 0, 0), QStringLiteral("A0"));
  EXPECT_EQ(window_->CellText(window_->RegGrid(), 1, 0), QString());

  // Clear All removes remaining rows.
  window_->RegClearAllButton()->click();
  EXPECT_EQ(window_->RegCount(), 0);
  EXPECT_EQ(window_->CellText(window_->RegGrid(), 0, 0), QString());
}

TEST_F(BreakpointsWindowTest, AddrMilestoneSetEditClearAndClearAll) {
  window_->OpenAddrRow(0);
  window_->AddrSelectEdit()->setText(QStringLiteral("2000"));
  window_->AddrOpCombo()->setCurrentIndex(3);    // >=
  window_->AddrSizeCombo()->setCurrentIndex(1);  // W
  window_->AddrValueEdit()->setText(QStringLiteral("5"));
  window_->AddrRwCombo()->setCurrentIndex(2);    // Write
  window_->AddrSetButton()->click();

  EXPECT_EQ(window_->AddrCount(), 1);
  EXPECT_EQ(window_->CellText(window_->AddrGrid(), 0, 0),
            QStringLiteral("002000"));
  EXPECT_EQ(window_->CellText(window_->AddrGrid(), 0, 1), QStringLiteral(">="));
  EXPECT_EQ(window_->CellText(window_->AddrGrid(), 0, 2),
            QStringLiteral("0005"));
  EXPECT_EQ(window_->CellText(window_->AddrGrid(), 0, 3), QStringLiteral("W"));
  EXPECT_EQ(window_->CellText(window_->AddrGrid(), 0, 4),
            QStringLiteral("Write"));

  // Reopen A1: verifies fix for BREAKPOINTSu.cpp bug where AddrStringGridDblClick
  // read breakPoints[reg_edit_row - 1] instead of breakPoints[addr_edit_row - 1 + 50].
  window_->OpenAddrRow(0);
  EXPECT_EQ(window_->AddrSelectEdit()->text(), QStringLiteral("002000"));
  EXPECT_EQ(window_->AddrOpCombo()->currentIndex(), 3);
  EXPECT_EQ(window_->AddrSizeCombo()->currentIndex(), 1);
  EXPECT_EQ(window_->AddrValueEdit()->text(), QStringLiteral("0005"));
  EXPECT_EQ(window_->AddrRwCombo()->currentIndex(), 2);

  window_->SelectAddrRow(0);
  window_->AddrClearButton()->click();
  EXPECT_EQ(window_->AddrCount(), 0);
}

TEST_F(BreakpointsWindowTest, ExpressionBuilderGrammarPrecedenceAndBackspace) {
  // Set up R1, R2 and A1.
  window_->OpenRegRow(0);
  window_->RegSelectCombo()->setCurrentIndex(0);
  window_->RegValueEdit()->setText(QStringLiteral("1"));
  window_->RegSetButton()->click();

  window_->OpenRegRow(1);
  window_->RegSelectCombo()->setCurrentIndex(0);
  window_->RegValueEdit()->setText(QStringLiteral("2"));
  window_->RegSetButton()->click();

  window_->OpenAddrRow(0);
  window_->AddrSelectEdit()->setText(QStringLiteral("2000"));
  window_->AddrValueEdit()->setText(QStringLiteral("5"));
  window_->AddrSetButton()->click();

  // Open new expression row E1.
  window_->OpenExprRow(0);
  EXPECT_EQ(window_->ExprCountEdit()->text(), QStringLiteral("01"));
  EXPECT_TRUE(window_->ExprRegAppendButton()->isEnabled());
  EXPECT_TRUE(window_->ExprAddrAppendButton()->isEnabled());
  EXPECT_TRUE(window_->ExprLParenButton()->isEnabled());
  EXPECT_FALSE(window_->ExprAndButton()->isEnabled());
  EXPECT_FALSE(window_->ExprOrButton()->isEnabled());
  EXPECT_FALSE(window_->ExprRParenButton()->isEnabled());
  EXPECT_FALSE(window_->ExprSetButton()->isEnabled());

  // Appending out-of-range R9 warns and does not lock operand buttons.
  notices_.clear();
  window_->ExprRegEdit()->setText(QStringLiteral("09"));
  window_->ExprRegAppendButton()->click();
  EXPECT_EQ(notices_.size(), 1);
  EXPECT_TRUE(window_->ExprRegAppendButton()->isEnabled());

  // Build "( R1   OR   A1  ) AND  R2  ", testing Backspace after A1.
  window_->ExprLParenButton()->click();
  window_->ExprRegEdit()->setText(QStringLiteral("01"));
  window_->ExprRegAppendButton()->click();
  window_->ExprOrButton()->click();
  window_->ExprAddrEdit()->setText(QStringLiteral("01"));
  window_->ExprAddrAppendButton()->click();
  EXPECT_EQ(window_->ExprEdit()->text(), QStringLiteral("( R1   OR   A1  "));

  // Backspace removes " A1  " (5 chars) cleanly, leaving "( R1   OR  ".
  window_->ExprBackspaceButton()->click();
  EXPECT_EQ(window_->ExprEdit()->text(), QStringLiteral("( R1   OR  "));
  EXPECT_TRUE(window_->ExprAddrAppendButton()->isEnabled());

  // Re-append A1, close paren, AND R2, Count = 10 (decimal!).
  window_->ExprAddrAppendButton()->click();
  window_->ExprRParenButton()->click();
  window_->ExprAndButton()->click();
  window_->ExprRegEdit()->setText(QStringLiteral("02"));
  window_->ExprRegAppendButton()->click();
  window_->ExprCountEdit()->setText(QStringLiteral("10"));
  ASSERT_TRUE(window_->ExprSetButton()->isEnabled());
  window_->ExprSetButton()->click();

  EXPECT_EQ(window_->ExprCount(), 1);
  EXPECT_EQ(window_->CellText(window_->ExprGrid(), 0, 0), QStringLiteral("ON"));
  EXPECT_EQ(window_->CellText(window_->ExprGrid(), 0, 1),
            QStringLiteral("( R1   OR   A1  ) AND  R2  "));
  EXPECT_EQ(window_->CellText(window_->ExprGrid(), 0, 2), QStringLiteral("10"));

  const auto& sim_exprs = session_.Simulator().GetBreakExpressions();
  ASSERT_EQ(sim_exprs.size(), 1u);
  EXPECT_EQ(sim_exprs[0].count, 10);
  // Postfix for (R1 OR A1) AND R2: [0, 50, kOr(101), 1, kAnd(100)].
  const std::vector<int> expected_postfix = {
      0, 50, sim::BreakExpression::kOr, 1, sim::BreakExpression::kAnd};
  EXPECT_EQ(sim_exprs[0].postfix, expected_postfix);
}

TEST_F(BreakpointsWindowTest, BreaksExecutionOnMemoryAndPassCount) {
  ASSERT_TRUE(session_.Open(Assemble("loop", kLoopProgram)));

  // R1: D0 >= 1 (L).
  window_->OpenRegRow(0);
  window_->RegSelectCombo()->setCurrentIndex(0);  // D0
  window_->RegOpCombo()->setCurrentIndex(3);      // >=
  window_->RegSizeCombo()->setCurrentIndex(2);    // L
  window_->RegValueEdit()->setText(QStringLiteral("1"));
  window_->RegSetButton()->click();

  // A1: $002000 == 0004 (W, Write).
  window_->OpenAddrRow(0);
  window_->AddrSelectEdit()->setText(QStringLiteral("2000"));
  window_->AddrOpCombo()->setCurrentIndex(0);    // ==
  window_->AddrSizeCombo()->setCurrentIndex(1);  // W
  window_->AddrValueEdit()->setText(QStringLiteral("4"));
  window_->AddrRwCombo()->setCurrentIndex(2);    // Write
  window_->AddrSetButton()->click();

  // E1: A1 (ON, Count 01) -> breaks when $2000 == 4 is written.
  window_->OpenExprRow(0);
  window_->ExprAddrEdit()->setText(QStringLiteral("01"));
  window_->ExprAddrAppendButton()->click();
  window_->ExprSetButton()->click();

  session_.Run();
  ASSERT_TRUE(Until([this] { return !session_.IsRunning(); }));
  EXPECT_EQ(session_.State(), RunState::kReady);
  EXPECT_EQ(session_.Simulator().State().d[0], 4u);

  // Replace E1 with R1 (D0 >= 1) and decimal Count = 10:
  // Rewind and run -> breaks after 10 instructions where D0 >= 1.
  window_->OpenExprRow(0);
  window_->ExprBackspaceButton()->click();
  window_->ExprRegEdit()->setText(QStringLiteral("01"));
  window_->ExprRegAppendButton()->click();
  window_->ExprCountEdit()->setText(QStringLiteral("10"));
  window_->ExprSetButton()->click();

  session_.Rewind();
  session_.Run();
  ASSERT_TRUE(Until([this] { return !session_.IsRunning(); }));
  EXPECT_EQ(session_.State(), RunState::kReady);
  // D0 reaches 3 on the 10th instruction after D0 becomes >= 1 (4 instructions per loop).
  EXPECT_EQ(session_.Simulator().State().d[0], 3u);
}

TEST_F(BreakpointsWindowTest, MainWindowViewMenuAndCtrlTab) {
  MainWindow main_win{QString()};
  ASSERT_NE(main_win.Breakpoints(), nullptr);
  EXPECT_FALSE(main_win.Breakpoints()->isVisible());

  main_win.BreakPointsAction()->trigger();
  EXPECT_TRUE(main_win.Breakpoints()->isVisible());

  QSignalSpy spy(main_win.Breakpoints(),
                 &BreakpointsWindow::NextWindowRequested);
  QTest::keyClick(main_win.Breakpoints(), Qt::Key_Tab, Qt::ControlModifier);
  EXPECT_EQ(spy.count(), 1);
}

}  // namespace
}  // namespace easym68k::gui
