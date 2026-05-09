// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Trap #15 dispatch tests — one test per task number.
// Verifies register parameter mapping and interface call routing using gmock.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <string_view>

#include "easym68k/sim/ifile_io.h"
#include "easym68k/sim/igraphics_io.h"
#include "easym68k/sim/ilogger.h"
#include "easym68k/sim/inetwork_io.h"
#include "easym68k/sim/iperipheral_io.h"
#include "easym68k/sim/iprint_io.h"
#include "easym68k/sim/iserial_io.h"
#include "easym68k/sim/isimulator_env.h"
#include "easym68k/sim/isound_io.h"
#include "easym68k/sim/itext_io.h"
#include "easym68k/sim/simulator.h"
#include "easym68k/sim/trap15.h"
#include "easym68k/sim/types.h"

namespace easym68k::sim {
namespace {

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;

// ---------------------------------------------------------------------------
// Mock classes
// ---------------------------------------------------------------------------

class MockTextIO : public ITextIO {
 public:
  MOCK_METHOD(void, TextOut, (std::string_view), (override));
  MOCK_METHOD(void, TextOutCRLF, (std::string_view), (override));
  MOCK_METHOD(void, TextIn, (char*, long*, long*), (override));
  MOCK_METHOD(void, CharIn, (char*), (override));
  MOCK_METHOD(void, CharOut, (char), (override));
  MOCK_METHOD(void, SetCursorPosition, (int, int), (override));
  MOCK_METHOD(void, GetCursorPosition, (int*, int*), (override));
  MOCK_METHOD(void, ClearScreen, (), (override));
  MOCK_METHOD(void, SetKeyboardEcho, (bool), (override));
  MOCK_METHOD(void, SetInputPrompt, (bool), (override));
  MOCK_METHOD(void, SetInputLFDisplay, (bool), (override));
  MOCK_METHOD(void, GetCharAt, (int, int, char*), (override));
  MOCK_METHOD(void, ScrollRect, (int, int, int, int, int), (override));
  MOCK_METHOD(void, SetFontProperties, (uint32_t, uint32_t), (override));
  MOCK_METHOD(void, GetKeyState, (long*), (override));
  MOCK_METHOD(bool, IsKeyPending, (), (override));
  MOCK_METHOD(void, SetKeyCommandsEnabled, (bool), (override));
};

class MockFileIO : public IFileIO {
 public:
  MOCK_METHOD(void, CloseAll, (short*), (override));
  MOCK_METHOD(void, OpenFile, (long*, const char*, short*), (override));
  MOCK_METHOD(void, NewFile, (long*, const char*, short*), (override));
  MOCK_METHOD(void, ReadFile, (long, char*, unsigned int*, short*), (override));
  MOCK_METHOD(void, WriteFile, (long, const char*, unsigned int, short*), (override));
  MOCK_METHOD(void, PositionFile, (long, int, short*), (override));
  MOCK_METHOD(void, CloseFile, (long, short*), (override));
  MOCK_METHOD(void, DeleteFile, (const char*, short*), (override));
  MOCK_METHOD(void, DisplayFileDialog, (long*, long, long, long, short*), (override));
  MOCK_METHOD(void, FileOp, (long*, const char*, short*), (override));
};

class MockSerialIO : public ISerialIO {
 public:
  MOCK_METHOD(void, InitPort, (int, const char*, short*), (override));
  MOCK_METHOD(void, SetPortParams, (int, int, short*), (override));
  MOCK_METHOD(void, ReadString, (int, unsigned char*, char*, short*), (override));
  MOCK_METHOD(void, SendString, (int, unsigned char*, const char*, short*), (override));
  MOCK_METHOD(void, CloseAll, (), (override));
};

class MockNetworkIO : public INetworkIO {
 public:
  MOCK_METHOD(void, CreateClient, (int, const char*, int*), (override));
  MOCK_METHOD(void, CreateServer, (int, int*), (override));
  MOCK_METHOD(void, Send, (int, const char*, const char*, int*, int*), (override));
  MOCK_METHOD(void, Receive, (int, char*, int*, char*, int*), (override));
  MOCK_METHOD(void, CloseConnection, (int, int*), (override));
  MOCK_METHOD(void, GetLocalIP, (char*, int*), (override));
  MOCK_METHOD(void, SendOnPort, (long*, long*, const char*, const char*), (override));
  MOCK_METHOD(void, ReceiveOnPort, (long*, long*, char*, char*), (override));
};

class MockGraphicsIO : public IGraphicsIO {
 public:
  MOCK_METHOD(void, SetPenColor, (uint32_t), (override));
  MOCK_METHOD(void, SetFillColor, (uint32_t), (override));
  MOCK_METHOD(void, DrawPixel, (int, int), (override));
  MOCK_METHOD(int, GetPixel, (int, int), (override));
  MOCK_METHOD(void, DrawLine, (int, int, int, int), (override));
  MOCK_METHOD(void, LineTo, (int, int), (override));
  MOCK_METHOD(void, MoveTo, (int, int), (override));
  MOCK_METHOD(void, GetXY, (int*, int*), (override));
  MOCK_METHOD(void, DrawRectangle, (int, int, int, int), (override));
  MOCK_METHOD(void, DrawEllipse, (int, int, int, int), (override));
  MOCK_METHOD(void, FloodFill, (int, int), (override));
  MOCK_METHOD(void, DrawUnfilledRectangle, (int, int, int, int), (override));
  MOCK_METHOD(void, DrawUnfilledEllipse, (int, int, int, int), (override));
  MOCK_METHOD(void, SetDrawingMode, (int), (override));
  MOCK_METHOD(void, SetPenWidth, (int), (override));
  MOCK_METHOD(void, Repaint, (), (override));
  MOCK_METHOD(void, DrawText, (const char*, int, int), (override));
};

class MockSoundIO : public ISoundIO {
 public:
  MOCK_METHOD(void, PlaySound, (const char*, short*), (override));
  MOCK_METHOD(void, LoadSound, (const char*, int), (override));
  MOCK_METHOD(void, PlaySoundMem, (int, short*), (override));
  MOCK_METHOD(void, ControlSound, (int, int, short*), (override));
  MOCK_METHOD(void, PlaySoundMulti, (const char*, short*), (override));
  MOCK_METHOD(void, LoadSoundMulti, (const char*, int, short*), (override));
  MOCK_METHOD(void, PlaySoundMemMulti, (int, short*), (override));
  MOCK_METHOD(void, StopSoundMemMulti, (int, short*), (override));
  MOCK_METHOD(void, ControlSoundMulti, (int, int, short*), (override));
  MOCK_METHOD(void, ResetSounds, (), (override));
};

class MockPeripheralIO : public IPeripheralIO {
 public:
  MOCK_METHOD(void, SetMouseIRQ, (uint8_t, uint8_t), (override));
  MOCK_METHOD(void, ReadMouse, (int, long*, long*), (override));
  MOCK_METHOD(void, SetKeyIRQ, (uint8_t, uint8_t), (override));
};

class MockSimulatorEnv : public ISimulatorEnv {
 public:
  MOCK_METHOD(uint32_t, GetTimeHundredths, (), (override));
  MOCK_METHOD(void, DelayHundredths, (unsigned int), (override));
  MOCK_METHOD(void, ShowHardware, (), (override));
  MOCK_METHOD(uint32_t, GetSeg7Address, (), (override));
  MOCK_METHOD(uint32_t, GetLEDAddress, (), (override));
  MOCK_METHOD(uint32_t, GetSwitchAddress, (), (override));
  MOCK_METHOD(uint32_t, GetPushButtonAddress, (), (override));
  MOCK_METHOD(void, SetAutoIRQ, (uint8_t, uint32_t), (override));
  MOCK_METHOD(void, ClearCycles, (), (override));
  MOCK_METHOD(uint32_t, GetCycles, (), (override));
  MOCK_METHOD(uint32_t, GetVersion, (), (override));
  MOCK_METHOD(void, SetExceptionsEnabled, (bool), (override));
  MOCK_METHOD(void, SetWindowSize, (unsigned short, unsigned short), (override));
  MOCK_METHOD(void, GetWindowSize, (unsigned short*, unsigned short*), (override));
  MOCK_METHOD(void, SetupWindow, (bool), (override));
};

class MockPrintIO : public IPrintIO {
 public:
  MOCK_METHOD(void, PrintChar, (char), (override));
  MOCK_METHOD(void, FormFeed, (), (override));
};

class MockLogger : public ILogger {
 public:
  MOCK_METHOD(void, Log, (std::string_view), (override));
};

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class Trap15Test : public ::testing::Test {
 protected:
  ::testing::NiceMock<MockTextIO> text_io_;
  ::testing::NiceMock<MockFileIO> file_io_;
  ::testing::NiceMock<MockSerialIO> serial_io_;
  ::testing::NiceMock<MockNetworkIO> network_io_;
  ::testing::NiceMock<MockGraphicsIO> graphics_io_;
  ::testing::NiceMock<MockSoundIO> sound_io_;
  ::testing::NiceMock<MockPeripheralIO> peripheral_io_;
  ::testing::NiceMock<MockSimulatorEnv> sim_env_;
  ::testing::NiceMock<MockPrintIO> print_io_;
  ::testing::NiceMock<MockLogger> logger_;

  std::unique_ptr<M68kSimulator> sim_;

  void SetUp() override {
    M68kSimulator::Config cfg;
    cfg.text_io = &text_io_;
    cfg.file_io = &file_io_;
    cfg.serial_io = &serial_io_;
    cfg.network_io = &network_io_;
    cfg.graphics_io = &graphics_io_;
    cfg.sound_io = &sound_io_;
    cfg.peripheral_io = &peripheral_io_;
    cfg.sim_env = &sim_env_;
    cfg.print_io = &print_io_;
    cfg.logger = &logger_;
    sim_ = std::make_unique<M68kSimulator>(std::move(cfg));
    sim_->GetMemory().WriteLong(0x0, 0x3FFE);
    sim_->GetMemory().WriteLong(0x4, 0x1000);
    sim_->Reset();
    sim_->State().sr = kSrSupervisor;
    sim_->GetMemory().WriteWord(0x1000, 0x4E4F);  // TRAP #15
  }

  // Write null-terminated string into 68000 memory at 0x2000, set A1.
  void SetStringA1(const char* str) {
    uint32_t addr = 0x2000;
    for (size_t i = 0; str[i]; ++i)
      sim_->GetMemory().WriteByte(addr + i, static_cast<uint8_t>(str[i]));
    sim_->GetMemory().WriteByte(addr + __builtin_strlen(str), 0);
    sim_->State().a[1] = addr;
  }

  SimResult RunTask(uint8_t task) {
    sim_->State().d[0] = task;
    return sim_->Step();
  }
};

// ===========================================================================
// TEXT I/O
// ===========================================================================

TEST_F(Trap15Test, Task0_DisplayStringCRLF) {
  SetStringA1("Hi");
  sim_->State().d[1] = 2;
  EXPECT_CALL(text_io_, TextOutCRLF(_)).Times(1);
  RunTask(kTrap15DisplayStringCRLF);
}

TEST_F(Trap15Test, Task1_DisplayString) {
  SetStringA1("Hi");
  sim_->State().d[1] = 2;
  EXPECT_CALL(text_io_, TextOut(_)).Times(1);
  RunTask(kTrap15DisplayString);
}

TEST_F(Trap15Test, Task2_InputString_UpdatesD1W) {
  sim_->State().a[1] = 0x2000;
  sim_->State().d[1] = 80;
  EXPECT_CALL(text_io_, TextIn(_, _, nullptr)).WillOnce(SetArgPointee<1>(5L));
  RunTask(kTrap15InputString);
  EXPECT_EQ(sim_->State().d[1], 5u);
}

TEST_F(Trap15Test, Task3_DisplayNumber_CallsTextOut) {
  sim_->State().d[1] = 42;
  EXPECT_CALL(text_io_, TextOut(_)).Times(1);
  RunTask(kTrap15DisplayNumber);
}

TEST_F(Trap15Test, Task4_InputNumber_WritesD1L) {
  EXPECT_CALL(text_io_, TextIn(_, _, _)).WillOnce(SetArgPointee<2>(99L));
  RunTask(kTrap15InputNumber);
  EXPECT_EQ(sim_->State().d[1], 99u);
}

TEST_F(Trap15Test, Task5_InputChar_WritesD1B) {
  EXPECT_CALL(text_io_, CharIn(_)).WillOnce(SetArgPointee<0>('Z'));
  sim_->State().d[1] = 0xABCD1200u;
  RunTask(kTrap15InputChar);
  EXPECT_EQ(sim_->State().d[1] & 0xFF, static_cast<uint32_t>('Z'));
  EXPECT_EQ(sim_->State().d[1] & 0xFFFFFF00u, 0xABCD1200u);  // upper bytes preserved
}

TEST_F(Trap15Test, Task6_DisplayChar_PassesD1B) {
  sim_->State().d[1] = 'A';
  EXPECT_CALL(text_io_, CharOut('A')).Times(1);
  RunTask(kTrap15DisplayChar);
}

TEST_F(Trap15Test, Task7_KeyPending_True_SetsD1B1) {
  EXPECT_CALL(text_io_, IsKeyPending()).WillOnce(Return(true));
  sim_->State().d[1] = 0xFFFFFF00u;
  RunTask(kTrap15KeyPending);
  EXPECT_EQ(sim_->State().d[1], 0xFFFFFF01u);
}

TEST_F(Trap15Test, Task7_KeyPending_False_ClearsD1B) {
  EXPECT_CALL(text_io_, IsKeyPending()).WillOnce(Return(false));
  sim_->State().d[1] = 0xFFFFFF01u;
  RunTask(kTrap15KeyPending);
  EXPECT_EQ(sim_->State().d[1], 0xFFFFFF00u);
}

TEST_F(Trap15Test, Task8_GetTime_WritesD1L) {
  EXPECT_CALL(sim_env_, GetTimeHundredths()).WillOnce(Return(360000u));
  RunTask(kTrap15GetTime);
  EXPECT_EQ(sim_->State().d[1], 360000u);
}

TEST_F(Trap15Test, Task9_Terminate_ReturnsKHalted) {
  EXPECT_EQ(RunTask(kTrap15Terminate), SimResult::kHalted);
}

TEST_F(Trap15Test, Task10_PrintChar_PrintsEachChar) {
  SetStringA1("AB");
  EXPECT_CALL(print_io_, PrintChar('A')).Times(1);
  EXPECT_CALL(print_io_, PrintChar('B')).Times(1);
  RunTask(kTrap15PrintChar);
}

TEST_F(Trap15Test, Task11_CursorControl_ClearScreen) {
  sim_->State().d[1] = 0xFF00;
  EXPECT_CALL(text_io_, ClearScreen()).Times(1);
  RunTask(kTrap15CursorControl);
}

TEST_F(Trap15Test, Task11_CursorControl_GetPosition) {
  sim_->State().d[1] = 0x00FF;
  EXPECT_CALL(text_io_, GetCursorPosition(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(5), SetArgPointee<1>(3)));
  RunTask(kTrap15CursorControl);
  // col in high byte, row in low byte of D1.W
  EXPECT_EQ(static_cast<uint16_t>(sim_->State().d[1]), static_cast<uint16_t>((5 << 8) | 3));
}

TEST_F(Trap15Test, Task11_CursorControl_SetPosition) {
  sim_->State().d[1] = (10 << 8) | 7;  // col=10, row=7
  EXPECT_CALL(text_io_, SetCursorPosition(10, 7)).Times(1);
  RunTask(kTrap15CursorControl);
}

TEST_F(Trap15Test, Task12_KeyboardEcho_Off) {
  sim_->State().d[1] = 0;
  EXPECT_CALL(text_io_, SetKeyboardEcho(false)).Times(1);
  RunTask(kTrap15KeyboardEcho);
}

TEST_F(Trap15Test, Task12_KeyboardEcho_On) {
  sim_->State().d[1] = 1;
  EXPECT_CALL(text_io_, SetKeyboardEcho(true)).Times(1);
  RunTask(kTrap15KeyboardEcho);
}

TEST_F(Trap15Test, Task13_DisplayNullStringCRLF) {
  SetStringA1("test");
  EXPECT_CALL(text_io_, TextOutCRLF(_)).Times(1);
  RunTask(kTrap15DisplayNullStringCRLF);
}

TEST_F(Trap15Test, Task14_DisplayNullString) {
  SetStringA1("test");
  EXPECT_CALL(text_io_, TextOut(_)).Times(1);
  RunTask(kTrap15DisplayNullString);
}

TEST_F(Trap15Test, Task15_DisplayRadix_Base16) {
  sim_->State().d[1] = 255;
  sim_->State().d[2] = 16;
  EXPECT_CALL(text_io_, TextOut(_)).Times(1);
  RunTask(kTrap15DisplayRadix);
}

TEST_F(Trap15Test, Task15_DisplayRadix_InvalidBase_NoCall) {
  sim_->State().d[1] = 255;
  sim_->State().d[2] = 1;  // invalid base
  EXPECT_CALL(text_io_, TextOut(_)).Times(0);
  RunTask(kTrap15DisplayRadix);
}

TEST_F(Trap15Test, Task16_InputControl_PromptOff) {
  sim_->State().d[1] = 0;
  EXPECT_CALL(text_io_, SetInputPrompt(false)).Times(1);
  RunTask(kTrap15InputControl);
}

TEST_F(Trap15Test, Task16_InputControl_PromptOn) {
  sim_->State().d[1] = 1;
  EXPECT_CALL(text_io_, SetInputPrompt(true)).Times(1);
  RunTask(kTrap15InputControl);
}

TEST_F(Trap15Test, Task16_InputControl_LFDisplayOff) {
  sim_->State().d[1] = 2;
  EXPECT_CALL(text_io_, SetInputLFDisplay(false)).Times(1);
  RunTask(kTrap15InputControl);
}

TEST_F(Trap15Test, Task16_InputControl_LFDisplayOn) {
  sim_->State().d[1] = 3;
  EXPECT_CALL(text_io_, SetInputLFDisplay(true)).Times(1);
  RunTask(kTrap15InputControl);
}

TEST_F(Trap15Test, Task17_DisplayNullStringNumber) {
  SetStringA1("val=");
  sim_->State().d[1] = 42;
  EXPECT_CALL(text_io_, TextOut(_)).Times(2);  // string + number
  RunTask(kTrap15DisplayNullStringNumber);
}

TEST_F(Trap15Test, Task18_DisplayNullStringInputNumber) {
  SetStringA1("enter: ");
  EXPECT_CALL(text_io_, TextOut(_)).Times(1);
  EXPECT_CALL(text_io_, TextIn(_, _, _)).WillOnce(SetArgPointee<2>(7L));
  RunTask(kTrap15DisplayNullStringInputNumber);
  EXPECT_EQ(sim_->State().d[1], 7u);
}

TEST_F(Trap15Test, Task19_GetKeyState_WritesD1L) {
  sim_->State().d[1] = 0x41000000u;
  EXPECT_CALL(text_io_, GetKeyState(_)).WillOnce(SetArgPointee<0>(static_cast<long>(0xFF000000L)));
  RunTask(kTrap15GetKeyState);
  EXPECT_EQ(sim_->State().d[1], 0xFF000000u);
}

TEST_F(Trap15Test, Task20_DisplaySignedField) {
  sim_->State().d[1] = 42;
  sim_->State().d[2] = 6;
  EXPECT_CALL(text_io_, TextOut(_)).Times(1);
  RunTask(kTrap15DisplaySignedField);
}

TEST_F(Trap15Test, Task21_SetFontProperties) {
  sim_->State().d[1] = 0x00FF0000u;  // color
  sim_->State().d[2] = 0x00000001u;  // style
  EXPECT_CALL(text_io_, SetFontProperties(0x00FF0000u, 0x00000001u)).Times(1);
  RunTask(kTrap15SetFontProperties);
}

TEST_F(Trap15Test, Task22_GetCharAt_WritesD1B) {
  sim_->State().d[1] = (5u << 16) | 10u;  // row=5, col=10
  EXPECT_CALL(text_io_, GetCharAt(5, 10, _)).WillOnce(SetArgPointee<2>('X'));
  RunTask(kTrap15GetCharAt);
  EXPECT_EQ(sim_->State().d[1] & 0xFF, static_cast<uint32_t>('X'));
}

TEST_F(Trap15Test, Task23_Delay_PassesD1W) {
  sim_->State().d[1] = 50;
  EXPECT_CALL(sim_env_, DelayHundredths(50u)).Times(1);
  RunTask(kTrap15Delay);
}

TEST_F(Trap15Test, Task24_KeyCommandControl_EnableOnD1Zero) {
  sim_->State().d[1] = 0;
  EXPECT_CALL(text_io_, SetKeyCommandsEnabled(true)).Times(1);
  RunTask(kTrap15KeyCommandControl);
}

TEST_F(Trap15Test, Task24_KeyCommandControl_DisableOnD1One) {
  sim_->State().d[1] = 1;
  EXPECT_CALL(text_io_, SetKeyCommandsEnabled(false)).Times(1);
  RunTask(kTrap15KeyCommandControl);
}

TEST_F(Trap15Test, Task25_ScrollRect) {
  sim_->State().d[1] = (2u << 16) | 3u;    // row=2, col=3
  sim_->State().d[2] = (10u << 16) | 20u;  // height=10, width=20
  sim_->State().d[3] = 1;                  // direction=down
  EXPECT_CALL(text_io_, ScrollRect(2, 3, 10, 20, 1)).Times(1);
  RunTask(kTrap15ScrollRect);
}

// ===========================================================================
// SIMULATOR ENVIRONMENT
// ===========================================================================

TEST_F(Trap15Test, Task30_ClearCycles) {
  EXPECT_CALL(sim_env_, ClearCycles()).Times(1);
  RunTask(kTrap15ClearCycles);
}

TEST_F(Trap15Test, Task31_GetCycles_WritesD1L) {
  EXPECT_CALL(sim_env_, GetCycles()).WillOnce(Return(99999u));
  RunTask(kTrap15GetCycles);
  EXPECT_EQ(sim_->State().d[1], 99999u);
}

TEST_F(Trap15Test, Task32_Sub0_ShowHardware) {
  sim_->State().d[1] = 0;
  EXPECT_CALL(sim_env_, ShowHardware()).Times(1);
  RunTask(kTrap15Hardware);
}

TEST_F(Trap15Test, Task32_Sub1_GetSeg7Address) {
  sim_->State().d[1] = 1;
  EXPECT_CALL(sim_env_, GetSeg7Address()).WillOnce(Return(0xE00010u));
  RunTask(kTrap15Hardware);
  EXPECT_EQ(sim_->State().d[1], 0xE00010u);
}

TEST_F(Trap15Test, Task32_Sub2_GetLEDAddress) {
  sim_->State().d[1] = 2;
  EXPECT_CALL(sim_env_, GetLEDAddress()).WillOnce(Return(0xE00020u));
  RunTask(kTrap15Hardware);
  EXPECT_EQ(sim_->State().d[1], 0xE00020u);
}

TEST_F(Trap15Test, Task32_Sub3_GetSwitchAddress) {
  sim_->State().d[1] = 3;
  EXPECT_CALL(sim_env_, GetSwitchAddress()).WillOnce(Return(0xE00030u));
  RunTask(kTrap15Hardware);
  EXPECT_EQ(sim_->State().d[1], 0xE00030u);
}

TEST_F(Trap15Test, Task32_Sub4_GetVersion) {
  sim_->State().d[1] = 4;
  EXPECT_CALL(sim_env_, GetVersion()).WillOnce(Return(0x00040703u));
  RunTask(kTrap15Hardware);
  EXPECT_EQ(sim_->State().d[1], 0x00040703u);
}

TEST_F(Trap15Test, Task32_Sub5_EnableExceptions) {
  sim_->State().d[1] = 5;
  EXPECT_CALL(sim_env_, SetExceptionsEnabled(true)).Times(1);
  RunTask(kTrap15Hardware);
}

TEST_F(Trap15Test, Task32_Sub6_SetAutoIRQ) {
  sim_->State().d[1] = 6;
  sim_->State().d[2] = 0x83;  // enable IRQ 3
  sim_->State().d[3] = 1000;  // 1000 ms
  EXPECT_CALL(sim_env_, SetAutoIRQ(0x83, 1000u)).Times(1);
  RunTask(kTrap15Hardware);
}

TEST_F(Trap15Test, Task32_Sub7_GetPushButtonAddress) {
  sim_->State().d[1] = 7;
  EXPECT_CALL(sim_env_, GetPushButtonAddress()).WillOnce(Return(0xE00040u));
  RunTask(kTrap15Hardware);
  EXPECT_EQ(sim_->State().d[1], 0xE00040u);
}

TEST_F(Trap15Test, Task33_GetWindowSize_D1Zero) {
  sim_->State().d[1] = 0;
  EXPECT_CALL(sim_env_, GetWindowSize(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(static_cast<unsigned short>(800)),
                      SetArgPointee<1>(static_cast<unsigned short>(600))));
  RunTask(kTrap15WindowControl);
  EXPECT_EQ(sim_->State().d[1], (800u << 16) | 600u);
}

TEST_F(Trap15Test, Task33_WindowedMode_D1One) {
  sim_->State().d[1] = 1;
  EXPECT_CALL(sim_env_, SetupWindow(false)).Times(1);
  RunTask(kTrap15WindowControl);
}

TEST_F(Trap15Test, Task33_FullscreenMode_D1Two) {
  sim_->State().d[1] = 2;
  EXPECT_CALL(sim_env_, SetupWindow(true)).Times(1);
  RunTask(kTrap15WindowControl);
}

TEST_F(Trap15Test, Task33_SetWindowSize) {
  sim_->State().d[1] = (1024u << 16) | 768u;
  EXPECT_CALL(sim_env_, SetWindowSize(1024, 768)).Times(1);
  RunTask(kTrap15WindowControl);
}

// ===========================================================================
// SERIAL I/O
// ===========================================================================

TEST_F(Trap15Test, Task40_SerialInit) {
  SetStringA1("COM1");
  sim_->State().d[0] = (2u << 16) | kTrap15SerialInit;  // CID=2
  EXPECT_CALL(serial_io_, InitPort(2, _, _)).Times(1);
  sim_->Step();
}

TEST_F(Trap15Test, Task41_SerialSetParams) {
  sim_->State().d[0] = (1u << 16) | kTrap15SerialSetParams;
  sim_->State().d[1] = 7;  // 9600 baud
  EXPECT_CALL(serial_io_, SetPortParams(1, 7, _)).Times(1);
  sim_->Step();
}

TEST_F(Trap15Test, Task42_SerialRead_UpdatesD1B) {
  sim_->State().a[1] = 0x2000;
  sim_->State().d[0] = (0u << 16) | kTrap15SerialRead;
  sim_->State().d[1] = 10;  // request 10 chars
  EXPECT_CALL(serial_io_, ReadString(0, _, _, _))
      .WillOnce(SetArgPointee<1>(static_cast<unsigned char>(5)));
  sim_->Step();
  EXPECT_EQ(sim_->State().d[1] & 0xFF, 5u);
}

TEST_F(Trap15Test, Task43_SerialSend) {
  SetStringA1("hello");
  sim_->State().d[0] = (0u << 16) | kTrap15SerialSend;
  sim_->State().d[1] = 5;
  EXPECT_CALL(serial_io_, SendString(0, _, _, _)).Times(1);
  sim_->Step();
}

// ===========================================================================
// FILE I/O
// ===========================================================================

TEST_F(Trap15Test, Task50_FileCloseAll) {
  EXPECT_CALL(file_io_, CloseAll(_)).Times(1);
  RunTask(kTrap15FileCloseAll);
}

TEST_F(Trap15Test, Task51_FileOpen_WritesFID) {
  SetStringA1("test.txt");
  EXPECT_CALL(file_io_, OpenFile(_, _, _)).WillOnce(SetArgPointee<0>(3L));
  RunTask(kTrap15FileOpen);
  EXPECT_EQ(sim_->State().d[1], 3u);
}

TEST_F(Trap15Test, Task52_FileNew_WritesFID) {
  SetStringA1("new.txt");
  EXPECT_CALL(file_io_, NewFile(_, _, _)).WillOnce(SetArgPointee<0>(4L));
  RunTask(kTrap15FileNew);
  EXPECT_EQ(sim_->State().d[1], 4u);
}

TEST_F(Trap15Test, Task53_FileRead_UpdatesD2L) {
  sim_->State().a[1] = 0x2000;
  sim_->State().d[1] = 2;    // FID
  sim_->State().d[2] = 100;  // request 100 bytes
  EXPECT_CALL(file_io_, ReadFile(2L, _, _, _)).WillOnce(SetArgPointee<2>(80u));
  RunTask(kTrap15FileRead);
  EXPECT_EQ(sim_->State().d[2], 80u);
}

TEST_F(Trap15Test, Task54_FileWrite) {
  sim_->State().a[1] = 0x2000;
  sim_->State().d[1] = 2;   // FID
  sim_->State().d[2] = 10;  // 10 bytes
  EXPECT_CALL(file_io_, WriteFile(2L, _, 10u, _)).Times(1);
  RunTask(kTrap15FileWrite);
}

TEST_F(Trap15Test, Task55_FilePosition) {
  sim_->State().d[1] = 2;    // FID
  sim_->State().d[2] = 512;  // position
  EXPECT_CALL(file_io_, PositionFile(2L, 512, _)).Times(1);
  RunTask(kTrap15FilePosition);
}

TEST_F(Trap15Test, Task56_FileClose) {
  sim_->State().d[1] = 3;  // FID
  EXPECT_CALL(file_io_, CloseFile(3L, _)).Times(1);
  RunTask(kTrap15FileClose);
}

TEST_F(Trap15Test, Task57_FileDelete) {
  SetStringA1("old.txt");
  EXPECT_CALL(file_io_, DeleteFile(_, _)).Times(1);
  RunTask(kTrap15FileDelete);
}

TEST_F(Trap15Test, Task58_FileDialog) {
  sim_->State().d[1] = 0;
  sim_->State().a[1] = 0x2100;
  sim_->State().a[2] = 0x2200;
  sim_->State().a[3] = 0x2300;
  EXPECT_CALL(file_io_, DisplayFileDialog(_, _, _, _, _)).Times(1);
  RunTask(kTrap15FileDialog);
}

TEST_F(Trap15Test, Task59_FileOp) {
  SetStringA1("check.txt");
  sim_->State().d[1] = 0;
  EXPECT_CALL(file_io_, FileOp(_, _, _)).Times(1);
  RunTask(kTrap15FileOp);
}

// ===========================================================================
// PERIPHERAL I/O
// ===========================================================================

TEST_F(Trap15Test, Task60_MouseIRQ_Enable) {
  sim_->State().d[1] = (1u << 8) | 0x07u;  // IRQ level 1, all events
  EXPECT_CALL(peripheral_io_, SetMouseIRQ(1, 7)).Times(1);
  RunTask(kTrap15MouseIRQ);
}

TEST_F(Trap15Test, Task60_MouseIRQ_InvalidLevel_NoCall) {
  sim_->State().d[1] = (8u << 8) | 0x07u;  // IRQ level 8, invalid
  EXPECT_CALL(peripheral_io_, SetMouseIRQ(_, _)).Times(0);
  RunTask(kTrap15MouseIRQ);
}

TEST_F(Trap15Test, Task61_MouseRead_WritesD0AndD1) {
  sim_->State().d[1] = 0;  // current state
  EXPECT_CALL(peripheral_io_, ReadMouse(0, _, _))
      .WillOnce(
          DoAll(SetArgPointee<1>(3L), SetArgPointee<2>(static_cast<long>((100 << 16) | 200))));
  RunTask(kTrap15MouseRead);
  EXPECT_EQ(sim_->State().d[0], 3u);
  EXPECT_EQ(sim_->State().d[1], static_cast<uint32_t>((100 << 16) | 200));
}

TEST_F(Trap15Test, Task62_KeyIRQ_Enable) {
  sim_->State().d[1] = (2u << 8) | 0x03u;  // IRQ level 2, up+down
  EXPECT_CALL(peripheral_io_, SetKeyIRQ(2, 3)).Times(1);
  RunTask(kTrap15KeyIRQ);
}

// ===========================================================================
// SOUND
// ===========================================================================

TEST_F(Trap15Test, Task70_SoundPlay) {
  SetStringA1("beep.wav");
  EXPECT_CALL(sound_io_, PlaySound(_, _)).Times(1);
  RunTask(kTrap15SoundPlay);
}

TEST_F(Trap15Test, Task71_SoundLoad_UsesD1B) {
  SetStringA1("beep.wav");
  sim_->State().d[1] = 5;  // index 5
  EXPECT_CALL(sound_io_, LoadSound(_, 5)).Times(1);
  RunTask(kTrap15SoundLoad);
}

TEST_F(Trap15Test, Task72_SoundPlayMem_UsesD1B) {
  sim_->State().d[1] = 3;
  EXPECT_CALL(sound_io_, PlaySoundMem(3, _)).Times(1);
  RunTask(kTrap15SoundPlayMem);
}

TEST_F(Trap15Test, Task73_SoundPlayDX) {
  SetStringA1("music.wav");
  EXPECT_CALL(sound_io_, PlaySoundMulti(_, _)).Times(1);
  RunTask(kTrap15SoundPlayDX);
}

TEST_F(Trap15Test, Task74_SoundLoadDX_UsesD1B) {
  SetStringA1("music.wav");
  sim_->State().d[1] = 2;
  EXPECT_CALL(sound_io_, LoadSoundMulti(_, 2, _)).Times(1);
  RunTask(kTrap15SoundLoadDX);
}

TEST_F(Trap15Test, Task75_SoundPlayMemDX_UsesD1B) {
  sim_->State().d[1] = 7;
  EXPECT_CALL(sound_io_, PlaySoundMemMulti(7, _)).Times(1);
  RunTask(kTrap15SoundPlayMemDX);
}

TEST_F(Trap15Test, Task76_SoundControl_PassesD2AndD1B) {
  sim_->State().d[1] = 4;  // index
  sim_->State().d[2] = 2;  // stop command
  EXPECT_CALL(sound_io_, ControlSound(2, 4, _)).Times(1);
  RunTask(kTrap15SoundControl);
}

TEST_F(Trap15Test, Task77_SoundControlDX_PassesD2AndD1B) {
  sim_->State().d[1] = 1;  // index
  sim_->State().d[2] = 3;  // stop all
  EXPECT_CALL(sound_io_, ControlSoundMulti(3, 1, _)).Times(1);
  RunTask(kTrap15SoundControlDX);
}

// ===========================================================================
// GRAPHICS
// ===========================================================================

TEST_F(Trap15Test, Task80_SetPenColor) {
  sim_->State().d[1] = 0x00FF0000u;
  EXPECT_CALL(graphics_io_, SetPenColor(0x00FF0000u)).Times(1);
  RunTask(kTrap15SetPenColor);
}

TEST_F(Trap15Test, Task81_SetFillColor) {
  sim_->State().d[1] = 0x0000FF00u;
  EXPECT_CALL(graphics_io_, SetFillColor(0x0000FF00u)).Times(1);
  RunTask(kTrap15SetFillColor);
}

TEST_F(Trap15Test, Task82_DrawPixel) {
  sim_->State().d[1] = 100;
  sim_->State().d[2] = 200;
  EXPECT_CALL(graphics_io_, DrawPixel(100, 200)).Times(1);
  RunTask(kTrap15DrawPixel);
}

TEST_F(Trap15Test, Task83_GetPixel_WritesD0L) {
  sim_->State().d[1] = 50;
  sim_->State().d[2] = 75;
  EXPECT_CALL(graphics_io_, GetPixel(50, 75)).WillOnce(Return(0x00AABBCC));
  RunTask(kTrap15GetPixel);
  EXPECT_EQ(sim_->State().d[0], static_cast<uint32_t>(0x00AABBCC));
}

TEST_F(Trap15Test, Task84_DrawLine) {
  sim_->State().d[1] = 10;
  sim_->State().d[2] = 20;
  sim_->State().d[3] = 100;
  sim_->State().d[4] = 200;
  EXPECT_CALL(graphics_io_, DrawLine(10, 20, 100, 200)).Times(1);
  RunTask(kTrap15DrawLine);
}

TEST_F(Trap15Test, Task85_LineTo) {
  sim_->State().d[1] = 300;
  sim_->State().d[2] = 400;
  EXPECT_CALL(graphics_io_, LineTo(300, 400)).Times(1);
  RunTask(kTrap15LineTo);
}

TEST_F(Trap15Test, Task86_MoveTo) {
  sim_->State().d[1] = 50;
  sim_->State().d[2] = 60;
  EXPECT_CALL(graphics_io_, MoveTo(50, 60)).Times(1);
  RunTask(kTrap15MoveTo);
}

TEST_F(Trap15Test, Task87_DrawRectangle) {
  sim_->State().d[1] = 10;
  sim_->State().d[2] = 20;
  sim_->State().d[3] = 100;
  sim_->State().d[4] = 200;
  EXPECT_CALL(graphics_io_, DrawRectangle(10, 20, 100, 200)).Times(1);
  RunTask(kTrap15DrawRectangle);
}

TEST_F(Trap15Test, Task88_DrawEllipse) {
  sim_->State().d[1] = 0;
  sim_->State().d[2] = 0;
  sim_->State().d[3] = 100;
  sim_->State().d[4] = 100;
  EXPECT_CALL(graphics_io_, DrawEllipse(0, 0, 100, 100)).Times(1);
  RunTask(kTrap15DrawEllipse);
}

TEST_F(Trap15Test, Task89_FloodFill) {
  sim_->State().d[1] = 150;
  sim_->State().d[2] = 250;
  EXPECT_CALL(graphics_io_, FloodFill(150, 250)).Times(1);
  RunTask(kTrap15FloodFill);
}

TEST_F(Trap15Test, Task90_DrawUnfilledRectangle) {
  sim_->State().d[1] = 5;
  sim_->State().d[2] = 5;
  sim_->State().d[3] = 50;
  sim_->State().d[4] = 50;
  EXPECT_CALL(graphics_io_, DrawUnfilledRectangle(5, 5, 50, 50)).Times(1);
  RunTask(kTrap15DrawUnfilledRectangle);
}

TEST_F(Trap15Test, Task91_DrawUnfilledEllipse) {
  sim_->State().d[1] = 0;
  sim_->State().d[2] = 0;
  sim_->State().d[3] = 80;
  sim_->State().d[4] = 40;
  EXPECT_CALL(graphics_io_, DrawUnfilledEllipse(0, 0, 80, 40)).Times(1);
  RunTask(kTrap15DrawUnfilledEllipse);
}

TEST_F(Trap15Test, Task92_SetDrawingMode) {
  sim_->State().d[1] = 4;
  EXPECT_CALL(graphics_io_, SetDrawingMode(4)).Times(1);
  RunTask(kTrap15SetDrawingMode);
}

TEST_F(Trap15Test, Task93_SetPenWidth) {
  sim_->State().d[1] = 3;
  EXPECT_CALL(graphics_io_, SetPenWidth(3)).Times(1);
  RunTask(kTrap15SetPenWidth);
}

TEST_F(Trap15Test, Task94_Repaint) {
  EXPECT_CALL(graphics_io_, Repaint()).Times(1);
  RunTask(kTrap15Repaint);
}

TEST_F(Trap15Test, Task95_DrawText) {
  SetStringA1("Hello!");
  sim_->State().d[1] = 10;
  sim_->State().d[2] = 20;
  EXPECT_CALL(graphics_io_, DrawText(_, 10, 20)).Times(1);
  RunTask(kTrap15DrawText);
}

TEST_F(Trap15Test, Task96_GetXY_WritesD1WAndD2W) {
  EXPECT_CALL(graphics_io_, GetXY(_, _))
      .WillOnce(DoAll(SetArgPointee<0>(320), SetArgPointee<1>(240)));
  RunTask(kTrap15GetXY);
  EXPECT_EQ(static_cast<uint16_t>(sim_->State().d[1]), 320u);
  EXPECT_EQ(static_cast<uint16_t>(sim_->State().d[2]), 240u);
}

// ===========================================================================
// NETWORK I/O
// ===========================================================================

TEST_F(Trap15Test, Task100_NetCreateClient) {
  sim_->State().d[1] = 0;
  sim_->State().a[2] = 0x2000;
  const char* ip = "127.0.0.1";
  for (size_t i = 0; i <= __builtin_strlen(ip); ++i)
    sim_->GetMemory().WriteByte(0x2000 + i, static_cast<uint8_t>(ip[i]));
  EXPECT_CALL(network_io_, CreateClient(0, _, _)).Times(1);
  RunTask(kTrap15NetCreateClient);
}

TEST_F(Trap15Test, Task101_NetCreateServer) {
  sim_->State().d[1] = (8080u << 16);
  EXPECT_CALL(network_io_, CreateServer(_, _)).Times(1);
  RunTask(kTrap15NetCreateServer);
}

TEST_F(Trap15Test, Task102_NetSend_UpdatesD1L) {
  sim_->State().a[1] = 0x2000;
  sim_->State().a[2] = 0x2100;
  sim_->State().d[1] = 5;
  EXPECT_CALL(network_io_, Send(_, _, _, _, _)).WillOnce(SetArgPointee<3>(5));
  RunTask(kTrap15NetSend);
  EXPECT_EQ(sim_->State().d[1], 5u);
}

TEST_F(Trap15Test, Task103_NetReceive_UpdatesD1L) {
  sim_->State().a[1] = 0x2000;
  sim_->State().a[2] = 0x2100;
  sim_->State().d[1] = 64;
  EXPECT_CALL(network_io_, Receive(_, _, _, _, _)).WillOnce(SetArgPointee<2>(32));
  RunTask(kTrap15NetReceive);
  EXPECT_EQ(sim_->State().d[1], 32u);
}

TEST_F(Trap15Test, Task104_NetClose) {
  sim_->State().d[1] = 0;  // close all
  EXPECT_CALL(network_io_, CloseConnection(0, _)).Times(1);
  RunTask(kTrap15NetClose);
}

TEST_F(Trap15Test, Task105_NetGetLocalIP) {
  sim_->State().a[2] = 0x2000;
  EXPECT_CALL(network_io_, GetLocalIP(_, _)).Times(1);
  RunTask(kTrap15NetGetLocalIP);
}

TEST_F(Trap15Test, Task106_NetSendOnPort) {
  sim_->State().a[1] = 0x2000;
  sim_->State().a[2] = 0x2100;
  EXPECT_CALL(network_io_, SendOnPort(_, _, _, _)).Times(1);
  RunTask(kTrap15NetSendOnPort);
}

TEST_F(Trap15Test, Task107_NetReceiveOnPort) {
  sim_->State().a[1] = 0x2000;
  sim_->State().a[2] = 0x2100;
  EXPECT_CALL(network_io_, ReceiveOnPort(_, _, _, _)).Times(1);
  RunTask(kTrap15NetReceiveOnPort);
}

// ===========================================================================
// EDGE CASES
// ===========================================================================

TEST_F(Trap15Test, NullTextIO_Task0_IsNoOp) {
  M68kSimulator::Config cfg;  // all nulls
  M68kSimulator sim_null{std::move(cfg)};
  sim_null.GetMemory().WriteLong(0x0, 0x3FFE);
  sim_null.GetMemory().WriteLong(0x4, 0x1000);
  sim_null.Reset();
  sim_null.State().sr = kSrSupervisor;
  sim_null.GetMemory().WriteWord(0x1000, 0x4E4F);
  sim_null.State().d[0] = kTrap15DisplayStringCRLF;
  EXPECT_EQ(sim_null.Step(), SimResult::kOk);
}

TEST_F(Trap15Test, UnknownTask_LogsWarning) {
  EXPECT_CALL(logger_, Log(_)).Times(1);
  RunTask(200);
  EXPECT_EQ(sim_->State().pc, 0x1002u);
}

TEST_F(Trap15Test, UnknownTask_ReturnsOk) {
  EXPECT_EQ(RunTask(250), SimResult::kOk);
}

}  // namespace
}  // namespace easym68k::sim
