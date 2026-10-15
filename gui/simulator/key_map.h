// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Qt key codes to Windows virtual-key codes. The 68000 program sees Windows
// VK codes through Trap #15 task 19 and the key IRQ scan codes (simIOu.cpp
// FormKeyDown()/FormKeyUp() store the VCL Key, a VK code), so the port maps
// Qt keys to the same values on every platform.

#ifndef EASY68K_GUI_SIMULATOR_KEY_MAP_H_
#define EASY68K_GUI_SIMULATOR_KEY_MAP_H_

#include <cstdint>
#include <Qt>

namespace easym68k::gui {

// Returns the Windows VK code for a Qt key (Qt::Key), or 0 when the key has
// none. Shifted symbols map to the key that produces them on a US keyboard
// (Qt::Key_Colon -> VK_OEM_1), as Windows reports the physical key.
// `keypad` selects the numeric keypad codes (VK_NUMPAD0.., VK_MULTIPLY..).
uint8_t QtKeyToVirtualKey(int qt_key, bool keypad);

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_KEY_MAP_H_
