// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// IGraphicsIO — 2D graphics drawing interface (Trap #15 tasks 80-96).
// Replaces VCL TCanvas/TBitmap drawing calls in original CODE9.CPP.

#pragma once

#include <cstdint>

namespace easym68k::sim {

class IGraphicsIO {
 public:
  virtual ~IGraphicsIO() = default;
  virtual void SetPenColor(uint32_t color) = 0;
  virtual void SetFillColor(uint32_t color) = 0;
  virtual void DrawPixel(int x, int y) = 0;
  virtual int GetPixel(int x, int y) = 0;
  virtual void DrawLine(int x1, int y1, int x2, int y2) = 0;
  virtual void LineTo(int x, int y) = 0;
  virtual void MoveTo(int x, int y) = 0;
  virtual void GetXY(int* x, int* y) = 0;
  virtual void DrawRectangle(int x1, int y1, int x2, int y2) = 0;
  virtual void DrawEllipse(int x1, int y1, int x2, int y2) = 0;
  virtual void FloodFill(int x, int y) = 0;
  virtual void DrawUnfilledRectangle(int x1, int y1, int x2, int y2) = 0;
  virtual void DrawUnfilledEllipse(int x1, int y1, int x2, int y2) = 0;
  virtual void SetDrawingMode(int mode) = 0;
  virtual void SetPenWidth(int width) = 0;
  virtual void Repaint() = 0;
  virtual void DrawText(const char* str, int x, int y) = 0;
};

}  // namespace easym68k::sim
