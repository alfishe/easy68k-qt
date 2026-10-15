// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mainwindow.h"

#include <algorithm>
#include <iterator>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QGuiApplication>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPageSetupDialog>
#include <QProcess>
#include <QScreen>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
#include <QToolBar>
#include <QVBoxLayout>

#include "auto_trace_options_dialog.h"
#include "breakpoints_window.h"
#include "find_dialog.h"
#include "fullscreen_options_dialog.h"
#include "hardware_window.h"
#include "io_window.h"
#include "log_window.h"
#include "memory_window.h"
#include "qt_print_io.h"
#include "qt_simulator_env.h"
#include "register_widget.h"
#include "sim_io_screen.h"
#include "sim_log.h"
#include "source_view.h"
#include "stack_window.h"

namespace easym68k::gui {

namespace {

const char kLostWarning[] = "The current debug environment will be lost, click OK to continue.";
const char kS68Filter[] = "EASy68K S-Record (*.S68 *.s68);;All Files (*)";

// def.h default window locations and sizes.
constexpr int kMainTop = 100;
constexpr int kMainLeft = 100;
constexpr int kMainWidth = 744;
constexpr int kMainHeight = 500;
constexpr int kSimIoTop = 300;
constexpr int kSimIoLeft = 200;
constexpr int kMemoryTop = 80;
constexpr int kMemoryLeft = 280;
constexpr int kStackTop = 200;
constexpr int kStackLeft = 40;
constexpr int kStackHeight = 538;
constexpr int kHardwareTop = 100;
constexpr int kHardwareLeft = 240;

QFont MakeFixedFont(const QString& family, int point_size) {
  QFont font(family);
  font.setPointSize(point_size);
  font.setStyleHint(QFont::TypeWriter);
  font.setFixedPitch(true);
  return font;
}

void SaveFont(QSettings& settings, const QString& prefix, const QFont& font) {
  settings.setValue(prefix + QStringLiteral("_name"), font.family());
  settings.setValue(prefix + QStringLiteral("_size"), font.pointSize() > 0 ? font.pointSize() : 10);
  settings.setValue(prefix + QStringLiteral("_bold"), font.bold());
  settings.setValue(prefix + QStringLiteral("_italic"), font.italic());
  settings.setValue(prefix + QStringLiteral("_underline"), font.underline());
  settings.setValue(prefix + QStringLiteral("_strikeout"), font.strikeOut());
}

QFont LoadFont(const QSettings& settings, const QString& prefix, const QFont& def,
               bool fixed_pitch) {
  const QString family = settings.value(prefix + QStringLiteral("_name"), def.family()).toString();
  const int size = settings.value(prefix + QStringLiteral("_size"), def.pointSize()).toInt();
  QFont font(family.isEmpty() ? def.family() : family);
  font.setPointSize(size > 0 ? size : def.pointSize());
  if (fixed_pitch) {
    font.setStyleHint(QFont::TypeWriter);
    font.setFixedPitch(true);
  }
  font.setBold(settings.value(prefix + QStringLiteral("_bold"), def.bold()).toBool());
  font.setItalic(settings.value(prefix + QStringLiteral("_italic"), def.italic()).toBool());
  font.setUnderline(
      settings.value(prefix + QStringLiteral("_underline"), def.underline()).toBool());
  font.setStrikeOut(
      settings.value(prefix + QStringLiteral("_strikeout"), def.strikeOut()).toBool());
  return font;
}

int ClampCoord(int value, int def, int limit) {
  return (value < 0 || value > limit - 20) ? def : value;
}

int ClampDimension(int value, int def, int limit) {
  if (value <= 0)
    return def;
  return std::min(value, limit);
}

QString DefaultSettingsPath() {
  if (QCoreApplication::organizationName() != QLatin1String("EASy68K"))
    return QString();
  return QSettings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("EASy68K"),
                   QStringLiteral("Sim68K"))
      .fileName();
}

}  // namespace

MainWindow::MainWindow(QWidget* parent) : MainWindow(DefaultSettingsPath(), parent) {}

