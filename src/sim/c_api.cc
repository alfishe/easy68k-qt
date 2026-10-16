// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "easym68k/sim/c_api.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "easym68k/sim/ifile_io.h"
#include "easym68k/sim/itext_io.h"
#include "easym68k/sim/simulator.h"
#include "easym68k/sim/std_file_io.h"

namespace {

class BufferedTextIO final : public easym68k::sim::ITextIO {
 public:
  void TextOut(std::string_view str) override { output_.append(str); }
  void TextOutCRLF(std::string_view str) override {
    output_.append(str);
    output_.append("\r\n");
  }
  void TextIn(char* buf, long* size, long* in_num) override {
    std::string line;
    if (!lines_.empty()) {
      line = std::move(lines_.front());
      lines_.pop_front();
    }
    if (in_num != nullptr) {
      *in_num = std::strtol(line.c_str(), nullptr, 10);
      if (size != nullptr)
        *size = static_cast<long>(line.size());
      return;
    }
    const size_t copy_len = std::min<size_t>(line.size(), 80);
    if (buf != nullptr) {
      std::memcpy(buf, line.data(), copy_len);
      buf[copy_len] = '\0';
    }
    if (size != nullptr)
      *size = static_cast<long>(copy_len);
  }
  void CharIn(char* ch) override {
    if (ch == nullptr)
      return;
    if (!lines_.empty() && !lines_.front().empty()) {
      *ch = lines_.front().front();
      lines_.front().erase(lines_.front().begin());
      if (lines_.front().empty())
        lines_.pop_front();
    } else {
      *ch = '\0';
    }
  }
  void CharOut(char ch) override { output_.push_back(ch); }
  void SetCursorPosition(int col, int row) override {
    col_ = col;
    row_ = row;
  }
  void GetCursorPosition(int* col, int* row) override {
    if (col != nullptr)
      *col = col_;
    if (row != nullptr)
      *row = row_;
  }
  void ClearScreen() override {
    output_.clear();
    col_ = 0;
    row_ = 0;
  }
  void SetKeyboardEcho(bool enabled) override { echo_ = enabled; }
  void SetInputPrompt(bool enabled) override { prompt_ = enabled; }
  void SetInputLFDisplay(bool enabled) override { lf_ = enabled; }
  void GetCharAt(int /*row*/, int /*col*/, char* ch) override {
    if (ch != nullptr)
      *ch = ' ';
  }
  void ScrollRect(int /*r*/, int /*c*/, int /*h*/, int /*w*/, int /*dir*/) override {}
  void SetFontProperties(uint32_t /*color*/, uint32_t /*style*/) override {}
  void GetKeyState(long* state) override {
    if (state != nullptr)
      *state = 0;
  }
  bool IsKeyPending() override { return !lines_.empty(); }
  void SetKeyCommandsEnabled(bool /*enabled*/) override {}

  const std::string& Output() const { return output_; }
  void ClearOutput() { output_.clear(); }
  void QueueLine(const char* line) {
    if (line != nullptr)
      lines_.emplace_back(line);
  }

 private:
  std::string output_;
  std::deque<std::string> lines_;
  int col_ = 0;
  int row_ = 0;
  bool echo_ = true;
  bool prompt_ = true;
  bool lf_ = true;
};

}  // namespace

struct Easy68kSim {
  BufferedTextIO default_text_io;
  easym68k::sim::StdFileIO default_file_io;
  easym68k::sim::ITextIO* custom_text_io = nullptr;
  easym68k::sim::IFileIO* custom_file_io = nullptr;
  std::unique_ptr<easym68k::sim::M68kSimulator> sim;

  Easy68kSim() { RebuildSimulator(); }

  void RebuildSimulator() {
    easym68k::sim::M68kSimulator::Config cfg;
    cfg.text_io = custom_text_io != nullptr ? custom_text_io : &default_text_io;
    cfg.file_io = custom_file_io != nullptr ? custom_file_io : &default_file_io;
    sim = std::make_unique<easym68k::sim::M68kSimulator>(cfg);
  }
};

