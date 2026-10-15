// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Sim68K logging (logU.cpp, RUN.CPP exec_inst(), SIM68Ku.cpp lineToLog(),
// simIOu.cpp charOut()): the trace line of each traced instruction for the
// message pane, the execution log file (memory rows, register block,
// listing line) and the output log file (every character shown in the
// output window).
//
// The original logs only while tracing (Step, Trace Into, Auto Trace): the
// session arms the logger for the first instruction of such a command. The
// simulator calls LogMemoryDump()/LogInstruction() on its thread; the output
// window calls OutputChar(). Start()/Stop() and the settings belong to the
// GUI thread.

#ifndef EASY68K_GUI_SIMULATOR_SIM_LOG_H_
#define EASY68K_GUI_SIMULATOR_SIM_LOG_H_

#include <functional>
#include <mutex>
#include <QFile>
#include <QString>
#include <QStringList>
#include <string>
#include <vector>

#include "easym68k/sim/iexecution_logger.h"

namespace easym68k::gui {

class SimLog : public sim::ExecutionLoggerBase {
 public:
  // logU.dfm Execution Log Type (ElogFlag) and Output Log Type (OlogFlag).
  enum ExecType { kExecDisabled, kExecInstruction, kExecRegisters, kExecMemory };
  enum OutputType { kOutputDisabled, kOutputText };

  // The Log Output dialog's fields.
  struct Settings {
    int exec_type = kExecDisabled;
    int output_type = kOutputDisabled;
    QString exec_path;
    QString output_path;
    QString mem_from;   // hex, up to 8 digits
    QString mem_bytes;  // hex, up to 2 digits
  };

  // prepareLogFile(): what to do with an existing file.
  enum class Exists { kReplace, kAppend, kCancel };
  enum class StartResult { kStarted, kCancelled, kExecOpenFailed, kOutputOpenFailed };

  SimLog();
  ~SimLog() override;

  // Trace lines ("PC=%08X  Code=%04X  NAME"), called on the simulator thread.
  void SetTraceSink(std::function<void(const QString&)> sink) { trace_sink_ = std::move(sink); }
  // Memory for the SIMHALT test of the trace line.
  void SetMemory(const sim::Memory* memory) { memory_ = memory; }
  // The .L68 listing for lineToLog() (GUI thread, program not running).
  void SetListing(const QStringList& listing);

  // The next instruction executed is traced (Step Over, Trace Into);
  // Disarm() for Run.
  void Arm();
  void Disarm();

  const Settings& GetSettings() const { return settings_; }
  void SetSettings(const Settings& settings) { settings_ = settings; }
  // setLogFileNames(): <name>_RunLog.txt and <name>_OutLog.txt.
  void SetFileNames(const QString& program_path);
  // Log Start is offered when a log type is selected.
  bool CanStart() const;

  // prepareLogFile() + startLog(): opens the selected files; `exists` asks
  // about an existing file ("Execution Log File exists!" / "Output Log File
  // exists!").
  StartResult Start(const std::function<Exists(const QString& message)>& exists);
  void Stop();
  bool Logging() const;
  // addMessage(): a line in both open logs (Rewind, Reload).
  void AddMessage(const char* text);

  // simIOu.cpp: a character shown in the output window (output window thread
  // or simulator thread).
  void OutputChar(char ch);

  // ---- IExecutionLogger (simulator thread) -----------------------------------
  void LogInstruction(uint32_t pc, uint16_t opcode, const sim::CpuState& state) override;
  void LogMemoryDump(uint32_t start, uint32_t length, const sim::Memory& memory) override;
  void LogOutput(const char* text, int length) override;

  // lineToLog(): the listing line of `pc` with "PC=", "Code=" and "Line="
  // labels, or empty when the listing has no line for it.
  static std::string ListingLine(const std::vector<std::string>& listing, uint32_t pc);

 private:
  Settings settings_;
  std::function<void(const QString&)> trace_sink_;
  const sim::Memory* memory_ = nullptr;
  mutable std::mutex mu_;
  bool armed_ = false;
  bool logging_ = false;
  int exec_type_ = kExecDisabled;  // of the running log
  int output_type_ = kOutputDisabled;
  QFile exec_file_;
  QFile output_file_;
  std::string dump_;  // memory rows of the armed instruction
  std::vector<std::string> listing_;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_SIM_LOG_H_
