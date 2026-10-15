// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "example_runner.h"

#include <algorithm>
#include <cctype>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>

#include "easym68k/asm/assembler.h"
#include "easym68k/sim/igraphics_io.h"
#include "easym68k/sim/iperipheral_io.h"
#include "easym68k/sim/isimulator_env.h"
#include "easym68k/sim/itext_io.h"
#include "easym68k/sim/simulator.h"
#include "easym68k/sim/srecord_loader.h"
#include "easym68k/sim/std_file_io.h"

namespace easym68k::tools {
namespace {

namespace fs = std::filesystem;

constexpr uint32_t kSeg7Addr = 0xE00000;
constexpr uint32_t kLedAddr = 0xE00010;
constexpr uint32_t kSwitchAddr = 0xE00012;
constexpr uint32_t kButtonAddr = 0xE00014;

std::string Trim(std::string_view sv) {
  size_t start = 0;
  while (start < sv.size() && std::isspace(static_cast<unsigned char>(sv[start]))) {
    ++start;
  }
  size_t end = sv.size();
  while (end > start && std::isspace(static_cast<unsigned char>(sv[end - 1]))) {
    --end;
  }
  return std::string(sv.substr(start, end - start));
}

std::string ToUpper(std::string_view sv) {
  std::string out(sv);
  for (char& c : out) {
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }
  return out;
}

std::string ToLower(std::string_view sv) {
  std::string out(sv);
  for (char& c : out) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return out;
}

// Strips `#` comments outside double-quoted strings.
std::string StripComment(std::string_view line) {
  bool in_quotes = false;
  bool escaped = false;
  for (size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];
    if (escaped) {
      escaped = false;
      continue;
    }
    if (c == '\\' && in_quotes) {
      escaped = true;
      continue;
    }
    if (c == '"') {
      in_quotes = !in_quotes;
      continue;
    }
    if (c == '#' && !in_quotes) {
      return Trim(line.substr(0, i));
    }
  }
  return Trim(line);
}

int HexDigitValue(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F')
    return 10 + (c - 'A');
  return -1;
}

bool ParseTomlString(std::string_view raw, std::string* out, std::string* error) {
  const std::string s = Trim(raw);
  if (s.size() < 2 || s.front() != '"' || s.back() != '"') {
    if (error)
      *error = "expected quoted string, got: " + s;
    return false;
  }
  std::string result;
  result.reserve(s.size() - 2);
  for (size_t i = 1; i + 1 < s.size(); ++i) {
    const char c = s[i];
    if (c != '\\') {
      result.push_back(c);
      continue;
    }
    if (i + 2 >= s.size()) {
      if (error)
        *error = "trailing backslash in string: " + s;
      return false;
    }
    const char esc = s[++i];
    switch (esc) {
      case '\\':
        result.push_back('\\');
        break;
      case '"':
        result.push_back('"');
        break;
      case 'n':
        result.push_back('\n');
        break;
      case 'r':
        result.push_back('\r');
        break;
      case 't':
        result.push_back('\t');
        break;
      case 'x': {
        if (i + 3 >= s.size()) {
          if (error)
            *error = "incomplete \\xHH escape in string: " + s;
          return false;
        }
        const int hi = HexDigitValue(s[i + 1]);
        const int lo = HexDigitValue(s[i + 2]);
        if (hi < 0 || lo < 0) {
          if (error)
            *error = "invalid \\xHH escape in string: " + s;
          return false;
        }
        result.push_back(static_cast<char>((hi << 4) | lo));
        i += 2;
        break;
      }
      default:
        if (error) {
          *error = std::string("unsupported escape \\") + esc + " in: " + s;
        }
        return false;
    }
  }
  *out = std::move(result);
  return true;
}

bool ParseTomlUint64(std::string_view raw, uint64_t* out, std::string* error) {
  std::string clean;
  for (char c : Trim(raw)) {
    if (c != '_')
      clean.push_back(c);
  }
  if (clean.empty()) {
    if (error)
      *error = "empty integer literal";
    return false;
  }
  bool negative = false;
  size_t idx = 0;
  if (clean[idx] == '-' || clean[idx] == '+') {
    negative = (clean[idx] == '-');
    ++idx;
  }
  int base = 10;
  if (idx + 2 <= clean.size() && clean[idx] == '0' &&
      (clean[idx + 1] == 'x' || clean[idx + 1] == 'X')) {
    base = 16;
    idx += 2;
  } else if (idx + 2 <= clean.size() && clean[idx] == '0' &&
             (clean[idx + 1] == 'b' || clean[idx + 1] == 'B')) {
    base = 2;
    idx += 2;
  }
  if (idx >= clean.size()) {
    if (error)
      *error = "invalid integer literal: " + clean;
    return false;
  }
  uint64_t val = 0;
  for (; idx < clean.size(); ++idx) {
    const int d = HexDigitValue(clean[idx]);
    if (d < 0 || d >= base) {
      if (error)
        *error = "invalid digit in integer literal: " + clean;
      return false;
    }
    val = val * static_cast<uint64_t>(base) + static_cast<uint64_t>(d);
  }
  *out = negative ? (0ULL - val) : val;
  return true;
}

bool ParseTomlBool(std::string_view raw, bool* out, std::string* error) {
  const std::string s = Trim(raw);
  if (s == "true") {
    *out = true;
    return true;
  }
  if (s == "false") {
    *out = false;
    return true;
  }
  if (error)
    *error = "expected boolean true/false, got: " + s;
  return false;
}

// Splits comma-separated items at top level (ignoring commas inside quotes or
// nested braces/brackets).
std::vector<std::string> SplitTopLevelComma(std::string_view body) {
  std::vector<std::string> items;
  bool in_quotes = false;
  bool escaped = false;
  int depth = 0;
  size_t item_start = 0;
  for (size_t i = 0; i < body.size(); ++i) {
    const char c = body[i];
    if (escaped) {
      escaped = false;
      continue;
    }
    if (c == '\\' && in_quotes) {
      escaped = true;
      continue;
    }
    if (c == '"') {
      in_quotes = !in_quotes;
      continue;
    }
    if (!in_quotes) {
      if (c == '{' || c == '[') {
        ++depth;
      } else if (c == '}' || c == ']') {
        --depth;
      } else if (c == ',' && depth == 0) {
        const std::string part = Trim(body.substr(item_start, i - item_start));
        if (!part.empty())
          items.push_back(part);
        item_start = i + 1;
      }
    }
  }
  const std::string tail = Trim(body.substr(item_start));
  if (!tail.empty())
    items.push_back(tail);
  return items;
}

bool ParseStringArray(std::string_view raw, std::vector<std::string>* out, std::string* error) {
  const std::string s = Trim(raw);
  if (s.size() < 2 || s.front() != '[' || s.back() != ']') {
    if (error)
      *error = "expected array [...], got: " + s;
    return false;
  }
  out->clear();
  const auto items = SplitTopLevelComma(std::string_view(s).substr(1, s.size() - 2));
  for (const std::string& item : items) {
    std::string val;
    if (!ParseTomlString(item, &val, error))
      return false;
    out->push_back(std::move(val));
  }
  return true;
}

bool ParseHexByteString(std::string_view hex_str, std::vector<uint8_t>* out, std::string* error) {
  out->clear();
  std::string hex_digits;
  for (char c : hex_str) {
    if (std::isspace(static_cast<unsigned char>(c)))
      continue;
    if (HexDigitValue(c) < 0) {
      if (error) {
        *error = "invalid hex character in memory bytes: " + std::string(hex_str);
      }
      return false;
    }
    hex_digits.push_back(c);
  }
  if (hex_digits.size() % 2 != 0) {
    if (error) {
      *error = "memory hex string must have an even number of hex digits: " + std::string(hex_str);
    }
    return false;
  }
  for (size_t i = 0; i < hex_digits.size(); i += 2) {
    const int hi = HexDigitValue(hex_digits[i]);
    const int lo = HexDigitValue(hex_digits[i + 1]);
    out->push_back(static_cast<uint8_t>((hi << 4) | lo));
  }
  return true;
}

bool ParseInlineMemoryTable(std::string_view raw, MemoryExpectation* mem, std::string* error) {
  const std::string s = Trim(raw);
  if (s.size() < 2 || s.front() != '{' || s.back() != '}') {
    if (error)
      *error = "expected inline table {...}, got: " + s;
    return false;
  }
  bool has_addr = false;
  bool has_data = false;
  const auto entries = SplitTopLevelComma(std::string_view(s).substr(1, s.size() - 2));
  for (const std::string& entry : entries) {
    const size_t eq = entry.find('=');
    if (eq == std::string::npos) {
      if (error)
        *error = "missing '=' in inline table entry: " + entry;
      return false;
    }
    const std::string k = Trim(entry.substr(0, eq));
    const std::string v = Trim(entry.substr(eq + 1));
    if (k == "address") {
      uint64_t addr = 0;
      if (!ParseTomlUint64(v, &addr, error))
        return false;
      mem->address = static_cast<uint32_t>(addr);
      has_addr = true;
    } else if (k == "bytes") {
      std::string hex_s;
      if (!ParseTomlString(v, &hex_s, error))
        return false;
      if (!ParseHexByteString(hex_s, &mem->bytes, error))
        return false;
      has_data = true;
    } else if (k == "text") {
      std::string txt;
      if (!ParseTomlString(v, &txt, error))
        return false;
      mem->bytes.assign(txt.begin(), txt.end());
      has_data = true;
    } else {
      if (error)
        *error = "unknown key in memory expectation: " + k;
      return false;
    }
  }
  if (!has_addr || !has_data) {
    if (error) {
      *error = "memory expectation requires 'address' and 'bytes' or 'text'";
    }
    return false;
  }
  return true;
}

// Scripted headless Trap #15 text I/O for example runs.
class ScriptedTextIO : public sim::ITextIO {
 public:
  ScriptedTextIO(std::vector<std::string> lines, std::string chars)
      : lines_(std::move(lines)), chars_(std::move(chars)) {}

