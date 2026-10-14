// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// easy68k-sim: headless command-line runner for the EASy68K-Qt simulator core.
//
//   easy68k-sim [-e] [-b] [-n MAX] [-t] [-o TRACE] [-m ADDR:LEN]... FILE.S68
//
//   -e   exceptions enabled (Sim68K /e; off by default as in Sim68K)
//   -b   bitfield instructions enabled (Sim68K /b)
//   -n   stop after MAX instructions (default 100000)
//   -t   write one register line per executed instruction
//   -o   trace file (default stderr)
//   -m   dump LEN bytes of memory at ADDR (hex) after the run
//
// Loads the program the way Sim68K's File > Open does (memory erased to $FF,
// toggle-switch byte cleared, registers initialized) and runs it. Trap #15
// text output goes to stdout and text input comes from stdin. The trace
// format matches tools/reference/sim68k (easy68k-ref-sim), the oracle built
// from the original sources, so tools/reference/compare_sim.sh can diff them.

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "easym68k/sim/itext_io.h"
#include "easym68k/sim/simulator.h"

namespace {

using easym68k::sim::kMemorySize;
using easym68k::sim::M68kSimulator;

// Default Hardware window device addresses (Sim68K).
constexpr uint32_t kSeg7Addr = 0xE00000;
constexpr uint32_t kLedAddr = 0xE00010;
constexpr uint32_t kSwitchAddr = 0xE00012;
constexpr uint32_t kButtonAddr = 0xE00014;

FILE* g_trace = stderr;

// Console text I/O, matching the oracle's headless simIO (sim68k/stubs.cc).
class ConsoleTextIO : public easym68k::sim::ITextIO {
 public:
  void TextOut(std::string_view text) override { std::fwrite(text.data(), 1, text.size(), stdout); }
  void TextOutCRLF(std::string_view text) override {
    TextOut(text);
    std::fputc('\n', stdout);
  }
  void TextIn(char* buf, long* size, long* result) override {
    char key_buf[256];
    int n = 0;
    int c;
    while (n < 79 && (c = std::fgetc(stdin)) != EOF && c != '\n') {
      if (c != '\r')
        key_buf[n++] = static_cast<char>(c);
    }
    key_buf[n] = '\0';
    if (echo_)
      std::fputs(key_buf, stdout);
    std::fputc('\n', stdout);
    if (result == nullptr) {
      std::memcpy(buf, key_buf, n + 1);
      *size = n;
    } else {
      std::strcpy(buf, key_buf);
      *size = n;
      *result = std::atoi(key_buf);
    }
  }
  void CharIn(char* ch) override {
    int c = std::fgetc(stdin);
    if (c == '\n')
      c = '\r';
    *ch = static_cast<char>(c == EOF ? 0 : c);
    if (echo_) {
      CharOut(*ch);
      if (*ch == '\r' && lf_display_)
        CharOut('\n');
    }
  }
  void CharOut(char ch) override {
    if (ch != '\r')
      std::fputc(ch, stdout);
  }
  void SetCursorPosition(int, int) override {}
  void GetCursorPosition(int* col, int* row) override { *col = *row = 0; }
  void ClearScreen() override {}
  void SetKeyboardEcho(bool enable) override { echo_ = enable; }
  void SetInputPrompt(bool) override {}
  void SetInputLFDisplay(bool enable) override { lf_display_ = enable; }
  void GetCharAt(int, int, char* ch) override { *ch = 0; }
  void ScrollRect(int, int, int, int, int) override {}
  void SetFontProperties(uint32_t, uint32_t) override {}
  bool IsKeyPending() override { return false; }
  void GetKeyState(long* state) override { *state = 0; }
  void SetKeyCommandsEnabled(bool) override {}