MainWindow::MainWindow(const QString& settings_file, QWidget* parent)
    : QMainWindow(parent), settings_file_(settings_file) {
  SimSession::Services services;
  services.file_dialog = [this](bool save, const QString& title, const QString& filter,
                                const QString& path) -> std::optional<QString> {
    const QString name = save ? QFileDialog::getSaveFileName(this, title, path, filter)
                              : QFileDialog::getOpenFileName(this, title, path, filter);
    if (name.isEmpty())
      return std::nullopt;
    return name;
  };
  services.printer = [this]() -> std::unique_ptr<QPagedPaintDevice> {
    auto printer = std::make_unique<QPrinter>(QPrinter::HighResolution);
    printer->setPrinterName(printer_.printerName());
    printer->setPageLayout(printer_.pageLayout());
    return printer;
  };
  session_ = std::make_unique<SimSession>(services);
  confirm_ = [this](const QString& text) {
    return QMessageBox::warning(this, QStringLiteral("Caution"), text,
                                QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok;
  };
  inform_ = [this](const QString& title, const QString& text) {
    QMessageBox box(QMessageBox::NoIcon, title, text, QMessageBox::Ok, this);
    box.exec();
  };
  ask_log_file_ = [this](const QString& message) {
    LogFileExistsDialog dialog(message, this);
    dialog.exec();
    return dialog.Choice();
  };
  choose_font_ = [this](const QFont& current, bool fixed_pitch) -> std::optional<QFont> {
    bool ok = false;
    const QFontDialog::FontDialogOptions opts =
        fixed_pitch ? QFontDialog::MonospacedFonts : QFontDialog::FontDialogOptions();
    const QFont chosen = QFontDialog::getFont(&ok, current, this, QStringLiteral("Font"), opts);
    if (!ok)
      return std::nullopt;
    return chosen;
  };
  easybin_launcher_ = [](const QString& program) {
    return !program.isEmpty() && QProcess::startDetached(program, {});
  };

  setWindowTitle(QStringLiteral("Sim68K"));
  setAcceptDrops(true);

  auto* central = new QWidget(this);
  auto* layout = new QVBoxLayout(central);
  registers_ = new RegisterWidget(central);
  layout->addWidget(registers_);
  auto* splitter = new QSplitter(Qt::Vertical, central);
  source_ = new SourceView(splitter);
  messages_ = new QPlainTextEdit(splitter);
  messages_->setReadOnly(true);
  messages_->setMaximumBlockCount(1000);
  splitter->setStretchFactor(0, 4);
  splitter->setStretchFactor(1, 1);
  layout->addWidget(splitter, 1);
  setCentralWidget(central);
  statusBar();

  io_window_ = new IoWindow(&session_->Screen(), &session_->Peripheral(), this);
  stack_window_ = new StackWindow(
      &session_->Simulator().GetMemory(), [this] { return !session_->IsRunning(); }, this);
  MemoryWindow::Access access;
  access.readable = [this] { return !session_->IsRunning(); };
  access.request_snapshot = [this](uint32_t address, uint32_t size) {
    session_->RequestMemorySnapshot(address, size);
  };
  memory_window_ = new MemoryWindow(&session_->Simulator().GetMemory(), access, this);
  connect(session_.get(), &SimSession::MemorySnapshot, memory_window_, &MemoryWindow::OnSnapshot);
  hardware_window_ = new HardwareWindow(session_.get(), this);
  breakpoints_window_ = new BreakpointsWindow(session_.get(), this);
  find_dialog_ = new FindDialog(this);
  connect(find_dialog_, &FindDialog::FindRequested, this,
          [this](const QString& text, bool next) { source_->Find(text, next); });
  auto_trace_timer_ = new QTimer(this);
  auto_trace_timer_->setInterval(auto_trace_interval_);
  connect(auto_trace_timer_, &QTimer::timeout, this, &MainWindow::OnAutoTraceTick);
  // Memory1.cpp: an edit refreshes the stack and hardware windows; a switch
  // or button repaints the memory and stack windows.
  connect(memory_window_, &MemoryWindow::MemoryEdited, this, [this] {
    stack_window_->UpdateState(session_->Simulator().State());
    hardware_window_->Refresh();
  });
  connect(hardware_window_, &HardwareWindow::MemoryChanged, this, [this] {
    memory_window_->Refresh();
    stack_window_->UpdateState(session_->Simulator().State());
  });
  // Ctrl+Tab: I/O -> memory -> stack -> hardware -> break points -> main window.
  connect(io_window_, &IoWindow::NextWindowRequested, this, [this] { BringToFront(1); });
  connect(memory_window_, &MemoryWindow::NextWindowRequested, this, [this] { BringToFront(2); });
  connect(stack_window_, &StackWindow::NextWindowRequested, this, [this] { BringToFront(3); });
  connect(hardware_window_, &HardwareWindow::NextWindowRequested, this,
          [this] { BringToFront(4); });
  connect(breakpoints_window_, &BreakpointsWindow::NextWindowRequested, this,
          [this] { BringToFront(5); });
  // Trap #15 task 33: windowed (1) or full screen (2) output window.
  connect(&session_->Env(), &QtSimulatorEnv::FullScreenRequested, io_window_,
          &IoWindow::SetFullScreen);
  // Trap #15 task 32.0: Hardware->Show().
  connect(&session_->Env(), &QtSimulatorEnv::ShowHardwareRequested, this,
          [this] { Raise(hardware_window_); });

  CreateActions();
  CreateMenus();
  CreateToolBar();
  // FormKeyDown(): Ctrl+Tab goes to the output window (not while task 24
  // disables the shortcut keys).
  auto* next_window = new QAction(this);
  next_window->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab));
  next_window->setShortcutContext(Qt::WindowShortcut);
  addAction(next_window);
  key_commands_.emplace_back(next_window, next_window->shortcut());
  connect(next_window, &QAction::triggered, this, [this] { BringToFront(0); });

  connect(session_.get(), &SimSession::StateChanged, this, [this] {
    UpdateActions();
    registers_->SetEditable(!session_->IsRunning() && !IsAutoTracing());
  });
  // SIM68Ku.cpp loadRegs()/register key handlers: edits go to the CPU.
  connect(registers_, &RegisterWidget::Edited, this, [this](const sim::CpuState& edited) {
    if (session_->IsRunning() || IsAutoTracing())
      return;
    sim::CpuState& cpu = session_->Simulator().State();
    std::copy(std::begin(edited.d), std::end(edited.d), std::begin(cpu.d));
    std::copy(std::begin(edited.a), std::end(edited.a), std::begin(cpu.a));
    cpu.usp = edited.a[7];
    cpu.ssp = edited.ssp;
    cpu.pc = edited.pc;
    cpu.sr = edited.sr;
    if (!(IsAutoTracing() && auto_trace_disable_display_)) {
      source_->HighlightPc(cpu.pc, Halted());  // regPCChange()
      stack_window_->UpdateState(cpu);         // StackFrm->updateDisplay()
    }
  });
  connect(registers_, &RegisterWidget::ClearCycles, this, [this] {
    if (session_->IsRunning() || IsAutoTracing())
      return;
    session_->Simulator().State().cycles = 0;
    ShowState();
  });
  connect(registers_, &RegisterWidget::PcDoubleClicked, this, &MainWindow::OnGotoPc);
  // regDblClick(): the memory window at the register's address.
  connect(registers_, &RegisterWidget::AddressDoubleClicked, this, [this](uint32_t address) {
    memory_window_->ShowAddress(address);
    Raise(memory_window_);
  });
  // Listing: breakpoint gutter and double click (SIM68Ku.cpp breakP, ListBox1).
  source_->SetBreakpointQuery([this](uint32_t a) { return session_->HasBreakpoint(a); });
  connect(source_, &SourceView::BreakpointToggled, session_.get(), &SimSession::ToggleBreakpoint);
  connect(source_, &SourceView::ClearAllBreakpoints, session_.get(), &SimSession::ClearBreakpoints);
  connect(session_.get(), &SimSession::BreakpointsChanged, source_,
          &SourceView::RefreshBreakpoints);
  connect(source_, &SourceView::PcRequested, this, [this](uint32_t address) {
    if (session_->IsRunning() || IsAutoTracing())
      return;
    session_->Simulator().State().pc = address;
    ShowState();
  });
  connect(session_.get(), &SimSession::ExecutionStopped, this, [this] {
    if (IsAutoTracing()) {
      const uint32_t pc = session_->Simulator().State().pc;
      if (session_->State() != SimSession::RunState::kReady || session_->HasBreakpoint(pc) ||
          session_->Simulator().AdvancedBreakpointHit()) {
        auto_trace_timer_->stop();
        UpdateActions();
        registers_->SetEditable(!session_->IsRunning());
      }
    }
    ShowState();
  });
  connect(session_.get(), &SimSession::Message, messages_, &QPlainTextEdit::appendPlainText);
  connect(session_.get(), &SimSession::MessagesCleared, messages_, &QPlainTextEdit::clear);
  connect(session_.get(), &SimSession::OptionsChanged, this, [this] {
    {
      const QSignalBlocker b1(exceptions_);
      const QSignalBlocker b2(bit_field_);
      exceptions_->setChecked(session_->ExceptionsEnabled());
      bit_field_->setChecked(session_->BitFieldEnabled());
    }
    SaveSettings();  // CODE9.CPP task 32 subtask 5
  });
  connect(session_.get(), &SimSession::ProgramLoaded, this, [this] {
    auto_trace_timer_->stop();
    program_loaded_ = true;
    hardware_window_->Initialize();  // Hardware->initialize()
    source_->SetListing(session_->Listing(), session_->HasListing());
    setWindowTitle(session_->ProgramPath());
    UpdateActions();
  });
  // SIMHALT / task 9: stopLogWithAnnounce().
  connect(session_.get(), &SimSession::ProgramEnded, this, [this] {
    auto_trace_timer_->stop();
    if (!session_->Log().Logging())
      return;
    inform_(QStringLiteral("Information"), QStringLiteral("Current Log Stopped."));
    session_->Log().Stop();
    UpdateLogActions();
  });
  connect(session_.get(), &SimSession::ProgramClosed, this, [this] {
    auto_trace_timer_->stop();
    source_->SetListing(session_->Listing(), false);
    UpdateActions();
  });
  connect(&session_->Screen(), &SimIoScreen::KeyCommandsChanged, this, &MainWindow::SetKeyCommands);
  connect(&session_->Screen(), &SimIoScreen::WindowSizeChanged, this, [this](int w, int h) {
    // SIM68Ku.cpp WindowSizeChecked(): the matching size is checked.
    const QString size = QStringLiteral("%1x%2").arg(w).arg(h);
    for (QAction* a : window_sizes_)
      a->setChecked(a->text() == size);
  });

  LoadSettings();
  UpdateActions();
  UpdateLogActions();
  ShowState();
}

