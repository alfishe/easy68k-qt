// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "breakpoints_window.h"

#include <algorithm>
#include <cstdint>
#include <QAbstractItemView>
#include <QApplication>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

#include "easym68k/sim/simulator.h"
#include "sim_session.h"

namespace easym68k::gui {

namespace {

void InitGrid(QTableWidget* grid, int rows, const QStringList& headers,
              const QString& row_prefix) {
  grid->setRowCount(rows);
  grid->setColumnCount(headers.size());
  grid->setHorizontalHeaderLabels(headers);
  QStringList vheaders;
  vheaders.reserve(rows);
  for (int r = 1; r <= rows; ++r) {
    vheaders.push_back(QString("%1%2").arg(row_prefix).arg(r));
  }
  grid->setVerticalHeaderLabels(vheaders);
  grid->setEditTriggers(QAbstractItemView::NoEditTriggers);
  grid->setSelectionBehavior(QAbstractItemView::SelectRows);
  grid->setSelectionMode(QAbstractItemView::SingleSelection);
  grid->horizontalHeader()->setStretchLastSection(true);
  grid->verticalHeader()->setDefaultSectionSize(20);
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < headers.size(); ++c) {
      grid->setItem(r, c, new QTableWidgetItem(QString()));
    }
  }
}

void AttachValidator(QLineEdit* edit, const QString& pattern) {
  auto* validator =
      new QRegularExpressionValidator(QRegularExpression(pattern), edit);
  edit->setValidator(validator);
  QObject::connect(edit, &QLineEdit::inputRejected,
                   []() { QApplication::beep(); });
}

QString FormatHexForSize(const QString& raw, int size_combo_index) {
  bool ok = false;
  const uint32_t value = raw.toUInt(&ok, 16);
  switch (size_combo_index) {
    case 0:
      return QString("%1").arg(value & 0xFFu, 2, 16, QChar('0')).toUpper();
    case 1:
      return QString("%1").arg(value & 0xFFFFu, 4, 16, QChar('0')).toUpper();
    default:
      return QString("%1").arg(value, 8, 16, QChar('0')).toUpper();
  }
}

}  // namespace

BreakpointsWindow::BreakpointsWindow(SimSession* session, QWidget* parent)
    : QWidget(parent, Qt::Window),
      session_(session),
      notify_([this](const QString& msg) {
        QMessageBox::information(this, tr("Notice"), msg);
      }) {
  setWindowTitle(tr("EASy68K Advanced Break Points"));
  resize(456, 513);

  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(6, 6, 6, 6);
  root->setSpacing(6);

  BuildRegistersGroup();
  BuildMemoryGroup();
  BuildExpressionsGroup();

  HideRegEditors();
  HideAddrEditors();
  HideExprEditors();
  UpdateRegButtons();
  UpdateAddrButtons();
  UpdateExprButtons();
}

void BreakpointsWindow::BuildRegistersGroup() {
  auto* group = new QGroupBox(tr("Registers"), this);
  auto* layout = new QHBoxLayout(group);

  reg_grid_ = new QTableWidget(group);
  InitGrid(reg_grid_, kMaxRegRows,
           {tr("Reg"), tr("Op"), tr("Value"), tr("Size")}, QStringLiteral("R"));
  layout->addWidget(reg_grid_, 1);

  auto* controls = new QVBoxLayout();
  auto* editor_row = new QHBoxLayout();
  reg_select_ = new QComboBox(group);
  reg_select_->addItems({QStringLiteral("D0"), QStringLiteral("D1"),
                         QStringLiteral("D2"), QStringLiteral("D3"),
                         QStringLiteral("D4"), QStringLiteral("D5"),
                         QStringLiteral("D6"), QStringLiteral("D7"),
                         QStringLiteral("A0"), QStringLiteral("A1"),
                         QStringLiteral("A2"), QStringLiteral("A3"),
                         QStringLiteral("A4"), QStringLiteral("A5"),
                         QStringLiteral("A6"), QStringLiteral("A7"),
                         QStringLiteral("PC")});
  reg_select_->setCurrentIndex(16);  // PC

  reg_op_ = new QComboBox(group);
  reg_op_->addItems({QStringLiteral("=="), QStringLiteral("!="),
                     QStringLiteral(">"), QStringLiteral(">="),
                     QStringLiteral("<"), QStringLiteral("<=")});
  reg_op_->setCurrentIndex(0);

  reg_value_ = new QLineEdit(QStringLiteral("00000000"), group);
  reg_value_->setMaxLength(8);
  AttachValidator(reg_value_, QStringLiteral("^[0-9A-Fa-f]{0,8}$"));

  reg_size_ = new QComboBox(group);
  reg_size_->addItems(
      {QStringLiteral("B"), QStringLiteral("W"), QStringLiteral("L")});
  reg_size_->setCurrentIndex(2);  // L

  editor_row->addWidget(reg_select_);
  editor_row->addWidget(reg_op_);
  editor_row->addWidget(reg_value_);
  editor_row->addWidget(reg_size_);
  controls->addLayout(editor_row);

  reg_set_ = new QPushButton(tr("Set"), group);
  reg_clear_ = new QPushButton(tr("Clear"), group);
  reg_clear_all_ = new QPushButton(tr("Clear All"), group);
  controls->addWidget(reg_set_);
  controls->addWidget(reg_clear_);
  controls->addWidget(reg_clear_all_);
  controls->addStretch(1);
  layout->addLayout(controls);

  static_cast<QVBoxLayout*>(this->layout())->addWidget(group, 1);

  connect(reg_grid_, &QTableWidget::cellClicked, this,
          [this](int row, int) { SelectRegRow(row); });
  connect(reg_grid_, &QTableWidget::cellDoubleClicked, this,
          [this](int row, int) { OpenRegRow(row); });
  connect(reg_size_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [this](int idx) {
            const int max_len = (idx == 0) ? 2 : (idx == 1) ? 4 : 8;
            reg_value_->setMaxLength(max_len);
            reg_value_->setText(FormatHexForSize(reg_value_->text(), idx));
          });
  connect(reg_set_, &QPushButton::clicked, this, &BreakpointsWindow::OnRegSet);
  connect(reg_clear_, &QPushButton::clicked, this,
          &BreakpointsWindow::OnRegClear);
  connect(reg_clear_all_, &QPushButton::clicked, this,
          &BreakpointsWindow::OnRegClearAll);
}