  void TextOut(std::string_view text) override { output_.append(text.data(), text.size()); }
  void TextOutCRLF(std::string_view text) override {
    output_.append(text.data(), text.size());
    output_.append("\r\n");
  }
  void CharOut(char ch) override { output_.push_back(ch); }

  void TextIn(char* buf, long* size, long* result) override {
    std::string line;
    if (line_index_ < lines_.size()) {
      line = lines_[line_index_++];
    }
    if (line.size() > 79) {
      line.resize(79);
    }
    std::memcpy(buf, line.c_str(), line.size() + 1);
    *size = static_cast<long>(line.size());
    if (result != nullptr) {
      *result = std::strtol(line.c_str(), nullptr, 10);
    }
  }

  void CharIn(char* ch) override {
    if (char_index_ < chars_.size()) {
      char c = chars_[char_index_++];
      if (c == '\n')
        c = '\r';
      *ch = c;
    } else {
      *ch = 0;
    }
  }

  void SetCursorPosition(int col, int row) override {
    col_ = col;
    row_ = row;
  }
  void GetCursorPosition(int* col, int* row) override {
    *col = col_;
    *row = row_;
  }
  void ClearScreen() override {
    output_.clear();
    col_ = 0;
    row_ = 0;
  }
  void SetKeyboardEcho(bool) override {}
  void SetInputPrompt(bool) override {}
  void SetInputLFDisplay(bool) override {}
  void GetCharAt(int, int, char* ch) override { *ch = 0; }
  void ScrollRect(int, int, int, int, int) override {}
  void SetFontProperties(uint32_t, uint32_t) override {}
  bool IsKeyPending() override {
    return char_index_ < chars_.size() || line_index_ < lines_.size();
  }
  void GetKeyState(long* state) override { *state = 0; }
  void SetKeyCommandsEnabled(bool) override {}

