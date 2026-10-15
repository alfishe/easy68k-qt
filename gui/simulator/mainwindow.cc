// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mainwindow.h"

#include <algorithm>
#include <iterator>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPageSetupDialog>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QToolBar>
#include <QVBoxLayout>

#include "io_window.h"
#include "qt_print_io.h"
#include "register_widget.h"
#include "sim_io_screen.h"
#include "source_view.h"
#include "stack_window.h"

namespace easym68k::gui {

namespace {

const char kLostWarning[] = "The current debug environment will be lost, click OK to continue.";
const char kS68Filter[] = "EASy68K S-Record (*.S68 *.s68);;All Files (*)";

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
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

  CreateActions();
  CreateMenus();
  CreateToolBar();

  connect(session_.get(), &SimSession::StateChanged, this, [this] {
    UpdateActions();
    registers_->SetEditable(!session_->IsRunning());
  });
  // SIM68Ku.cpp loadRegs()/register key handlers: edits go to the CPU.
  connect(registers_, &RegisterWidget::Edited, this, [this](const sim::CpuState& edited) {
    if (session_->IsRunning())
      return;
    sim::CpuState& cpu = session_->Simulator().State();
    std::copy(std::begin(edited.d), std::end(edited.d), std::begin(cpu.d));
    std::copy(std::begin(edited.a), std::end(edited.a), std::begin(cpu.a));
    cpu.usp = edited.a[7];
    cpu.ssp = edited.ssp;
    cpu.pc = edited.pc;
    cpu.sr = edited.sr;
    source_->HighlightPc(cpu.pc, Halted());  // regPCChange()
    stack_window_->UpdateState(cpu);         // StackFrm->updateDisplay()
  });
  connect(registers_, &RegisterWidget::ClearCycles, this, [this] {
    if (session_->IsRunning())
      return;
    session_->Simulator().State().cycles = 0;
    ShowState();
  });
  connect(registers_, &RegisterWidget::PcDoubleClicked, this, &MainWindow::OnGotoPc);
  // Listing: breakpoint gutter and double click (SIM68Ku.cpp breakP, ListBox1).
  source_->SetBreakpointQuery([this](uint32_t a) { return session_->HasBreakpoint(a); });
  connect(source_, &SourceView::BreakpointToggled, session_.get(), &SimSession::ToggleBreakpoint);
  connect(source_, &SourceView::ClearAllBreakpoints, session_.get(), &SimSession::ClearBreakpoints);
  connect(session_.get(), &SimSession::BreakpointsChanged, source_,
          &SourceView::RefreshBreakpoints);
  connect(source_, &SourceView::PcRequested, this, [this](uint32_t address) {
    if (session_->IsRunning())
      return;
    session_->Simulator().State().pc = address;
    ShowState();
  });
  connect(session_.get(), &SimSession::ExecutionStopped, this, &MainWindow::ShowState);
  connect(session_.get(), &SimSession::Message, messages_, &QPlainTextEdit::appendPlainText);
  connect(session_.get(), &SimSession::MessagesCleared, messages_, &QPlainTextEdit::clear);
  connect(session_.get(), &SimSession::OptionsChanged, this, [this] {
    exceptions_->setChecked(session_->ExceptionsEnabled());
    bit_field_->setChecked(session_->BitFieldEnabled());
  });
  connect(session_.get(), &SimSession::ProgramLoaded, this, [this] {
    program_loaded_ = true;
    source_->SetListing(session_->Listing(), session_->HasListing());
    setWindowTitle(session_->ProgramPath());
    UpdateActions();
  });
  connect(session_.get(), &SimSession::ProgramClosed, this, [this] {
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

  UpdateActions();
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
  find_ = Pending(QStringLiteral("&Find..."), QStringLiteral("13.23"));
  find_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F));
  goto_pc_ = Action(QStringLiteral("&Go to PC"), QKeySequence(Qt::CTRL | Qt::Key_G),
                    &MainWindow::OnGotoPc);
  run_ = Action(QStringLiteral("&Run"), QKeySequence(Qt::Key_F9), nullptr);
  connect(run_, &QAction::triggered, this, [this] { session_->Run(); });
  run_to_cursor_ = Action(QStringLiteral("Run To &Cursor"), QKeySequence(Qt::CTRL | Qt::Key_F9),
                          &MainWindow::OnRunToCursor);
  auto_trace_ = Pending(QStringLiteral("&Auto Trace"), QStringLiteral("13.24"));
  auto_trace_->setShortcut(QKeySequence(Qt::Key_F10));
  step_ = Action(QStringLiteral("&Step Over"), QKeySequence(Qt::Key_F8), nullptr);
  connect(step_, &QAction::triggered, this, [this] { session_->StepOver(); });
  trace_ = Action(QStringLiteral("&Trace Into"), QKeySequence(Qt::Key_F7), nullptr);
  connect(trace_, &QAction::triggered, this, [this] { session_->TraceInto(); });
  pause_ = Action(QStringLiteral("&Pause"), QKeySequence(Qt::Key_F6), nullptr);
  connect(pause_, &QAction::triggered, this, [this] { session_->Pause(); });
  rewind_ = Action(QStringLiteral("R&ewind Program"), QKeySequence(Qt::CTRL | Qt::Key_F2), nullptr);
  connect(rewind_, &QAction::triggered, this, [this] { session_->Rewind(); });
  reload_ = Action(QStringLiteral("Re&load Program"), QKeySequence(Qt::CTRL | Qt::Key_F3), nullptr);
  connect(reload_, &QAction::triggered, this, [this] { session_->Reload(); });
  log_start_ = Pending(QStringLiteral("L&og Start"), QStringLiteral("13.12"));
  log_start_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F4));
  log_stop_ = Pending(QStringLiteral("Lo&g Stop"), QStringLiteral("13.12"));
  log_stop_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F5));

  const QStyle* style = this->style();
  open_->setIcon(style->standardIcon(QStyle::SP_DialogOpenButton));
  run_->setIcon(style->standardIcon(QStyle::SP_MediaPlay));
  pause_->setIcon(style->standardIcon(QStyle::SP_MediaPause));
  step_->setIcon(style->standardIcon(QStyle::SP_ArrowRight));
  trace_->setIcon(style->standardIcon(QStyle::SP_ArrowDown));
  run_to_cursor_->setIcon(style->standardIcon(QStyle::SP_MediaSeekForward));
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
  connect(output_window_, &QAction::triggered, this, [this] {
    io_window_->show();
    io_window_->raise();
    io_window_->activateWindow();
  });
  view->addAction(Pending(QStringLiteral("&Memory"), QStringLiteral("13.6")));
  QAction* stack = view->addAction(QStringLiteral("&Stack"));
  connect(stack, &QAction::triggered, this, [this] {
    stack_window_->show();
    stack_window_->raise();
    stack_window_->activateWindow();
  });
  view->addAction(Pending(QStringLiteral("&Hardware"), QStringLiteral("13.8")));
  view->addAction(Pending(QStringLiteral("&Break Points"), QStringLiteral("13.22")));
  view->addAction(Pending(QStringLiteral("&EASyBIN"), QStringLiteral("13.20")));

  QMenu* options = menuBar()->addMenu(QStringLiteral("&Options"));
  options->addAction(Pending(QStringLiteral("Source Window Font..."), QStringLiteral("13.18")));
  options->addAction(Pending(QStringLiteral("Output Window Font..."), QStringLiteral("13.18")));
  options->addAction(Pending(QStringLiteral("Printer Font..."), QStringLiteral("13.18")));
  options->addSeparator();
  options->addAction(Pending(QStringLiteral("&Log Output..."), QStringLiteral("13.12")));
  options->addSeparator();
  exceptions_ = options->addAction(QStringLiteral("&Enable Exceptions"));
  exceptions_->setCheckable(true);
  exceptions_->setChecked(session_->ExceptionsEnabled());
  connect(exceptions_, &QAction::toggled, this,
          [this](bool on) { session_->SetExceptionsEnabled(on); });
  options->addSeparator();
  options->addAction(Pending(QStringLiteral("&Auto Trace Options..."), QStringLiteral("13.24")));
  options->addSeparator();
  options->addAction(Pending(QStringLiteral("&Fullscreen Options..."), QStringLiteral("13.10")));
  options->addSeparator();
  bit_field_ = options->addAction(QStringLiteral("Enable &Bit Field Instructions"));
  bit_field_->setCheckable(true);
  bit_field_->setChecked(session_->BitFieldEnabled());
  connect(bit_field_, &QAction::toggled, this,
          [this](bool on) { session_->SetBitFieldEnabled(on); });
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
  logging_ = new QLabel(QStringLiteral("LOGGING"), bar);
  logging_->setStyleSheet(QStringLiteral("color: red; font-weight: bold"));
  bar->addWidget(logging_)->setVisible(false);  // Task 13.12
}

