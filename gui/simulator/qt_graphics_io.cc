// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "qt_graphics_io.h"

#include "sim_io_screen.h"

namespace easym68k::gui {

void QtGraphicsIO::SetPenColor(uint32_t color) {
  screen_->SetPenColor(color);
}

void QtGraphicsIO::SetFillColor(uint32_t color) {
  screen_->SetFillColor(color);
}

void QtGraphicsIO::DrawPixel(int x, int y) {
  screen_->DrawPixel(x, y);
}

int QtGraphicsIO::GetPixel(int x, int y) {
  return static_cast<int>(screen_->GetPixel(x, y));
}

void QtGraphicsIO::DrawLine(int x1, int y1, int x2, int y2) {
  screen_->Line(x1, y1, x2, y2);
}

void QtGraphicsIO::LineTo(int x, int y) {
  screen_->LineTo(x, y);
}

void QtGraphicsIO::MoveTo(int x, int y) {
  screen_->MoveTo(x, y);
}

void QtGraphicsIO::GetXY(int* x, int* y) {
  screen_->GetXY(x, y);
}

void QtGraphicsIO::DrawRectangle(int x1, int y1, int x2, int y2) {
  screen_->Rectangle(x1, y1, x2, y2);
}

void QtGraphicsIO::DrawEllipse(int x1, int y1, int x2, int y2) {
  screen_->Ellipse(x1, y1, x2, y2);
}

void QtGraphicsIO::FloodFill(int x, int y) {
  screen_->FloodFill(x, y);
}

void QtGraphicsIO::DrawUnfilledRectangle(int x1, int y1, int x2, int y2) {
  screen_->UnfilledRectangle(x1, y1, x2, y2);
}

void QtGraphicsIO::DrawUnfilledEllipse(int x1, int y1, int x2, int y2) {
  screen_->UnfilledEllipse(x1, y1, x2, y2);
}

void QtGraphicsIO::SetDrawingMode(int mode) {
  screen_->SetDrawingMode(mode);
}

void QtGraphicsIO::SetPenWidth(int width) {
  screen_->SetPenWidth(width);
}

void QtGraphicsIO::Repaint() {
  screen_->Repaint();
}

void QtGraphicsIO::DrawText(const char* str, int x, int y) {
  screen_->DrawText(str, x, y);
}

}  // namespace easym68k::gui
