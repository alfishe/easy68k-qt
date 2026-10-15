// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The Sim68K I/O window ("Sim68K I/O", simIOu.dfm): shows the SimIoScreen
// canvas, flashes the input prompt, and feeds the keyboard and mouse to the
// screen and the peripheral back end.

#ifndef EASY68K_GUI_SIMULATOR_IO_WINDOW_H_
#define EASY68K_GUI_SIMULATOR_IO_WINDOW_H_

#include <QTimer>
#include <QWidget>

namespace easym68k::gui {

class SimIoScreen;
class QtPeripheralIO;

class IoWindow : public QWidget {
  Q_OBJECT

 public:
  IoWindow(SimIoScreen* screen, QtPeripheralIO* peripheral, QWidget* parent = nullptr);

 protected:
  void paintEvent(QPaintEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;
  void changeEvent(QEvent* event) override;
  bool focusNextPrevChild(bool next) override;  // Tab goes to the program

 private:
  void Resize(int width, int height);

  SimIoScreen* screen_;
  QtPeripheralIO* peripheral_;
  QTimer prompt_timer_;  // simIOu.dfm prompt: 500 ms
  bool prompt_visible_ = false;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_IO_WINDOW_H_
