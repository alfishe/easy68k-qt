// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "qt_text_io.h"

#include <cstdlib>
#include <cstring>
#include <string>

#include "sim_io_screen.h"

namespace easym68k::gui {

void QtTextIO::TextOut(std::string_view text) {
  screen_->TextOut(text);
}

void QtTextIO::TextOutCRLF(std::string_view text) {
  screen_->TextOutCRLF(text);
}

// simIOu.cpp FormKeyPress() on Enter: the string (task 2) or its value
// (tasks 4, 18: atoi; out-of-range values wrap to 32 bits here instead of
// being undefined).
void QtTextIO::TextIn(char* buf, long* size, long* result) {
  std::string line;
  if (!screen_->ReadLine(&line))
    return;
  std::memcpy(buf, line.c_str(), line.size() + 1);
  *size = static_cast<long>(line.size());
  if (result != nullptr)
    *result = static_cast<long>(static_cast<int32_t>(std::strtoll(line.c_str(), nullptr, 10)));
}

void QtTextIO::CharIn(char* ch) {
  char key = 0;
  if (screen_->ReadChar(&key))
    *ch = key;
}

void QtTextIO::CharOut(char ch) {
  screen_->CharOut(ch);
}

void QtTextIO::SetCursorPosition(int col, int row) {
  screen_->GotoRC(row, col);
}

void QtTextIO::GetCursorPosition(int* col, int* row) {
  screen_->GetRC(col, row);
}

void QtTextIO::ClearScreen() {
  screen_->ClearScreen();
}

void QtTextIO::SetKeyboardEcho(bool enable) {
  screen_->SetKeyboardEcho(enable);
}

void QtTextIO::SetInputPrompt(bool enable) {
  screen_->SetInputPrompt(enable);
}

void QtTextIO::SetInputLFDisplay(bool enable) {
  screen_->SetInputLFDisplay(enable);
}

void QtTextIO::GetCharAt(int row, int col, char* ch) {
  *ch = screen_->CharAt(row, col);
}

void QtTextIO::ScrollRect(int row, int col, int height, int width, int direction) {
  screen_->ScrollRect(row, col, height, width, direction);
}

void QtTextIO::SetFontProperties(uint32_t color, uint32_t style) {
  screen_->SetFontProperties(color, style);
}

bool QtTextIO::IsKeyPending() {
  return screen_->IsKeyPending();
}

void QtTextIO::GetKeyState(long* state) {
  screen_->GetKeyState(state);
}

void QtTextIO::SetKeyCommandsEnabled(bool enabled) {
  screen_->SetKeyCommandsEnabled(enabled);
}

}  // namespace easym68k::gui
