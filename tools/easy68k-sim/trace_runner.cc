// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "trace_runner.h"

#include <cinttypes>
#include <cstdlib>
#include <cstring>

#include "easym68k/sim/itext_io.h"
#include "easym68k/sim/simulator.h"

namespace easym68k::tools {
namespace {

using sim::kMemorySize;
using sim::M68kSimulator;

// Default Hardware window device addresses (Sim68K).
constexpr uint32_t kSeg7Addr = 0xE00000;
constexpr uint32_t kLedAddr = 0xE00010;
constexpr uint32_t kSwitchAddr = 0xE00012;
constexpr uint32_t kButtonAddr = 0xE00014;

// Console text I/O, matching the oracle's headless simIO (sim68k/stubs.cc):
// output with CR dropped, input read a line at a time.
class ConsoleTextIO : public sim::ITextIO {
 public:
  ConsoleTextIO(FILE* in, FILE* out) : in_(in), out_(out) {}

  void TextOut(std::string_view text) override { std::fwrite(text.data(), 1, text.size(), out_); }
  void TextOutCRLF(std::string_view text) override {
    TextOut(text);
    std::fputc('\n', out_);
  }
  void TextIn(char* buf, long* size, long* result) override {
    char key_buf[256];
    int n = 0;
    int c;
    // The original accepts at most 79 keys before forcing Enter.
    while (n < 79 && (c = std::fgetc(in_)) != EOF && c != '\n') {
      if (c != '\r')
        key_buf[n++] = static_cast<char>(c);
    }
    key_buf[n] = '\0';
    if (echo_)
      std::fputs(key_buf, out_);
    std::fputc('\n', out_);
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
    int c = std::fgetc(in_);
    if (c == '\n')
      c = '\r';  // Enter key
    *ch = static_cast<char>(c == EOF ? 0 : c);
    if (echo_) {
      CharOut(*ch);
      if (*ch == '\r' && lf_display_)
        CharOut('\n');
    }
  }
  void CharOut(char ch) override {
    if (ch != '\r')
      std::fputc(ch, out_);
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
  FILE* in_;
  FILE* out_;
  bool echo_ = true;
  bool lf_display_ = true;
};

void PrintRegs(FILE* f, const char* tag, const M68kSimulator& sim) {
  const auto& s = sim.State();
  std::fprintf(f, "%s PC=%08X SR=%04X", tag, s.pc, s.sr);
  for (int i = 0; i < 8; i++)
    std::fprintf(f, " D%d=%08X", i, s.d[i]);
  for (int i = 0; i < 7; i++)
    std::fprintf(f, " A%d=%08X", i, s.a[i]);
  std::fprintf(f, " US=%08X SS=%08X CYC=%" PRIu64 "\n", s.a[7], s.ssp, s.cycles);
}

uint32_t MemoryHash(const M68kSimulator& sim) {
  const uint8_t* m = sim.GetMemory().RawData();
  uint32_t h = 2166136261u;  // FNV-1a over the whole address space
  for (uint32_t i = 0; i < kMemorySize; i++) {
    h ^= m[i];
    h *= 16777619u;
  }
  return h;
}

void DumpMemory(FILE* f, const M68kSimulator& sim, uint32_t addr, uint32_t len) {
  const uint8_t* m = sim.GetMemory().RawData();
  for (uint32_t row = 0; row < len; row += 16) {
    std::fprintf(f, "MEM %08X:", addr + row);
    for (uint32_t i = row; i < len && i < row + 16; i++)
      std::fprintf(f, " %02X", m[(addr + i) & (kMemorySize - 1)]);
    std::fprintf(f, "\n");
  }
}

}  // namespace

bool RunTrace(const std::string& s68_path, const TraceOptions& options, FILE* trace, FILE* in,
              FILE* out) {
  ConsoleTextIO text_io(in, out);
  M68kSimulator::Config config;
  config.text_io = &text_io;
  config.exceptions_enabled = options.exceptions;
  config.bitfield_enabled = options.bitfield;
  M68kSimulator sim(config);
  sim.SetHardwareAddresses(kSeg7Addr, kLedAddr, kSwitchAddr, kButtonAddr);

  // TForm1::OpenFile: erase memory, clear the toggle switches, load.
  sim.GetMemory().Erase();
  sim.GetMemory().RawData()[kSwitchAddr] = 0x00;
  if (!sim.LoadSRecord(s68_path)) {
    std::fprintf(trace, "END LOADFAIL steps=0\n");
    return false;
  }
  sim.InitProgram();
  PrintRegs(trace, "INIT", sim);

  long steps = 0;
  const char* reason = "LIMIT";
  while (steps < options.max_steps) {
    sim.Step();
    steps++;
    if (options.trace_steps)
      PrintRegs(trace, "STEP", sim);
    if (sim.State().halted) {
      reason = "HALT";
      break;
    }
    if (sim.State().stopped) {
      reason = "STOP";
      break;
    }
  }
  std::fflush(out);
  std::fprintf(trace, "END %s steps=%ld\n", reason, steps);
  PrintRegs(trace, "FINAL", sim);
  std::fprintf(trace, "HASH %08X\n", MemoryHash(sim));
  for (const auto& d : options.dumps)
    DumpMemory(trace, sim, d.first, d.second);
  return true;
}

}  // namespace easym68k::tools
