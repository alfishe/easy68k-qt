// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The advanced breakpoints window ("EASy68K Advanced Break Points",
// BREAKPOINTSu.dfm / BREAKPOINTSu.cpp):
//  - Registers (up to 50 conditions on D0-D7, A0-A7, PC with ==, !=, >, >=,
//    <, <= and size B/W/L);
//  - Memory (up to 50 conditions on a 24-bit address with ==, !=, >, >=, <,
//    <=, N/A, size B/W/L and access R/W, Read, Write, N/A);
//  - Expression Builder (up to 50 infix expressions of up to 10 tokens over
//    R1..R50 and A1..A50 combined with AND, OR and parentheses, converted to
//    postfix for the simulator's breakpoint engine, with ON/OFF and pass
//    count 00..99).

#ifndef EASY68K_GUI_SIMULATOR_BREAKPOINTS_WINDOW_H_
#define EASY68K_GUI_SIMULATOR_BREAKPOINTS_WINDOW_H_

#include <array>
#include <functional>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>
#include <vector>

#include "easym68k/sim/break_condition.h"

namespace easym68k::gui {

class SimSession;

class BreakpointsWindow : public QWidget {
  Q_OBJECT

 public:
  static constexpr int kMaxRegRows = 50;
  static constexpr int kMaxAddrRows = 50;
  static constexpr int kMaxExprRows = 50;
  static constexpr int kAddrIdOffset = 50;
  static constexpr int kMaxInfixTokens = 10;  // MAX_LB_NODES

  explicit BreakpointsWindow(SimSession* session, QWidget* parent = nullptr);
  ~BreakpointsWindow() override = default;

  // Message box callback (replaceable for tests).
  void SetNotify(std::function<void(const QString&)> notify) { notify_ = std::move(notify); }

  // Row selection (single click) and inline editor opening (double click),
  // 0-based row index (0..49).
  void SelectRegRow(int row);
  void OpenRegRow(int row);
  void SelectAddrRow(int row);
  void OpenAddrRow(int row);
  void SelectExprRow(int row);
  void OpenExprRow(int row);

  int RegCount() const { return reg_count_; }
  int AddrCount() const { return addr_count_; }
  int ExprCount() const { return expr_count_; }
  QString CellText(const QTableWidget* grid, int row, int col) const;

  // Widgets, for tests.
  QTableWidget* RegGrid() const { return reg_grid_; }
  QComboBox* RegSelectCombo() const { return reg_select_; }
  QComboBox* RegOpCombo() const { return reg_op_; }
  QLineEdit* RegValueEdit() const { return reg_value_; }
  QComboBox* RegSizeCombo() const { return reg_size_; }
  QPushButton* RegSetButton() const { return reg_set_; }
  QPushButton* RegClearButton() const { return reg_clear_; }
  QPushButton* RegClearAllButton() const { return reg_clear_all_; }

  QTableWidget* AddrGrid() const { return addr_grid_; }
  QLineEdit* AddrSelectEdit() const { return addr_select_; }
  QComboBox* AddrOpCombo() const { return addr_op_; }
  QLineEdit* AddrValueEdit() const { return addr_value_; }
  QComboBox* AddrSizeCombo() const { return addr_size_; }
  QComboBox* AddrRwCombo() const { return addr_rw_; }
  QPushButton* AddrSetButton() const { return addr_set_; }
  QPushButton* AddrClearButton() const { return addr_clear_; }
  QPushButton* AddrClearAllButton() const { return addr_clear_all_; }

  QTableWidget* ExprGrid() const { return expr_grid_; }
  QComboBox* ExprEnabledCombo() const { return expr_enabled_; }
  QLineEdit* ExprEdit() const { return expr_edit_; }
  QLineEdit* ExprCountEdit() const { return expr_count_edit_; }
  QPushButton* ExprSetButton() const { return expr_set_; }
  QPushButton* ExprClearButton() const { return expr_clear_; }
  QPushButton* ExprClearAllButton() const { return expr_clear_all_; }
  QPushButton* ExprRegAppendButton() const { return expr_reg_append_; }
  QLineEdit* ExprRegEdit() const { return expr_reg_edit_; }
  QPushButton* ExprAddrAppendButton() const { return expr_addr_append_; }
  QLineEdit* ExprAddrEdit() const { return expr_addr_edit_; }
  QPushButton* ExprAndButton() const { return expr_and_; }
  QPushButton* ExprOrButton() const { return expr_or_; }
  QPushButton* ExprLParenButton() const { return expr_lparen_; }
  QPushButton* ExprRParenButton() const { return expr_rparen_; }
  QPushButton* ExprBackspaceButton() const { return expr_backspace_; }