MainWindow::~MainWindow() = default;

// ---------------------------------------------------------------------------
// Actions and menus (SIM68Ku.dfm ActionList1, MainMenu1, ToolBar1)
// ---------------------------------------------------------------------------

QAction* MainWindow::Action(const QString& text, const QKeySequence& shortcut,
                            void (MainWindow::*slot)()) {
  auto* a = new QAction(text, this);
  if (!shortcut.isEmpty()) {
    a->setShortcut(shortcut);
    a->setShortcutContext(Qt::ApplicationShortcut);
    key_commands_.emplace_back(a, shortcut);
  }
  if (slot != nullptr)
    connect(a, &QAction::triggered, this, slot);
  return a;
}

QAction* MainWindow::Pending(const QString& text, const QString& task) {
  auto* a = new QAction(text, this);
  a->setEnabled(false);
  a->setToolTip(QStringLiteral("Not available yet (Task %1)").arg(task));
  a->setStatusTip(a->toolTip());
  return a;
}

void MainWindow::CreateActions() {
  open_ =
      Action(QStringLiteral("&Open..."), QKeySequence(Qt::CTRL | Qt::Key_O), &MainWindow::OnOpen);
  open_data_ = Action(QStringLiteral("Open &Data..."), {}, &MainWindow::OnOpenData);
  close_ = Action(QStringLiteral("&Close"), {}, &MainWindow::OnClose);
  printer_setup_ = Action(QStringLiteral("&Printer Setup..."), {}, &MainWindow::OnPrinterSetup);
  exit_ = Action(QStringLiteral("E&xit"), {}, nullptr);
  connect(exit_, &QAction::triggered, this, &QWidget::close);
  find_ =
      Action(QStringLiteral("&Find..."), QKeySequence(Qt::CTRL | Qt::Key_F), &MainWindow::OnFind);
  auto* find_next = new QAction(this);
  find_next->setShortcut(QKeySequence(Qt::Key_F3));
  find_next->setShortcutContext(Qt::WindowShortcut);
  addAction(find_next);
  connect(find_next, &QAction::triggered, this, [this] {
    if (!find_dialog_->Text().isEmpty())
      source_->Find(find_dialog_->Text(), true);
  });
  goto_pc_ = Action(QStringLiteral("&Go to PC"), QKeySequence(Qt::CTRL | Qt::Key_G),
                    &MainWindow::OnGotoPc);
  run_ = Action(QStringLiteral("&Run"), QKeySequence(Qt::Key_F9), nullptr);
  connect(run_, &QAction::triggered, this, [this] { session_->Run(); });
  run_to_cursor_ = Action(QStringLiteral("Run To &Cursor"), QKeySequence(Qt::CTRL | Qt::Key_F9),
                          &MainWindow::OnRunToCursor);
  auto_trace_ =
      Action(QStringLiteral("&Auto Trace"), QKeySequence(Qt::Key_F10), &MainWindow::OnAutoTrace);
  step_ = Action(QStringLiteral("&Step Over"), QKeySequence(Qt::Key_F8), nullptr);
  connect(step_, &QAction::triggered, this, [this] { session_->StepOver(); });
  trace_ = Action(QStringLiteral("&Trace Into"), QKeySequence(Qt::Key_F7), nullptr);
  connect(trace_, &QAction::triggered, this, [this] { session_->TraceInto(); });
  pause_ = Action(QStringLiteral("&Pause"), QKeySequence(Qt::Key_F6), nullptr);
  connect(pause_, &QAction::triggered, this, [this] {
    StopAutoTrace();
    session_->Pause();
  });
  rewind_ = Action(QStringLiteral("R&ewind Program"), QKeySequence(Qt::CTRL | Qt::Key_F2), nullptr);
  connect(rewind_, &QAction::triggered, this, [this] {
    StopAutoTrace();
    session_->Rewind();
  });
  reload_ = Action(QStringLiteral("Re&load Program"), QKeySequence(Qt::CTRL | Qt::Key_F3), nullptr);
  connect(reload_, &QAction::triggered, this, [this] {
    StopAutoTrace();
    session_->Reload();
  });
  log_start_ = Action(QStringLiteral("L&og Start"), QKeySequence(Qt::CTRL | Qt::Key_F4),
                      &MainWindow::OnLogStart);
  log_stop_ = Action(QStringLiteral("Lo&g Stop"), QKeySequence(Qt::CTRL | Qt::Key_F5),
                     &MainWindow::OnLogStop);

  const QStyle* style = this->style();
  open_->setIcon(style->standardIcon(QStyle::SP_DialogOpenButton));
  run_->setIcon(style->standardIcon(QStyle::SP_MediaPlay));
  pause_->setIcon(style->standardIcon(QStyle::SP_MediaPause));
  step_->setIcon(style->standardIcon(QStyle::SP_ArrowRight));
  trace_->setIcon(style->standardIcon(QStyle::SP_ArrowDown));
  run_to_cursor_->setIcon(style->standardIcon(QStyle::SP_MediaSeekForward));
  auto_trace_->setIcon(style->standardIcon(QStyle::SP_CommandLink));
  rewind_->setIcon(style->standardIcon(QStyle::SP_MediaSkipBackward));
  reload_->setIcon(style->standardIcon(QStyle::SP_BrowserReload));
}

