// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// IPeripheralIO for Sim68K-Qt (Trap #15 tasks 60-62): mouse state on the I/O
// window and the mouse/keyboard interrupts (simIOu.cpp FormMouseDown/Up/
// Move, FormKeyDown/Up; CODE9.CPP tasks 60-62). The I/O window feeds the
// events on the GUI thread; the simulator thread reads the state. The
// interrupt request goes to `irq` (the hardware window's IRQprocess()).

#ifndef EASY68K_GUI_SIMULATOR_QT_PERIPHERAL_IO_H_
#define EASY68K_GUI_SIMULATOR_QT_PERIPHERAL_IO_H_

#include <functional>
#include <mutex>

#include "easym68k/sim/iperipheral_io.h"

namespace easym68k::gui {

class QtPeripheralIO : public sim::IPeripheralIO {
 public:
  using IrqFn = std::function<void(int level)>;

  // Mouse buttons and modifier keys of one event.
  struct Buttons {
    bool left = false;
    bool right = false;
    bool middle = false;
    bool double_click = false;
    bool shift = false;
    bool alt = false;
    bool ctrl = false;
  };

  explicit QtPeripheralIO(IrqFn irq) : irq_(std::move(irq)) {}

  // Task 60/62: `events` bit 0 = button/key down, 1 = up, 2 = mouse move;
  // the selected events interrupt at `irq_level` (0 = off).
  void SetMouseIRQ(uint8_t irq_level, uint8_t events) override;
  void SetKeyIRQ(uint8_t irq_level, uint8_t events) override;
  // Task 61: mode 0 current state, 1 at the last button up, 2 at the last
  // button down. *button_state: bit 0 left, 1 right, 2 middle, 3 double,
  // 4 Shift, 5 Alt, 6 Ctrl; *position = Y << 16 | X. Other modes: state 0,
  // position unchanged.
  void ReadMouse(int mode, long* button_state, long* position) override;

  // I/O window events (GUI thread), in canvas pixels.
  void MouseDown(int x, int y, const Buttons& buttons);
  void MouseUp(int x, int y, const Buttons& buttons);
  void MouseMove(int x, int y, bool shift, bool alt, bool ctrl);
  void KeyEvent(bool down);  // SimIoScreen key hook

  // Program start (STARTSIM.CPP): clears the mouse state and the IRQs.
  void Reset();

 private:
  struct Snapshot {
    int x = 0;
    int y = 0;
    Buttons buttons;
  };
  static long Encode(const Buttons& b);
  void Raise(int level);

  IrqFn irq_;
  std::mutex mu_;
  Snapshot current_;
  Snapshot up_;
  Snapshot down_;
  int mouse_down_irq_ = 0;
  int mouse_up_irq_ = 0;
  int mouse_move_irq_ = 0;
  int key_down_irq_ = 0;
  int key_up_irq_ = 0;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_QT_PERIPHERAL_IO_H_
