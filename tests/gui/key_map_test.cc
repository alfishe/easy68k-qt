// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "key_map.h"

#include <gtest/gtest.h>

namespace easym68k::gui {
namespace {

TEST(KeyMapTest, LettersDigitsAndFunctionKeys) {
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_A, false), 0x41);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Z, false), 0x5A);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_0, false), 0x30);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_9, false), 0x39);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_F1, false), 0x70);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_F12, false), 0x7B);
}

TEST(KeyMapTest, NavigationAndControlKeys) {
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Left, false), 0x25);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Up, false), 0x26);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Right, false), 0x27);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Down, false), 0x28);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Return, false), 0x0D);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Escape, false), 0x1B);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Space, false), 0x20);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Shift, false), 0x10);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Control, false), 0x11);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Alt, false), 0x12);
}

TEST(KeyMapTest, ShiftedSymbolsMapToTheirKey) {
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Exclam, false), '1');
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_ParenRight, false), '0');
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Semicolon, false), 0xBA);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Colon, false), 0xBA);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Question, false), 0xBF);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_BraceRight, false), 0xDD);
}

TEST(KeyMapTest, Keypad) {
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_0, true), 0x60);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_9, true), 0x69);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Asterisk, true), 0x6A);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Plus, true), 0x6B);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Minus, true), 0x6D);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Slash, true), 0x6F);
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_Left, true), 0x25);  // as the main key
}

TEST(KeyMapTest, UnknownKeyIsZero) {
  EXPECT_EQ(QtKeyToVirtualKey(Qt::Key_unknown, false), 0);
}

}  // namespace
}  // namespace easym68k::gui