void MainWindow::CreateMenus() {
  QMenu* file = menuBar()->addMenu(QStringLiteral("&File"));
  file->addAction(open_);
  file->addAction(open_data_);
  file->addSeparator();
  file->addAction(close_);
  file->addSeparator();
  file->addAction(printer_setup_);
  file->addSeparator();
  file->addAction(exit_);

  QMenu* search = menuBar()->addMenu(QStringLiteral("&Search"));
  search->addAction(find_);
  search->addSeparator();
  search->addAction(goto_pc_);

  QMenu* run = menuBar()->addMenu(QStringLiteral("&Run"));
  run->addAction(run_);
  run->addSeparator();
  run->addAction(run_to_cursor_);
  run->addAction(auto_trace_);
  run->addAction(step_);
  run->addAction(trace_);
  run->addAction(pause_);
  run->addSeparator();
  run->addAction(rewind_);
  run->addAction(reload_);
  run->addSeparator();
  run->addAction(log_start_);
  run->addAction(log_stop_);

  QMenu* view = menuBar()->addMenu(QStringLiteral("&View"));
  output_window_ = view->addAction(QStringLiteral("&Output Window"));
  // OutputWindow1Click(): leaves full screen, else shows the window.
  connect(output_window_, &QAction::triggered, this, [this] {
    if (io_window_->FullScreen()) {
      io_window_->SetFullScreen(false);
      return;
    }
    io_window_->show();
    io_window_->raise();
    io_window_->activateWindow();
  });
  QAction* memory = view->addAction(QStringLiteral("&Memory"));
  connect(memory, &QAction::triggered, this, [this] { Raise(memory_window_); });
  QAction* stack = view->addAction(QStringLiteral("&Stack"));
  connect(stack, &QAction::triggered, this, [this] { Raise(stack_window_); });
  QAction* hardware = view->addAction(QStringLiteral("&Hardware"));
  connect(hardware, &QAction::triggered, this, [this] { Raise(hardware_window_); });
  break_points_action_ = view->addAction(QStringLiteral("&Break Points"));
  connect(break_points_action_, &QAction::triggered, this, [this] { Raise(breakpoints_window_); });
  view_easybin_ = view->addAction(QStringLiteral("&EASyBIN"), this, &MainWindow::OnViewEasyBin);

  QMenu* options = menuBar()->addMenu(QStringLiteral("&Options"));
  source_font_action_ =
      options->addAction(QStringLiteral("&Source Window Font..."), this, &MainWindow::OnSourceFont);
  output_font_action_ =
      options->addAction(QStringLiteral("&Output Window Font..."), this, &MainWindow::OnOutputFont);
  printer_font_action_ =
      options->addAction(QStringLiteral("&Printer Font..."), this, &MainWindow::OnPrinterFont);
  options->addSeparator();
  QAction* log_output = options->addAction(QStringLiteral("&Log Output..."));
  connect(log_output, &QAction::triggered, this, &MainWindow::OnLogOutput);
  options->addSeparator();
  exceptions_ = options->addAction(QStringLiteral("&Enable Exceptions"));
  exceptions_->setCheckable(true);
  exceptions_->setChecked(session_->ExceptionsEnabled());
  connect(exceptions_, &QAction::toggled, this, [this](bool on) {
    session_->SetExceptionsEnabled(on);
    SaveSettings();
  });
  options->addSeparator();
  auto_trace_options_action_ = options->addAction(QStringLiteral("&Auto Trace Options..."), this,
                                                  &MainWindow::OnAutoTraceOptions);
  options->addSeparator();
  QAction* fullscreen = options->addAction(QStringLiteral("&Fullscreen Options..."));
  connect(fullscreen, &QAction::triggered, this, &MainWindow::OnFullscreenOptions);
  options->addSeparator();
  bit_field_ = options->addAction(QStringLiteral("Enable &Bit Field Instructions"));
  bit_field_->setCheckable(true);
  bit_field_->setChecked(session_->BitFieldEnabled());
  connect(bit_field_, &QAction::toggled, this, [this](bool on) {
    session_->SetBitFieldEnabled(on);
    SaveSettings();
  });
  options->addSeparator();
  QMenu* sizes = options->addMenu(QStringLiteral("Output Window &Size"));
  auto* group = new QActionGroup(this);
  for (const char* size : {"640x480", "800x600", "1024x768", "1280x800", "1280x1024", "1440x900",
                           "1680x1050", "1920x1080", "1920x1200"}) {
    QAction* a = sizes->addAction(QString::fromLatin1(size));
    a->setCheckable(true);
    group->addAction(a);
    window_sizes_.push_back(a);
  }
  window_sizes_.front()->setChecked(true);
  connect(group, &QActionGroup::triggered, this, &MainWindow::OnWindowSize);
  text_wrap_ = options->addAction(QStringLiteral("Output Window Text &Wrap"));
  text_wrap_->setCheckable(true);
  connect(text_wrap_, &QAction::toggled, this,
          [this](bool on) { session_->Screen().SetTextWrap(on); });

  QMenu* help = menuBar()->addMenu(QStringLiteral("&Help"));
  help->addAction(Pending(QStringLiteral("&Sim68K Help"), QStringLiteral("13.25")));
  help->addAction(QStringLiteral("&About Sim68K"), this, &MainWindow::OnAbout);
}

