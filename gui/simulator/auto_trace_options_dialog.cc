// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "auto_trace_options_dialog.h"

#include <algorithm>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace easym68k::gui {

AutoTraceOptionsDialog::AutoTraceOptionsDialog(int interval_ms,
                                               bool disable_display,
                                               QWidget* parent)
    : QDialog(parent) {
  setWindowTitle(tr("Auto Trace Options"));
  resize(247, 119);

  auto* root = new QVBoxLayout(this);

  auto* interval_row = new QHBoxLayout();
  auto* label = new QLabel(tr("Auto Trace Interval"), this);
  interval_spin_ = new QSpinBox(this);
  interval_spin_->setRange(kMinIntervalMs, kMaxIntervalMs);
  interval_spin_->setSingleStep(kStepIntervalMs);
  interval_spin_->setValue(
      std::clamp(interval_ms, kMinIntervalMs, kMaxIntervalMs));
  auto* unit = new QLabel(tr("mSec"), this);
  interval_row->addWidget(label);
  interval_row->addWidget(interval_spin_);
  interval_row->addWidget(unit);
  root->addLayout(interval_row);

  disable_display_check_ = new QCheckBox(tr("Disable display update"), this);
  disable_display_check_->setChecked(disable_display);
  root->addWidget(disable_display_check_);

  auto* btn_row = new QHBoxLayout();
  ok_btn_ = new QPushButton(tr("&OK"), this);
  ok_btn_->setDefault(true);
  cancel_btn_ = new QPushButton(tr("&Cancel"), this);
  btn_row->addWidget(ok_btn_);
  btn_row->addWidget(cancel_btn_);
  root->addLayout(btn_row);

  connect(ok_btn_, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancel_btn_, &QPushButton::clicked, this, &QDialog::reject);
}

}  // namespace easym68k::gui
