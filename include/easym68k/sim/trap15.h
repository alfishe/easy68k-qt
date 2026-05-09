// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Trap #15 task number constants.
// Values match CODE9.CPP exactly — loaded from D0.B by the 68000 program.

#pragma once

#include <cstdint>

namespace easym68k::sim {

// Text I/O (tasks 0-25)
inline constexpr uint8_t kTrap15DisplayStringCRLF = 0;
inline constexpr uint8_t kTrap15DisplayString = 1;
inline constexpr uint8_t kTrap15InputString = 2;
inline constexpr uint8_t kTrap15DisplayNumber = 3;
inline constexpr uint8_t kTrap15InputNumber = 4;
inline constexpr uint8_t kTrap15InputChar = 5;
inline constexpr uint8_t kTrap15DisplayChar = 6;
inline constexpr uint8_t kTrap15KeyPending = 7;
inline constexpr uint8_t kTrap15GetTime = 8;
inline constexpr uint8_t kTrap15Terminate = 9;
inline constexpr uint8_t kTrap15PrintChar = 10;
inline constexpr uint8_t kTrap15CursorControl = 11;
inline constexpr uint8_t kTrap15KeyboardEcho = 12;
inline constexpr uint8_t kTrap15DisplayNullStringCRLF = 13;
inline constexpr uint8_t kTrap15DisplayNullString = 14;
inline constexpr uint8_t kTrap15DisplayRadix = 15;
inline constexpr uint8_t kTrap15InputControl = 16;
inline constexpr uint8_t kTrap15DisplayNullStringNumber = 17;
inline constexpr uint8_t kTrap15DisplayNullStringInputNumber = 18;
inline constexpr uint8_t kTrap15GetKeyState = 19;
inline constexpr uint8_t kTrap15DisplaySignedField = 20;
inline constexpr uint8_t kTrap15SetFontProperties = 21;
inline constexpr uint8_t kTrap15GetCharAt = 22;
inline constexpr uint8_t kTrap15Delay = 23;
inline constexpr uint8_t kTrap15KeyCommandControl = 24;
inline constexpr uint8_t kTrap15ScrollRect = 25;

// Simulator environment (tasks 30-33)
inline constexpr uint8_t kTrap15ClearCycles = 30;
inline constexpr uint8_t kTrap15GetCycles = 31;
inline constexpr uint8_t kTrap15Hardware = 32;
inline constexpr uint8_t kTrap15WindowControl = 33;

// Serial I/O (tasks 40-43)
inline constexpr uint8_t kTrap15SerialInit = 40;
inline constexpr uint8_t kTrap15SerialSetParams = 41;
inline constexpr uint8_t kTrap15SerialRead = 42;
inline constexpr uint8_t kTrap15SerialSend = 43;

// File I/O (tasks 50-59)
inline constexpr uint8_t kTrap15FileCloseAll = 50;
inline constexpr uint8_t kTrap15FileOpen = 51;
inline constexpr uint8_t kTrap15FileNew = 52;
inline constexpr uint8_t kTrap15FileRead = 53;
inline constexpr uint8_t kTrap15FileWrite = 54;
inline constexpr uint8_t kTrap15FilePosition = 55;
inline constexpr uint8_t kTrap15FileClose = 56;
inline constexpr uint8_t kTrap15FileDelete = 57;
inline constexpr uint8_t kTrap15FileDialog = 58;
inline constexpr uint8_t kTrap15FileOp = 59;

// Peripheral I/O (tasks 60-62)
inline constexpr uint8_t kTrap15MouseIRQ = 60;
inline constexpr uint8_t kTrap15MouseRead = 61;
inline constexpr uint8_t kTrap15KeyIRQ = 62;

// Sound (tasks 70-77)
inline constexpr uint8_t kTrap15SoundPlay = 70;
inline constexpr uint8_t kTrap15SoundLoad = 71;
inline constexpr uint8_t kTrap15SoundPlayMem = 72;
inline constexpr uint8_t kTrap15SoundPlayDX = 73;
inline constexpr uint8_t kTrap15SoundLoadDX = 74;
inline constexpr uint8_t kTrap15SoundPlayMemDX = 75;
inline constexpr uint8_t kTrap15SoundControl = 76;
inline constexpr uint8_t kTrap15SoundControlDX = 77;

// Graphics (tasks 80-96)
inline constexpr uint8_t kTrap15SetPenColor = 80;
inline constexpr uint8_t kTrap15SetFillColor = 81;
inline constexpr uint8_t kTrap15DrawPixel = 82;
inline constexpr uint8_t kTrap15GetPixel = 83;
inline constexpr uint8_t kTrap15DrawLine = 84;
inline constexpr uint8_t kTrap15LineTo = 85;
inline constexpr uint8_t kTrap15MoveTo = 86;
inline constexpr uint8_t kTrap15DrawRectangle = 87;
inline constexpr uint8_t kTrap15DrawEllipse = 88;
inline constexpr uint8_t kTrap15FloodFill = 89;
inline constexpr uint8_t kTrap15DrawUnfilledRectangle = 90;
inline constexpr uint8_t kTrap15DrawUnfilledEllipse = 91;
inline constexpr uint8_t kTrap15SetDrawingMode = 92;
inline constexpr uint8_t kTrap15SetPenWidth = 93;
inline constexpr uint8_t kTrap15Repaint = 94;
inline constexpr uint8_t kTrap15DrawText = 95;
inline constexpr uint8_t kTrap15GetXY = 96;

// Network I/O (tasks 100-107)
inline constexpr uint8_t kTrap15NetCreateClient = 100;
inline constexpr uint8_t kTrap15NetCreateServer = 101;
inline constexpr uint8_t kTrap15NetSend = 102;
inline constexpr uint8_t kTrap15NetReceive = 103;
inline constexpr uint8_t kTrap15NetClose = 104;
inline constexpr uint8_t kTrap15NetGetLocalIP = 105;
inline constexpr uint8_t kTrap15NetSendOnPort = 106;
inline constexpr uint8_t kTrap15NetReceiveOnPort = 107;

}  // namespace easym68k::sim