extern "C" {

Easy68kSim* easy68k_sim_create(void) {
  return new Easy68kSim();
}

void easy68k_sim_destroy(Easy68kSim* sim) {
  delete sim;
}

int easy68k_sim_reset(Easy68kSim* sim) {
  if (sim == nullptr)
    return -1;
  sim->sim->Reset();
  return static_cast<int>(easym68k::sim::SimResult::kOk);
}

int easy68k_sim_init_program(Easy68kSim* sim) {
  if (sim == nullptr)
    return -1;
  sim->sim->InitProgram();
  return static_cast<int>(easym68k::sim::SimResult::kOk);
}

int easy68k_sim_step(Easy68kSim* sim) {
  if (sim == nullptr)
    return -1;
  return static_cast<int>(sim->sim->Step());
}

int easy68k_sim_step_over(Easy68kSim* sim) {
  if (sim == nullptr)
    return -1;
  return static_cast<int>(sim->sim->StepOver());
}

int easy68k_sim_run(Easy68kSim* sim, int max_instructions) {
  if (sim == nullptr)
    return -1;
  return static_cast<int>(sim->sim->Run(max_instructions));
}

void easy68k_sim_request_stop(Easy68kSim* sim) {
  if (sim != nullptr)
    sim->sim->RequestStop();
}

uint32_t easy68k_get_register(const Easy68kSim* sim, int reg) {
  if (sim == nullptr)
    return 0;
  const auto& s = sim->sim->State();
  if (reg >= EASY68K_REG_D0 && reg <= EASY68K_REG_D7)
    return s.d[reg - EASY68K_REG_D0];
  if (reg >= EASY68K_REG_A0 && reg <= EASY68K_REG_A7)
    return s.GetAReg(reg - EASY68K_REG_A0);
  switch (reg) {
    case EASY68K_REG_PC:
      return s.pc;
    case EASY68K_REG_SR:
      return s.sr;
    case EASY68K_REG_USP:
      return s.usp;
    case EASY68K_REG_SSP:
      return s.ssp;
    default:
      return 0;
  }
}

void easy68k_set_register(Easy68kSim* sim, int reg, uint32_t value) {
  if (sim == nullptr)
    return;
  auto& s = sim->sim->State();
  if (reg >= EASY68K_REG_D0 && reg <= EASY68K_REG_D7) {
    s.d[reg - EASY68K_REG_D0] = value;
    return;
  }
  if (reg >= EASY68K_REG_A0 && reg <= EASY68K_REG_A7) {
    s.SetAReg(reg - EASY68K_REG_A0, value);
    return;
  }
  switch (reg) {
    case EASY68K_REG_PC:
      s.pc = value;
      break;
    case EASY68K_REG_SR:
      s.sr = static_cast<uint16_t>(value);
      sim->sim->GetMemory().SetSupervisor(s.IsSupervisor());
      break;
    case EASY68K_REG_USP:
      s.usp = value;
      break;
    case EASY68K_REG_SSP:
      s.ssp = value;
      break;
    default:
      break;
  }
}

uint64_t easy68k_get_cycles(const Easy68kSim* sim) {
  if (sim == nullptr)
    return 0;
  return sim->sim->State().cycles;
}

void easy68k_clear_cycles(Easy68kSim* sim) {
  if (sim != nullptr)
    sim->sim->State().cycles = 0;
}

int easy68k_is_halted(const Easy68kSim* sim) {
  if (sim == nullptr)
    return 0;
  return sim->sim->State().halted ? 1 : 0;
}

int easy68k_is_stopped(const Easy68kSim* sim) {
  if (sim == nullptr)
    return 0;
  return sim->sim->State().stopped ? 1 : 0;
}

uint8_t easy68k_mem_read_byte(const Easy68kSim* sim, uint32_t addr) {
  if (sim == nullptr)
    return 0;
  return static_cast<uint8_t>(sim->sim->GetMemory().ReadByte(addr).value);
}

uint16_t easy68k_mem_read_word(const Easy68kSim* sim, uint32_t addr) {
  if (sim == nullptr)
    return 0;
  return static_cast<uint16_t>(sim->sim->GetMemory().ReadWord(addr).value);
}

uint32_t easy68k_mem_read_long(const Easy68kSim* sim, uint32_t addr) {
  if (sim == nullptr)
    return 0;
  return sim->sim->GetMemory().ReadLong(addr).value;
}

void easy68k_mem_write_byte(Easy68kSim* sim, uint32_t addr, uint8_t val) {
  if (sim != nullptr)
    sim->sim->GetMemory().WriteByte(addr, val);
}

void easy68k_mem_write_word(Easy68kSim* sim, uint32_t addr, uint16_t val) {
  if (sim != nullptr)
    sim->sim->GetMemory().WriteWord(addr, val);
}

void easy68k_mem_write_long(Easy68kSim* sim, uint32_t addr, uint32_t val) {
  if (sim != nullptr)
    sim->sim->GetMemory().WriteLong(addr, val);
}

int easy68k_load_srecord_file(Easy68kSim* sim, const char* path) {
  if (sim == nullptr || path == nullptr)
    return 0;
  sim->sim->GetMemory().Erase();
  if (!sim->sim->LoadSRecord(path))
    return 0;
  sim->sim->InitProgram();
  return 1;
}

int easy68k_load_srecord_data(Easy68kSim* sim, const char* data, int len) {
  if (sim == nullptr || data == nullptr || len < 0)
    return 0;
  sim->sim->GetMemory().Erase();
  std::istringstream iss(std::string(data, static_cast<size_t>(len)));
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(iss, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (!line.empty())
      lines.push_back(std::move(line));
  }
  if (!sim->sim->LoadSRecordLines(lines))
    return 0;
  sim->sim->InitProgram();
  return 1;
}

void easy68k_add_breakpoint(Easy68kSim* sim, uint32_t addr) {
  if (sim != nullptr)
    sim->sim->AddBreakpoint(addr);
}

void easy68k_remove_breakpoint(Easy68kSim* sim, uint32_t addr) {
  if (sim != nullptr)
    sim->sim->RemoveBreakpoint(addr);
}

void easy68k_clear_breakpoints(Easy68kSim* sim) {
  if (sim != nullptr)
    sim->sim->ClearBreakpoints();
}

void easy68k_interrupt(Easy68kSim* sim, int level) {
  if (sim != nullptr && level >= 1 && level <= 7)
    sim->sim->Interrupt(static_cast<uint8_t>(level));
}

const char* easy68k_get_output(const Easy68kSim* sim) {
  if (sim == nullptr)
    return "";
  return sim->default_text_io.Output().c_str();
}

void easy68k_clear_output(Easy68kSim* sim) {
  if (sim != nullptr)
    sim->default_text_io.ClearOutput();
}

void easy68k_queue_input_line(Easy68kSim* sim, const char* line) {
  if (sim != nullptr)
    sim->default_text_io.QueueLine(line);
}

void easy68k_set_text_io(Easy68kSim* sim, void* text_io) {
  if (sim == nullptr)
    return;
  sim->custom_text_io = static_cast<easym68k::sim::ITextIO*>(text_io);
  sim->RebuildSimulator();
}

void easy68k_set_file_io(Easy68kSim* sim, void* file_io) {
  if (sim == nullptr)
    return;
  sim->custom_file_io = static_cast<easym68k::sim::IFileIO*>(file_io);
  sim->RebuildSimulator();
}

}  // extern "C"
