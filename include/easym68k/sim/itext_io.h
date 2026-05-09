// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// ITextIO — text/console I/O interface (Trap #15 tasks 0-2, 5-7, 11-14,
// 16-19, 22, 24-25). Replaces VCL TMemo and console I/O in original CODE9.CPP.

#pragma once

#include <cstdint>
#include <string_view>

namespace easym68k::sim {

class ITextIO {
 public:
  virtual ~ITextIO() = default;
  virtual void TextOut(std::string_view text) = 0;
  virtual void TextOutCRLF(std::string_view text) = 0;
  virtual void TextIn(char* buf, long* size, long* result) = 0;
  virtual void CharIn(char* ch) = 0;
  virtual void CharOut(char ch) = 0;
  virtual void SetCursorPosition(int col, int row) = 0;
  virtual void GetCursorPosition(int* col, int* row) = 0;
  virtual void ClearScreen() = 0;
  virtual void SetKeyboardEcho(bool enable) = 0;
  virtual void SetInputPrompt(bool enable) = 0;
  virtual void SetInputLFDisplay(bool enable) = 0;
  virtual void GetCharAt(int row, int col, char* ch) = 0;
  virtual void ScrollRect(int row, int col, int height, int width, int direction) = 0;
  virtual void SetFontProperties(uint32_t color, uint32_t style) = 0;
  virtual bool IsKeyPending() = 0;
  virtual void GetKeyState(long* state) = 0;
  virtual void SetKeyCommandsEnabled(bool enabled) = 0;
};

}  // namespace easym68k::sim
