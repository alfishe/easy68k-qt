// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "register_widget.h"

#include <QFontDatabase>
#include <QGridLayout>

namespace easym68k::gui {

RegisterWidget::RegisterWidget(QWidget* parent) : QGroupBox(QStringLiteral("Registers"), parent) {
  auto* grid = new QGridLayout(this);
  grid->setHorizontalSpacing(12);
  for (int i = 0; i < 8; i++) {
    d_[i] = Value(i, 0, QStringLiteral("D%1=").arg(i));
    a_[i] = Value(i, 2, QStringLiteral("A%1=").arg(i));
  }
  pc_ = Value(0, 4, QStringLiteral("PC="));
  ss_ = Value(1, 4, QStringLiteral("SS="));
  us_ = Value(2, 4, QStringLiteral("US="));
  auto* flags = new QLabel(QStringLiteral("T S  INT   XNZVC"), this);
  flags->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  grid->addWidget(flags, 3, 5);
  sr_ = Value(4, 4, QStringLiteral("SR="));
  cycles_ = Value(6, 4, QStringLiteral("Cycles"));
  grid->setColumnStretch(6, 1);
}

QLabel* RegisterWidget::Value(int row, int column, const QString& name) {
  auto* grid = static_cast<QGridLayout*>(layout());
  grid->addWidget(new QLabel(name, this), row, column, Qt::AlignRight);
  auto* value = new QLabel(this);
  value->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  value->setTextInteractionFlags(Qt::TextSelectableByMouse);
  grid->addWidget(value, row, column + 1);
  return value;
}

void RegisterWidget::Show(const sim::CpuState& s) {
  auto hex = [](uint32_t v) {
    return QStringLiteral("%1").arg(v, 8, 16, QLatin1Char('0')).toUpper();
  };
  for (int i = 0; i < 8; i++)
    d_[i]->setText(hex(s.d[i]));
  for (int i = 0; i < 7; i++)
    a_[i]->setText(hex(s.a[i]));
  a_[7]->setText(hex(s.GetSP()));
  pc_->setText(hex(s.pc));
  ss_->setText(hex(s.ssp));
  us_->setText(hex(s.a[7]));
  sr_->setText(QStringLiteral("%1").arg(s.sr, 16, 2, QLatin1Char('0')));
  cycles_->setText(QString::number(s.cycles));
}

}  // namespace easym68k::gui