  const std::string& output() const { return output_; }

 private:
  std::vector<std::string> lines_;
  std::string chars_;
  size_t line_index_ = 0;
  size_t char_index_ = 0;
  int col_ = 0;
  int row_ = 0;
  std::string output_;
};

// Deterministic headless simulator environment (tasks 8, 23, 32, 33).
class HeadlessSimEnv : public sim::ISimulatorEnv {
 public:
  uint32_t GetTimeHundredths() override { return time_hundredths_; }
  void DelayHundredths(unsigned int n) override { time_hundredths_ += n; }
  void ShowHardware() override {}
  uint32_t GetSeg7Address() override { return kSeg7Addr; }
  uint32_t GetLEDAddress() override { return kLedAddr; }
  uint32_t GetSwitchAddress() override { return kSwitchAddr; }
  uint32_t GetPushButtonAddress() override { return kButtonAddr; }
  void SetAutoIRQ(uint8_t irq_config, uint32_t) override {
    const int level = irq_config & 0x7F;
    if (irq_config == 0) {
      for (bool& b : auto_irq_)
        b = false;
    } else if (level >= 1 && level <= 7) {
      auto_irq_[level] = (irq_config & 0x80) != 0;
    }
  }
  uint32_t GetVersion() override { return 59150; }
  void SetExceptionsEnabled(bool) override {}
  void SetWindowSize(unsigned short width, unsigned short height) override {
    width_ = width;
    height_ = height;
  }
  void GetWindowSize(unsigned short* width, unsigned short* height) override {
    *width = width_;
    *height = height_;
  }
  void SetupWindow(bool) override {}

  int FirstActiveAutoIrq() const {
    for (int lvl = 7; lvl >= 1; --lvl) {
      if (auto_irq_[lvl])
        return lvl;
    }
    return 0;
  }

 private:
  uint32_t time_hundredths_ = 1000;
  unsigned short width_ = 640;
  unsigned short height_ = 480;
  bool auto_irq_[8] = {};
};

// Headless 2D graphics tracker (tasks 80-96) for deterministic example runs.
class HeadlessGraphicsIO : public sim::IGraphicsIO {
 public:
  void SetPenColor(uint32_t color) override { pen_color_ = color & 0x00FFFFFFu; }
  void SetFillColor(uint32_t color) override { fill_color_ = color & 0x00FFFFFFu; }
  void DrawPixel(int x, int y) override { pixels_[{x, y}] = pen_color_; }
  int GetPixel(int x, int y) override {
    auto it = pixels_.find({x, y});
    return it != pixels_.end() ? static_cast<int>(it->second) : 0;
  }
  void DrawLine(int x1, int y1, int x2, int y2) override {
    pixels_[{x1, y1}] = pen_color_;
    pixels_[{x2, y2}] = pen_color_;
    pen_x_ = x2;
    pen_y_ = y2;
  }
  void LineTo(int x, int y) override {
    pixels_[{pen_x_, pen_y_}] = pen_color_;
    pixels_[{x, y}] = pen_color_;
    pen_x_ = x;
    pen_y_ = y;
  }
  void MoveTo(int x, int y) override {
    pen_x_ = x;
    pen_y_ = y;
  }
  void GetXY(int* x, int* y) override {
    *x = pen_x_;
    *y = pen_y_;
  }
  void DrawRectangle(int x1, int y1, int x2, int y2) override {
    pixels_[{x1, y1}] = pen_color_;
    pixels_[{x2, y2}] = pen_color_;
    if (x2 > x1 + 1 && y2 > y1 + 1) {
      pixels_[{(x1 + x2) / 2, (y1 + y2) / 2}] = fill_color_;
    }
  }
  void DrawEllipse(int x1, int y1, int x2, int y2) override {
    if (x2 > x1 + 1 && y2 > y1 + 1) {
      pixels_[{(x1 + x2) / 2, (y1 + y2) / 2}] = fill_color_;
    }
  }
  void FloodFill(int x, int y) override { pixels_[{x, y}] = fill_color_; }
  void DrawUnfilledRectangle(int x1, int y1, int x2, int y2) override {
    pixels_[{x1, y1}] = pen_color_;
    pixels_[{x2, y2}] = pen_color_;
  }
  void DrawUnfilledEllipse(int, int, int, int) override {}
  void SetDrawingMode(int) override {}
  void SetPenWidth(int) override {}
  void Repaint() override {}
  void DrawText(const char*, int, int) override {}