// SIM68Ku.cpp setMenuActive() / setMenuTrace() / setMenuInactive().
void MainWindow::UpdateActions() {
  using S = SimSession::RunState;
  const S state = session_->State();
  const bool active = state == S::kReady || state == S::kStopped;
  const bool loaded = state != S::kNoProgram;
  open_->setEnabled(state != S::kRunning);
  open_data_->setEnabled(state != S::kRunning);
  close_->setEnabled(loaded);
  for (QAction* a : {run_, run_to_cursor_, step_, trace_})
    a->setEnabled(active);
  pause_->setEnabled(loaded);
  rewind_->setEnabled(loaded);
  reload_->setEnabled(program_loaded_ && !session_->ProgramPath().isEmpty() &&
                      state != S::kRunning);
  goto_pc_->setEnabled(loaded);
}

// SIM68Ku.cpp scrshow(): registers and the PC's listing line.
void MainWindow::ShowState() {
  if (session_->IsRunning())
    return;
  const sim::CpuState& cpu = session_->Simulator().State();
  registers_->Show(cpu);
  source_->HighlightPc(cpu.pc, Halted());
  stack_window_->UpdateState(cpu);
}

// SIM68Ku.cpp setMenuTask19()/restoreMenuTask19().
void MainWindow::SetKeyCommands(bool enabled) {
  for (auto& [action, shortcut] : key_commands_)
    action->setShortcut(enabled ? shortcut : QKeySequence());
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

bool MainWindow::OpenFile(const QString& path) {
  return session_->Open(path);
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
    }
    // "/f" (full screen output window): Task 13.10.
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
  session_->Pause();
  session_->Print().EndJob();
  io_window_->close();
  event->accept();
}

}  // namespace easym68k::gui