void MainWindow::CreateToolBar() {
  QToolBar* bar = addToolBar(QStringLiteral("ToolBar1"));
  bar->setObjectName(QStringLiteral("ToolBar1"));
  bar->addAction(open_);
  bar->addSeparator();
  bar->addAction(run_);
  bar->addSeparator();
  bar->addAction(run_to_cursor_);
  bar->addAction(auto_trace_);
  bar->addAction(step_);
  bar->addAction(trace_);
  bar->addAction(pause_);
  bar->addSeparator();
  bar->addAction(rewind_);
  bar->addSeparator();
  bar->addAction(reload_);
  bar->addSeparator();
  bar->addAction(log_start_);
  bar->addAction(log_stop_);
  logging_ = new QLabel(QStringLiteral("LOGGING"), bar);  // LoggingLbl
  logging_->setAlignment(Qt::AlignCenter);
  logging_->setStyleSheet(QStringLiteral("color: black; background: yellow; padding: 2px 6px"));
  logging_action_ = bar->addWidget(logging_);
  logging_action_->setVisible(false);
}

// SIM68Ku.cpp setMenuActive() / setMenuTrace() / setMenuInactive().
void MainWindow::UpdateActions() {
  using S = SimSession::RunState;
  const S state = session_->State();
  const bool running = state == S::kRunning || IsAutoTracing();
  const bool active = !running && (state == S::kReady || state == S::kStopped);
  const bool loaded = state != S::kNoProgram;
  open_->setEnabled(!running);
  open_data_->setEnabled(!running);
  close_->setEnabled(loaded);
  for (QAction* a : {run_, run_to_cursor_, auto_trace_, step_, trace_})
    a->setEnabled(active);
  pause_->setEnabled(loaded);
  rewind_->setEnabled(loaded);
  reload_->setEnabled(program_loaded_ && !session_->ProgramPath().isEmpty() && !running);
}

// SIM68Ku.cpp scrshow(): registers and the PC's listing line.
void MainWindow::ShowState() {
  if (session_->IsRunning())
    return;
  const sim::CpuState& cpu = session_->Simulator().State();
  registers_->Show(cpu);
  if (!(IsAutoTracing() && auto_trace_disable_display_)) {
    source_->HighlightPc(cpu.pc, Halted());
    stack_window_->UpdateState(cpu);
    memory_window_->Refresh();
  }
  hardware_window_->Refresh();
}

