// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Force-included before every original Sim68K source compiled into the
// reference simulator oracle (tools/reference/sim68k). Provides the Borland
// runtime / Win32 names the sources use and headless stand-ins for the VCL
// forms (Form1, simIO, Hardware, Log, ...). Text output of the simulated
// program goes to stdout, simulator messages to stderr. Developer tool only.

#ifndef EASY68K_REFERENCE_SIM_PRELUDE_H_
#define EASY68K_REFERENCE_SIM_PRELUDE_H_

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "vcl.h"

// The sources rely on Borland's global iostream names (<iostream.h>).
using std::cin;
using std::cout;

// --- Borland / Win32 scalar types -----------------------------------------
// `byte` is a macro (not a typedef) because the sources also say
// `using namespace std;`, which would make ::byte and std::byte ambiguous.
#define byte unsigned char
#define __int64 long long
#define __fastcall
#define __published public
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef long HRESULT;
typedef void* LPVOID;
typedef void* HWND;
typedef void* HANDLE;
typedef const char* LPCTSTR;
typedef struct _devicemodeA {
} DEVMODEA, *LPDEVMODEA, DEVMODE, *LPDEVMODE;
typedef struct _DISPLAY_DEVICE {
} DISPLAY_DEVICE, *PDISPLAY_DEVICE;
#define CALLBACK
inline void Sleep(int) {}
inline char* ultoa(unsigned long value, char* buf, int radix) {
  if (radix == 10) {  // itoa() prints radix 10 signed
    std::sprintf(buf, "%lu", value);
    return buf;
  }
  return itoa(static_cast<int>(value), buf, radix);
}
inline int netCloseSockets() {
  return 0;
}

// --- A value that converts to anything (results of ignored GUI calls) -----
struct GuiAny {
  template <class T>
  operator T() const {
    return T();
  }
};

// Generic GUI control: only the properties the core sources touch.
struct TFont {
  template <class... A>
  void Assign(A&&...) {}
};
struct TCanvas {
  TFont* Font = new TFont;
  template <class... A>
  int TextHeight(A&&...) {
    return 1;
  }
  template <class... A>
  int TextWidth(A&&...) {
    return 1;
  }
  template <class... A>
  void TextOut(A&&...) {}
};
// Trace stream of the oracle driver (main.cc); simulator messages go there.
extern FILE* g_trace_out;
struct TLines {
  // Messages the simulator posts in its message pane.
  void Add(const AnsiString& s) { std::fprintf(g_trace_out, "MSG %s\n", s.c_str()); }
  void Clear() {}
};
struct TControl {
  TLines* Lines = new TLines;
  TFont* Font = new TFont;
  TCanvas* Canvas = new TCanvas;
  bool Enabled = false;
  bool Checked = false;
  AnsiString Text;
  AnsiString& EditText = Text;  // TMaskEdit: EditText mirrors Text
  AnsiString Caption;
  int MonitorNum = 0;
  int Interval = 0;
  void Repaint() {}
  void Show() {}
  void Hide() {}
  void SetFocus() {}
};

#define EASY68K_GUI_METHOD(name) \
  template <class... A>          \
  GuiAny name(A&&...) {          \
    return {};                   \
  }

struct TForm1 {
  TControl *Message = new TControl, *AutoTraceTimer = new TControl,
           *FontDialogSource = new TControl, *FontDialogSimIO = new TControl,
           *FontDialogPrinter = new TControl, *cyclesDisplay = new TControl,
           *Monitor = new TControl, *ListBox1 = new TControl, *HeadingsLbl = new TControl,
           *ExceptionsEnabled = new TControl;
  TControl *regD0 = new TControl, *regD1 = new TControl, *regD2 = new TControl,
           *regD3 = new TControl, *regD4 = new TControl, *regD5 = new TControl,
           *regD6 = new TControl, *regD7 = new TControl, *regA0 = new TControl,
           *regA1 = new TControl, *regA2 = new TControl, *regA3 = new TControl,
           *regA4 = new TControl, *regA5 = new TControl, *regA6 = new TControl,
           *regA7 = new TControl, *regA8 = new TControl, *regUS = new TControl,
           *regSR = new TControl, *regPC = new TControl;
  EASY68K_GUI_METHOD(SetFocus)
  EASY68K_GUI_METHOD(setMenuActive)
  EASY68K_GUI_METHOD(restoreMenuTask19)
  EASY68K_GUI_METHOD(setMenuTrace)
  EASY68K_GUI_METHOD(setMenuTask19)
  EASY68K_GUI_METHOD(SaveSettings)
  template <class... A>
  bool lineToLog(A&&...) {
    return false;
  }
  EASY68K_GUI_METHOD(highlight)
};
extern TForm1* Form1;

