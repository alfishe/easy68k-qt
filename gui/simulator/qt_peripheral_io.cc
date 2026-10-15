// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "qt_peripheral_io.h"

#include <cstdint>

namespace easym68k::gui {

void QtPeripheralIO::SetMouseIRQ(uint8_t irq_level, uint8_t events) {
  if (irq_level > 7)
    return;
  std::lock_guard<std::mutex> lock(mu_);
  if (events & 0x01)
    mouse_down_irq_ = irq_level;
  if (events & 0x02)
    mouse_up_irq_ = irq_level;
  if (events & 0x04)
    mouse_move_irq_ = irq_level;
}

void QtPeripheralIO::SetKeyIRQ(uint8_t irq_level, uint8_t events) {
  if (irq_level > 7)
    return;
  std::lock_guard<std::mutex> lock(mu_);
  if (events & 0x01)
    key_down_irq_ = irq_level;
  if (events & 0x02)
    key_up_irq_ = irq_level;
}

long QtPeripheralIO::Encode(const Buttons& b) {
  return (b.left ? 0x01 : 0) | (b.right ? 0x02 : 0) | (b.middle ? 0x04 : 0) |
         (b.double_click ? 0x08 : 0) | (b.shift ? 0x10 : 0) | (b.alt ? 0x20 : 0) |
         (b.ctrl ? 0x40 : 0);
}

void QtPeripheralIO::ReadMouse(int mode, long* button_state, long* position) {
  std::lock_guard<std::mutex> lock(mu_);
  const Snapshot* s = nullptr;
  switch (mode) {
    case 0:
      s = &current_;
      break;
    case 1:
      s = &up_;
      break;
    case 2:
      s = &down_;
      break;
    default:
      *button_state = 0;
      return;
  }
  *button_state = Encode(s->buttons);
  *position = static_cast<long>(
      static_cast<int32_t>((static_cast<uint32_t>(s->y) << 16) | static_cast<uint16_t>(s->x)));
}

void QtPeripheralIO::MouseDown(int x, int y, const Buttons& buttons) {
  int irq = 0;
  {
    std::lock_guard<std::mutex> lock(mu_);
    current_ = {x, y, buttons};
    down_ = current_;
    irq = mouse_down_irq_;
  }
  Raise(irq);
}

void QtPeripheralIO::MouseUp(int x, int y, const Buttons& buttons) {
  int irq = 0;
  {
    std::lock_guard<std::mutex> lock(mu_);
    current_ = {x, y, buttons};
    up_ = current_;
    irq = mouse_up_irq_;
  }
  Raise(irq);
}

// FormMouseMove: position and modifier keys; the buttons keep their state.
void QtPeripheralIO::MouseMove(int x, int y, bool shift, bool alt, bool ctrl) {
  int irq = 0;
  {
    std::lock_guard<std::mutex> lock(mu_);
    current_.x = x;
    current_.y = y;
    current_.buttons.shift = shift;
    current_.buttons.alt = alt;
    current_.buttons.ctrl = ctrl;
    irq = mouse_move_irq_;
  }
  Raise(irq);
}

void QtPeripheralIO::KeyEvent(bool down) {
  int irq = 0;
  {
    std::lock_guard<std::mutex> lock(mu_);
    irq = down ? key_down_irq_ : key_up_irq_;
  }
  Raise(irq);
}

void QtPeripheralIO::Reset() {
  std::lock_guard<std::mutex> lock(mu_);
  current_ = {};
  up_ = {};
  down_ = {};
  mouse_down_irq_ = mouse_up_irq_ = mouse_move_irq_ = 0;
  key_down_irq_ = key_up_irq_ = 0;
}

void QtPeripheralIO::Raise(int level) {
  if (level != 0 && irq_)
    irq_(level);
}

}  // namespace easym68k::gui
