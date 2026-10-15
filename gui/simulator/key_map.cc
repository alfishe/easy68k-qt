// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "key_map.h"

namespace easym68k::gui {

uint8_t QtKeyToVirtualKey(int qt_key, bool keypad) {
  if (keypad) {
    if (qt_key >= Qt::Key_0 && qt_key <= Qt::Key_9)
      return static_cast<uint8_t>(0x60 + (qt_key - Qt::Key_0));  // VK_NUMPAD0..9
    switch (qt_key) {
      case Qt::Key_Asterisk:
        return 0x6A;  // VK_MULTIPLY
      case Qt::Key_Plus:
        return 0x6B;  // VK_ADD
      case Qt::Key_Minus:
        return 0x6D;  // VK_SUBTRACT
      case Qt::Key_Period:
      case Qt::Key_Comma:
        return 0x6E;  // VK_DECIMAL
      case Qt::Key_Slash:
        return 0x6F;  // VK_DIVIDE
      case Qt::Key_Enter:
        return 0x0D;
      default:
        break;  // navigation keys on the keypad: as the main keys
    }
  }
  if (qt_key >= Qt::Key_A && qt_key <= Qt::Key_Z)
    return static_cast<uint8_t>('A' + (qt_key - Qt::Key_A));
  if (qt_key >= Qt::Key_0 && qt_key <= Qt::Key_9)
    return static_cast<uint8_t>('0' + (qt_key - Qt::Key_0));
  if (qt_key >= Qt::Key_F1 && qt_key <= Qt::Key_F24)
    return static_cast<uint8_t>(0x70 + (qt_key - Qt::Key_F1));  // VK_F1..VK_F24
  switch (qt_key) {
    case Qt::Key_Backspace:
      return 0x08;
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
      return 0x09;
    case Qt::Key_Clear:
      return 0x0C;
    case Qt::Key_Return:
    case Qt::Key_Enter:
      return 0x0D;
    case Qt::Key_Shift:
      return 0x10;
    case Qt::Key_Control:
      return 0x11;
    case Qt::Key_Alt:
    case Qt::Key_AltGr:
      return 0x12;  // VK_MENU
    case Qt::Key_Pause:
      return 0x13;
    case Qt::Key_CapsLock:
      return 0x14;
    case Qt::Key_Escape:
      return 0x1B;
    case Qt::Key_Space:
      return 0x20;
    case Qt::Key_PageUp:
      return 0x21;
    case Qt::Key_PageDown:
      return 0x22;
    case Qt::Key_End:
      return 0x23;
    case Qt::Key_Home:
      return 0x24;
    case Qt::Key_Left:
      return 0x25;
    case Qt::Key_Up:
      return 0x26;
    case Qt::Key_Right:
      return 0x27;
    case Qt::Key_Down:
      return 0x28;
    case Qt::Key_Print:
      return 0x2C;  // VK_SNAPSHOT
    case Qt::Key_Insert:
      return 0x2D;
    case Qt::Key_Delete:
      return 0x2E;
    case Qt::Key_Help:
      return 0x2F;
    case Qt::Key_Meta:
    case Qt::Key_Super_L:
      return 0x5B;  // VK_LWIN
    case Qt::Key_Super_R:
      return 0x5C;  // VK_RWIN
    case Qt::Key_Menu:
      return 0x5D;  // VK_APPS
    case Qt::Key_NumLock:
      return 0x90;
    case Qt::Key_ScrollLock:
      return 0x91;
    // Shifted digits (US layout): the VK code is the digit key's.
    case Qt::Key_Exclam:
      return '1';
    case Qt::Key_At:
      return '2';
    case Qt::Key_NumberSign:
      return '3';
    case Qt::Key_Dollar:
      return '4';
    case Qt::Key_Percent:
      return '5';
    case Qt::Key_AsciiCircum:
      return '6';
    case Qt::Key_Ampersand:
      return '7';
    case Qt::Key_Asterisk:
      return '8';
    case Qt::Key_ParenLeft:
      return '9';
    case Qt::Key_ParenRight:
      return '0';
    // OEM keys (US layout), unshifted and shifted.
    case Qt::Key_Semicolon:
    case Qt::Key_Colon:
      return 0xBA;  // VK_OEM_1
    case Qt::Key_Equal:
    case Qt::Key_Plus:
      return 0xBB;  // VK_OEM_PLUS
    case Qt::Key_Comma:
    case Qt::Key_Less:
      return 0xBC;  // VK_OEM_COMMA
    case Qt::Key_Minus:
    case Qt::Key_Underscore:
      return 0xBD;  // VK_OEM_MINUS
    case Qt::Key_Period:
    case Qt::Key_Greater:
      return 0xBE;  // VK_OEM_PERIOD
    case Qt::Key_Slash:
    case Qt::Key_Question:
      return 0xBF;  // VK_OEM_2
    case Qt::Key_QuoteLeft:
    case Qt::Key_AsciiTilde:
      return 0xC0;  // VK_OEM_3
    case Qt::Key_BracketLeft:
    case Qt::Key_BraceLeft:
      return 0xDB;  // VK_OEM_4
    case Qt::Key_Backslash:
    case Qt::Key_Bar:
      return 0xDC;  // VK_OEM_5
    case Qt::Key_BracketRight:
    case Qt::Key_BraceRight:
      return 0xDD;  // VK_OEM_6
    case Qt::Key_Apostrophe:
    case Qt::Key_QuoteDbl:
      return 0xDE;  // VK_OEM_7
    default:
      return 0;
  }
}

}  // namespace easym68k::gui