 signals:
  void NextWindowRequested();  // Ctrl+Tab (main window)

 protected:
  void keyPressEvent(QKeyEvent* event) override;

 private:
  static constexpr int kTokenAnd = sim::BreakExpression::kAnd;
  static constexpr int kTokenOr = sim::BreakExpression::kOr;
  static constexpr int kTokenLParen = sim::kMaxBreakConditions + 2;
  static constexpr int kTokenRParen = sim::kMaxBreakConditions + 3;

  struct StoredExpr {
    sim::BreakExpression expr;
    std::vector<int> infix;
  };

  void BuildRegistersGroup();
  void BuildMemoryGroup();
  void BuildExpressionsGroup();

  void HideRegEditors();
  void HideAddrEditors();
  void HideExprEditors();
  void UpdateRegButtons();
  void UpdateAddrButtons();
  void UpdateExprButtons();

  void OnRegSet();
  void OnRegClear();
  void OnRegClearAll();

  void OnAddrSet();
  void OnAddrClear();
  void OnAddrClearAll();

  void OnExprSet();
  void OnExprClear();
  void OnExprClearAll();
  void OnExprRegAppend();
  void OnExprAddrAppend();
  void OnExprAndAppend();
  void OnExprOrAppend();
  void OnExprLParenAppend();
  void OnExprRParenAppend();
  void OnExprBackspace();

  static std::vector<int> InfixToPostfix(const std::vector<int>& infix);
  static int SizeFromComboIndex(int index);
  static int ComboIndexFromSize(int size);
  void SyncToSimulator();

  SimSession* session_;
  std::function<void(const QString&)> notify_;

  // Registers group
  QTableWidget* reg_grid_ = nullptr;
  QComboBox* reg_select_ = nullptr;
  QComboBox* reg_op_ = nullptr;
  QLineEdit* reg_value_ = nullptr;
  QComboBox* reg_size_ = nullptr;
  QPushButton* reg_set_ = nullptr;
  QPushButton* reg_clear_ = nullptr;
  QPushButton* reg_clear_all_ = nullptr;
  bool reg_editing_ = false;
  int reg_edit_row_ = 1;  // 1-based (1..50), as in BREAKPOINTSu.cpp
  int reg_count_ = 0;

  // Memory group
  QTableWidget* addr_grid_ = nullptr;
  QLineEdit* addr_select_ = nullptr;
  QComboBox* addr_op_ = nullptr;
  QLineEdit* addr_value_ = nullptr;
  QComboBox* addr_size_ = nullptr;
  QComboBox* addr_rw_ = nullptr;
  QPushButton* addr_set_ = nullptr;
  QPushButton* addr_clear_ = nullptr;
  QPushButton* addr_clear_all_ = nullptr;
  bool addr_editing_ = false;
  int addr_edit_row_ = 1;  // 1-based (1..50)
  int addr_count_ = 0;

  // Expression Builder group
  QTableWidget* expr_grid_ = nullptr;
  QComboBox* expr_enabled_ = nullptr;
  QLineEdit* expr_edit_ = nullptr;
  QLineEdit* expr_count_edit_ = nullptr;
  QPushButton* expr_set_ = nullptr;
  QPushButton* expr_clear_ = nullptr;
  QPushButton* expr_clear_all_ = nullptr;
  QPushButton* expr_reg_append_ = nullptr;
  QLineEdit* expr_reg_edit_ = nullptr;
  QPushButton* expr_addr_append_ = nullptr;
  QLineEdit* expr_addr_edit_ = nullptr;
  QPushButton* expr_and_ = nullptr;
  QPushButton* expr_or_ = nullptr;
  QPushButton* expr_lparen_ = nullptr;
  QPushButton* expr_rparen_ = nullptr;
  QPushButton* expr_backspace_ = nullptr;
  bool expr_editing_ = false;
  int expr_edit_row_ = 1;  // 1-based (1..50)
  int expr_count_ = 0;
  std::vector<int> infix_;
  int paren_count_ = 0;
  bool mru_operand_ = false;
  bool mru_operator_ = false;

  std::array<sim::BreakCondition, sim::kMaxBreakConditions> conditions_{};
  std::array<StoredExpr, kMaxExprRows> expressions_{};
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_BREAKPOINTS_WINDOW_H_
