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
#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPrinter>
#include <vector>

#include "sim_session.h"

namespace easym68k::gui {

class IoWindow;
class RegisterWidget;
class SourceView;
class StackWindow;
class MemoryWindow;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

  // Opens a program as File > Open does (no dialog).
  bool OpenFile(const QString& path);
  // SIM68Ku.cpp FormShow(): `program.S68 [/r] [/e] [/b] [/f]` (run,
  // exceptions, bit field, full screen). Run starts when the window shows.
  void ApplyArguments(const QStringList& args);

  SimSession& Session() { return *session_; }
  QString MessageText() const;  // the message pane
  // OK/Cancel confirmation ("The current debug environment will be lost");
  // replaceable for tests.
  void SetConfirm(std::function<bool(const QString&)> confirm) { confirm_ = std::move(confirm); }

  // Actions, for tests.
  QAction* RunAction() const { return run_; }
  QAction* StepAction() const { return step_; }
  QAction* TraceAction() const { return trace_; }
  QAction* PauseAction() const { return pause_; }
  QAction* RewindAction() const { return rewind_; }
  QAction* ReloadAction() const { return reload_; }
  QAction* OpenAction() const { return open_; }
  QAction* CloseAction() const { return close_; }
  RegisterWidget* Registers() const { return registers_; }
  SourceView* Source() const { return source_; }
  StackWindow* Stack() const { return stack_window_; }
  MemoryWindow* MemoryView() const { return memory_window_; }

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
  void OnRunToCursor();
  void OnGotoPc();
  void OnAbout();
  void OnWindowSize(QAction* action);
  bool Confirm();
  bool Halted() const;
  static void Raise(QWidget* window);

  std::unique_ptr<SimSession> session_;
  QPrinter printer_;
  IoWindow* io_window_ = nullptr;
  StackWindow* stack_window_ = nullptr;
  MemoryWindow* memory_window_ = nullptr;
  RegisterWidget* registers_ = nullptr;
  SourceView* source_ = nullptr;
  QPlainTextEdit* messages_ = nullptr;
  QLabel* logging_ = nullptr;
  std::function<bool(const QString&)> confirm_;
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
  QAction* exceptions_ = nullptr;
  QAction* bit_field_ = nullptr;
  QAction* text_wrap_ = nullptr;
  std::vector<QAction*> window_sizes_;
  // Actions whose shortcuts task 24 disables, with their shortcuts.
  std::vector<std::pair<QAction*, QKeySequence>> key_commands_;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_MAINWINDOW_H_
