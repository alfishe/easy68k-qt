// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// IGraphicsIO on the Sim68K I/O window (Trap #15 tasks 80-96): a thin
// adapter over SimIoScreen, callable from the simulator thread.

#ifndef EASY68K_GUI_SIMULATOR_QT_GRAPHICS_IO_H_
#define EASY68K_GUI_SIMULATOR_QT_GRAPHICS_IO_H_

#include "easym68k/sim/igraphics_io.h"

namespace easym68k::gui {

class SimIoScreen;

class QtGraphicsIO : public sim::IGraphicsIO {
 public:
  explicit QtGraphicsIO(SimIoScreen* screen) : screen_(screen) {}

  void SetPenColor(uint32_t color) override;
  void SetFillColor(uint32_t color) override;
  void DrawPixel(int x, int y) override;
  int GetPixel(int x, int y) override;
  void DrawLine(int x1, int y1, int x2, int y2) override;
  void LineTo(int x, int y) override;
  void MoveTo(int x, int y) override;
  void GetXY(int* x, int* y) override;
  void DrawRectangle(int x1, int y1, int x2, int y2) override;
  void DrawEllipse(int x1, int y1, int x2, int y2) override;
  void FloodFill(int x, int y) override;
  void DrawUnfilledRectangle(int x1, int y1, int x2, int y2) override;
  void DrawUnfilledEllipse(int x1, int y1, int x2, int y2) override;
  void SetDrawingMode(int mode) override;
  void SetPenWidth(int width) override;
  void Repaint() override;
  void DrawText(const char* str, int x, int y) override;

 private:
  SimIoScreen* screen_;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_QT_GRAPHICS_IO_H_
