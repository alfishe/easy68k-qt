// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The Sim68K I/O window ("Sim68K I/O", simIOu.dfm): shows the SimIoScreen
// canvas, flashes the input prompt, and feeds the keyboard and mouse to the
// screen and the peripheral back end. Alt+Enter toggles full screen (the
// canvas scaled to the chosen screen, as the original's display mode change
// to the canvas size), Shift+Insert types the clipboard, Ctrl+Tab goes on to
// the next window; Trap #15 task 24 disables these keys.

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

  // simIOu.cpp fullScreen / setupWindow(): applied when the window shows.
  void SetFullScreen(bool full);
  bool FullScreen() const { return full_screen_; }
  // FullScreenMonitor: 0 = primary screen; an index past the screens uses
  // the primary one.
  void SetFullScreenMonitor(int index) { monitor_ = index; }
  int FullScreenMonitor() const { return monitor_; }
  // Window position of a canvas point and back (full screen scaling).
  QPoint CanvasToWindow(QPoint p) const;
  QPoint WindowToCanvas(QPoint p) const;

 signals:
  void NextWindowRequested();  // Ctrl+Tab (memory window)

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
  void ApplyWindowMode();
  QRect CanvasRect() const;  // where the canvas is drawn

  SimIoScreen* screen_;
  QtPeripheralIO* peripheral_;
  QTimer prompt_timer_;  // simIOu.dfm prompt: 500 ms
  bool prompt_visible_ = false;
  int canvas_width_ = 0;
  int canvas_height_ = 0;
  bool full_screen_ = false;
  bool shown_full_ = false;  // the window is in full screen mode
  int monitor_ = 0;
  QPoint saved_pos_;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_IO_WINDOW_H_
