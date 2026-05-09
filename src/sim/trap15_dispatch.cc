// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Trap #15 I/O dispatch — DispatchTrap15().
// Ported from EASy68K/Sim68K/CODE9.CPP TRAP() function.
//
// Register conventions (per CODE9.CPP):
//   D0.B  — task number (consumed; result code written back for many tasks)
//   D1, D2, D3, D4 — parameters and return values, task-specific
//   A1, A2, A3 — memory buffer addresses, task-specific

#include <cstdint>
#include <cstdio>
#include <cstring>

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

// Copy at most max_len-1 bytes from 68000 memory at addr into buf, null-terminate.
void ReadStrFromMem(const uint8_t* raw, uint32_t addr, char* buf, size_t max_len) {
  if (max_len == 0)
    return;
  const uint8_t* src = raw + (addr & kAddressMask);
  size_t i = 0;
  while (i < max_len - 1 && src[i]) {
    buf[i] = static_cast<char>(src[i]);
    ++i;
  }
  buf[i] = '\0';
}

// Write value in any base 2-36 into buf (uppercase, unsigned).
void ToBase(uint32_t val, int base, char* buf, size_t len) {
  static const char kDigits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  char tmp[64];
  int i = 0;
  if (val == 0) {
    tmp[i++] = '0';
  } else {
    while (val && i < 63) {
      tmp[i++] = kDigits[val % static_cast<uint32_t>(base)];
      val /= static_cast<uint32_t>(base);
    }
  }
  size_t j = 0;
  while (i-- > 0 && j < len - 1)
    buf[j++] = tmp[i];
  buf[j] = '\0';
}

}  // namespace