// Simulated-program text I/O (Trap #15). Defined in stubs.cc.
struct TsimIO {
  bool fullScreen = false;
  TFont* Font = new TFont;
  TControl* BackBuffer = new TControl;
  void textOut(const AnsiString& s);
  void textOutCR(const AnsiString& s);
  void charOut(char c);
  void textIn(char* str, long* size, long* inNum);
  void charIn(char* ch);
  EASY68K_GUI_METHOD(setupWindow)
  EASY68K_GUI_METHOD(setDrawingMode)
  EASY68K_GUI_METHOD(setWindowSize)
  EASY68K_GUI_METHOD(setPenWidth)
  EASY68K_GUI_METHOD(setLineColor)
  EASY68K_GUI_METHOD(setFillColor)
  EASY68K_GUI_METHOD(clear)
  EASY68K_GUI_METHOD(unfilledRectangle)
  EASY68K_GUI_METHOD(unfilledEllipse)
  EASY68K_GUI_METHOD(setFontProperties)
  EASY68K_GUI_METHOD(setCommParams)
  EASY68K_GUI_METHOD(sendPortNet)
  EASY68K_GUI_METHOD(sendNet)
  EASY68K_GUI_METHOD(sendComm)
  EASY68K_GUI_METHOD(scrollRect)
  EASY68K_GUI_METHOD(ResetSounds)
  EASY68K_GUI_METHOD(rectangle)
  EASY68K_GUI_METHOD(receivePortNet)
  EASY68K_GUI_METHOD(receiveNet)
  EASY68K_GUI_METHOD(readComm)
  EASY68K_GUI_METHOD(playSoundMemDX)
  EASY68K_GUI_METHOD(playSoundMem)
  EASY68K_GUI_METHOD(playSoundDX)
  EASY68K_GUI_METHOD(playSound)
  EASY68K_GUI_METHOD(moveTo)
  EASY68K_GUI_METHOD(loadSoundDX)
  EASY68K_GUI_METHOD(loadSound)
  EASY68K_GUI_METHOD(lineTo)
  EASY68K_GUI_METHOD(line)
  EASY68K_GUI_METHOD(initComm)
  EASY68K_GUI_METHOD(gotorc)
  EASY68K_GUI_METHOD(getXY)
  EASY68K_GUI_METHOD(getWindowSize)
  EASY68K_GUI_METHOD(getrc)
  EASY68K_GUI_METHOD(getPixel)
  EASY68K_GUI_METHOD(getLocalIP)
  EASY68K_GUI_METHOD(getKeyState)
  EASY68K_GUI_METHOD(getCharAt)
  EASY68K_GUI_METHOD(FormPaint)
  EASY68K_GUI_METHOD(floodFill)
  EASY68K_GUI_METHOD(ellipse)
  EASY68K_GUI_METHOD(drawText)
  EASY68K_GUI_METHOD(drawPixel)
  EASY68K_GUI_METHOD(displayFileDialog)
  EASY68K_GUI_METHOD(createNetServer)
  EASY68K_GUI_METHOD(createNetClient)
  EASY68K_GUI_METHOD(controlSoundDX)
  EASY68K_GUI_METHOD(controlSound)
  EASY68K_GUI_METHOD(closeNetConnection)
  EASY68K_GUI_METHOD(Show)
};
extern TsimIO* simIO;

struct THardware {
  TControl *switchAddr = new TControl, *seg7addr = new TControl, *pbAddr = new TControl,
           *LEDaddr = new TControl, *ROMStartEdit = new TControl, *ROMEndEdit = new TControl,
           *ROMChk = new TControl, *ReadStartEdit = new TControl, *ReadEndEdit = new TControl,
           *ReadChk = new TControl, *ProtectedStartEdit = new TControl,
           *ProtectedEndEdit = new TControl, *ProtectedChk = new TControl,
           *InvalidStartEdit = new TControl, *InvalidEndEdit = new TControl,
           *InvalidChk = new TControl;
  EASY68K_GUI_METHOD(disable)
  EASY68K_GUI_METHOD(enable)
  EASY68K_GUI_METHOD(autoIRQoff)
  EASY68K_GUI_METHOD(updateIfNeeded)
  EASY68K_GUI_METHOD(Show)
  EASY68K_GUI_METHOD(setAutoIRQ)
};
extern THardware* Hardware;

struct TLog {
  EASY68K_GUI_METHOD(stopLogWithAnnounce)
};
extern TLog* Log;

struct TGenericForm {
  TControl* DisableDisplay = new TControl;
  EASY68K_GUI_METHOD(Repaint)
  EASY68K_GUI_METHOD(LivePaint)
  EASY68K_GUI_METHOD(updateDisplay)
};
extern TGenericForm *MemoryFrm, *StackFrm, *AutoTraceOptions, *BreaksFrm;

struct TPrinter {
  TCanvas* Canvas = new TCanvas;
  int PageHeight = 1100, PageWidth = 850;
  EASY68K_GUI_METHOD(BeginDoc)
  EASY68K_GUI_METHOD(EndDoc)
  EASY68K_GUI_METHOD(NewPage)
  EASY68K_GUI_METHOD(Abort)
};
TPrinter* Printer();

#endif  // EASY68K_REFERENCE_SIM_PRELUDE_H_
