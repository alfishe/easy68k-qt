// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// ITextIO on the Sim68K I/O window (Trap #15 text tasks): a thin adapter
// over SimIoScreen, callable from the simulator thread.

#ifndef EASY68K_GUI_SIMULATOR_QT_TEXT_IO_H_
#define EASY68K_GUI_SIMULATOR_QT_TEXT_IO_H_

#include "easym68k/sim/itext_io.h"

namespace easym68k::gui {

class SimIoScreen;

class QtTextIO : public sim::ITextIO {
 public:
  explicit QtTextIO(SimIoScreen* screen) : screen_(screen) {}

  void TextOut(std::string_view text) override;
  void TextOutCRLF(std::string_view text) override;
  // Blocks until Enter (or the 79-key limit). `buf` receives the string and
  // its terminator (at most 80 bytes). When the input is cancelled (stop,
  // reset) nothing is stored.
  void TextIn(char* buf, long* size, long* result) override;
  void CharIn(char* ch) override;
  void CharOut(char ch) override;
  void SetCursorPosition(int col, int row) override;
  void GetCursorPosition(int* col, int* row) override;
  void ClearScreen() override;
  void SetKeyboardEcho(bool enable) override;
  void SetInputPrompt(bool enable) override;
  void SetInputLFDisplay(bool enable) override;
  void GetCharAt(int row, int col, char* ch) override;
  void ScrollRect(int row, int col, int height, int width, int direction) override;
  void SetFontProperties(uint32_t color, uint32_t style) override;
  bool IsKeyPending() override;
  void GetKeyState(long* state) override;
  void SetKeyCommandsEnabled(bool enabled) override;

 private:
  SimIoScreen* screen_;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_QT_TEXT_IO_H_
