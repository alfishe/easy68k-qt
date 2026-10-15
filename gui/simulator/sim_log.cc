// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "sim_log.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <QDateTime>
#include <QLocale>

#include "easym68k/sim/decode.h"
#include "easym68k/sim/types.h"

namespace easym68k::gui {

namespace {

constexpr int64_t kMemSize = sim::kMemorySize;

std::string Format(const char* format, ...) {
  char text[256];
  va_list args;
  va_start(args, format);
  std::vsnprintf(text, sizeof text, format, args);
  va_end(args);
  return text;
}

// DateTimeToStr(Now()): the short date and the time with seconds.
QByteArray TimeText() {
  const QDateTime now = QDateTime::currentDateTime();
  const QLocale locale = QLocale::system();
  return (locale.toString(now.date(), QLocale::ShortFormat) + QLatin1Char(' ') +
          now.time().toString(QStringLiteral("HH:mm:ss")))
      .toLocal8Bit();
}

// Delphi Insert(): past the end appends.
void Insert(std::string* s, size_t pos, const char* text) {
  s->insert(std::min(pos, s->size()), text);
}

}  // namespace

SimLog::SimLog() = default;

SimLog::~SimLog() {
  Stop();
}

void SimLog::SetListing(const QStringList& listing) {
  std::lock_guard<std::mutex> lock(mu_);
  listing_.clear();
  listing_.reserve(static_cast<size_t>(listing.size()));
  for (const QString& line : listing)
    listing_.push_back(line.toLatin1().toStdString());
}

void SimLog::Arm() {
  std::lock_guard<std::mutex> lock(mu_);
  armed_ = true;
  dump_.clear();
  SetMode(logging_ && exec_type_ == kExecMemory ? Mode::kInstructionsRegistersAndMemory
                                                : Mode::kInstructions);
}

void SimLog::Disarm() {
  std::lock_guard<std::mutex> lock(mu_);
  armed_ = false;
  SetMode(Mode::kOff);
}

// setLogFileNames(): ChangeFileExt(name, "_RunLog.txt") / "_OutLog.txt".
void SimLog::SetFileNames(const QString& program_path) {
  const int slash = static_cast<int>(
      std::max(program_path.lastIndexOf(QLatin1Char('/')), program_path.lastIndexOf('\\')));
  const int dot = static_cast<int>(program_path.lastIndexOf(QLatin1Char('.')));
  const QString base = dot > slash ? program_path.left(dot) : program_path;
  settings_.exec_path = base + QStringLiteral("_RunLog.txt");
  settings_.output_path = base + QStringLiteral("_OutLog.txt");
}

bool SimLog::CanStart() const {
  return settings_.exec_type != kExecDisabled || settings_.output_type != kOutputDisabled;
}

bool SimLog::Logging() const {
  std::lock_guard<std::mutex> lock(mu_);
  return logging_;
}

// logU.cpp prepareLogFile()/startLog(). A cancelled or failed file stops the
// whole start (the original went on logging with the files it had).
SimLog::StartResult SimLog::Start(const std::function<Exists(const QString&)>& exists) {
  Stop();
  const QByteArray time = TimeText();
  auto open = [&](QFile* file, const QString& path, const QString& message, bool text) {
    QIODevice::OpenMode mode = QIODevice::WriteOnly;
    if (text)
      mode |= QIODevice::Text;
    if (QFile::exists(path)) {
      switch (exists(message)) {
        case Exists::kReplace:
          mode |= QIODevice::Truncate;
          break;
        case Exists::kAppend:
          mode |= QIODevice::Append;
          break;
        case Exists::kCancel:
          return StartResult::kCancelled;
      }
    }
    file->setFileName(path);
    return file->open(mode) ? StartResult::kStarted : StartResult::kExecOpenFailed;
  };
  std::lock_guard<std::mutex> lock(mu_);
  auto fail = [this](StartResult result) {
    exec_file_.close();
    output_file_.close();
    return result;
  };
  exec_type_ = settings_.exec_type;
  output_type_ = settings_.output_type;
  if (exec_type_ != kExecDisabled) {
    const StartResult r =
        open(&exec_file_, settings_.exec_path, QStringLiteral("Execution Log File exists!"), true);
    if (r != StartResult::kStarted)
      return fail(r);
    exec_file_.write("EASy68K execution log file: " + time + "\n\n");
    exec_file_.flush();
    if (exec_type_ == kExecMemory) {
      int32_t from = static_cast<int32_t>(settings_.mem_from.toUInt(nullptr, 16));
      from -= from % 16;  // $10 boundary
      int32_t bytes = static_cast<int32_t>(settings_.mem_bytes.toUInt(nullptr, 16)) + 15;
      bytes -= bytes % 16;  // whole rows
      SetDumpRange(static_cast<uint32_t>(from), static_cast<uint32_t>(bytes));
    }
  }
  if (output_type_ != kOutputDisabled) {
    StartResult r = open(&output_file_, settings_.output_path,
                         QStringLiteral("Output Log File exists!"), false);
    if (r == StartResult::kExecOpenFailed)
      r = StartResult::kOutputOpenFailed;
    if (r != StartResult::kStarted)
      return fail(r);
    output_file_.write("EASy68K output log file: " + time + "\r\n\r\n");
    output_file_.flush();
  }
  logging_ = true;
  return StartResult::kStarted;
}

void SimLog::Stop() {
  std::lock_guard<std::mutex> lock(mu_);
  exec_file_.close();
  output_file_.close();
  logging_ = false;
}

void SimLog::AddMessage(const char* text) {
  std::lock_guard<std::mutex> lock(mu_);
  if (!logging_)
    return;
  if (exec_type_ != kExecDisabled) {
    exec_file_.write(text);
    exec_file_.flush();
  }
  if (output_type_ != kOutputDisabled) {
    output_file_.write(text);
    output_file_.flush();
  }
}

void SimLog::OutputChar(char ch) {
  std::lock_guard<std::mutex> lock(mu_);
  if (logging_ && output_type_ == kOutputText) {
    output_file_.putChar(ch);
    output_file_.flush();
  }
}

// RUN.CPP exec_inst(): rows of 16 bytes from the log range before the
// instruction executes.
void SimLog::LogMemoryDump(uint32_t start, uint32_t length, const sim::Memory& memory) {
  std::lock_guard<std::mutex> lock(mu_);
  if (!armed_ || !logging_ || exec_type_ != kExecMemory)
    return;
  const uint8_t* m = memory.RawData();
  dump_ = "\n";
  int64_t addr = static_cast<int32_t>(start);
  for (uint32_t r = 0; r < length / 16; r++, addr += 16) {
    if (addr < 0 || addr >= kMemSize)
      dump_ += Format("%08X: Invalid Address", static_cast<uint32_t>(addr));
    else
      dump_ += Format("%08X: ", static_cast<uint32_t>(addr));
    for (int i = 0; i < 16; i++) {
      if (addr + i >= kMemSize)
        dump_ += "xx ";
      else
        dump_ += Format("%02X ", m[(addr + i) & sim::kAddressMask]);
    }
    for (int i = 0; i < 16; i++) {
      const auto c = static_cast<signed char>(m[(addr + i) & sim::kAddressMask]);
      dump_ += addr + i >= kMemSize || c < ' ' ? '-' : static_cast<char>(c);
    }
    dump_ += "\n";
  }
}

void SimLog::LogInstruction(uint32_t pc, uint16_t opcode, const sim::CpuState& state) {
  std::lock_guard<std::mutex> lock(mu_);
  if (!armed_)
    return;
  armed_ = false;
  SetMode(Mode::kOff);
  // The SIMHALT test needs the next word: state.pc points to it.
  bool simhalt = false;
  if (opcode == 0xFFFF && memory_ != nullptr) {
    const uint8_t* m = memory_->RawData();
    simhalt =
        m[state.pc & sim::kAddressMask] == 0xFF && m[(state.pc + 1) & sim::kAddressMask] == 0xFF;
  }
  const std::string line =
      simhalt ? Format("PC=%08X  Code=%04X  SIMHALT", pc, opcode)
              : Format("PC=%08X  Code=%04X  %s", pc, opcode, sim::InstructionName(opcode));
  if (trace_sink_)
    trace_sink_(QString::fromLatin1(line.c_str()));
  if (!logging_ || exec_type_ == kExecDisabled)
    return;
  std::string out;
  if (exec_type_ == kExecMemory)
    out += dump_;
  if (exec_type_ == kExecRegisters || exec_type_ == kExecMemory) {
    out += Format("\nD0=%08X D4=%08X A0=%08X A4=%08X    T_S__INT___XNZVC\n", state.d[0], state.d[4],
                  state.a[0], state.a[4]);
    out += Format("D1=%08X D5=%08X A1=%08X A5=%08X SR=", state.d[1], state.d[5], state.a[1],
                  state.a[5]);
    for (int j = 0; j < 16; j++)
      out += ((0x8000 >> j) & state.sr) ? '1' : '0';
    out += Format("\nD2=%08X D6=%08X A2=%08X A6=%08X US=%08X\n", state.d[2], state.d[6], state.a[2],
                  state.a[6], state.a[7]);
    out += Format("D3=%08X D7=%08X A3=%08X A7=%08X SS=%08X\n", state.d[3], state.d[7], state.a[3],
                  state.GetSP(), state.ssp);
  }
  const std::string source = ListingLine(listing_, pc);
  out += source.empty() ? line : source;
  out += "\n";
  exec_file_.write(out.data(), static_cast<qint64>(out.size()));
  exec_file_.flush();
}

void SimLog::LogOutput(const char* /*text*/, int /*length*/) {
  // The output window logs what it shows (OutputChar()).
}

// SIM68Ku.cpp lineToLog(): binary search of the listing from line 3 for
// the PC's 8 hex digits, then its last line. A probe past the end reads an
// empty line (the original raised an exception).
std::string SimLog::ListingLine(const std::vector<std::string>& listing, uint32_t pc) {
  constexpr int kTop = 3;
  const int count = static_cast<int>(listing.size());
  if (count <= kTop)
    return {};
  const std::string pc_text = Format("%08X", pc);
  auto key = [&](int i) {
    return i >= 0 && i < count ? listing[static_cast<size_t>(i)].substr(0, 8) : std::string();
  };
  int top = kTop, bot = count, mid = 0;
  bool found = false;
  do {
    mid = (bot - top) / 2 + top;
    const std::string k = key(mid);
    if (pc_text == k)
      found = true;
    else if (pc_text < k)
      bot = mid - 1;
    else
      top = mid + 1;
  } while (!found && top <= bot);
  if (!found)
    return {};
  do {
    mid++;
  } while (pc_text == key(mid));
  std::string line = listing[static_cast<size_t>(mid - 1)];
  Insert(&line, 0, "PC=");
  Insert(&line, 13, "Code=");
  Insert(&line, 41, "Line=");
  return line;
}

}  // namespace easym68k::gui