void BreakpointsWindow::BuildMemoryGroup() {
  auto* group = new QGroupBox(tr("Memory"), this);
  auto* layout = new QHBoxLayout(group);

  addr_grid_ = new QTableWidget(group);
  InitGrid(addr_grid_, kMaxAddrRows,
           {tr("Address"), tr("Op"), tr("Value"), tr("Size"), tr("R/W")},
           QStringLiteral("A"));
  layout->addWidget(addr_grid_, 1);

  auto* controls = new QVBoxLayout();
  auto* editor_row = new QHBoxLayout();
  addr_select_ = new QLineEdit(QStringLiteral("000000"), group);
  addr_select_->setMaxLength(6);
  AttachValidator(addr_select_, QStringLiteral("^[0-9A-Fa-f]{0,6}$"));

  addr_op_ = new QComboBox(group);
  addr_op_->addItems({QStringLiteral("=="), QStringLiteral("!="),
                      QStringLiteral(">"), QStringLiteral(">="),
                      QStringLiteral("<"), QStringLiteral("<="),
                      QStringLiteral("N/A")});
  addr_op_->setCurrentIndex(0);

  addr_value_ = new QLineEdit(QStringLiteral("00000000"), group);
  addr_value_->setMaxLength(8);
  AttachValidator(addr_value_, QStringLiteral("^[0-9A-Fa-f]{0,8}$"));

  addr_size_ = new QComboBox(group);
  addr_size_->addItems(
      {QStringLiteral("B"), QStringLiteral("W"), QStringLiteral("L")});
  addr_size_->setCurrentIndex(2);  // L

  addr_rw_ = new QComboBox(group);
  addr_rw_->addItems({QStringLiteral("R/W"), QStringLiteral("Read"),
                      QStringLiteral("Write"), QStringLiteral("N/A")});
  addr_rw_->setCurrentIndex(0);

  editor_row->addWidget(addr_select_);
  editor_row->addWidget(addr_op_);
  editor_row->addWidget(addr_value_);
  editor_row->addWidget(addr_size_);
  editor_row->addWidget(addr_rw_);
  controls->addLayout(editor_row);

  addr_set_ = new QPushButton(tr("Set"), group);
  addr_clear_ = new QPushButton(tr("Clear"), group);
  addr_clear_all_ = new QPushButton(tr("Clear All"), group);
  controls->addWidget(addr_set_);
  controls->addWidget(addr_clear_);
  controls->addWidget(addr_clear_all_);
  controls->addStretch(1);
  layout->addLayout(controls);

  static_cast<QVBoxLayout*>(this->layout())->addWidget(group, 1);

  connect(addr_grid_, &QTableWidget::cellClicked, this,
          [this](int row, int) { SelectAddrRow(row); });
  connect(addr_grid_, &QTableWidget::cellDoubleClicked, this,
          [this](int row, int) { OpenAddrRow(row); });
  connect(addr_size_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [this](int idx) {
            const int max_len = (idx == 0) ? 2 : (idx == 1) ? 4 : 8;
            addr_value_->setMaxLength(max_len);
            addr_value_->setText(FormatHexForSize(addr_value_->text(), idx));
          });
  connect(addr_set_, &QPushButton::clicked, this,
          &BreakpointsWindow::OnAddrSet);
  connect(addr_clear_, &QPushButton::clicked, this,
          &BreakpointsWindow::OnAddrClear);
  connect(addr_clear_all_, &QPushButton::clicked, this,
          &BreakpointsWindow::OnAddrClearAll);
}

