// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The register panel of the Sim68K main window (SIM68Ku.dfm GroupBox1
// "Registers": D0-D7, A0-A7, PC, SR with "T S  INT   XNZVC", US, SS,
// Cycles). Task 13.2 shows the values; editing and change highlighting are
// Task 13.3.

#ifndef EASY68K_GUI_SIMULATOR_REGISTER_WIDGET_H_
#define EASY68K_GUI_SIMULATOR_REGISTER_WIDGET_H_

#include <array>
#include <QGroupBox>
#include <QLabel>

#include "easym68k/sim/cpu_state.h"

namespace easym68k::gui {

class RegisterWidget : public QGroupBox {
  Q_OBJECT

 public:
  explicit RegisterWidget(QWidget* parent = nullptr);

  void Show(const sim::CpuState& state);  // SIM68Ku.cpp scrshow()

 private:
  QLabel* Value(int row, int column, const QString& name);

  std::array<QLabel*, 8> d_{};
  std::array<QLabel*, 8> a_{};  // A0-A6, A7 = current stack pointer
  QLabel* pc_ = nullptr;
  QLabel* sr_ = nullptr;
  QLabel* us_ = nullptr;
  QLabel* ss_ = nullptr;
  QLabel* cycles_ = nullptr;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_REGISTER_WIDGET_H_