 private:
  uint32_t pen_color_ = 0x00FFFFFFu;
  uint32_t fill_color_ = 0x00000000u;
  int pen_x_ = 0;
  int pen_y_ = 0;
  std::map<std::pair<int, int>, uint32_t> pixels_;
};

// Headless peripheral I/O stub (tasks 60-62).
class HeadlessPeripheralIO : public sim::IPeripheralIO {
 public:
  void SetMouseIRQ(uint8_t, uint8_t) override {}
  void ReadMouse(int mode, long* button_state, long* position) override {
    *button_state = 0;
    if (mode >= 0 && mode <= 2) {
      *position = 0;
    }
  }
  void SetKeyIRQ(uint8_t, uint8_t) override {}
};

std::optional<std::string> ReadFileWithCaseFallback(const fs::path& base_dir,
                                                    const std::string& requested) {
  fs::path direct = fs::path(requested);
  if (!direct.is_absolute()) {
    direct = base_dir / direct;
  }
  if (fs::is_regular_file(direct)) {
    std::ifstream in(direct, std::ios::binary);
    if (in) {
      std::ostringstream ss;
      ss << in.rdbuf();
      return ss.str();
    }
  }
  // Case-insensitive lookup in parent directory (as in original EASy68K).
  const fs::path parent = direct.parent_path();
  const std::string target_lower = ToLower(direct.filename().string());
  std::error_code ec;
  if (fs::is_directory(parent, ec)) {
    for (const auto& entry : fs::directory_iterator(parent, ec)) {
      if (entry.is_regular_file(ec) && ToLower(entry.path().filename().string()) == target_lower) {
        std::ifstream in(entry.path(), std::ios::binary);
        if (in) {
          std::ostringstream ss;
          ss << in.rdbuf();
          return ss.str();
        }
      }
    }
  }
  return std::nullopt;
}

bool GetRegisterValue(const sim::CpuState& state, std::string_view name, uint32_t* val) {
  const std::string u = ToUpper(name);
  if (u.size() == 2 && u[0] == 'D' && u[1] >= '0' && u[1] <= '7') {
    *val = state.d[u[1] - '0'];
    return true;
  }
  if (u.size() == 2 && u[0] == 'A' && u[1] >= '0' && u[1] <= '7') {
    const int idx = u[1] - '0';
    if (idx == 7) {
      *val = state.IsSupervisor() ? state.ssp : state.a[7];
    } else {
      *val = state.a[idx];
    }
    return true;
  }
  if (u == "PC") {
    *val = state.pc;
    return true;
  }
  if (u == "SR") {
    *val = state.sr;
    return true;
  }
  if (u == "USP" || u == "US") {
    *val = state.a[7];
    return true;
  }
  if (u == "SSP" || u == "SS") {
    *val = state.ssp;
    return true;
  }
  return false;
}

std::string EscapeJsonString(std::string_view sv) {
  std::string out;
  out.reserve(sv.size() + 8);
  for (unsigned char c : sv) {
    switch (c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (c < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof(buf), "\\u%04x", c);
          out += buf;
        } else {
          out.push_back(static_cast<char>(c));
        }
        break;
    }
  }
  return out;
}

std::string FormatHexBytes(const std::vector<uint8_t>& bytes) {
  std::string out;
  char buf[4];
  for (size_t i = 0; i < bytes.size(); ++i) {
    if (i > 0)
      out.push_back(' ');
    std::snprintf(buf, sizeof(buf), "%02X", bytes[i]);
    out += buf;
  }
  return out;
}

}  // namespace

std::string SimResultToString(sim::SimResult result) {
  switch (result) {
    case sim::SimResult::kOk:
      return "kOk";
    case sim::SimResult::kBadInstruction:
      return "kBadInstruction";
    case sim::SimResult::kPrivilegeViolation:
      return "kPrivilegeViolation";
    case sim::SimResult::kAddressError:
      return "kAddressError";
    case sim::SimResult::kBusError:
      return "kBusError";
    case sim::SimResult::kDivideByZero:
      return "kDivideByZero";
    case sim::SimResult::kChkException:
      return "kChkException";
    case sim::SimResult::kTrapVException:
      return "kTrapVException";
    case sim::SimResult::kTraceException:
      return "kTraceException";
    case sim::SimResult::kLine1010:
      return "kLine1010";
    case sim::SimResult::kLine1111:
      return "kLine1111";
    case sim::SimResult::kStopInstruction:
      return "kStopInstruction";
    case sim::SimResult::kTrapException:
      return "kTrapException";
    case sim::SimResult::kHalted:
      return "kHalted";
  }
  return "kUnknown";
}