void BreakpointsWindow::BuildExpressionsGroup() {
  auto* group = new QGroupBox(tr("Expression Builder"), this);
  auto* outer = new QVBoxLayout(group);

  auto* top = new QHBoxLayout();
  expr_grid_ = new QTableWidget(group);
  InitGrid(expr_grid_, kMaxExprRows,
           {tr("Status"), tr("Expression"), tr("Count")}, QStringLiteral("E"));
  top->addWidget(expr_grid_, 1);

  auto* controls = new QVBoxLayout();
  auto* editor_row = new QHBoxLayout();
  expr_enabled_ = new QComboBox(group);
  expr_enabled_->addItems({QStringLiteral("OFF"), QStringLiteral("ON")});
  expr_enabled_->setCurrentIndex(1);  // ON

  expr_edit_ = new QLineEdit(group);
  expr_edit_->setReadOnly(true);

  expr_count_edit_ = new QLineEdit(QStringLiteral("01"), group);
  expr_count_edit_->setMaxLength(2);
  AttachValidator(expr_count_edit_, QStringLiteral("^[0-9]{0,2}$"));

  editor_row->addWidget(expr_enabled_);
  editor_row->addWidget(expr_edit_, 1);
  editor_row->addWidget(expr_count_edit_);
  controls->addLayout(editor_row);

  expr_set_ = new QPushButton(tr("Set"), group);
  expr_clear_ = new QPushButton(tr("Clear"), group);
  expr_clear_all_ = new QPushButton(tr("Clear All"), group);
  controls->addWidget(expr_set_);
  controls->addWidget(expr_clear_);
  controls->addWidget(expr_clear_all_);
  controls->addStretch(1);
  top->addLayout(controls);
  outer->addLayout(top, 1);

  auto* builder_row = new QHBoxLayout();
  expr_reg_append_ = new QPushButton(tr("PC/Reg"), group);
  expr_reg_edit_ = new QLineEdit(QStringLiteral("01"), group);
  expr_reg_edit_->setMaxLength(2);
  AttachValidator(expr_reg_edit_, QStringLiteral("^[0-9]{0,2}$"));

  expr_addr_append_ = new QPushButton(tr("Memory"), group);
  expr_addr_edit_ = new QLineEdit(QStringLiteral("01"), group);
  expr_addr_edit_->setMaxLength(2);
  AttachValidator(expr_addr_edit_, QStringLiteral("^[0-9]{0,2}$"));

  expr_and_ = new QPushButton(tr("AND"), group);
  expr_or_ = new QPushButton(tr("OR"), group);
  expr_lparen_ = new QPushButton(tr("("), group);
  expr_rparen_ = new QPushButton(tr(")"), group);
  expr_backspace_ = new QPushButton(tr("Backspace"), group);

  builder_row->addWidget(expr_reg_append_);
  builder_row->addWidget(expr_reg_edit_);
  builder_row->addWidget(expr_addr_append_);
  builder_row->addWidget(expr_addr_edit_);
  builder_row->addWidget(expr_and_);
  builder_row->addWidget(expr_or_);
  builder_row->addWidget(expr_lparen_);
  builder_row->addWidget(expr_rparen_);
  builder_row->addWidget(expr_backspace_);
  outer->addLayout(builder_row);

  static_cast<QVBoxLayout*>(this->layout())->addWidget(group, 1);

  connect(expr_grid_, &QTableWidget::cellClicked, this,
          [this](int row, int) { SelectExprRow(row); });
  connect(expr_grid_, &QTableWidget::cellDoubleClicked, this,
          [this](int row, int) { OpenExprRow(row); });
  connect(expr_set_, &QPushButton::clicked, this,
          &BreakpointsWindow::OnExprSet);
  connect(expr_clear_, &QPushButton::clicked, this,
          &BreakpointsWindow::OnExprClear);
  connect(expr_clear_all_, &QPushButton::clicked, this,
          &BreakpointsWindow::OnExprClearAll);
  connect(expr_reg_append_, &QPushButton::clicked, this,
          &BreakpointsWindow::OnExprRegAppend);
  connect(expr_addr_append_, &QPushButton::clicked, this,
          &BreakpointsWindow::OnExprAddrAppend);
  connect(expr_and_, &QPushButton::clicked, this,
          &BreakpointsWindow::OnExprAndAppend);
  connect(expr_or_, &QPushButton::clicked, this,
          &BreakpointsWindow::OnExprOrAppend);
  connect(expr_lparen_, &QPushButton::clicked, this,
          &BreakpointsWindow::OnExprLParenAppend);
  connect(expr_rparen_, &QPushButton::clicked, this,
          &BreakpointsWindow::OnExprRParenAppend);
  connect(expr_backspace_, &QPushButton::clicked, this,
          &BreakpointsWindow::OnExprBackspace);
}