// SIM68Ku.cpp setMenuTask19()/restoreMenuTask19().
void MainWindow::SetKeyCommands(bool enabled) {
  for (auto& [action, shortcut] : key_commands_)
    action->setShortcut(enabled ? shortcut : QKeySequence());
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

// OpenFile() + Log->setLogFileNames(): a running log stops; the log files
// are named after the program.
bool MainWindow::OpenFile(const QString& path) {
  const bool opened = session_->Open(path);
  SimLog& log = session_->Log();
  if (log.Logging()) {
    log.Stop();
    inform_(QStringLiteral("Information"), QStringLiteral("Current Log Stopped."));
  }
  log.SetFileNames(path);
  UpdateLogActions();
  return opened;
}

void MainWindow::ApplyArguments(const QStringList& args) {
  if (args.isEmpty())
    return;
  const QString file = args.first();
  if (QFileInfo(file).suffix().compare(QLatin1String("S68"), Qt::CaseInsensitive) == 0 &&
      QFileInfo::exists(file))
    OpenFile(file);
  for (int i = 1; i < args.size(); i++) {
    const QString option = args[i].toLower();
    if (option == QLatin1String("/r")) {
      run_on_show_ = true;
    } else if (option == QLatin1String("/e")) {
      exceptions_->setChecked(true);
    } else if (option == QLatin1String("/b")) {
      bit_field_->setChecked(true);
    } else if (option == QLatin1String("/f")) {
      io_window_->SetFullScreen(true);  // shown full screen
    }
  }
}

// SIM68Ku.cpp FormActivate(): the command line's /r runs the program.
void MainWindow::showEvent(QShowEvent* event) {
  QMainWindow::showEvent(event);
  if (run_on_show_) {
    run_on_show_ = false;
    QMetaObject::invokeMethod(this, [this] { session_->Run(); }, Qt::QueuedConnection);
  }
}

bool MainWindow::Confirm() {
  return confirm_(QString::fromLatin1(kLostWarning));
}

void MainWindow::OnOpen() {
  const QString start = session_->ProgramPath().isEmpty()
                            ? QString()
                            : QFileInfo(session_->ProgramPath()).absolutePath();
  const QString name = QFileDialog::getOpenFileName(this, QStringLiteral("Open"), start,
                                                    QString::fromLatin1(kS68Filter));
  if (!name.isEmpty())
    OpenFile(name);
}

void MainWindow::OnOpenData() {
  const QString name = QFileDialog::getOpenFileName(this, QStringLiteral("Open Data"), QString(),
                                                    QString::fromLatin1(kS68Filter));
  if (!name.isEmpty())
    session_->OpenData(name);
}

void MainWindow::OnClose() {
  if (Confirm())
    session_->Close();
}

void MainWindow::OnPrinterSetup() {
  QPageSetupDialog dialog(&printer_, this);
  dialog.exec();
}

void MainWindow::OnFind() {
  Raise(find_dialog_);
}

// SIM68Ku.cpp RunToCursorExecute(): runs to the selected line's address
// unless it is a breakpoint; without a valid line it warns and runs.
void MainWindow::OnRunToCursor() {
  const auto address = source_->SelectedAddress();
  if (address) {
    session_->RunTo(*address);
    return;
  }
  QMessageBox::warning(this, QStringLiteral("Error"),
                       QStringLiteral("Selected line does not have a valid address."));
  session_->Run();
}

void MainWindow::SetAutoTraceInterval(int ms) {
  auto_trace_interval_ = std::max(1, ms);
  if (auto_trace_timer_)
    auto_trace_timer_->setInterval(auto_trace_interval_);
}

// SIM68Ku.cpp AutoTraceExecute().
void MainWindow::OnAutoTrace() {
  if (session_->IsRunning() || IsAutoTracing())
    return;
  if (session_->State() != SimSession::RunState::kReady &&
      session_->State() != SimSession::RunState::kStopped)
    return;
  session_->Simulator().ResetBreakPassCounts();
  auto_trace_timer_->setInterval(auto_trace_interval_);
  auto_trace_timer_->start();
  UpdateActions();
  registers_->SetEditable(false);
}

// SIM68Ku.cpp AutoTraceTimerTimer().
void MainWindow::OnAutoTraceTick() {
  if (session_->IsRunning())
    return;
  if (session_->State() != SimSession::RunState::kReady &&
      session_->State() != SimSession::RunState::kStopped) {
    StopAutoTrace();
    return;
  }
  session_->TraceInto();
}

void MainWindow::StopAutoTrace() {
  if (!IsAutoTracing())
    return;
  auto_trace_timer_->stop();
  UpdateActions();
  registers_->SetEditable(!session_->IsRunning());
  ShowState();
}

void MainWindow::OnGotoPc() {
  if (!session_->IsRunning())
    source_->HighlightPc(session_->Simulator().State().pc, Halted());
}

void MainWindow::OnAbout() {
  QMessageBox::about(this, QStringLiteral("About Sim68K"),
                     QStringLiteral("<b>Sim68K</b><br>EASy68K 68000 Simulator, Qt port.<br>"
                                    "Original by Chuck Kelly, www.easy68k.com.<br>"
                                    "Licensed under the GNU GPL v2 or later."));
}

void MainWindow::OnWindowSize(QAction* action) {
  const QStringList wh = action->text().split(QLatin1Char('x'));
  session_->Screen().SetWindowSize(wh.at(0).toInt(), wh.at(1).toInt());
}

QString MainWindow::MessageText() const {
  return messages_->toPlainText();
}

void MainWindow::Raise(QWidget* window) {
  window->show();
  window->raise();
  window->activateWindow();
}

// FontSourceExecute() / STARTSIM.CPP initSim(): listing, heading and messages.
void MainWindow::SetSourceFont(const QFont& font) {
  source_font_ = font;
  source_->SetListFont(font);
  messages_->setFont(font);
}

// FontOutputExecute() / STARTSIM.CPP initSim().
void MainWindow::SetOutputFont(const QFont& font) {
  output_font_ = font;
  session_->Screen().SetBaseFont(font, 0x00FFFFFF);
}

// FontPrinterExecute() / CODE9.CPP initPrint().
void MainWindow::SetPrinterFont(const QFont& font) {
  printer_font_ = font;
  session_->Print().SetFont(font);
}

void MainWindow::OnSourceFont() {
  if (const auto chosen = choose_font_(source_font_, true)) {
    SetSourceFont(*chosen);
    SaveSettings();
  }
}

void MainWindow::OnOutputFont() {
  if (const auto chosen = choose_font_(output_font_, true)) {
    SetOutputFont(*chosen);
    SaveSettings();
  }
}

void MainWindow::OnPrinterFont() {
  if (const auto chosen = choose_font_(printer_font_, false)) {
    SetPrinterFont(*chosen);
    SaveSettings();
  }
}

// SIM68Ku.cpp defaultSettings().
void MainWindow::DefaultSettings() {
  SimLog::Settings log = session_->Log().GetSettings();
  log.exec_type = SimLog::kExecDisabled;
  log.output_type = SimLog::kOutputDisabled;
  session_->Log().SetSettings(log);

  session_->SetExceptionsEnabled(false);
  session_->SetBitFieldEnabled(false);
  {
    const QSignalBlocker b1(exceptions_);
    const QSignalBlocker b2(bit_field_);
    exceptions_->setChecked(false);
    bit_field_->setChecked(false);
  }

  move(kMainLeft, kMainTop);
  resize(kMainWidth, kMainHeight);
  io_window_->move(kSimIoLeft, kSimIoTop);
  io_window_->SetFullScreenMonitor(0);
  memory_window_->move(kMemoryLeft, kMemoryTop);
  memory_window_->ShowAddress(0);
  stack_window_->move(kStackLeft, kStackTop);
  stack_window_->resize(stack_window_->width(), kStackHeight);
  stack_window_->Selector()->setCurrentIndex(8);  // SS
  hardware_window_->move(kHardwareLeft, kHardwareTop);
  hardware_window_->AddressField(HardwareWindow::kSeg7)->setText(QStringLiteral("E00000"));
  hardware_window_->AddressField(HardwareWindow::kLeds)->setText(QStringLiteral("E00010"));
  hardware_window_->AddressField(HardwareWindow::kSwitches)->setText(QStringLiteral("E00012"));
  hardware_window_->AddressField(HardwareWindow::kButtons)->setText(QStringLiteral("E00014"));

  SetOutputFont(MakeFixedFont(QStringLiteral("Fixedsys"), 9));
  SetSourceFont(MakeFixedFont(QStringLiteral("Courier New"), 10));
  SetPrinterFont(QtPrintIO::DefaultFont());

  auto_trace_interval_ = 200;
  auto_trace_disable_display_ = false;
  UpdateLogActions();
}

void MainWindow::SetSettingsFile(const QString& path) {
  settings_file_ = path;
  LoadSettings();
}

void MainWindow::LoadSettings() {
  DefaultSettings();
  if (settings_file_.isEmpty() || !QFileInfo::exists(settings_file_))
    return;
  QSettings settings(settings_file_, QSettings::IniFormat);
  LoadSettings(settings);
}

// SIM68Ku.cpp LoadSettings().
void MainWindow::LoadSettings(QSettings& settings) {
  DefaultSettings();
  if (settings.allKeys().isEmpty())
    return;

  SimLog::Settings log = session_->Log().GetSettings();
  const int exec_log = settings.value(QStringLiteral("exec_log"), 0).toInt();
  if (exec_log >= 0 && exec_log <= 3)
    log.exec_type = exec_log;
  const int out_log = settings.value(QStringLiteral("output_log"), 0).toInt();
  if (out_log >= 0 && out_log <= 1)
    log.output_type = out_log;
  session_->Log().SetSettings(log);

  const bool exc = settings.value(QStringLiteral("exceptions"), false).toBool();
  const bool bf = settings.value(QStringLiteral("bit_field_enabled"), false).toBool();
  session_->SetExceptionsEnabled(exc);
  session_->SetBitFieldEnabled(bf);
  {
    const QSignalBlocker b1(exceptions_);
    const QSignalBlocker b2(bit_field_);
    exceptions_->setChecked(exc);
    bit_field_->setChecked(bf);
  }

  const QScreen* screen = QGuiApplication::primaryScreen();
  const QSize screen_size = screen ? screen->size() : QSize(1920, 1080);
  const int sw = screen_size.width();
  const int sh = screen_size.height();

  io_window_->move(
      ClampCoord(settings.value(QStringLiteral("simio_left"), kSimIoLeft).toInt(), kSimIoLeft, sw),
      ClampCoord(settings.value(QStringLiteral("simio_top"), kSimIoTop).toInt(), kSimIoTop, sh));
  io_window_->setVisible(settings.value(QStringLiteral("simio_visible"), false).toBool());

  memory_window_->move(
      ClampCoord(settings.value(QStringLiteral("memory_left"), kMemoryLeft).toInt(), kMemoryLeft,
                 sw),
      ClampCoord(settings.value(QStringLiteral("memory_top"), kMemoryTop).toInt(), kMemoryTop, sh));
  const QString mem_addr =
      settings.value(QStringLiteral("memory_address"), QStringLiteral("000000"))
          .toString()
          .trimmed();
  bool ok = false;
  const uint32_t parsed_mem = mem_addr.toUInt(&ok, 16);
  if (ok) {
    memory_window_->ShowAddress(parsed_mem);
    memory_window_->AddressField()->setText(mem_addr.toUpper());
  }
  memory_window_->setVisible(settings.value(QStringLiteral("memory_visible"), false).toBool());

  stack_window_->move(
      ClampCoord(settings.value(QStringLiteral("stack_left"), kStackLeft).toInt(), kStackLeft, sw),
      ClampCoord(settings.value(QStringLiteral("stack_top"), kStackTop).toInt(), kStackTop, sh));
  stack_window_->resize(
      stack_window_->width(),
      ClampDimension(settings.value(QStringLiteral("stack_height"), kStackHeight).toInt(),
                     kStackHeight, sh));
  const int which_stack = settings.value(QStringLiteral("which_stack"), 8).toInt();
  if (which_stack >= 0 && which_stack < stack_window_->Selector()->count())
    stack_window_->Selector()->setCurrentIndex(which_stack);
  stack_window_->setVisible(settings.value(QStringLiteral("stack_visible"), false).toBool());

  hardware_window_->move(
      ClampCoord(settings.value(QStringLiteral("hardware_left"), kHardwareLeft).toInt(),
                 kHardwareLeft, sw),
      ClampCoord(settings.value(QStringLiteral("hardware_top"), kHardwareTop).toInt(), kHardwareTop,
                 sh));
  hardware_window_->AddressField(HardwareWindow::kSeg7)
      ->setText(settings.value(QStringLiteral("seg7_address"), QStringLiteral("E00000"))
                    .toString()
                    .trimmed()
                    .toUpper());
  hardware_window_->AddressField(HardwareWindow::kLeds)
      ->setText(settings.value(QStringLiteral("led_address"), QStringLiteral("E00010"))
                    .toString()
                    .trimmed()
                    .toUpper());
  hardware_window_->AddressField(HardwareWindow::kSwitches)
      ->setText(settings.value(QStringLiteral("switch_address"), QStringLiteral("E00012"))
                    .toString()
                    .trimmed()
                    .toUpper());
  hardware_window_->AddressField(HardwareWindow::kButtons)
      ->setText(settings.value(QStringLiteral("button_address"), QStringLiteral("E00014"))
                    .toString()
                    .trimmed()
                    .toUpper());
  hardware_window_->setVisible(settings.value(QStringLiteral("hardware_visible"), false).toBool());

  move(ClampCoord(settings.value(QStringLiteral("main_left"), kMainLeft).toInt(), kMainLeft, sw),
       ClampCoord(settings.value(QStringLiteral("main_top"), kMainTop).toInt(), kMainTop, sh));
  resize(ClampDimension(settings.value(QStringLiteral("main_width"), kMainWidth).toInt(),
                        kMainWidth, sw),
         ClampDimension(settings.value(QStringLiteral("main_height"), kMainHeight).toInt(),
                        kMainHeight, sh));

  SetOutputFont(LoadFont(settings, QStringLiteral("output_font"), output_font_, true));
  SetSourceFont(LoadFont(settings, QStringLiteral("listing_font"), source_font_, true));
  SetPrinterFont(LoadFont(settings, QStringLiteral("printer_font"), printer_font_, false));

  io_window_->SetFullScreenMonitor(
      std::max(0, settings.value(QStringLiteral("fullscreen_monitor"), 0).toInt()));
  const int interval = settings.value(QStringLiteral("auto_trace_interval"), 200).toInt();
  if (interval > 0)
    SetAutoTraceInterval(interval);
  auto_trace_disable_display_ = settings.value(QStringLiteral("disable_display"), false).toBool();

  UpdateLogActions();
}

void MainWindow::SaveSettings() const {
  if (settings_file_.isEmpty())
    return;
  QSettings settings(settings_file_, QSettings::IniFormat);
  SaveSettings(settings);
}

// SIM68Ku.cpp SaveSettings().
void MainWindow::SaveSettings(QSettings& settings) const {
  const SimLog::Settings log = session_->Log().GetSettings();
  settings.setValue(QStringLiteral("exec_log"), log.exec_type);
  settings.setValue(QStringLiteral("output_log"), log.output_type);
  settings.setValue(QStringLiteral("exceptions"), session_->ExceptionsEnabled());
  settings.setValue(QStringLiteral("main_top"), y());
  settings.setValue(QStringLiteral("main_left"), x());
  settings.setValue(QStringLiteral("main_height"), height());
  settings.setValue(QStringLiteral("main_width"), width());
  settings.setValue(QStringLiteral("simio_top"), io_window_->y());
  settings.setValue(QStringLiteral("simio_left"), io_window_->x());
  settings.setValue(QStringLiteral("simio_visible"), io_window_->isVisible());
  settings.setValue(QStringLiteral("memory_top"), memory_window_->y());
  settings.setValue(QStringLiteral("memory_left"), memory_window_->x());
  settings.setValue(QStringLiteral("memory_address"),
                    memory_window_->AddressField()->text().trimmed());
  settings.setValue(QStringLiteral("memory_visible"), memory_window_->isVisible());
  settings.setValue(QStringLiteral("stack_top"), stack_window_->y());
  settings.setValue(QStringLiteral("stack_left"), stack_window_->x());
  settings.setValue(QStringLiteral("stack_height"), stack_window_->height());
  settings.setValue(QStringLiteral("which_stack"), stack_window_->Selector()->currentIndex());
  settings.setValue(QStringLiteral("stack_visible"), stack_window_->isVisible());
  settings.setValue(QStringLiteral("hardware_top"), hardware_window_->y());
  settings.setValue(QStringLiteral("hardware_left"), hardware_window_->x());
  settings.setValue(QStringLiteral("seg7_address"),
                    hardware_window_->AddressField(HardwareWindow::kSeg7)->text().trimmed());
  settings.setValue(QStringLiteral("led_address"),
                    hardware_window_->AddressField(HardwareWindow::kLeds)->text().trimmed());
  settings.setValue(QStringLiteral("switch_address"),
                    hardware_window_->AddressField(HardwareWindow::kSwitches)->text().trimmed());
  settings.setValue(QStringLiteral("button_address"),
                    hardware_window_->AddressField(HardwareWindow::kButtons)->text().trimmed());
  settings.setValue(QStringLiteral("hardware_visible"), hardware_window_->isVisible());
  SaveFont(settings, QStringLiteral("output_font"), output_font_);
  SaveFont(settings, QStringLiteral("listing_font"), source_font_);
  SaveFont(settings, QStringLiteral("printer_font"), printer_font_);
  settings.setValue(QStringLiteral("fullscreen_monitor"), io_window_->FullScreenMonitor());
  settings.setValue(QStringLiteral("bit_field_enabled"), session_->BitFieldEnabled());
  settings.setValue(QStringLiteral("auto_trace_interval"), auto_trace_interval_);
  settings.setValue(QStringLiteral("disable_display"), auto_trace_disable_display_);
  settings.sync();
}

// SimulatorOptionsClick() (Optionsu.cpp).
void MainWindow::OnAutoTraceOptions() {
  AutoTraceOptionsDialog dialog(auto_trace_interval_, auto_trace_disable_display_, this);
  if (dialog.exec() == QDialog::Accepted) {
    SetAutoTraceInterval(dialog.Interval());
    SetAutoTraceDisableDisplay(dialog.DisableDisplay());
    SaveSettings();
  }
}

// mmuFullscreenOptionsClick().
void MainWindow::OnFullscreenOptions() {
  if (io_window_->FullScreen()) {
    QMessageBox box(QMessageBox::NoIcon, QStringLiteral("Error"),
                    QStringLiteral("Please deactivate fullscreen mode before attempting to change "
                                   "the fullscreen settings."),
                    QMessageBox::Ok, this);
    box.exec();
    return;
  }
  FullscreenOptionsDialog dialog(io_window_->FullScreenMonitor(),
                                 static_cast<int>(QGuiApplication::screens().size()), this);
  if (dialog.exec() == QDialog::Accepted) {
    io_window_->SetFullScreenMonitor(dialog.Monitor());
    SaveSettings();
  }
}

QString MainWindow::ResolveEasyBinExecutable() {
  const QDir app_dir(QCoreApplication::applicationDirPath());
  for (const QString& rel :
       {QStringLiteral("easybin"), QStringLiteral("easybin.exe"),
        QStringLiteral("../easybin/easybin"), QStringLiteral("../easybin/easybin.exe"),
        QStringLiteral("../gui/easybin/easybin"), QStringLiteral("../gui/easybin/easybin.exe")}) {
    const QFileInfo candidate(app_dir.filePath(rel));
    if (candidate.exists() && candidate.isExecutable()) {
      return candidate.canonicalFilePath();
    }
  }
  return QStandardPaths::findExecutable(QStringLiteral("easybin"));
}

// SIM68Ku.cpp EASyBIN1Click(): launches EASyBIN asynchronously.
void MainWindow::OnViewEasyBin() {
  const QString exe = ResolveEasyBinExecutable();
  if (easybin_launcher_) {
    easybin_launcher_(exe);
  }
}

// Options > Log Output (logU.cpp OKBtnClick()/CancelBtnClick()).
void MainWindow::OnLogOutput() {
  LogOutputDialog dialog(session_->Log().GetSettings(), this);
  if (dialog.exec() == QDialog::Accepted) {
    session_->Log().SetSettings(dialog.Settings());
    SaveSettings();
  }
  UpdateLogActions();
}

// logU.cpp startLog()/prepareLogFile().
void MainWindow::OnLogStart() {
  switch (session_->Log().Start(ask_log_file_)) {
    case SimLog::StartResult::kStarted:
    case SimLog::StartResult::kCancelled:
      break;
    case SimLog::StartResult::kExecOpenFailed:
      inform_(QStringLiteral("Error"), QStringLiteral("Can't open log file. Check log file name."));
      UpdateLogActions();
      OnLogOutput();  // Log->Show()
      return;
    case SimLog::StartResult::kOutputOpenFailed:
      inform_(QStringLiteral("Error"), QStringLiteral("Can't open log file."));
      break;
  }
  UpdateLogActions();
}

void MainWindow::OnLogStop() {
  session_->Log().Stop();
  UpdateLogActions();
}

// ToolLogStart/LogStart, ToolLogStop/LogStop, LoggingLbl.
void MainWindow::UpdateLogActions() {
  const SimLog& log = session_->Log();
  const bool logging = log.Logging();
  log_start_->setEnabled(!logging && log.CanStart());
  log_stop_->setEnabled(logging);
  logging_action_->setVisible(logging);
}

void MainWindow::BringToFront(int index) {
  QWidget* const chain[] = {io_window_, memory_window_, stack_window_, hardware_window_,
                            breakpoints_window_};
  for (int i = index; i < 5; i++) {
    if (chain[i]->isVisible()) {
      Raise(chain[i]);
      return;
    }
  }
  Raise(this);
}

bool MainWindow::Halted() const {
  return session_->State() == SimSession::RunState::kHalted;
}

// ---------------------------------------------------------------------------
// Drag and drop, close
// ---------------------------------------------------------------------------

namespace {

QString DroppedS68(const QMimeData* mime) {
  if (!mime->hasUrls() || mime->urls().isEmpty())
    return QString();
  const QString path = mime->urls().first().toLocalFile();
  if (path.isEmpty() ||
      QFileInfo(path).suffix().compare(QLatin1String("S68"), Qt::CaseInsensitive) != 0 ||
      !QFileInfo::exists(path))
    return QString();
  return path;
}

}  // namespace

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
  if (!DroppedS68(event->mimeData()).isEmpty())
    event->acceptProposedAction();
}

// SIM68Ku.cpp WmDropFiles(): the first file, if it is an .S68 file; asks
// first when a program is loaded.
void MainWindow::dropEvent(QDropEvent* event) {
  const QString path = DroppedS68(event->mimeData());
  if (path.isEmpty())
    return;
  if (program_loaded_ && !Confirm())
    return;
  event->acceptProposedAction();
  OpenFile(path);
}

void MainWindow::closeEvent(QCloseEvent* event) {
  auto_trace_timer_->stop();
  session_->Pause();
  SaveSettings();          // FormClose(): SaveSettings()
  session_->Log().Stop();  // FormClose(): stopLog()
  session_->Print().EndJob();
  io_window_->close();
  memory_window_->close();
  stack_window_->close();
  hardware_window_->close();
  breakpoints_window_->close();
  find_dialog_->close();
  event->accept();
}

}  // namespace easym68k::gui
