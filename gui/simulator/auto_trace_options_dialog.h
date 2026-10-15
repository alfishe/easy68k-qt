// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The Auto Trace Options dialog (Optionsu.dfm / Optionsu.cpp):
//  - Auto Trace Interval (10..1000 mSec, step 10, default 200);
//  - Disable display update checkbox;
//  - OK and Cancel buttons.

#ifndef EASY68K_GUI_SIMULATOR_AUTO_TRACE_OPTIONS_DIALOG_H_
#define EASY68K_GUI_SIMULATOR_AUTO_TRACE_OPTIONS_DIALOG_H_

#include <QCheckBox>
#include <QDialog>
#include <QPushButton>
#include <QSpinBox>

namespace easym68k::gui {

class AutoTraceOptionsDialog : public QDialog {
  Q_OBJECT

 public:
  static constexpr int kMinIntervalMs = 10;
  static constexpr int kMaxIntervalMs = 1000;
  static constexpr int kStepIntervalMs = 10;
  static constexpr int kDefaultIntervalMs = 200;

  AutoTraceOptionsDialog(int interval_ms, bool disable_display,
                         QWidget* parent = nullptr);
  ~AutoTraceOptionsDialog() override = default;

  int Interval() const { return interval_spin_->value(); }
  bool DisableDisplay() const { return disable_display_check_->isChecked(); }

  QSpinBox* IntervalSpinBox() const { return interval_spin_; }
  QCheckBox* DisableDisplayCheckBox() const { return disable_display_check_; }
  QPushButton* OkButton() const { return ok_btn_; }
  QPushButton* CancelButton() const { return cancel_btn_; }

 private:
  QSpinBox* interval_spin_ = nullptr;
  QCheckBox* disable_display_check_ = nullptr;
  QPushButton* ok_btn_ = nullptr;
  QPushButton* cancel_btn_ = nullptr;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_AUTO_TRACE_OPTIONS_DIALOG_H_