QString BreakpointsWindow::CellText(const QTableWidget* grid, int row,
                                    int col) const {
  if (!grid || row < 0 || row >= grid->rowCount() || col < 0 ||
      col >= grid->columnCount()) {
    return QString();
  }
  const QTableWidgetItem* item = grid->item(row, col);
  return item ? item->text() : QString();
}

void BreakpointsWindow::HideRegEditors() {
  reg_editing_ = false;
  reg_select_->setVisible(false);
  reg_op_->setVisible(false);
  reg_value_->setVisible(false);
  reg_size_->setVisible(false);
}

void BreakpointsWindow::HideAddrEditors() {
  addr_editing_ = false;
  addr_select_->setVisible(false);
  addr_op_->setVisible(false);
  addr_value_->setVisible(false);
  addr_size_->setVisible(false);
  addr_rw_->setVisible(false);
}

void BreakpointsWindow::HideExprEditors() {
  expr_editing_ = false;
  expr_enabled_->setVisible(false);
  expr_edit_->setVisible(false);
  expr_count_edit_->setVisible(false);
}

void BreakpointsWindow::UpdateRegButtons() {
  reg_set_->setEnabled(reg_editing_);
  reg_clear_->setEnabled(reg_count_ > 0 && reg_edit_row_ >= 1 &&
                         reg_edit_row_ <= reg_count_);
  reg_clear_all_->setEnabled(reg_count_ > 0);
}

void BreakpointsWindow::UpdateAddrButtons() {
  addr_set_->setEnabled(addr_editing_);
  addr_clear_->setEnabled(addr_count_ > 0 && addr_edit_row_ >= 1 &&
                          addr_edit_row_ <= addr_count_);
  addr_clear_all_->setEnabled(addr_count_ > 0);
}

void BreakpointsWindow::UpdateExprButtons() {
  const bool can_clear =
      expr_count_ > 0 && expr_edit_row_ >= 1 && expr_edit_row_ <= expr_count_;
  expr_clear_->setEnabled(can_clear);
  expr_clear_all_->setEnabled(expr_count_ > 0);

  if (!expr_editing_) {
    expr_set_->setEnabled(false);
    expr_reg_append_->setEnabled(false);
    expr_addr_append_->setEnabled(false);
    expr_and_->setEnabled(false);
    expr_or_->setEnabled(false);
    expr_lparen_->setEnabled(false);
    expr_rparen_->setEnabled(false);
    expr_backspace_->setEnabled(false);
    return;
  }

  const int cur = static_cast<int>(infix_.size());
  expr_set_->setEnabled(cur > 0 && paren_count_ == 0 && mru_operand_);

  const bool room_for_operand =
      ((cur + paren_count_) < kMaxInfixTokens) && !mru_operand_;
  expr_reg_append_->setEnabled(room_for_operand);
  expr_addr_append_->setEnabled(room_for_operand);

  const bool room_for_operator =
      ((cur + paren_count_) < (kMaxInfixTokens - 1)) && !mru_operator_ &&
      (cur > 0) && mru_operand_;
  expr_and_->setEnabled(room_for_operator);
  expr_or_->setEnabled(room_for_operator);

  const bool room_for_lparen =
      ((cur + paren_count_) < (kMaxInfixTokens - 3)) && !mru_operand_;
  expr_lparen_->setEnabled(room_for_lparen);

  const bool can_rparen =
      (paren_count_ > 0) && mru_operand_ && !mru_operator_;
  expr_rparen_->setEnabled(can_rparen);

  expr_backspace_->setEnabled(cur > 0);
}

void BreakpointsWindow::SelectRegRow(int row) {
  row = std::clamp(row, 0, kMaxRegRows - 1);
  reg_grid_->setCurrentCell(row, 0);
  HideRegEditors();
  reg_edit_row_ = std::min(row + 1, reg_count_);
  UpdateRegButtons();
}

