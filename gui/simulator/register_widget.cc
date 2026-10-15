// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "register_widget.h"

#include <QEvent>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QGridLayout>
#include <QRegularExpressionValidator>

namespace easym68k::gui {

namespace {

QString Hex(uint32_t v) {
  return QStringLiteral("%1").arg(v, 8, 16, QLatin1Char('0')).toUpper();
}

}  // namespace

RegisterWidget::RegisterWidget(QWidget* parent) : QGroupBox(QStringLiteral("Registers"), parent) {
  auto* grid = new QGridLayout(this);
  grid->setHorizontalSpacing(8);
  grid->setVerticalSpacing(2);
  for (int i = 0; i < 8; i++) {
    d_[i] = HexField(i, 0, QStringLiteral("D%1=").arg(i));
    a_[i] = HexField(i, 2, QStringLiteral("A%1=").arg(i));
  }
  pc_ = HexField(0, 4, QStringLiteral("PC="));
  ss_ = HexField(1, 4, QStringLiteral("SS="));
  us_ = HexField(2, 4, QStringLiteral("US="));

  const QFont fixed = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  auto* flags = new QLabel(QStringLiteral("T S  INT   XNZVC"), this);
  flags->setFont(fixed);
  grid->addWidget(flags, 3, 5);
  grid->addWidget(new QLabel(QStringLiteral("SR="), this), 4, 4, Qt::AlignRight);
  sr_ = new QLineEdit(this);
  sr_->setFont(fixed);
  sr_->setInputMask(QStringLiteral("BBBBBBBBBBBBBBBB"));  // 0/1 only, overwrite
  sr_->setMinimumWidth(QFontMetrics(fixed).horizontalAdvance(QString(18, QLatin1Char('0'))));
  connect(sr_, &QLineEdit::textEdited, this, [this] { OnEdited(sr_); });
  grid->addWidget(sr_, 4, 5);

  grid->addWidget(new QLabel(QStringLiteral("Cycles"), this), 6, 4, Qt::AlignRight);
  cycles_ = new QLabel(QStringLiteral("0"), this);
  cycles_->setFont(fixed);
  cycles_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
  grid->addWidget(cycles_, 6, 5);
  clear_cycles_ = new QPushButton(QStringLiteral("Clear Cycles"), this);
  connect(clear_cycles_, &QPushButton::clicked, this, &RegisterWidget::ClearCycles);
  grid->addWidget(clear_cycles_, 7, 5, Qt::AlignLeft);
  grid->setColumnStretch(6, 1);
}

// A hex register field: 8 digits, upper case (regKeyPress()).
QLineEdit* RegisterWidget::HexField(int row, int column, const QString& name) {
  auto* grid = static_cast<QGridLayout*>(layout());
  grid->addWidget(new QLabel(name, this), row, column, Qt::AlignRight);
  auto* field = new QLineEdit(this);
  const QFont fixed = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  field->setFont(fixed);
  field->setMaxLength(8);
  field->setValidator(new QRegularExpressionValidator(
      QRegularExpression(QStringLiteral("[0-9A-Fa-f]{0,8}")), field));
  field->setFixedWidth(QFontMetrics(fixed).horizontalAdvance(QString(10, QLatin1Char('0'))));
  connect(field, &QLineEdit::textEdited, this, [this, field](const QString& text) {
    if (text != text.toUpper()) {
      const int cursor = field->cursorPosition();
      field->setText(text.toUpper());
      field->setCursorPosition(cursor);
    }
    OnEdited(field);
  });
  field->installEventFilter(this);
  grid->addWidget(field, row, column + 1);
  return field;
}

uint32_t RegisterWidget::Value(const QLineEdit* field) {
  return field->text().toUInt(nullptr, 16);  // empty = 0
}

void RegisterWidget::Show(const sim::CpuState& s) {
  state_ = s;
  showing_ = true;
  for (int i = 0; i < 8; i++)
    d_[i]->setText(Hex(s.d[i]));
  for (int i = 0; i < 7; i++)
    a_[i]->setText(Hex(s.a[i]));
  a_[7]->setText(Hex(s.GetSP()));
  us_->setText(Hex(s.a[7]));
  ss_->setText(Hex(s.ssp));
  pc_->setText(Hex(s.pc));
  sr_->setText(QStringLiteral("%1").arg(s.sr, 16, 2, QLatin1Char('0')));
  cycles_->setText(s.cycles > 0xFFFFFFFFFFFFull ? QStringLiteral("Overflow")
                                                : QString::number(s.cycles));
  showing_ = false;
}

void RegisterWidget::SetEditable(bool editable) {
  for (QLineEdit* f : d_)
    f->setReadOnly(!editable);
  for (QLineEdit* f : a_)
    f->setReadOnly(!editable);
  for (QLineEdit* f : {pc_, sr_, us_, ss_})
    f->setReadOnly(!editable);
  clear_cycles_->setEnabled(editable);
}

// SIM68Ku.cpp loadRegs() and the A7/US/SS key handlers: A7 is the active
// stack pointer, so it and US or SS show the same value; a change of the S
// bit shows the other one in A7 (regSRChange()).
void RegisterWidget::OnEdited(QLineEdit* field) {
  if (showing_)
    return;
  sim::CpuState& s = state_;
  for (int i = 0; i < 8; i++)
    if (field == d_[i])
      s.d[i] = Value(field);
  for (int i = 0; i < 7; i++)
    if (field == a_[i])
      s.a[i] = Value(field);
  showing_ = true;
  if (field == a_[7]) {
    s.SetSP(Value(field));
    (s.IsSupervisor() ? ss_ : us_)->setText(field->text());
  } else if (field == us_) {
    s.a[7] = Value(field);
    if (!s.IsSupervisor())
      a_[7]->setText(field->text());
  } else if (field == ss_) {
    s.ssp = Value(field);
    if (s.IsSupervisor())
      a_[7]->setText(field->text());
  } else if (field == pc_) {
    s.pc = Value(field);
  } else if (field == sr_) {
    uint16_t sr = 0;
    const QString bits = sr_->text();
    for (int i = 0; i < bits.size() && i < 16; i++)
      if (bits.at(i) == QLatin1Char('1'))
        sr |= static_cast<uint16_t>(1u << (15 - i));
    s.sr = sr;
    a_[7]->setText(Hex(s.GetSP()));
  }
  showing_ = false;
  emit Edited(s);
}

bool RegisterWidget::eventFilter(QObject* watched, QEvent* event) {
  if (event->type() == QEvent::MouseButtonDblClick) {
    if (watched == pc_) {
      emit PcDoubleClicked();
      return false;
    }
    for (QLineEdit* f : {a_[0], a_[1], a_[2], a_[3], a_[4], a_[5], a_[6], a_[7], us_, ss_}) {
      if (watched == f) {
        emit AddressDoubleClicked(Value(f));
        return false;
      }
    }
  }
  return QGroupBox::eventFilter(watched, event);
}

}  // namespace easym68k::gui