 private:
  bool echo_ = true;
  bool lf_display_ = true;
};

void PrintRegs(const char* tag, const M68kSimulator& sim) {
  const auto& s = sim.State();
  std::fprintf(g_trace, "%s PC=%08X SR=%04X", tag, s.pc, s.sr);
  for (int i = 0; i < 8; i++)
    std::fprintf(g_trace, " D%d=%08X", i, s.d[i]);
  for (int i = 0; i < 7; i++)
    std::fprintf(g_trace, " A%d=%08X", i, s.a[i]);
  std::fprintf(g_trace, " US=%08X SS=%08X CYC=%" PRIu64 "\n", s.a[7], s.ssp, s.cycles);
}

uint32_t MemoryHash(const M68kSimulator& sim) {
  const uint8_t* m = sim.GetMemory().RawData();
  uint32_t h = 2166136261u;  // FNV-1a
  for (uint32_t i = 0; i < kMemorySize; i++) {
    h ^= m[i];
    h *= 16777619u;
  }
  return h;
}

void DumpMemory(const M68kSimulator& sim, uint32_t addr, uint32_t len) {
  const uint8_t* m = sim.GetMemory().RawData();
  for (uint32_t row = 0; row < len; row += 16) {
    std::fprintf(g_trace, "MEM %08X:", addr + row);
    for (uint32_t i = row; i < len && i < row + 16; i++)
      std::fprintf(g_trace, " %02X", m[(addr + i) & (kMemorySize - 1)]);
    std::fprintf(g_trace, "\n");
  }
}

int Usage() {
  std::fprintf(stderr,
               "usage: easy68k-sim [-e] [-b] [-n MAX] [-t] [-o TRACE] [-m ADDR:LEN]... FILE.S68\n");
  return 2;
}

}  // namespace

int main(int argc, char** argv) {
  bool exceptions = false, bitfield = false, trace = false;
  long max_steps = 100000;
  const char* file = nullptr;
  std::vector<std::pair<uint32_t, uint32_t>> dumps;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "-e") {
      exceptions = true;
    } else if (a == "-b") {
      bitfield = true;
    } else if (a == "-t") {
      trace = true;
    } else if (a == "-n" && i + 1 < argc) {
      max_steps = std::strtol(argv[++i], nullptr, 0);
    } else if (a == "-o" && i + 1 < argc) {
      g_trace = std::fopen(argv[++i], "w");
      if (!g_trace) {
        std::perror(argv[i]);
        return 2;
      }
    } else if (a == "-m" && i + 1 < argc) {
      unsigned addr = 0, len = 0;
      if (std::sscanf(argv[++i], "%x:%x", &addr, &len) != 2)
        return Usage();
      dumps.emplace_back(addr, len);
    } else if (!a.empty() && a[0] != '-' && !file) {
      file = argv[i];
    } else {
      return Usage();
    }
  }
  if (!file)
    return Usage();

  ConsoleTextIO text_io;
  M68kSimulator::Config config;
  config.text_io = &text_io;
  config.exceptions_enabled = exceptions;
  config.bitfield_enabled = bitfield;
  M68kSimulator sim(config);
  sim.SetHardwareAddresses(kSeg7Addr, kLedAddr, kSwitchAddr, kButtonAddr);

  sim.GetMemory().Erase();
  sim.GetMemory().RawData()[kSwitchAddr] = 0x00;
  if (!sim.LoadSRecord(file)) {
    std::fprintf(g_trace, "END LOADFAIL steps=0\n");
    return 1;
  }
  sim.InitProgram();
  PrintRegs("INIT", sim);

  long steps = 0;
  const char* reason = "LIMIT";
  while (steps < max_steps) {
    sim.Step();
    steps++;
    if (trace)
      PrintRegs("STEP", sim);
    if (sim.State().halted) {
      reason = "HALT";
      break;
    }
    if (sim.State().stopped) {
      reason = "STOP";
      break;
    }
  }
  std::fflush(stdout);
  std::fprintf(g_trace, "END %s steps=%ld\n", reason, steps);
  PrintRegs("FINAL", sim);
  std::fprintf(g_trace, "HASH %08X\n", MemoryHash(sim));
  for (const auto& d : dumps)
    DumpMemory(sim, d.first, d.second);
  if (g_trace != stderr)
    std::fclose(g_trace);
  return 0;
}