void BreakpointsWindow::OpenRegRow(int row) {
  row = std::clamp(row, 0, kMaxRegRows - 1);
  reg_grid_->setCurrentCell(row, 0);
  reg_edit_row_ = row + 1;
  if (reg_edit_row_ <= reg_count_ + 1) {
    reg_editing_ = true;
    if (reg_edit_row_ <= reg_count_) {
      const sim::BreakCondition& cond = conditions_[reg_edit_row_ - 1];
      reg_select_->setCurrentIndex(static_cast<int>(cond.type_id));
      reg_op_->setCurrentIndex(static_cast<int>(cond.op));
      reg_size_->setCurrentIndex(ComboIndexFromSize(cond.size));
      reg_value_->setText(CellText(reg_grid_, reg_edit_row_ - 1, 2));
    }
    reg_select_->setVisible(true);
    reg_op_->setVisible(true);
    reg_value_->setVisible(true);
    reg_size_->setVisible(true);
    UpdateRegButtons();
  } else {
    HideRegEditors();
    UpdateRegButtons();
    if (notify_) {
      notify_(tr(
          "Wait a second! Go back and enter milestone conditions for all the "
          "rows preceding this one first."));
    }
  }
}

void BreakpointsWindow::SelectAddrRow(int row) {
  row = std::clamp(row, 0, kMaxAddrRows - 1);
  addr_grid_->setCurrentCell(row, 0);
  HideAddrEditors();
  addr_edit_row_ = std::min(row + 1, addr_count_);
  UpdateAddrButtons();
}

void BreakpointsWindow::OpenAddrRow(int row) {
  row = std::clamp(row, 0, kMaxAddrRows - 1);
  addr_grid_->setCurrentCell(row, 0);
  addr_edit_row_ = row + 1;
  if (addr_edit_row_ <= addr_count_ + 1) {
    addr_editing_ = true;
    if (addr_edit_row_ <= addr_count_) {
      // Fix original BREAKPOINTSu.cpp bug where AddrStringGridDblClick indexed
      // breakPoints[reg_edit_row - 1] instead of addr_edit_row - 1 + 50.
      const sim::BreakCondition& cond =
          conditions_[addr_edit_row_ - 1 + kAddrIdOffset];
      addr_select_->setText(CellText(addr_grid_, addr_edit_row_ - 1, 0));
      addr_op_->setCurrentIndex(static_cast<int>(cond.op));
      addr_size_->setCurrentIndex(ComboIndexFromSize(cond.size));
      addr_value_->setText(CellText(addr_grid_, addr_edit_row_ - 1, 2));
      addr_rw_->setCurrentIndex(static_cast<int>(cond.access));
    }
    addr_select_->setVisible(true);
    addr_op_->setVisible(true);
    addr_value_->setVisible(true);
    addr_size_->setVisible(true);
    addr_rw_->setVisible(true);
    UpdateAddrButtons();
  } else {
    HideAddrEditors();
    UpdateAddrButtons();
    if (notify_) {
      notify_(tr(
          "Wait a second! Go back and enter milestone conditions for all the "
          "rows preceding this one first."));
    }
  }
}

void BreakpointsWindow::SelectExprRow(int row) {
  row = std::clamp(row, 0, kMaxExprRows - 1);
  expr_grid_->setCurrentCell(row, 0);
  HideExprEditors();
  expr_edit_row_ = std::min(row + 1, expr_count_);
  UpdateExprButtons();
}

void BreakpointsWindow::OpenExprRow(int row) {
  row = std::clamp(row, 0, kMaxExprRows - 1);
  expr_grid_->setCurrentCell(row, 0);
  expr_edit_row_ = row + 1;
  if (expr_edit_row_ <= expr_count_ + 1) {
    expr_editing_ = true;
    paren_count_ = 0;
    if (expr_edit_row_ <= expr_count_) {
      const StoredExpr& stored = expressions_[expr_edit_row_ - 1];
      infix_ = stored.infix;
      expr_enabled_->setCurrentIndex(stored.expr.enabled ? 1 : 0);
      expr_edit_->setText(CellText(expr_grid_, expr_edit_row_ - 1, 1));
      expr_count_edit_->setText(CellText(expr_grid_, expr_edit_row_ - 1, 2));
      mru_operand_ = !infix_.empty();
      mru_operator_ = false;
    } else {
      infix_.clear();
      expr_enabled_->setCurrentIndex(1);  // ON
      expr_edit_->clear();
      expr_count_edit_->setText(QStringLiteral("01"));
      mru_operand_ = false;
      mru_operator_ = false;
    }
    expr_enabled_->setVisible(true);
    expr_edit_->setVisible(true);
    expr_count_edit_->setVisible(true);
    UpdateExprButtons();
  } else {
    HideExprEditors();
    UpdateExprButtons();
    if (notify_) {
      notify_(tr(
          "Wait a second! Go back and enter breakpoint expressions for all "
          "the rows preceding this one first."));
    }
  }
}