// ---------------------------------------------------------------------------
// DispatchTrap15 — map D0.B task number to interface method call.
// Returns kHalted for task 9 (terminate). Returns kOk for all others.
// Null interface pointer → task is a silent no-op.
// ---------------------------------------------------------------------------
SimResult M68kSimulator::DispatchTrap15() {
  uint8_t task = static_cast<uint8_t>(state_.d[0] & 0xFF);
  uint8_t* raw = memory_.RawData();
  char buf[256];

  switch (task) {
      // ---- TEXT I/O --------------------------------------------------------

    case kTrap15DisplayStringCRLF:  // 0: display D1.W chars at (A1) with CRLF
    case kTrap15DisplayString: {    // 1: display D1.W chars at (A1) without CRLF
      if (!config_.text_io)
        break;
      int16_t len = static_cast<int16_t>(state_.d[1]);
      size_t copy_len = (len <= 0) ? 0 : (len > 255 ? 255 : static_cast<size_t>(len));
      std::memcpy(buf, raw + (state_.a[1] & kAddressMask), copy_len);
      buf[copy_len] = '\0';
      if (task == kTrap15DisplayStringCRLF)
        config_.text_io->TextOutCRLF(buf);
      else
        config_.text_io->TextOut(buf);
      break;
    }

    case kTrap15InputString: {  // 2: read string into (A1), length in D1.W
      if (!config_.text_io)
        break;
      char* dst = reinterpret_cast<char*>(raw + (state_.a[1] & kAddressMask));
      long size = static_cast<long>(state_.d[1]);
      config_.text_io->TextIn(dst, &size, nullptr);
      state_.d[1] = static_cast<uint32_t>(size);
      break;
    }

    case kTrap15DisplayNumber: {  // 3: display D1.L as signed decimal
      if (!config_.text_io)
        break;
      std::snprintf(buf, sizeof(buf), "%d", static_cast<int32_t>(state_.d[1]));
      config_.text_io->TextOut(buf);
      break;
    }

    case kTrap15InputNumber: {  // 4: read decimal number into D1.L
      if (!config_.text_io)
        break;
      long result = 0;
      long size = 0;
      config_.text_io->TextIn(buf, &size, &result);
      state_.d[1] = static_cast<uint32_t>(result);
      break;
    }

    case kTrap15InputChar: {  // 5: read single char into D1.B
      if (!config_.text_io)
        break;
      char ch = 0;
      config_.text_io->CharIn(&ch);
      state_.d[1] = (state_.d[1] & 0xFFFFFF00u) | static_cast<uint8_t>(ch);
      break;
    }

    case kTrap15DisplayChar:  // 6: display char in D1.B
      if (!config_.text_io)
        break;
      config_.text_io->CharOut(static_cast<char>(state_.d[1] & 0xFF));
      break;

    case kTrap15KeyPending: {  // 7: D1.B = 1 if key pending, else 0
      bool pending = config_.text_io && config_.text_io->IsKeyPending();
      state_.d[1] = (state_.d[1] & 0xFFFFFF00u) | (pending ? 1u : 0u);
      break;
    }

    case kTrap15GetTime:  // 8: D1.L = hundredths of second since midnight
      if (!config_.sim_env)
        break;
      state_.d[1] = config_.sim_env->GetTimeHundredths();
      break;

    case kTrap15Terminate:  // 9: terminate program
      return SimResult::kHalted;

    case kTrap15PrintChar: {  // 10: print null-terminated string at (A1)
      if (!config_.print_io)
        break;
      const char* src = reinterpret_cast<const char*>(raw + (state_.a[1] & kAddressMask));
      while (*src)
        config_.print_io->PrintChar(*src++);
      break;
    }

    case kTrap15CursorControl: {  // 11: cursor position / clear screen
      if (!config_.text_io)
        break;
      uint16_t d1w = static_cast<uint16_t>(state_.d[1]);
      if (d1w == 0xFF00u) {
        config_.text_io->ClearScreen();
      } else if (d1w == 0x00FFu) {
        int col = 0, row = 0;
        config_.text_io->GetCursorPosition(&col, &row);
        state_.d[1] = (state_.d[1] & 0xFFFF0000u) | (static_cast<uint16_t>(col) << 8) |
                      static_cast<uint16_t>(row);
      } else {
        config_.text_io->SetCursorPosition(static_cast<int>(static_cast<uint8_t>(d1w >> 8)),
                                           static_cast<int>(static_cast<uint8_t>(d1w)));
      }
      break;
    }

    case kTrap15KeyboardEcho:  // 12: D1.B 0=echo off, else on
      if (!config_.text_io)
        break;
      config_.text_io->SetKeyboardEcho((state_.d[1] & 0xFF) != 0);
      break;

    case kTrap15DisplayNullStringCRLF:  // 13: display null-terminated at (A1) with CRLF
    case kTrap15DisplayNullString: {    // 14: display null-terminated at (A1) without CRLF
      if (!config_.text_io)
        break;
      ReadStrFromMem(raw, state_.a[1], buf, sizeof(buf));
      if (task == kTrap15DisplayNullStringCRLF)
        config_.text_io->TextOutCRLF(buf);
      else
        config_.text_io->TextOut(buf);
      break;
    }

    case kTrap15DisplayRadix: {  // 15: display D1.L in base D2.B (2-36)
      if (!config_.text_io)
        break;
      int base = static_cast<int8_t>(state_.d[2]);
      if (base >= 2 && base <= 36) {
        ToBase(state_.d[1], base, buf, sizeof(buf));
        config_.text_io->TextOut(buf);
      }
      break;
    }

    case kTrap15InputControl:  // 16: input prompt / LF display control
      if (!config_.text_io)
        break;
      switch (static_cast<int8_t>(state_.d[1])) {
        case 0:
          config_.text_io->SetInputPrompt(false);
          break;
        case 1:
          config_.text_io->SetInputPrompt(true);
          break;
        case 2:
          config_.text_io->SetInputLFDisplay(false);
          break;
        case 3:
          config_.text_io->SetInputLFDisplay(true);
          break;
        default:
          break;
      }
      break;

    case kTrap15DisplayNullStringNumber: {  // 17: display null string at (A1) then D1.L decimal
      if (!config_.text_io)
        break;
      ReadStrFromMem(raw, state_.a[1], buf, sizeof(buf));
      config_.text_io->TextOut(buf);
      char num[32];
      std::snprintf(num, sizeof(num), "%d", static_cast<int32_t>(state_.d[1]));
      config_.text_io->TextOut(num);
      break;
    }

    case kTrap15DisplayNullStringInputNumber: {  // 18: display null string then read number
      if (!config_.text_io)
        break;
      ReadStrFromMem(raw, state_.a[1], buf, sizeof(buf));
      config_.text_io->TextOut(buf);
      long result = 0;
      long size = 0;
      config_.text_io->TextIn(buf, &size, &result);
      state_.d[1] = static_cast<uint32_t>(result);
      break;
    }

    case kTrap15GetKeyState: {  // 19: get keypress state for up to 4 keys in D1.L
      if (!config_.text_io)
        break;
      long state = static_cast<long>(state_.d[1]);
      config_.text_io->GetKeyState(&state);
      state_.d[1] = static_cast<uint32_t>(state);
      break;
    }

    case kTrap15DisplaySignedField: {  // 20: display D1.L in field D2.B columns wide
      if (!config_.text_io)
        break;
      std::snprintf(buf, sizeof(buf), "%*d", static_cast<int>(static_cast<int8_t>(state_.d[2])),
                    static_cast<int32_t>(state_.d[1]));
      config_.text_io->TextOut(buf);
      break;
    }

    case kTrap15SetFontProperties:  // 21: D1.L=color 0x00BBGGRR, D2.L=style/size/font
      if (!config_.text_io)
        break;
      config_.text_io->SetFontProperties(state_.d[1], state_.d[2]);
      break;

    case kTrap15GetCharAt: {  // 22: get char at row,col; D1.L high=row, low=col
      if (!config_.text_io)
        break;
      int row = static_cast<int>(state_.d[1] >> 16);
      int col = static_cast<int>(state_.d[1] & 0xFFFF);
      char ch = 0;
      config_.text_io->GetCharAt(row, col, &ch);
      state_.d[1] = (state_.d[1] & 0xFFFFFF00u) | static_cast<uint8_t>(ch);
      break;
    }

    case kTrap15Delay:  // 23: delay D1.W / 100 seconds
      if (!config_.sim_env)
        break;
      config_.sim_env->DelayHundredths(static_cast<uint16_t>(state_.d[1]));
      break;

    case kTrap15KeyCommandControl:  // 24: D1=0 enable, D1=1 disable key commands
      if (!config_.text_io)
        break;
      config_.text_io->SetKeyCommandsEnabled(state_.d[1] == 0);
      break;

    case kTrap15ScrollRect:  // 25: scroll D1.L(row,col), D2.L(h,w), D3.W dir
      if (!config_.text_io)
        break;
      config_.text_io->ScrollRect(
          static_cast<int>(state_.d[1] >> 16), static_cast<int>(state_.d[1] & 0xFFFF),
          static_cast<int>(state_.d[2] >> 16), static_cast<int>(state_.d[2] & 0xFFFF),
          static_cast<int>(static_cast<uint16_t>(state_.d[3])));
      break;

      // ---- SIMULATOR ENVIRONMENT -------------------------------------------

    case kTrap15ClearCycles:  // 30: clear cycle counter
      if (!config_.sim_env)
        break;
      config_.sim_env->ClearCycles();
      break;

    case kTrap15GetCycles:  // 31: D1.L = cycle counter (0 if overflow)
      if (!config_.sim_env)
        break;
      state_.d[1] = config_.sim_env->GetCycles();
      break;

    case kTrap15Hardware:  // 32: hardware subtasks via D1.B
      if (!config_.sim_env)
        break;
      switch (static_cast<uint8_t>(state_.d[1])) {
        case 0:
          config_.sim_env->ShowHardware();
          break;
        case 1:
          state_.d[1] = config_.sim_env->GetSeg7Address();
          break;
        case 2:
          state_.d[1] = config_.sim_env->GetLEDAddress();
          break;
        case 3:
          state_.d[1] = config_.sim_env->GetSwitchAddress();
          break;
        case 4:
          state_.d[1] = config_.sim_env->GetVersion();
          break;
        case 5:
          config_.sim_env->SetExceptionsEnabled(true);
          break;
        case 6:
          config_.sim_env->SetAutoIRQ(static_cast<uint8_t>(state_.d[2]), state_.d[3]);
          break;
        case 7:
          state_.d[1] = config_.sim_env->GetPushButtonAddress();
          break;
        default:
          break;
      }
      break;

    case kTrap15WindowControl: {  // 33: get/set output window size
      if (!config_.sim_env)
        break;
      uint32_t d1 = state_.d[1];
      if (d1 == 0) {
        uint16_t w = 0, h = 0;
        config_.sim_env->GetWindowSize(&w, &h);
        state_.d[1] = (static_cast<uint32_t>(w) << 16) | h;
      } else if (d1 == 1 || d1 == 2) {
        config_.sim_env->SetupWindow(d1 == 2);  // 1=windowed, 2=fullscreen
      } else {
        config_.sim_env->SetWindowSize(static_cast<uint16_t>(d1 >> 16), static_cast<uint16_t>(d1));
      }
      break;
    }

      // ---- SERIAL I/O ------------------------------------------------------

    case kTrap15SerialInit: {  // 40: init COM port; CID in D0[31:16], name at (A1)
      if (!config_.serial_io)
        break;
      int cid = static_cast<int>(state_.d[0] >> 16);
      ReadStrFromMem(raw, state_.a[1], buf, sizeof(buf));
      short result = 0;
      config_.serial_io->InitPort(cid, buf, &result);
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

    case kTrap15SerialSetParams: {  // 41: set port params; CID in D0[31:16], params in D1
      if (!config_.serial_io)
        break;
      int cid = static_cast<int>(state_.d[0] >> 16);
      short result = 0;
      config_.serial_io->SetPortParams(cid, static_cast<int>(state_.d[1]), &result);
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

    case kTrap15SerialRead: {  // 42: read D1.B chars from CID into (A1)
      if (!config_.serial_io)
        break;
      int cid = static_cast<int>(state_.d[0] >> 16);
      char* dst = reinterpret_cast<char*>(raw + (state_.a[1] & kAddressMask));
      unsigned char count = static_cast<uint8_t>(state_.d[1]);
      short result = 0;
      config_.serial_io->ReadString(cid, &count, dst, &result);
      state_.d[1] = (state_.d[1] & 0xFFFFFF00u) | count;
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

    case kTrap15SerialSend: {  // 43: send D1.B chars from (A1) via CID
      if (!config_.serial_io)
        break;
      int cid = static_cast<int>(state_.d[0] >> 16);
      const char* src = reinterpret_cast<const char*>(raw + (state_.a[1] & kAddressMask));
      unsigned char count = static_cast<uint8_t>(state_.d[1]);
      short result = 0;
      config_.serial_io->SendString(cid, &count, src, &result);
      state_.d[1] = (state_.d[1] & 0xFFFFFF00u) | count;
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

      // ---- FILE I/O --------------------------------------------------------

    case kTrap15FileCloseAll: {  // 50: close all open files
      if (!config_.file_io)
        break;
      short result = 0;
      config_.file_io->CloseAll(&result);
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

    case kTrap15FileOpen: {  // 51: open existing file at (A1), FID in D1.L
      if (!config_.file_io)
        break;
      ReadStrFromMem(raw, state_.a[1], buf, sizeof(buf));
      long fid = 0;
      short result = 0;
      config_.file_io->OpenFile(&fid, buf, &result);
      state_.d[1] = static_cast<uint32_t>(fid);
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

    case kTrap15FileNew: {  // 52: create/open file at (A1), FID in D1.L
      if (!config_.file_io)
        break;
      ReadStrFromMem(raw, state_.a[1], buf, sizeof(buf));
      long fid = 0;
      short result = 0;
      config_.file_io->NewFile(&fid, buf, &result);
      state_.d[1] = static_cast<uint32_t>(fid);
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

    case kTrap15FileRead: {  // 53: read D2.L bytes from FID=D1 into (A1)
      if (!config_.file_io)
        break;
      char* dst = reinterpret_cast<char*>(raw + (state_.a[1] & kAddressMask));
      unsigned int size = state_.d[2];
      short result = 0;
      config_.file_io->ReadFile(static_cast<long>(state_.d[1]), dst, &size, &result);
      state_.d[2] = size;
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

    case kTrap15FileWrite: {  // 54: write D2.L bytes from (A1) to FID=D1
      if (!config_.file_io)
        break;
      const char* src = reinterpret_cast<const char*>(raw + (state_.a[1] & kAddressMask));
      short result = 0;
      config_.file_io->WriteFile(static_cast<long>(state_.d[1]), src, state_.d[2], &result);
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

    case kTrap15FilePosition: {  // 55: seek FID=D1 to position D2
      if (!config_.file_io)
        break;
      short result = 0;
      config_.file_io->PositionFile(static_cast<long>(state_.d[1]), static_cast<int>(state_.d[2]),
                                    &result);
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

    case kTrap15FileClose: {  // 56: close FID=D1
      if (!config_.file_io)
        break;
      short result = 0;
      config_.file_io->CloseFile(static_cast<long>(state_.d[1]), &result);
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

    case kTrap15FileDelete: {  // 57: delete file at (A1)
      if (!config_.file_io)
        break;
      ReadStrFromMem(raw, state_.a[1], buf, sizeof(buf));
      short result = 0;
      config_.file_io->DeleteFile(buf, &result);
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

    case kTrap15FileDialog: {  // 58: file dialog; D1=mode, A1=title, A2=ext, A3=path buf
      if (!config_.file_io)
        break;
      short result = 0;
      config_.file_io->DisplayFileDialog(
          reinterpret_cast<long*>(&state_.d[1]), static_cast<long>(state_.a[1]),
          static_cast<long>(state_.a[2]), static_cast<long>(state_.a[3]), &result);
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

    case kTrap15FileOp: {  // 59: file operation; D1=op, (A1)=path
      if (!config_.file_io)
        break;
      ReadStrFromMem(raw, state_.a[1], buf, sizeof(buf));
      short result = 0;
      config_.file_io->FileOp(reinterpret_cast<long*>(&state_.d[1]), buf, &result);
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

      // ---- PERIPHERAL I/O --------------------------------------------------

    case kTrap15MouseIRQ: {  // 60: enable/disable mouse IRQ
      if (!config_.peripheral_io)
        break;
      uint8_t irq = static_cast<uint8_t>(state_.d[1] >> 8);
      if (irq <= 7)
        config_.peripheral_io->SetMouseIRQ(irq, static_cast<uint8_t>(state_.d[1]));
      break;
    }

    case kTrap15MouseRead: {  // 61: read mouse; D1.B=mode → D0=buttons, D1=Y<<16|X
      if (!config_.peripheral_io)
        break;
      long buttons = 0, pos = 0;
      config_.peripheral_io->ReadMouse(static_cast<int>(static_cast<uint8_t>(state_.d[1])),
                                       &buttons, &pos);
      state_.d[0] = static_cast<uint32_t>(buttons);
      state_.d[1] = static_cast<uint32_t>(pos);
      break;
    }

    case kTrap15KeyIRQ: {  // 62: enable/disable keyboard IRQ
      if (!config_.peripheral_io)
        break;
      uint8_t irq = static_cast<uint8_t>(state_.d[1] >> 8);
      if (irq <= 7)
        config_.peripheral_io->SetKeyIRQ(irq, static_cast<uint8_t>(state_.d[1]));
      break;
    }

      // ---- SOUND -----------------------------------------------------------

    case kTrap15SoundPlay: {  // 70: play WAV file at (A1)
      if (!config_.sound_io)
        break;
      ReadStrFromMem(raw, state_.a[1], buf, sizeof(buf));
      short result = 0;
      config_.sound_io->PlaySound(buf, &result);
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

    case kTrap15SoundLoad: {  // 71: load WAV at (A1) as index D1.B
      if (!config_.sound_io)
        break;
      ReadStrFromMem(raw, state_.a[1], buf, sizeof(buf));
      config_.sound_io->LoadSound(buf, static_cast<int>(static_cast<uint8_t>(state_.d[1])));
      break;
    }

    case kTrap15SoundPlayMem: {  // 72: play from memory index D1.B (standard player)
      if (!config_.sound_io)
        break;
      short result = 0;
      config_.sound_io->PlaySoundMem(static_cast<int>(static_cast<uint8_t>(state_.d[1])), &result);
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

    case kTrap15SoundPlayDX: {  // 73: play WAV at (A1) via multi-voice player
      if (!config_.sound_io)
        break;
      ReadStrFromMem(raw, state_.a[1], buf, sizeof(buf));
      short result = 0;
      config_.sound_io->PlaySoundMulti(buf, &result);
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

    case kTrap15SoundLoadDX: {  // 74: load WAV at (A1) into multi-voice index D1.B
      if (!config_.sound_io)
        break;
      ReadStrFromMem(raw, state_.a[1], buf, sizeof(buf));
      short result = 0;
      config_.sound_io->LoadSoundMulti(buf, static_cast<int>(static_cast<uint8_t>(state_.d[1])),
                                       &result);
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

    case kTrap15SoundPlayMemDX: {  // 75: play multi-voice memory index D1.B
      if (!config_.sound_io)
        break;
      short result = 0;
      config_.sound_io->PlaySoundMemMulti(static_cast<int>(static_cast<uint8_t>(state_.d[1])),
                                          &result);
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

    case kTrap15SoundControl: {  // 76: control standard player; D2.L=cmd, D1.B=index
      if (!config_.sound_io)
        break;
      short result = 0;
      config_.sound_io->ControlSound(static_cast<int>(state_.d[2]),
                                     static_cast<int>(static_cast<uint8_t>(state_.d[1])), &result);
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

    case kTrap15SoundControlDX: {  // 77: control multi-voice player; D2.L=cmd, D1.B=index
      if (!config_.sound_io)
        break;
      short result = 0;
      config_.sound_io->ControlSoundMulti(static_cast<int>(state_.d[2]),
                                          static_cast<int>(static_cast<uint8_t>(state_.d[1])),
                                          &result);
      state_.d[0] = (state_.d[0] & 0xFFFF0000u) | static_cast<uint16_t>(result);
      break;
    }

      // ---- GRAPHICS --------------------------------------------------------

    case kTrap15SetPenColor:  // 80: pen color D1.L as 0x00BBGGRR
      if (!config_.graphics_io)
        break;
      config_.graphics_io->SetPenColor(state_.d[1]);
      break;

    case kTrap15SetFillColor:  // 81: fill color D1.L as 0x00BBGGRR
      if (!config_.graphics_io)
        break;
      config_.graphics_io->SetFillColor(state_.d[1]);
      break;

    case kTrap15DrawPixel:  // 82: draw pixel at D1.W, D2.W
      if (!config_.graphics_io)
        break;
      config_.graphics_io->DrawPixel(static_cast<int16_t>(state_.d[1]),
                                     static_cast<int16_t>(state_.d[2]));
      break;

    case kTrap15GetPixel:  // 83: get pixel color at D1.W, D2.W → D0.L
      if (!config_.graphics_io)
        break;
      state_.d[0] = static_cast<uint32_t>(config_.graphics_io->GetPixel(
          static_cast<int16_t>(state_.d[1]), static_cast<int16_t>(state_.d[2])));
      break;

    case kTrap15DrawLine:  // 84: line D1.W,D2.W → D3.W,D4.W
      if (!config_.graphics_io)
        break;
      config_.graphics_io->DrawLine(
          static_cast<int16_t>(state_.d[1]), static_cast<int16_t>(state_.d[2]),
          static_cast<int16_t>(state_.d[3]), static_cast<int16_t>(state_.d[4]));
      break;

    case kTrap15LineTo:  // 85: line to D1.W, D2.W
      if (!config_.graphics_io)
        break;
      config_.graphics_io->LineTo(static_cast<int16_t>(state_.d[1]),
                                  static_cast<int16_t>(state_.d[2]));
      break;

    case kTrap15MoveTo:  // 86: move to D1.W, D2.W
      if (!config_.graphics_io)
        break;
      config_.graphics_io->MoveTo(static_cast<int16_t>(state_.d[1]),
                                  static_cast<int16_t>(state_.d[2]));
      break;

    case kTrap15DrawRectangle:  // 87: filled rectangle corners D1-D4.W
      if (!config_.graphics_io)
        break;
      config_.graphics_io->DrawRectangle(
          static_cast<int16_t>(state_.d[1]), static_cast<int16_t>(state_.d[2]),
          static_cast<int16_t>(state_.d[3]), static_cast<int16_t>(state_.d[4]));
      break;

    case kTrap15DrawEllipse:  // 88: filled ellipse bounded by D1-D4.W
      if (!config_.graphics_io)
        break;
      config_.graphics_io->DrawEllipse(
          static_cast<int16_t>(state_.d[1]), static_cast<int16_t>(state_.d[2]),
          static_cast<int16_t>(state_.d[3]), static_cast<int16_t>(state_.d[4]));
      break;

    case kTrap15FloodFill:  // 89: flood fill at D1.W, D2.W
      if (!config_.graphics_io)
        break;
      config_.graphics_io->FloodFill(static_cast<int16_t>(state_.d[1]),
                                     static_cast<int16_t>(state_.d[2]));
      break;

    case kTrap15DrawUnfilledRectangle:  // 90: unfilled rectangle corners D1-D4.W
      if (!config_.graphics_io)
        break;
      config_.graphics_io->DrawUnfilledRectangle(
          static_cast<int16_t>(state_.d[1]), static_cast<int16_t>(state_.d[2]),
          static_cast<int16_t>(state_.d[3]), static_cast<int16_t>(state_.d[4]));
      break;

    case kTrap15DrawUnfilledEllipse:  // 91: unfilled ellipse bounded by D1-D4.W
      if (!config_.graphics_io)
        break;
      config_.graphics_io->DrawUnfilledEllipse(
          static_cast<int16_t>(state_.d[1]), static_cast<int16_t>(state_.d[2]),
          static_cast<int16_t>(state_.d[3]), static_cast<int16_t>(state_.d[4]));
      break;

    case kTrap15SetDrawingMode:  // 92: drawing mode D1.W (0-17)
      if (!config_.graphics_io)
        break;
      config_.graphics_io->SetDrawingMode(static_cast<int>(state_.d[1]));
      break;

    case kTrap15SetPenWidth:  // 93: pen width D1.B pixels
      if (!config_.graphics_io)
        break;
      config_.graphics_io->SetPenWidth(static_cast<int>(static_cast<uint8_t>(state_.d[1])));
      break;

    case kTrap15Repaint:  // 94: repaint (copy offscreen buffer to screen)
      if (!config_.graphics_io)
        break;
      config_.graphics_io->Repaint();
      break;

    case kTrap15DrawText: {  // 95: draw null string at (A1) to X=D1.W, Y=D2.W
      if (!config_.graphics_io)
        break;
      ReadStrFromMem(raw, state_.a[1], buf, sizeof(buf));
      config_.graphics_io->DrawText(buf, static_cast<int16_t>(state_.d[1]),
                                    static_cast<int16_t>(state_.d[2]));
      break;
    }

    case kTrap15GetXY: {  // 96: get pen position → D1.W=X, D2.W=Y
      if (!config_.graphics_io)
        break;
      int x = 0, y = 0;
      config_.graphics_io->GetXY(&x, &y);
      state_.d[1] = (state_.d[1] & 0xFFFF0000u) | static_cast<uint16_t>(x);
      state_.d[2] = (state_.d[2] & 0xFFFF0000u) | static_cast<uint16_t>(y);
      break;
    }

      // ---- NETWORK I/O -----------------------------------------------------

    case kTrap15NetCreateClient: {  // 100: create client; D1=settings, A2=server IP
      if (!config_.network_io)
        break;
      char server[256];
      ReadStrFromMem(raw, state_.a[2], server, sizeof(server));
      int result = 0;
      config_.network_io->CreateClient(static_cast<int>(state_.d[1]), server, &result);
      state_.d[0] = static_cast<uint32_t>(result);
      break;
    }

    case kTrap15NetCreateServer: {  // 101: create server; D1=settings
      if (!config_.network_io)
        break;
      int result = 0;
      config_.network_io->CreateServer(static_cast<int>(state_.d[1]), &result);
      state_.d[0] = static_cast<uint32_t>(result);
      break;
    }

    case kTrap15NetSend: {  // 102: send D1.W bytes from (A1) to server at (A2)
      if (!config_.network_io)
        break;
      char remote_ip[256];
      ReadStrFromMem(raw, state_.a[2], remote_ip, sizeof(remote_ip));
      const char* data = reinterpret_cast<const char*>(raw + (state_.a[1] & kAddressMask));
      int count = 0, result = 0;
      config_.network_io->Send(static_cast<int>(state_.d[1]), data, remote_ip, &count, &result);
      state_.d[1] = static_cast<uint32_t>(count);
      state_.d[0] = static_cast<uint32_t>(result);
      break;
    }

    case kTrap15NetReceive: {  // 103: receive D1.W bytes into (A1); sender IP at (A2)
      if (!config_.network_io)
        break;
      char* recv_buf = reinterpret_cast<char*>(raw + (state_.a[1] & kAddressMask));
      char* sender_ip = reinterpret_cast<char*>(raw + (state_.a[2] & kAddressMask));
      int count = 0, result = 0;
      config_.network_io->Receive(static_cast<int>(state_.d[1]), recv_buf, &count, sender_ip,
                                  &result);
      state_.d[1] = static_cast<uint32_t>(count);
      state_.d[0] = static_cast<uint32_t>(result);
      break;
    }

    case kTrap15NetClose: {  // 104: close connection; D1.L=IP address (0=all)
      if (!config_.network_io)
        break;
      int result = 0;
      config_.network_io->CloseConnection(static_cast<int>(state_.d[1]), &result);
      state_.d[0] = static_cast<uint32_t>(result);
      break;
    }

    case kTrap15NetGetLocalIP: {  // 105: get local IP; result string at (A2)
      if (!config_.network_io)
        break;
      char* ip_buf = reinterpret_cast<char*>(raw + (state_.a[2] & kAddressMask));
      int result = 0;
      config_.network_io->GetLocalIP(ip_buf, &result);
      state_.d[0] = static_cast<uint32_t>(result);
      break;
    }

    case kTrap15NetSendOnPort: {  // 106: send on port; D0,D1=settings, (A1)=data, (A2)=IP
      if (!config_.network_io)
        break;
      char remote_ip[256];
      ReadStrFromMem(raw, state_.a[2], remote_ip, sizeof(remote_ip));
      const char* data = reinterpret_cast<const char*>(raw + (state_.a[1] & kAddressMask));
      long d0 = static_cast<long>(state_.d[0]);
      long d1 = static_cast<long>(state_.d[1]);
      config_.network_io->SendOnPort(&d0, &d1, data, remote_ip);
      state_.d[0] = static_cast<uint32_t>(d0);
      state_.d[1] = static_cast<uint32_t>(d1);
      break;
    }

    case kTrap15NetReceiveOnPort: {  // 107: receive on port; (A1)=buf, (A2)=sender IP
      if (!config_.network_io)
        break;
      char* recv_buf = reinterpret_cast<char*>(raw + (state_.a[1] & kAddressMask));
      char* sender_ip = reinterpret_cast<char*>(raw + (state_.a[2] & kAddressMask));
      long d0 = static_cast<long>(state_.d[0]);
      long d1 = static_cast<long>(state_.d[1]);
      config_.network_io->ReceiveOnPort(&d0, &d1, recv_buf, sender_ip);
      state_.d[0] = static_cast<uint32_t>(d0);
      state_.d[1] = static_cast<uint32_t>(d1);
      break;
    }

    default:
      if (config_.logger) {
        char msg[64];
        std::snprintf(msg, sizeof(msg), "Trap #15: unknown task %u", static_cast<unsigned>(task));
        config_.logger->Log(msg);
      }
      break;
  }

  return SimResult::kOk;
}

}  // namespace easym68k::sim
