// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The stack window ("68000 Stack", Stack1.dfm / Stack1.cpp): rows of four
// bytes around the selected stack register (A0-A6, US, SS; SS by default),
// that register's byte on aqua, the active stack pointer's byte on yellow,
// bytes in the Invalid memory map range in red; scrolled by a row with the
// spin buttons or the mouse wheel.
//
// Memory is read only while the program is stopped; while it runs the last
// image stays on screen (the original refreshed only after a stop).

#ifndef EASY68K_GUI_SIMULATOR_STACK_WINDOW_H_
#define EASY68K_GUI_SIMULATOR_STACK_WINDOW_H_

#include <functional>
#include <QColor>
#include <QComboBox>
#include <QPixmap>
#include <QWidget>

#include "easym68k/sim/cpu_state.h"
#include "easym68k/sim/memory.h"

namespace easym68k::gui {

class StackWindow : public QWidget {
  Q_OBJECT

 public:
  // `readable`: true while memory may be read (program not running).
  StackWindow(const sim::Memory* memory, std::function<bool()> readable, QWidget* parent = nullptr);

  // Stack1.cpp updateDisplay(): centres on the selected register.
  void UpdateState(const sim::CpuState& state);

  // Model of the display, for tests.
  int Rows() const;
  QString RowText(int row) const;  // "00FFFFF8: 00 00 10 04 "
  QColor Background(int row, int byte) const;
  QColor Foreground(int row, int byte) const;
  QComboBox* Selector() const { return which_; }
  void ScrollRows(int rows);  // + = higher addresses

 signals:
  void NextWindowRequested();  // Ctrl+Tab (Hardware window)

 protected:
  void paintEvent(QPaintEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

 private:
  uint32_t TopAddress() const;
  uint32_t SelectedRegister() const;

  const sim::Memory* memory_;
  std::function<bool()> readable_;
  QComboBox* which_ = nullptr;
  QWidget* panel_ = nullptr;
  sim::CpuState state_{};
  uint32_t mid_addr_ = 0;   // midAddr
  uint32_t areg_addr_ = 0;  // AregAddr
  QPixmap last_image_;      // shown while memory may not be read
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_STACK_WINDOW_H_