void BreakpointsWindow::OnRegSet() {
  if (!reg_editing_ || reg_edit_row_ < 1 || reg_edit_row_ > kMaxRegRows) {
    return;
  }
  const int row = reg_edit_row_ - 1;
  const QString val_hex =
      FormatHexForSize(reg_value_->text(), reg_size_->currentIndex());
  reg_value_->setText(val_hex);

  reg_grid_->item(row, 0)->setText(reg_select_->currentText());
  reg_grid_->item(row, 1)->setText(reg_op_->currentText());
  reg_grid_->item(row, 2)->setText(val_hex);
  reg_grid_->item(row, 3)->setText(reg_size_->currentText());

  sim::BreakCondition& cond = conditions_[row];
  cond.type = sim::BreakCondition::kPcReg;
  cond.type_id = static_cast<uint32_t>(reg_select_->currentIndex());
  cond.op = reg_op_->currentIndex();
  cond.value = static_cast<int32_t>(val_hex.toUInt(nullptr, 16));
  cond.size = SizeFromComboIndex(reg_size_->currentIndex());
  cond.access = sim::BreakCondition::kAnyAccess;
  cond.enabled = true;

  if (reg_edit_row_ > reg_count_) {
    ++reg_count_;
  }

  HideRegEditors();
  UpdateRegButtons();
  SyncToSimulator();
}

void BreakpointsWindow::OnRegClear() {
  if (reg_count_ <= 0 || reg_edit_row_ < 1 || reg_edit_row_ > reg_count_) {
    return;
  }
  for (int c = reg_edit_row_; c < kMaxRegRows; ++c) {
    conditions_[c - 1] = conditions_[c];
    for (int col = 0; col < 4; ++col) {
      reg_grid_->item(c - 1, col)->setText(CellText(reg_grid_, c, col));
    }
  }
  conditions_[kMaxRegRows - 1] = sim::BreakCondition{};
  for (int col = 0; col < 4; ++col) {
    reg_grid_->item(kMaxRegRows - 1, col)->setText(QString());
  }
  --reg_count_;
  reg_edit_row_ = std::min(reg_edit_row_, reg_count_);
  HideRegEditors();
  UpdateRegButtons();
  SyncToSimulator();
}

void BreakpointsWindow::OnRegClearAll() {
  for (int r = 0; r < kMaxRegRows; ++r) {
    conditions_[r] = sim::BreakCondition{};
    for (int col = 0; col < 4; ++col) {
      reg_grid_->item(r, col)->setText(QString());
    }
  }
  reg_count_ = 0;
  reg_edit_row_ = 0;
  HideRegEditors();
  UpdateRegButtons();
  SyncToSimulator();
}

void BreakpointsWindow::OnAddrSet() {
  if (!addr_editing_ || addr_edit_row_ < 1 || addr_edit_row_ > kMaxAddrRows) {
    return;
  }
  const int row = addr_edit_row_ - 1;
  const uint32_t addr_val = addr_select_->text().toUInt(nullptr, 16) & 0xFFFFFFu;
  const QString addr_hex =
      QString("%1").arg(addr_val, 6, 16, QChar('0')).toUpper();
  addr_select_->setText(addr_hex);

  const QString val_hex =
      FormatHexForSize(addr_value_->text(), addr_size_->currentIndex());
  addr_value_->setText(val_hex);

  addr_grid_->item(row, 0)->setText(addr_hex);
  addr_grid_->item(row, 1)->setText(addr_op_->currentText());
  addr_grid_->item(row, 2)->setText(val_hex);
  addr_grid_->item(row, 3)->setText(addr_size_->currentText());
  addr_grid_->item(row, 4)->setText(addr_rw_->currentText());

  sim::BreakCondition& cond = conditions_[row + kAddrIdOffset];
  cond.type = sim::BreakCondition::kAddress;
  cond.type_id = addr_val;
  cond.op = addr_op_->currentIndex();
  cond.value = static_cast<int32_t>(val_hex.toUInt(nullptr, 16));
  cond.size = SizeFromComboIndex(addr_size_->currentIndex());
  cond.access = addr_rw_->currentIndex();
  cond.enabled = true;

  if (addr_edit_row_ > addr_count_) {
    ++addr_count_;
  }

  HideAddrEditors();
  UpdateAddrButtons();
  SyncToSimulator();
}

void BreakpointsWindow::OnAddrClear() {
  if (addr_count_ <= 0 || addr_edit_row_ < 1 || addr_edit_row_ > addr_count_) {
    return;
  }
  for (int c = addr_edit_row_; c < kMaxAddrRows; ++c) {
    conditions_[c - 1 + kAddrIdOffset] = conditions_[c + kAddrIdOffset];
    for (int col = 0; col < 5; ++col) {
      addr_grid_->item(c - 1, col)->setText(CellText(addr_grid_, c, col));
    }
  }
  conditions_[kMaxAddrRows - 1 + kAddrIdOffset] = sim::BreakCondition{};
  for (int col = 0; col < 5; ++col) {
    addr_grid_->item(kMaxAddrRows - 1, col)->setText(QString());
  }
  --addr_count_;
  addr_edit_row_ = std::min(addr_edit_row_, addr_count_);
  HideAddrEditors();
  UpdateAddrButtons();
  SyncToSimulator();
}

