// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The Sim68K-Qt main window (SIM68Ku.dfm Form1): menus, toolbar and
// shortcuts as the original, the register panel, the listing, the message
// pane and the I/O window, driving a SimSession.

#ifndef EASY68K_GUI_SIMULATOR_MAINWINDOW_H_
#define EASY68K_GUI_SIMULATOR_MAINWINDOW_H_

#include <functional>
#include <memory>
#include <optional>
#include <QAction>
#include <QFont>
#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPrinter>
#include <QSettings>
#include <QTimer>
#include <vector>

#include "sim_log.h"
#include "sim_session.h"

namespace easym68k::gui {

class IoWindow;
class RegisterWidget;
class SourceView;
class StackWindow;
class MemoryWindow;
class HardwareWindow;
class BreakpointsWindow;
class FindDialog;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);
  explicit MainWindow(const QString& settings_file, QWidget* parent = nullptr);
  ~MainWindow() override;

  // Opens a program as File > Open does (no dialog).
  bool OpenFile(const QString& path);
  // SIM68Ku.cpp FormShow(): `program.S68 [/r] [/e] [/b] [/f]` (run,
  // exceptions, bit field, full screen). Run starts when the window shows.
  void ApplyArguments(const QStringList& args);

  // SIM68Ku.cpp defaultSettings(), LoadSettings(), SaveSettings().
  void DefaultSettings();
  void LoadSettings();
  void LoadSettings(QSettings& settings);
  void SaveSettings() const;
  void SaveSettings(QSettings& settings) const;
  void SetSettingsFile(const QString& path);

  // Options > Source Window Font / Output Window Font / Printer Font.
  const QFont& SourceFont() const { return source_font_; }
  void SetSourceFont(const QFont& font);
  const QFont& OutputFont() const { return output_font_; }
  void SetOutputFont(const QFont& font);
  const QFont& PrinterFont() const { return printer_font_; }
  void SetPrinterFont(const QFont& font);
  // Font dialog (current font, fixed_pitch_only); replaceable for tests.
  void SetFontDialog(std::function<std::optional<QFont>(const QFont&, bool fixed_pitch)> dialog) {
    choose_font_ = std::move(dialog);
  }

  // Auto Trace options (SIM68Ku.cpp AutoTraceOptions; Task 13.24).
  int AutoTraceInterval() const { return auto_trace_interval_; }
  void SetAutoTraceInterval(int ms);
  bool AutoTraceDisableDisplay() const { return auto_trace_disable_display_; }
  void SetAutoTraceDisableDisplay(bool disable) { auto_trace_disable_display_ = disable; }
  bool IsAutoTracing() const { return auto_trace_timer_ && auto_trace_timer_->isActive(); }

  SimSession& Session() { return *session_; }
  QString MessageText() const;  // the message pane
  // OK/Cancel confirmation ("The current debug environment will be lost");
  // replaceable for tests.
  void SetConfirm(std::function<bool(const QString&)> confirm) { confirm_ = std::move(confirm); }
  // Information/error messages (title, text); replaceable for tests.
  void SetInform(std::function<void(const QString&, const QString&)> inform) {
    inform_ = std::move(inform);
  }
  // Replace/Append/Cancel for an existing log file; replaceable for tests.
  void SetLogFileQuestion(std::function<SimLog::Exists(const QString&)> ask) {
    ask_log_file_ = std::move(ask);
  }

  // Actions, for tests.
  QAction* RunAction() const { return run_; }
  QAction* RunToCursorAction() const { return run_to_cursor_; }
  QAction* AutoTraceAction() const { return auto_trace_; }
  QAction* StepAction() const { return step_; }
  QAction* TraceAction() const { return trace_; }
  QAction* PauseAction() const { return pause_; }
  QAction* RewindAction() const { return rewind_; }
  QAction* ReloadAction() const { return reload_; }
  QAction* OpenAction() const { return open_; }
  QAction* CloseAction() const { return close_; }
  QAction* FindAction() const { return find_; }
  QAction* GotoPcAction() const { return goto_pc_; }
  QAction* LogStartAction() const { return log_start_; }
  QAction* LogStopAction() const { return log_stop_; }
  QAction* SourceFontAction() const { return source_font_action_; }
  QAction* OutputFontAction() const { return output_font_action_; }
  QAction* PrinterFontAction() const { return printer_font_action_; }
  QAction* AutoTraceOptionsAction() const { return auto_trace_options_action_; }
  QAction* BreakPointsAction() const { return break_points_action_; }
  QAction* ViewEasyBinAction() const { return view_easybin_; }
  QAction* ExceptionsAction() const { return exceptions_; }
  QAction* BitFieldAction() const { return bit_field_; }
  void SetEasyBinLauncher(std::function<bool(const QString&)> launcher) {
    easybin_launcher_ = std::move(launcher);
  }
  static QString ResolveEasyBinExecutable();
  bool LoggingShown() const { return logging_action_->isVisible(); }
  RegisterWidget* Registers() const { return registers_; }
  SourceView* Source() const { return source_; }
  StackWindow* Stack() const { return stack_window_; }
  MemoryWindow* MemoryView() const { return memory_window_; }
  IoWindow* Io() const { return io_window_; }
  HardwareWindow* Hardware() const { return hardware_window_; }
  BreakpointsWindow* Breakpoints() const { return breakpoints_window_; }
  FindDialog* FindDialogView() const { return find_dialog_; }

 protected:
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  void closeEvent(QCloseEvent* event) override;
  void showEvent(QShowEvent* event) override;

 private:
  void CreateActions();
  void CreateMenus();
  void CreateToolBar();
  void UpdateActions();               // setMenuActive / setMenuTrace / setMenuInactive
  void ShowState();                   // scrshow(): registers and listing highlight
  void SetKeyCommands(bool enabled);  // Trap #15 task 24
  QAction* Action(const QString& text, const QKeySequence& shortcut, void (MainWindow::*slot)());
  QAction* Pending(const QString& text, const QString& task);  // later task

  void OnOpen();
  void OnOpenData();
  void OnClose();
  void OnPrinterSetup();
  void OnFind();
  void OnRunToCursor();
  void OnAutoTrace();
  void OnAutoTraceTick();
  void StopAutoTrace();
  void OnGotoPc();
  void OnAbout();
  void OnSourceFont();
  void OnOutputFont();
  void OnPrinterFont();
  void OnAutoTraceOptions();
  void OnViewEasyBin();
  void OnFullscreenOptions();
  void OnLogOutput();
  void OnLogStart();
  void OnLogStop();
  void UpdateLogActions();
  void OnWindowSize(QAction* action);
  bool Confirm();
  bool Halted() const;
  static void Raise(QWidget* window);
  // Ctrl+Tab (simIOu.cpp, Memory1.cpp, Stack1.cpp, hardwareu.cpp
  // BringToFront()): the first visible window of I/O, memory, stack,
  // hardware from `index`, else the main window.
  void BringToFront(int index);

  std::unique_ptr<SimSession> session_;
  QPrinter printer_;
  QString settings_file_;
  QFont source_font_;
  QFont output_font_;
  QFont printer_font_;
  int auto_trace_interval_ = 200;
  bool auto_trace_disable_display_ = false;
  QTimer* auto_trace_timer_ = nullptr;
  IoWindow* io_window_ = nullptr;
  StackWindow* stack_window_ = nullptr;
  MemoryWindow* memory_window_ = nullptr;
  HardwareWindow* hardware_window_ = nullptr;
  BreakpointsWindow* breakpoints_window_ = nullptr;
  FindDialog* find_dialog_ = nullptr;
  RegisterWidget* registers_ = nullptr;
  SourceView* source_ = nullptr;
  QPlainTextEdit* messages_ = nullptr;
  QLabel* logging_ = nullptr;
  std::function<bool(const QString&)> confirm_;
  std::function<void(const QString&, const QString&)> inform_;
  std::function<SimLog::Exists(const QString&)> ask_log_file_;
  std::function<std::optional<QFont>(const QFont&, bool)> choose_font_;
  std::function<bool(const QString&)> easybin_launcher_;
  QAction* logging_action_ = nullptr;
  bool program_loaded_ = false;  // Reload available
  bool run_on_show_ = false;     // command line /r

  QAction* open_ = nullptr;
  QAction* open_data_ = nullptr;
  QAction* close_ = nullptr;
  QAction* printer_setup_ = nullptr;
  QAction* exit_ = nullptr;
  QAction* find_ = nullptr;
  QAction* goto_pc_ = nullptr;
  QAction* run_ = nullptr;
  QAction* run_to_cursor_ = nullptr;
  QAction* auto_trace_ = nullptr;
  QAction* step_ = nullptr;
  QAction* trace_ = nullptr;
  QAction* pause_ = nullptr;
  QAction* rewind_ = nullptr;
  QAction* reload_ = nullptr;
  QAction* log_start_ = nullptr;
  QAction* log_stop_ = nullptr;
  QAction* output_window_ = nullptr;
  QAction* break_points_action_ = nullptr;
  QAction* view_easybin_ = nullptr;
  QAction* source_font_action_ = nullptr;
  QAction* output_font_action_ = nullptr;
  QAction* printer_font_action_ = nullptr;
  QAction* auto_trace_options_action_ = nullptr;
  QAction* exceptions_ = nullptr;
  QAction* bit_field_ = nullptr;
  QAction* text_wrap_ = nullptr;
  std::vector<QAction*> window_sizes_;
  // Actions whose shortcuts task 24 disables, with their shortcuts.
  std::vector<std::pair<QAction*, QKeySequence>> key_commands_;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_MAINWINDOW_H_