bool ParseExampleSpec(std::string_view toml_text, ExampleSpec* out, std::string* error) {
  *out = ExampleSpec{};
  std::string current_section;
  MemoryExpectation current_mem_table{};
  bool in_mem_table = false;
  bool mem_table_has_addr = false;
  bool mem_table_has_data = false;

  auto flush_mem_table = [&]() -> bool {
    if (!in_mem_table)
      return true;
    if (!mem_table_has_addr || !mem_table_has_data) {
      if (error) {
        *error = "[[expect.memory]] table missing 'address' or 'bytes'/'text'";
      }
      return false;
    }
    out->expect_memory.push_back(current_mem_table);
    current_mem_table = MemoryExpectation{};
    in_mem_table = false;
    mem_table_has_addr = false;
    mem_table_has_data = false;
    return true;
  };

  std::istringstream stream{std::string(toml_text)};
  std::string raw_line;
  int line_no = 0;

  while (std::getline(stream, raw_line)) {
    ++line_no;
    std::string line = StripComment(raw_line);
    if (line.empty())
      continue;

    // Handle multi-line [...] or {...} values if brackets aren't balanced yet.
    auto unbalanced = [](const std::string& s) {
      bool in_q = false;
      bool esc = false;
      int depth = 0;
      for (char c : s) {
        if (esc) {
          esc = false;
          continue;
        }
        if (c == '\\' && in_q) {
          esc = true;
          continue;
        }
        if (c == '"') {
          in_q = !in_q;
          continue;
        }
        if (!in_q) {
          if (c == '[' || c == '{')
            ++depth;
          if (c == ']' || c == '}')
            --depth;
        }
      }
      return depth > 0;
    };

    if (line.front() != '[' && line.find('=') != std::string::npos) {
      while (unbalanced(line) && std::getline(stream, raw_line)) {
        ++line_no;
        const std::string next_part = StripComment(raw_line);
        if (!next_part.empty()) {
          line += " " + next_part;
        }
      }
    }

    if (line.size() >= 4 && line.substr(0, 2) == "[[" && line.substr(line.size() - 2) == "]]") {
      if (!flush_mem_table())
        return false;
      current_section = Trim(line.substr(2, line.size() - 4));
      if (current_section == "expect.memory") {
        in_mem_table = true;
      } else {
        if (error) {
          *error = "line " + std::to_string(line_no) + ": unsupported array-of-tables [[" +
                   current_section + "]]";
        }
        return false;
      }
      continue;
    }

    if (line.size() >= 2 && line.front() == '[' && line.back() == ']') {
      if (!flush_mem_table())
        return false;
      current_section = Trim(line.substr(1, line.size() - 2));
      if (current_section != "expect" && current_section != "input") {
        if (error) {
          *error =
              "line " + std::to_string(line_no) + ": unknown section [" + current_section + "]";
        }
        return false;
      }
      continue;
    }

    const size_t eq = line.find('=');
    if (eq == std::string::npos) {
      if (error) {
        *error = "line " + std::to_string(line_no) + ": expected key = value";
      }
      return false;
    }
    const std::string key = Trim(line.substr(0, eq));
    const std::string val = Trim(line.substr(eq + 1));
    std::string parse_err;

    if (current_section.empty()) {
      if (key == "id") {
        if (!ParseTomlString(val, &out->id, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
      } else if (key == "title") {
        if (!ParseTomlString(val, &out->title, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
      } else if (key == "track") {
        if (!ParseTomlString(val, &out->track, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
      } else if (key == "source") {
        if (!ParseTomlString(val, &out->source, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
      } else if (key == "toolchain") {
        if (!ParseTomlString(val, &out->toolchain, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
      } else if (key == "level") {
        uint64_t v = 0;
        if (!ParseTomlUint64(val, &v, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
        out->level = static_cast<int>(v);
      } else if (key == "difficulty") {
        uint64_t v = 0;
        if (!ParseTomlUint64(val, &v, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
        out->difficulty = static_cast<int>(v);
      } else if (key == "max_instructions") {
        if (!ParseTomlUint64(val, &out->max_instructions, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
      } else if (key == "requires") {
        if (!ParseStringArray(val, &out->requires_ids, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
      } else if (key == "simhalt") {
        if (!ParseTomlBool(val, &out->simhalt, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
      } else if (key == "exceptions") {
        if (!ParseTomlBool(val, &out->exceptions, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
      } else if (key == "bitfield") {
        if (!ParseTomlBool(val, &out->bitfield, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
      } else if (key == "interactive") {
        if (!ParseTomlBool(val, &out->interactive, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
      } else {
        if (error) {
          *error = "line " + std::to_string(line_no) + ": unknown top-level key '" + key + "'";
        }
        return false;
      }
    } else if (current_section == "expect") {
      if (key == "halted") {
        bool b = false;
        if (!ParseTomlBool(val, &b, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
        out->expect_halted = b;
      } else if (key == "result") {
        std::string r;
        if (!ParseTomlString(val, &r, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
        out->expect_result = r;
      } else if (key == "output") {
        std::string o;
        if (!ParseTomlString(val, &o, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
        out->expect_output = o;
      } else if (key == "output_contains") {
        if (!ParseStringArray(val, &out->expect_output_contains, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
      } else if (key == "max_cycles") {
        uint64_t v = 0;
        if (!ParseTomlUint64(val, &v, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
        out->expect_max_cycles = v;
      } else if (key == "min_cycles") {
        uint64_t v = 0;
        if (!ParseTomlUint64(val, &v, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
        out->expect_min_cycles = v;
      } else if (key == "registers") {
        if (val.size() < 2 || val.front() != '{' || val.back() != '}') {
          if (error) {
            *error =
                "line " + std::to_string(line_no) + ": registers must be an inline table {...}";
          }
          return false;
        }
        const auto entries = SplitTopLevelComma(std::string_view(val).substr(1, val.size() - 2));
        for (const std::string& entry : entries) {
          const size_t reg_eq = entry.find('=');
          if (reg_eq == std::string::npos) {
            if (error) {
              *error = "line " + std::to_string(line_no) + ": invalid register entry: " + entry;
            }
            return false;
          }
          const std::string reg_name = ToUpper(Trim(entry.substr(0, reg_eq)));
          uint64_t reg_val = 0;
          if (!ParseTomlUint64(entry.substr(reg_eq + 1), &reg_val, &parse_err)) {
            if (error) {
              *error = "line " + std::to_string(line_no) + ": " + parse_err;
            }
            return false;
          }
          uint32_t dummy = 0;
          sim::CpuState dummy_state{};
          if (!GetRegisterValue(dummy_state, reg_name, &dummy)) {
            if (error) {
              *error =
                  "line " + std::to_string(line_no) + ": unknown register name '" + reg_name + "'";
            }
            return false;
          }
          out->expect_registers[reg_name] = static_cast<uint32_t>(reg_val);
        }
      } else if (key == "memory") {
        if (val.size() < 2 || val.front() != '[' || val.back() != ']') {
          if (error) {
            *error = "line " + std::to_string(line_no) +
                     ": expect.memory must be an array of inline tables";
          }
          return false;
        }
        const auto tables = SplitTopLevelComma(std::string_view(val).substr(1, val.size() - 2));
        for (const std::string& tbl : tables) {
          MemoryExpectation mem;
          if (!ParseInlineMemoryTable(tbl, &mem, &parse_err)) {
            if (error) {
              *error = "line " + std::to_string(line_no) + ": " + parse_err;
            }
            return false;
          }
          out->expect_memory.push_back(std::move(mem));
        }
      } else {
        if (error) {
          *error = "line " + std::to_string(line_no) + ": unknown [expect] key '" + key + "'";
        }
        return false;
      }
    } else if (current_section == "expect.memory") {
      if (key == "address") {
        uint64_t addr = 0;
        if (!ParseTomlUint64(val, &addr, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
        current_mem_table.address = static_cast<uint32_t>(addr);
        mem_table_has_addr = true;
      } else if (key == "bytes") {
        std::string hex_s;
        if (!ParseTomlString(val, &hex_s, &parse_err) ||
            !ParseHexByteString(hex_s, &current_mem_table.bytes, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
        mem_table_has_data = true;
      } else if (key == "text") {
        std::string txt;
        if (!ParseTomlString(val, &txt, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
        current_mem_table.bytes.assign(txt.begin(), txt.end());
        mem_table_has_data = true;
      } else {
        if (error) {
          *error =
              "line " + std::to_string(line_no) + ": unknown [[expect.memory]] key '" + key + "'";
        }
        return false;
      }
    } else if (current_section == "input") {
      if (key == "lines" || key == "text") {
        if (!ParseStringArray(val, &out->input_lines, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
      } else if (key == "chars") {
        if (!ParseTomlString(val, &out->input_chars, &parse_err)) {
          if (error)
            *error = "line " + std::to_string(line_no) + ": " + parse_err;
          return false;
        }
      } else {
        if (error) {
          *error = "line " + std::to_string(line_no) + ": unknown [input] key '" + key + "'";
        }
        return false;
      }
    }
  }

  if (!flush_mem_table())
    return false;
  return true;
}

bool LoadExampleSpecFile(const std::string& path, ExampleSpec* out, std::string* error) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    if (error)
      *error = "cannot open spec file: " + path;
    return false;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ParseExampleSpec(ss.str(), out, error);
}

ExampleRunOutcome RunExampleSpec(const ExampleSpec& spec, const std::string& base_dir,
                                 const std::string& source_override) {
  ExampleRunOutcome outcome;
  const fs::path dir_path = base_dir.empty() ? fs::current_path() : fs::path(base_dir);
  const std::string src_rel = source_override.empty() ? spec.source : source_override;
  if (src_rel.empty()) {
    outcome.failures.push_back("spec has no 'source' file");
    return outcome;
  }

  const fs::path src_path =
      fs::path(src_rel).is_absolute() ? fs::path(src_rel) : (dir_path / src_rel);
  const std::string ext_lower = ToLower(src_path.extension().string());

  std::string srecord_data;

  if (spec.toolchain == "gcc-c" || spec.toolchain == "gcc-cpp") {
    // Check if a pre-built .S68 exists alongside the source; otherwise check
    // whether m68k-linux-gnu-gcc / m68k-elf-gcc is available.
    fs::path prebuilt = src_path;
    prebuilt.replace_extension(".S68");
    if (!fs::is_regular_file(prebuilt)) {
      prebuilt.replace_extension(".s68");
    }
    if (fs::is_regular_file(prebuilt)) {
      std::ifstream in(prebuilt, std::ios::binary);
      std::ostringstream ss;
      ss << in.rdbuf();
      srecord_data = ss.str();
      outcome.assembled = true;
    } else {
      outcome.skipped = true;
      outcome.passed = true;
      outcome.skip_reason = "cross-compiler toolchain '" + spec.toolchain +
                            "' not invoked (no prebuilt .S68 and Track C builder not active)";
      return outcome;
    }
  } else if (ext_lower == ".s68") {
    auto content = ReadFileWithCaseFallback(dir_path, src_rel);
    if (!content.has_value()) {
      outcome.failures.push_back("cannot open S-record file: " + src_path.string());
      return outcome;
    }
    srecord_data = std::move(*content);
    outcome.assembled = true;
  } else {
    auto content = ReadFileWithCaseFallback(dir_path, src_rel);
    if (!content.has_value()) {
      outcome.failures.push_back("cannot open source file: " + src_path.string());
      return outcome;
    }
    asm_::Assembler assembler;
    asm_::AssemblerOptions asm_opts;
    asm_opts.generate_listing = false;
    asm_opts.generate_srecord = true;
    asm_opts.bitfield = spec.bitfield;
    assembler.SetOptions(asm_opts);
    assembler.SetFileReader([dir_path](const std::string& inc_name) -> std::string {
      auto res = ReadFileWithCaseFallback(dir_path, inc_name);
      return res.has_value() ? *res : std::string();
    });
    const asm_::AssemblyResult asm_res = assembler.Assemble(*content);
    if (!asm_res.success) {
      for (const auto& err : asm_res.errors) {
        if (err.severity == asm_::ErrorSeverity::kError) {
          outcome.failures.push_back("line " + std::to_string(err.line_number) + ": " +
                                     err.message);
        }
      }
      if (outcome.failures.empty()) {
        outcome.failures.push_back("assembly failed");
      }
      return outcome;
    }
    srecord_data = asm_res.srecord;
    outcome.assembled = true;
  }

  // Interactive examples (graphics/sound/hardware) only verify assembly.
  if (spec.interactive) {
    outcome.passed = outcome.failures.empty();
    return outcome;
  }

  ScriptedTextIO text_io(spec.input_lines, spec.input_chars);
  sim::StdFileIO file_io(dir_path.string());
  HeadlessSimEnv sim_env;
  HeadlessGraphicsIO graphics_io;
  HeadlessPeripheralIO peripheral_io;
  sim::M68kSimulator::Config config;
  config.text_io = &text_io;
  config.file_io = &file_io;
  config.sim_env = &sim_env;
  config.graphics_io = &graphics_io;
  config.peripheral_io = &peripheral_io;
  config.simhalt_enabled = spec.simhalt;
  config.exceptions_enabled = spec.exceptions;
  config.bitfield_enabled = spec.bitfield;

  sim::M68kSimulator sim(config);
  sim.SetHardwareAddresses(kSeg7Addr, kLedAddr, kSwitchAddr, kButtonAddr);
  sim.GetMemory().Erase();
  sim.GetMemory().RawData()[kSwitchAddr] = 0x00;

  std::vector<std::string> srec_lines;
  {
    std::istringstream ss(srecord_data);
    std::string srec_line;
    while (std::getline(ss, srec_line)) {
      if (!srec_line.empty() && srec_line.back() == '\r') {
        srec_line.pop_back();
      }
      if (!srec_line.empty()) {
        srec_lines.push_back(std::move(srec_line));
      }
    }
  }
  if (!sim.LoadSRecordLines(srec_lines)) {
    outcome.failures.push_back("failed to load generated S-record image");
    return outcome;
  }
  {
    sim::SRecordLoader srec_loader;
    const auto load_res = srec_loader.LoadFromLines(srec_lines, sim.GetMemory());
    static const std::string kHeader = "S0 = 68KPROG   20CREATED BY EASY68K";
    bool easy68k_s0 = false;
    bool map_changed = false;
    sim::M68kSimulator::MemoryMapConfig map{};
    for (const std::string& text : load_res.s0_texts) {
      if (!easy68k_s0) {
        easy68k_s0 = (text == kHeader);
        continue;
      }
      if (text.size() < 28)
        continue;
      char* end_s = nullptr;
      char* end_e = nullptr;
      const std::string s_hex = text.substr(15, 6);
      const std::string e_hex = text.substr(22, 6);
      const uint32_t start = static_cast<uint32_t>(std::strtoul(s_hex.c_str(), &end_s, 16));
      const uint32_t end = static_cast<uint32_t>(std::strtoul(e_hex.c_str(), &end_e, 16));
      if (end_s != s_hex.c_str() + 6 || end_e != e_hex.c_str() + 6)
        continue;
      const std::string kind = text.substr(5, 9);
      sim::M68kSimulator::MemoryMapConfig::Range* range = nullptr;
      if (kind == "      ROM")
        range = &map.rom;
      else if (kind == "     READ")
        range = &map.read;
      else if (kind == "PROTECTED")
        range = &map.protected_;
      else if (kind == "  INVALID")
        range = &map.invalid;
      if (range != nullptr) {
        *range = {true, start, end};
        map_changed = true;
      }
    }
    if (map_changed) {
      sim.ConfigureMemoryMap(map);
    }
  }
  sim.InitProgram();

  uint64_t steps = 0;
  sim::SimResult last_res = sim::SimResult::kOk;
  while (steps < spec.max_instructions) {
    last_res = sim.Step();
    ++steps;
    if (sim.State().stopped) {
      const int auto_lvl = sim_env.FirstActiveAutoIrq();
      if (auto_lvl > 0) {
        sim.Interrupt(auto_lvl);
      }
    }
    if (sim.State().halted || sim.State().stopped) {
      break;
    }
  }

  outcome.executed = true;
  outcome.last_result = last_res;
  outcome.halted = sim.State().halted;
  outcome.stopped = sim.State().stopped;
  outcome.instructions_executed = steps;
  outcome.cycles = sim.State().cycles;
  outcome.final_state = sim.State();
  outcome.output = text_io.output();

  for (const auto& [dump_addr, dump_len] : spec.dump_ranges) {
    std::vector<uint8_t> bytes;
    bytes.reserve(dump_len);
    for (uint32_t i = 0; i < dump_len; ++i) {
      bytes.push_back(sim.GetMemory().ReadByte(dump_addr + i).value);
    }
    outcome.memory_dumps.emplace_back(dump_addr, std::move(bytes));
  }

  if (steps >= spec.max_instructions && !outcome.halted && !outcome.stopped) {
    outcome.failures.push_back("exceeded max_instructions (" +
                               std::to_string(spec.max_instructions) + ") without halting");
  }

  if (spec.expect_halted.has_value() && outcome.halted != *spec.expect_halted) {
    outcome.failures.push_back(std::string("expected halted = ") +
                               (*spec.expect_halted ? "true" : "false") + ", got " +
                               (outcome.halted ? "true" : "false"));
  }

  if (spec.expect_result.has_value()) {
    const std::string actual_res = SimResultToString(outcome.last_result);
    if (actual_res != *spec.expect_result) {
      outcome.failures.push_back("expected result = \"" + *spec.expect_result + "\", got \"" +
                                 actual_res + "\"");
    }
  }

  if (spec.expect_output.has_value() && outcome.output != *spec.expect_output) {
    outcome.failures.push_back("expected output = \"" + EscapeJsonString(*spec.expect_output) +
                               "\", got \"" + EscapeJsonString(outcome.output) + "\"");
  }

  for (const std::string& substr : spec.expect_output_contains) {
    if (outcome.output.find(substr) == std::string::npos) {
      outcome.failures.push_back("output missing expected substring \"" + EscapeJsonString(substr) +
                                 "\" (output was \"" + EscapeJsonString(outcome.output) + "\")");
    }
  }

  for (const auto& [reg_name, expected_val] : spec.expect_registers) {
    uint32_t actual_val = 0;
    if (!GetRegisterValue(outcome.final_state, reg_name, &actual_val)) {
      outcome.failures.push_back("unknown register in expectation: " + reg_name);
      continue;
    }
    if (actual_val != expected_val) {
      char buf[128];
      std::snprintf(buf, sizeof(buf), "register %s: expected 0x%08X (%u), got 0x%08X (%u)",
                    reg_name.c_str(), expected_val, expected_val, actual_val, actual_val);
      outcome.failures.push_back(buf);
    }
  }

  if (spec.expect_max_cycles.has_value() && outcome.cycles > *spec.expect_max_cycles) {
    outcome.failures.push_back("cycles " + std::to_string(outcome.cycles) +
                               " exceeded max_cycles " + std::to_string(*spec.expect_max_cycles));
  }

  if (spec.expect_min_cycles.has_value() && outcome.cycles < *spec.expect_min_cycles) {
    outcome.failures.push_back("cycles " + std::to_string(outcome.cycles) + " below min_cycles " +
                               std::to_string(*spec.expect_min_cycles));
  }

  for (const MemoryExpectation& mem_exp : spec.expect_memory) {
    std::vector<uint8_t> actual_bytes;
    actual_bytes.reserve(mem_exp.bytes.size());
    for (size_t i = 0; i < mem_exp.bytes.size(); ++i) {
      actual_bytes.push_back(
          sim.GetMemory().ReadByte(mem_exp.address + static_cast<uint32_t>(i)).value);
    }
    if (actual_bytes != mem_exp.bytes) {
      char addr_buf[32];
      std::snprintf(addr_buf, sizeof(addr_buf), "0x%06X", mem_exp.address);
      outcome.failures.push_back(std::string("memory at ") + addr_buf + ": expected [" +
                                 FormatHexBytes(mem_exp.bytes) + "], got [" +
                                 FormatHexBytes(actual_bytes) + "]");
    }
  }

  outcome.passed = outcome.failures.empty();
  return outcome;
}

ExampleRunOutcome RunExampleFile(const std::string& toml_path, const std::string& source_override) {
  ExampleSpec spec;
  std::string err;
  if (!LoadExampleSpecFile(toml_path, &spec, &err)) {
    ExampleRunOutcome outcome;
    outcome.failures.push_back(err);
    return outcome;
  }
  const fs::path parent = fs::path(toml_path).parent_path();
  return RunExampleSpec(spec, parent.string(), source_override);
}

std::vector<std::pair<std::string, ExampleRunOutcome>> RunExampleDirectory(
    const std::string& dir_path) {
  std::vector<std::pair<std::string, ExampleRunOutcome>> results;
  const fs::path root(dir_path);
  const fs::path main_toml = root / "example.toml";
  if (fs::is_regular_file(main_toml)) {
    results.emplace_back(main_toml.string(), RunExampleFile(main_toml.string()));
  }

  const fs::path exercises_dir = root / "exercises";
  const fs::path solutions_dir = root / "solutions";
  std::error_code ec;
  if (fs::is_directory(exercises_dir, ec)) {
    std::vector<fs::path> ex_tomls;
    for (const auto& entry : fs::directory_iterator(exercises_dir, ec)) {
      if (entry.is_regular_file(ec) && entry.path().extension() == ".toml") {
        ex_tomls.push_back(entry.path());
      }
    }
    std::sort(ex_tomls.begin(), ex_tomls.end());
    for (const fs::path& ex_toml : ex_tomls) {
      const std::string stem = ex_toml.stem().string();
      std::string solution_rel;
      for (const char* ext : {".x68", ".X68", ".c", ".cpp", ".s68", ".S68"}) {
        const fs::path candidate = solutions_dir / (stem + ext);
        if (fs::is_regular_file(candidate, ec)) {
          solution_rel = (fs::path("..") / "solutions" / (stem + ext)).string();
          break;
        }
      }
      if (solution_rel.empty()) {
        ExampleRunOutcome fail_outcome;
        fail_outcome.failures.push_back("no matching solution file in solutions/ for " +
                                        ex_toml.string());
        results.emplace_back(ex_toml.string(), std::move(fail_outcome));
      } else {
        results.emplace_back(ex_toml.string(), RunExampleFile(ex_toml.string(), solution_rel));
      }
    }
  }
  return results;
}

std::string FormatOutcomeJson(const ExampleSpec& spec, const ExampleRunOutcome& outcome) {
  std::ostringstream os;
  os << "{\n";
  os << "  \"id\": \"" << EscapeJsonString(spec.id) << "\",\n";
  os << "  \"title\": \"" << EscapeJsonString(spec.title) << "\",\n";
  os << "  \"passed\": " << (outcome.passed ? "true" : "false") << ",\n";
  os << "  \"skipped\": " << (outcome.skipped ? "true" : "false") << ",\n";
  os << "  \"assembled\": " << (outcome.assembled ? "true" : "false") << ",\n";
  os << "  \"executed\": " << (outcome.executed ? "true" : "false") << ",\n";
  os << "  \"halted\": " << (outcome.halted ? "true" : "false") << ",\n";
  os << "  \"stopped\": " << (outcome.stopped ? "true" : "false") << ",\n";
  os << "  \"result\": \"" << SimResultToString(outcome.last_result) << "\",\n";
  os << "  \"instructions\": " << outcome.instructions_executed << ",\n";
  os << "  \"cycles\": " << outcome.cycles << ",\n";
  os << "  \"output\": \"" << EscapeJsonString(outcome.output) << "\",\n";

  char hex_buf[32];
  os << "  \"registers\": {\n";
  for (int i = 0; i < 8; ++i) {
    std::snprintf(hex_buf, sizeof(hex_buf), "0x%08X", outcome.final_state.d[i]);
    os << "    \"D" << i << "\": \"" << hex_buf << "\",\n";
  }
  for (int i = 0; i < 7; ++i) {
    std::snprintf(hex_buf, sizeof(hex_buf), "0x%08X", outcome.final_state.a[i]);
    os << "    \"A" << i << "\": \"" << hex_buf << "\",\n";
  }
  const uint32_t a7 =
      outcome.final_state.IsSupervisor() ? outcome.final_state.ssp : outcome.final_state.a[7];
  std::snprintf(hex_buf, sizeof(hex_buf), "0x%08X", a7);
  os << "    \"A7\": \"" << hex_buf << "\",\n";
  std::snprintf(hex_buf, sizeof(hex_buf), "0x%08X", outcome.final_state.pc);
  os << "    \"PC\": \"" << hex_buf << "\",\n";
  std::snprintf(hex_buf, sizeof(hex_buf), "0x%04X", outcome.final_state.sr);
  os << "    \"SR\": \"" << hex_buf << "\",\n";
  std::snprintf(hex_buf, sizeof(hex_buf), "0x%08X", outcome.final_state.a[7]);
  os << "    \"USP\": \"" << hex_buf << "\",\n";
  std::snprintf(hex_buf, sizeof(hex_buf), "0x%08X", outcome.final_state.ssp);
  os << "    \"SSP\": \"" << hex_buf << "\"\n";
  os << "  },\n";

  os << "  \"memory\": [\n";
  for (size_t i = 0; i < outcome.memory_dumps.size(); ++i) {
    const auto& [addr, bytes] = outcome.memory_dumps[i];
    std::snprintf(hex_buf, sizeof(hex_buf), "0x%06X", addr);
    os << "    { \"address\": \"" << hex_buf << "\", \"bytes\": \"" << FormatHexBytes(bytes)
       << "\" }" << (i + 1 < outcome.memory_dumps.size() ? "," : "") << "\n";
  }
  os << "  ],\n";

  os << "  \"failures\": [\n";
  for (size_t i = 0; i < outcome.failures.size(); ++i) {
    os << "    \"" << EscapeJsonString(outcome.failures[i]) << "\""
       << (i + 1 < outcome.failures.size() ? "," : "") << "\n";
  }
  os << "  ]\n";
  os << "}\n";
  return os.str();
}

}  // namespace easym68k::tools