void BreakpointsWindow::OnAddrClearAll() {
  for (int r = 0; r < kMaxAddrRows; ++r) {
    conditions_[r + kAddrIdOffset] = sim::BreakCondition{};
    for (int col = 0; col < 5; ++col) {
      addr_grid_->item(r, col)->setText(QString());
    }
  }
  addr_count_ = 0;
  addr_edit_row_ = 0;
  HideAddrEditors();
  UpdateAddrButtons();
  SyncToSimulator();
}

void BreakpointsWindow::OnExprSet() {
  if (!expr_editing_ || expr_edit_row_ < 1 || expr_edit_row_ > kMaxExprRows) {
    return;
  }
  const int row = expr_edit_row_ - 1;
  // Parse Count as decimal (00..99), fixing original BREAKPOINTSu.cpp bug where
  // "0x" + ExprCountEdit->Text parsed a decimal mask field as hex.
  const int count =
      std::clamp(expr_count_edit_->text().toInt(nullptr, 10), 1, 99);
  const QString count_str = QString("%1").arg(count, 2, 10, QChar('0'));
  expr_count_edit_->setText(count_str);

  expr_grid_->item(row, 0)->setText(expr_enabled_->currentText());
  expr_grid_->item(row, 1)->setText(expr_edit_->text());
  expr_grid_->item(row, 2)->setText(count_str);

  StoredExpr& stored = expressions_[row];
  stored.infix = infix_;
  stored.expr.enabled = (expr_enabled_->currentIndex() == 1);
  stored.expr.count = count;
  stored.expr.text = expr_edit_->text().toStdString();
  stored.expr.postfix = InfixToPostfix(infix_);

  if (expr_edit_row_ > expr_count_) {
    ++expr_count_;
  }
  HideExprEditors();
  UpdateExprButtons();
  SyncToSimulator();
}

void BreakpointsWindow::OnExprClear() {
  if (expr_count_ <= 0 || expr_edit_row_ < 1 || expr_edit_row_ > expr_count_) {
    return;
  }
  for (int c = expr_edit_row_; c < kMaxExprRows; ++c) {
    expressions_[c - 1] = expressions_[c];
    for (int col = 0; col < 3; ++col) {
      expr_grid_->item(c - 1, col)->setText(CellText(expr_grid_, c, col));
    }
  }
  expressions_[kMaxExprRows - 1] = StoredExpr{};
  for (int col = 0; col < 3; ++col) {
    expr_grid_->item(kMaxExprRows - 1, col)->setText(QString());
  }
  --expr_count_;
  expr_edit_row_ = std::min(expr_edit_row_, expr_count_);
  HideExprEditors();
  UpdateExprButtons();
  SyncToSimulator();
}

void BreakpointsWindow::OnExprClearAll() {
  for (int r = 0; r < kMaxExprRows; ++r) {
    expressions_[r] = StoredExpr{};
    for (int col = 0; col < 3; ++col) {
      expr_grid_->item(r, col)->setText(QString());
    }
  }
  expr_count_ = 0;
  expr_edit_row_ = 0;
  HideExprEditors();
  UpdateExprButtons();
  SyncToSimulator();
}

void BreakpointsWindow::OnExprRegAppend() {
  const int reg_num = expr_reg_edit_->text().toInt(nullptr, 10);
  if (reg_num >= 1 && reg_num <= reg_count_) {
    infix_.push_back(reg_num - 1);
    const QString tok = (reg_num < 10) ? QString(" R%1  ").arg(reg_num)
                                       : QString(" R%1 ").arg(reg_num);
    expr_edit_->setText(expr_edit_->text() + tok);
    mru_operand_ = true;
    mru_operator_ = false;
  } else if (notify_) {
    notify_(tr(
        "Invalid Input! Pick a valid value in the range of existing Register "
        "conditions."));
  }
  UpdateExprButtons();
}

void BreakpointsWindow::OnExprAddrAppend() {
  const int addr_num = expr_addr_edit_->text().toInt(nullptr, 10);
  if (addr_num >= 1 && addr_num <= addr_count_) {
    infix_.push_back(addr_num - 1 + kAddrIdOffset);
    // Pad single-digit A1..A9 to 5 chars (" A1  ") to match R1..R9 so Backspace
    // (which removes 5 chars) does not corrupt the preceding token.
    const QString tok = (addr_num < 10) ? QString(" A%1  ").arg(addr_num)
                                        : QString(" A%1 ").arg(addr_num);
    expr_edit_->setText(expr_edit_->text() + tok);
    mru_operand_ = true;
    mru_operator_ = false;
  } else if (notify_) {
    notify_(tr(
        "Invalid Input! Pick a valid value in the range of existing Address "
        "conditions."));
  }
  UpdateExprButtons();
}

