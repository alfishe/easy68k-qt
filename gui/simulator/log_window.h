// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Options > Log Output (logU.dfm, "Log Output") and the dialog asking about
// an existing log file (LogfileDialogu.dfm, "Log File Options").

#ifndef EASY68K_GUI_SIMULATOR_LOG_WINDOW_H_
#define EASY68K_GUI_SIMULATOR_LOG_WINDOW_H_

#include <array>
#include <functional>
#include <QDialog>
#include <QGroupBox>
#include <QLineEdit>
#include <QRadioButton>

#include "sim_log.h"

namespace easym68k::gui {

class LogOutputDialog : public QDialog {
  Q_OBJECT

 public:
  // Edits a copy of `settings`; Settings() after OK (Cancel keeps nothing,
  // CancelBtnClick()).
  explicit LogOutputDialog(const SimLog::Settings& settings, QWidget* parent = nullptr);

  SimLog::Settings Settings() const;
  // Message for an invalid From address (MemFromExit()); replaceable for
  // tests.
  void SetInform(std::function<void(const QString&)> inform) { inform_ = std::move(inform); }

  QLineEdit* ExecFileName() const { return exec_name_; }
  QLineEdit* OutputFileName() const { return output_name_; }
  QRadioButton* ExecType(int type) const { return exec_type_[type]; }
  QRadioButton* OutputType(int type) const { return output_type_[type]; }
  QGroupBox* MemoryRange() const { return mem_range_; }
  QLineEdit* MemFrom() const { return mem_from_; }
  QLineEdit* MemBytes() const { return mem_bytes_; }

 private:
  QLineEdit* exec_name_ = nullptr;
  QLineEdit* output_name_ = nullptr;
  std::array<QRadioButton*, 4> exec_type_{};
  std::array<QRadioButton*, 2> output_type_{};
  QGroupBox* mem_range_ = nullptr;
  QLineEdit* mem_from_ = nullptr;
  QLineEdit* mem_bytes_ = nullptr;
  std::function<void(const QString&)> inform_;
};

// Replace / Append / Cancel for an existing log file.
class LogFileExistsDialog : public QDialog {
  Q_OBJECT

 public:
  explicit LogFileExistsDialog(const QString& message, QWidget* parent = nullptr);
  SimLog::Exists Choice() const { return choice_; }

 private:
  SimLog::Exists choice_ = SimLog::Exists::kCancel;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_LOG_WINDOW_H_
