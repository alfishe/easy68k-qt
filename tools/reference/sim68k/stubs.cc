// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Definitions for the reference simulator oracle: the original globals
// (var.h, as SIM68Ku.cpp includes it) and the headless GUI stand-ins
// declared in compat/sim_prelude.h.

#include "def.h"
#include "proto.h"
#include "var.h"

TForm1* Form1 = new TForm1;
TsimIO* simIO = new TsimIO;
THardware* Hardware = new THardware;
TLog* Log = new TLog;
TGenericForm* MemoryFrm = new TGenericForm;
TGenericForm* StackFrm = new TGenericForm;
TGenericForm* AutoTraceOptions = new TGenericForm;
TGenericForm* BreaksFrm = new TGenericForm;
TApplication* Application = new TApplication;

TPrinter* Printer() {
  static TPrinter printer;
  return &printer;
}

// --- Trap #15 text I/O -------------------------------------------------------
// Output goes to stdout with CR/LF reduced to LF. Input is read synchronously
// from stdin a line at a time, emulating TsimIO::FormKeyPress for the
// completed line (the original collects keys asynchronously).

void TsimIO::textOut(const AnsiString& s) {
  std::fputs(s.c_str(), stdout);
}

void TsimIO::textOutCR(const AnsiString& s) {
  std::fputs(s.c_str(), stdout);
  std::fputc('\n', stdout);
}

void TsimIO::charOut(char c) {
  if (c != '\r')
    std::fputc(c, stdout);
}

void TsimIO::textIn(char* str, long* size, long* inNum) {
  char keyBuf[256];
  int keyI = 0;
  int c;
  // The original accepts at most 79 keys before forcing Enter.
  while (keyI < 79 && (c = std::fgetc(stdin)) != EOF && c != '\n') {
    if (c != '\r')
      keyBuf[keyI++] = static_cast<char>(c);
  }
  keyBuf[keyI] = '\0';
  if (keyboardEcho)
    std::fputs(keyBuf, stdout);
  std::fputc('\n', stdout);  // doCRLF()
  if (inNum == NULL) {
    for (int i = 0; i <= keyI; i++)
      mem_put(keyBuf[i], (int)&str[i] - (int)&memory[0], BYTE_MASK);
    *size = keyI;
  } else {
    strcpy(inputBuf, keyBuf);
    *size = keyI;
    *inNum = atoi(inputBuf);
  }
  inputMode = false;
}

void TsimIO::charIn(char* ch) {
  int c = std::fgetc(stdin);
  if (c == '\n')
    c = '\r';  // Enter key
  *ch = static_cast<char>(c == EOF ? 0 : c);
  if (keyboardEcho) {
    charOut(*ch);
    if (*ch == '\r' && inputLFdisplay)
      charOut('\n');
  }
  inputMode = false;
}

// --- hardwareu.cpp globals (Hardware window) ---------------------------------
// Device addresses: the Hardware window defaults; memory maps disabled.
int seg7loc = 0xE00000, LEDloc = 0xE00010, switchLoc = 0xE00012, pbLoc = 0xE00014;
int ROMStart = 0, ROMEnd = 0, ReadStart = 0, ReadEnd = 0;
int ProtectedStart = 0, ProtectedEnd = 0, InvalidStart = 0, InvalidEnd = 0;
bool ROMMap = false, ReadMap = false, ProtectedMap = false, InvalidMap = false;

// Console-era display helpers (SIM68Ku.cpp); no display here.
void windowLine() {}
void scrollWindow() {}