void BreakpointsWindow::OnExprAndAppend() {
  infix_.push_back(kTokenAnd);
  expr_edit_->setText(expr_edit_->text() + QStringLiteral(" AND "));
  mru_operator_ = true;
  mru_operand_ = false;
  UpdateExprButtons();
}

void BreakpointsWindow::OnExprOrAppend() {
  infix_.push_back(kTokenOr);
  expr_edit_->setText(expr_edit_->text() + QStringLiteral(" OR  "));
  mru_operator_ = true;
  mru_operand_ = false;
  UpdateExprButtons();
}

void BreakpointsWindow::OnExprLParenAppend() {
  infix_.push_back(kTokenLParen);
  ++paren_count_;
  expr_edit_->setText(expr_edit_->text() + QStringLiteral("("));
  mru_operand_ = false;
  mru_operator_ = false;
  UpdateExprButtons();
}

void BreakpointsWindow::OnExprRParenAppend() {
  infix_.push_back(kTokenRParen);
  --paren_count_;
  expr_edit_->setText(expr_edit_->text() + QStringLiteral(")"));
  mru_operand_ = true;
  mru_operator_ = false;
  UpdateExprButtons();
}

void BreakpointsWindow::OnExprBackspace() {
  if (infix_.empty()) {
    return;
  }
  const int last = infix_.back();
  infix_.pop_back();
  QString text = expr_edit_->text();
  if (last == kTokenLParen) {
    --paren_count_;
    text.chop(1);
  } else if (last == kTokenRParen) {
    ++paren_count_;
    text.chop(1);
  } else {
    text.chop(5);
  }
  expr_edit_->setText(text);

  if (infix_.empty()) {
    mru_operand_ = false;
    mru_operator_ = false;
  } else {
    const int prev = infix_.back();
    if (prev == kTokenAnd || prev == kTokenOr) {
      mru_operator_ = true;
      mru_operand_ = false;
    } else if (prev == kTokenLParen) {
      mru_operator_ = false;
      mru_operand_ = false;
    } else {
      mru_operator_ = false;
      mru_operand_ = true;
    }
  }
  UpdateExprButtons();
}

std::vector<int> BreakpointsWindow::InfixToPostfix(
    const std::vector<int>& infix) {
  std::vector<int> postfix;
  std::vector<int> op_stack;
  for (int tok : infix) {
    if (tok >= 0 && tok < sim::kMaxBreakConditions) {
      postfix.push_back(tok);
    } else if (tok == kTokenLParen) {
      op_stack.push_back(tok);
    } else if (tok == kTokenRParen) {
      while (!op_stack.empty()) {
        const int top = op_stack.back();
        op_stack.pop_back();
        if (top == kTokenLParen) {
          break;
        }
        postfix.push_back(top);
      }
    } else if (tok == kTokenAnd || tok == kTokenOr) {
      // kTokenAnd (100) has higher precedence than kTokenOr (101).
      while (!op_stack.empty() && op_stack.back() != kTokenLParen &&
             op_stack.back() <= tok) {
        postfix.push_back(op_stack.back());
        op_stack.pop_back();
      }
      op_stack.push_back(tok);
    }
  }
  while (!op_stack.empty()) {
    const int top = op_stack.back();
    op_stack.pop_back();
    if (top != kTokenLParen) {
      postfix.push_back(top);
    }
  }
  return postfix;
}

int BreakpointsWindow::SizeFromComboIndex(int index) {
  switch (index) {
    case 0:
      return sim::BreakCondition::kByte;
    case 1:
      return sim::BreakCondition::kWord;
    default:
      return sim::BreakCondition::kLong;
  }
}

int BreakpointsWindow::ComboIndexFromSize(int size) {
  switch (size) {
    case sim::BreakCondition::kByte:
      return 0;
    case sim::BreakCondition::kWord:
      return 1;
    default:
      return 2;
  }
}

void BreakpointsWindow::SyncToSimulator() {
  if (!session_) {
    return;
  }
  const auto conds = conditions_;
  std::vector<sim::BreakExpression> exprs;
  exprs.reserve(expr_count_);
  for (int i = 0; i < expr_count_; ++i) {
    exprs.push_back(expressions_[i].expr);
  }
  session_->RunOnSimulator([conds, exprs = std::move(exprs)](
                               sim::M68kSimulator& sim) {
    for (int i = 0; i < sim::kMaxBreakConditions; ++i) {
      sim.SetBreakCondition(i, conds[i]);
    }
    sim.SetBreakExpressions(exprs);
  });
}

void BreakpointsWindow::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Tab &&
      (event->modifiers() & Qt::ControlModifier)) {
    emit NextWindowRequested();
    event->accept();
    return;
  }
  QWidget::keyPressEvent(event);
}

}  // namespace easym68k::gui
