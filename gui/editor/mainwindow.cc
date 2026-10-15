// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "editor/mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPageSetupDialog>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QProcess>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextStream>
#include <QToolBar>
#include <QTreeWidget>
#include <QUrl>

#include "help/help_viewer.h"

namespace easym68k::editor {

namespace {

constexpr const char* kDefaultTemplate =
    "*-----------------------------------------------------------\n"
    "* Program Number:\n"
    "* Written by    :\n"
    "* Date Created  :\n"
    "* Description   :\n"
    "*\n"
    "*-----------------------------------------------------------\n"
    "START\tORG\t$1000\n"
    "\n"
    "\n"
    "\n"
    "\tSTOP\t#$2000\n"
    "\tEND\tSTART\n";

void SaveStyleToSettings(QSettings& s, const QString& prefix, const TextStyle& style) {
  s.setValue(prefix + QStringLiteral("_Color"), style.color.name(QColor::HexRgb));
  s.setValue(prefix + QStringLiteral("_Bold"), style.bold);
  s.setValue(prefix + QStringLiteral("_Italic"), style.italic);
  s.setValue(prefix + QStringLiteral("_Underline"), style.underline);
}

void LoadStyleFromSettings(const QSettings& s, const QString& prefix, TextStyle& style) {
  const QString color_str =
      s.value(prefix + QStringLiteral("_Color"), style.color.name()).toString();
  const QColor parsed(color_str);
  if (parsed.isValid()) {
    style.color = parsed;
  }
  style.bold = s.value(prefix + QStringLiteral("_Bold"), style.bold).toBool();
  style.italic = s.value(prefix + QStringLiteral("_Italic"), style.italic).toBool();
  style.underline = s.value(prefix + QStringLiteral("_Underline"), style.underline).toBool();
}

}  // namespace

MainWindow::MainWindow(const QString& settings_path, QWidget* parent)
    : QMainWindow(parent), settings_path_(settings_path) {
  if (settings_path_.isEmpty()) {
    const QString cfg_dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(cfg_dir);
    settings_path_ = cfg_dir + QStringLiteral("/edit68k.ini");
  }
#ifdef EASY68K_TEMPLATE_PATH
  template_path_ = QStringLiteral(EASY68K_TEMPLATE_PATH);
#endif

  setWindowTitle(QStringLiteral("Edit68K"));
  resize(900, 680);
  setAcceptDrops(true);

  mdi_area_ = new QMdiArea(this);
  mdi_area_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  mdi_area_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setCentralWidget(mdi_area_);

  BuildActionsAndMenus();
  LoadSettings();

  connect(mdi_area_, &QMdiArea::subWindowActivated, this,
          [this](QMdiSubWindow*) { UpdateActionStates(); });

  UpdateActionStates();
}

MainWindow::~MainWindow() = default;

void MainWindow::BuildActionsAndMenus() {
  // File menu
  QMenu* file_menu = menuBar()->addMenu(QStringLiteral("&File"));
  QMenu* new_submenu = file_menu->addMenu(QStringLiteral("&New"));
  new_act_ = new_submenu->addAction(QStringLiteral("&68K Source File"));
  new_act_->setShortcut(QKeySequence(QStringLiteral("Ctrl+N")));
  connect(new_act_, &QAction::triggered, this, &MainWindow::NewSourceWindow);

  open_act_ = file_menu->addAction(QStringLiteral("&Open..."));
  open_act_->setShortcut(QKeySequence(QStringLiteral("Ctrl+O")));
  connect(open_act_, &QAction::triggered, this, [this]() {
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Open File"), QString(),
        QStringLiteral("68000 Source file (*.X68);;68000 Listing file (*.L68);;"
                       "Text file (*.TXT);;All files (*.*)"));
    if (!path.isEmpty()) {
      OpenFile(path);
    }
  });

  examples_act_ = file_menu->addAction(QStringLiteral("E&xamples..."));
  examples_act_->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+E")));
  connect(examples_act_, &QAction::triggered, this, [this]() { ShowExamplesBrowser(); });

  reload_act_ = file_menu->addAction(QStringLiteral("&Reload File"));
  reload_act_->setShortcut(QKeySequence(QStringLiteral("Ctrl+R")));
  connect(reload_act_, &QAction::triggered, this, &MainWindow::ReloadActiveFile);

  save_act_ = file_menu->addAction(QStringLiteral("&Save"));
  save_act_->setShortcut(QKeySequence(QStringLiteral("Ctrl+S")));
  connect(save_act_, &QAction::triggered, this, [this]() { SaveChild(ActiveChild()); });

  save_as_act_ = file_menu->addAction(QStringLiteral("Save &As..."));
  connect(save_as_act_, &QAction::triggered, this, [this]() { SaveChildAs(ActiveChild()); });

  close_act_ = file_menu->addAction(QStringLiteral("&Close File"));
  close_act_->setShortcut(QKeySequence(QStringLiteral("Ctrl+F4")));
  connect(close_act_, &QAction::triggered, this, &MainWindow::CloseActiveFile);

  file_menu->addSeparator();
  print_act_ = file_menu->addAction(QStringLiteral("&Print..."));
  print_act_->setShortcut(QKeySequence(QStringLiteral("Ctrl+P")));
  connect(print_act_, &QAction::triggered, this, &MainWindow::ShowPrintDialog);

  print_setup_act_ = file_menu->addAction(QStringLiteral("P&rint Setup..."));
  connect(print_setup_act_, &QAction::triggered, this, [this]() {
    QPrinter printer(QPrinter::HighResolution);
    QPageSetupDialog dlg(&printer, this);
    dlg.exec();
  });

  print_preview_act_ = file_menu->addAction(QStringLiteral("Print Pre&view"));
  connect(print_preview_act_, &QAction::triggered, this, &MainWindow::ShowPrintPreviewDialog);

  file_menu->addSeparator();
  exit_act_ = file_menu->addAction(QStringLiteral("E&xit"));
  connect(exit_act_, &QAction::triggered, this, &QWidget::close);

  // Edit menu
  QMenu* edit_menu = menuBar()->addMenu(QStringLiteral("&Edit"));
  undo_act_ = edit_menu->addAction(QStringLiteral("&Undo"));
  undo_act_->setShortcut(QKeySequence::Undo);
  connect(undo_act_, &QAction::triggered, this, [this]() {
    if (EditorChild* c = ActiveChild())
      c->editor()->undo();
  });

  redo_act_ = edit_menu->addAction(QStringLiteral("&Redo"));
  redo_act_->setShortcut(QKeySequence::Redo);
  connect(redo_act_, &QAction::triggered, this, [this]() {
    if (EditorChild* c = ActiveChild())
      c->editor()->redo();
  });

  edit_menu->addSeparator();
  cut_act_ = edit_menu->addAction(QStringLiteral("Cu&t"));
  cut_act_->setShortcut(QKeySequence::Cut);
  connect(cut_act_, &QAction::triggered, this, [this]() {
    if (EditorChild* c = ActiveChild())
      c->editor()->cut();
  });

  copy_act_ = edit_menu->addAction(QStringLiteral("&Copy"));
  copy_act_->setShortcut(QKeySequence::Copy);
  connect(copy_act_, &QAction::triggered, this, [this]() {
    if (EditorChild* c = ActiveChild())
      c->editor()->copy();
  });

  paste_act_ = edit_menu->addAction(QStringLiteral("&Paste"));
  paste_act_->setShortcut(QKeySequence::Paste);
  connect(paste_act_, &QAction::triggered, this, [this]() {
    if (EditorChild* c = ActiveChild())
      c->editor()->paste();
  });

  select_all_act_ = edit_menu->addAction(QStringLiteral("Select &All"));
  select_all_act_->setShortcut(QKeySequence::SelectAll);
  connect(select_all_act_, &QAction::triggered, this, [this]() {
    if (EditorChild* c = ActiveChild())
      c->editor()->selectAll();
  });

  edit_menu->addSeparator();
  find_act_ = edit_menu->addAction(QStringLiteral("&Find..."));
  find_act_->setShortcut(QKeySequence::Find);
  connect(find_act_, &QAction::triggered, this, &MainWindow::ShowFindDialog);

  find_next_act_ = edit_menu->addAction(QStringLiteral("Find &Next"));
  find_next_act_->setShortcut(QKeySequence(Qt::Key_F3));
  connect(find_next_act_, &QAction::triggered, this, &MainWindow::FindNext);

  replace_act_ = edit_menu->addAction(QStringLiteral("&Replace..."));
  replace_act_->setShortcut(QKeySequence(QStringLiteral("Ctrl+H")));
  connect(replace_act_, &QAction::triggered, this, &MainWindow::ShowReplaceDialog);

  edit_menu->addSeparator();
  comment_act_ = edit_menu->addAction(QStringLiteral("C&omment Selection"));
  connect(comment_act_, &QAction::triggered, this, [this]() {
    if (EditorChild* c = ActiveChild())
      c->CommentSelection();
  });

  uncomment_act_ = edit_menu->addAction(QStringLiteral("U&ncomment Selection"));
  connect(uncomment_act_, &QAction::triggered, this, [this]() {
    if (EditorChild* c = ActiveChild())
      c->UncommentSelection();
  });

  edit_menu->addSeparator();
  indent_act_ = edit_menu->addAction(QStringLiteral("&Indent Selection"));
  connect(indent_act_, &QAction::triggered, this, [this]() {
    if (EditorChild* c = ActiveChild())
      c->IndentSelection();
  });

  outdent_act_ = edit_menu->addAction(QStringLiteral("O&utdent Selection"));
  connect(outdent_act_, &QAction::triggered, this, [this]() {
    if (EditorChild* c = ActiveChild())
      c->OutdentSelection();
  });

  // Project menu
  QMenu* project_menu = menuBar()->addMenu(QStringLiteral("&Project"));
  assemble_act_ = project_menu->addAction(QStringLiteral("&Assemble Source..."));
  assemble_act_->setShortcut(QKeySequence(Qt::Key_F9));
  connect(assemble_act_, &QAction::triggered, this, [this]() { AssembleActiveDocument(); });

  // Options menu
  QMenu* options_menu = menuBar()->addMenu(QStringLiteral("&Options"));
  asm_options_act_ = options_menu->addAction(QStringLiteral("&Assembler Options..."));
  connect(asm_options_act_, &QAction::triggered, this, &MainWindow::ShowAssemblerOptionsDialog);

  editor_options_act_ = options_menu->addAction(QStringLiteral("&Editor Options..."));
  connect(editor_options_act_, &QAction::triggered, this, &MainWindow::ShowEditorOptionsDialog);

  options_menu->addSeparator();
  change_font_act_ = options_menu->addAction(QStringLiteral("&Change Active File Font..."));
  connect(change_font_act_, &QAction::triggered, this, &MainWindow::ShowChangeFontDialog);

  replace_tabs_act_ = options_menu->addAction(QStringLiteral("&Replace Tabs"));
  connect(replace_tabs_act_, &QAction::triggered, this, [this]() {
    if (EditorChild* c = ActiveChild())
      c->ReplaceTabsInSelection();
  });

  // Window menu
  window_menu_ = menuBar()->addMenu(QStringLiteral("&Window"));
  cascade_act_ = window_menu_->addAction(QStringLiteral("&Cascade"));
  connect(cascade_act_, &QAction::triggered, mdi_area_, &QMdiArea::cascadeSubWindows);

  tile_horz_act_ = window_menu_->addAction(QStringLiteral("Tile &Horizontally"));
  connect(tile_horz_act_, &QAction::triggered, this, [this]() {
    const QList<QMdiSubWindow*> wins = mdi_area_->subWindowList();
    if (wins.isEmpty())
      return;
    const QRect area = mdi_area_->viewport()->rect();
    const int h = std::max(1, area.height() / static_cast<int>(wins.size()));
    for (int i = 0; i < wins.size(); ++i) {
      wins[i]->showNormal();
      wins[i]->setGeometry(0, i * h, area.width(), h);
    }
  });

  tile_vert_act_ = window_menu_->addAction(QStringLiteral("Tile &Vertically"));
  connect(tile_vert_act_, &QAction::triggered, mdi_area_, &QMdiArea::tileSubWindows);

  arrange_icons_act_ = window_menu_->addAction(QStringLiteral("&Arrange Icons"));
  connect(arrange_icons_act_, &QAction::triggered, this, [this]() {
    for (QMdiSubWindow* sub : mdi_area_->subWindowList()) {
      if (sub->isMinimized())
        sub->move(0, 0);
    }
  });

  minimize_all_act_ = window_menu_->addAction(QStringLiteral("&Minimize All"));
  connect(minimize_all_act_, &QAction::triggered, this, [this]() {
    for (QMdiSubWindow* sub : mdi_area_->subWindowList()) {
      sub->showMinimized();
    }
  });

  connect(window_menu_, &QMenu::aboutToShow, this, &MainWindow::UpdateWindowMenu);

  // Help menu
  QMenu* help_menu = menuBar()->addMenu(QStringLiteral("&Help"));
  help_act_ = help_menu->addAction(QStringLiteral("&Help"));
  help_act_->setShortcut(QKeySequence(Qt::Key_F1));
  connect(help_act_, &QAction::triggered, this, [this]() { OpenContextHelp(); });

  help_menu->addSeparator();
  about_act_ = help_menu->addAction(QStringLiteral("&About"));
  connect(about_act_, &QAction::triggered, this, &MainWindow::ShowAboutDialog);

  // Toolbar (Edit68K/mainS.dfm ToolBar1)
  QToolBar* toolbar = addToolBar(QStringLiteral("ToolBar1"));
  toolbar->setMovable(false);
  toolbar->addAction(new_act_);
  toolbar->addAction(open_act_);
  toolbar->addAction(save_act_);
  toolbar->addSeparator();
  toolbar->addAction(print_act_);
  toolbar->addSeparator();
  toolbar->addAction(find_act_);
  toolbar->addAction(replace_act_);
  toolbar->addSeparator();
  toolbar->addAction(undo_act_);
  toolbar->addAction(redo_act_);
  toolbar->addSeparator();
  toolbar->addAction(cut_act_);
  toolbar->addAction(copy_act_);
  toolbar->addAction(paste_act_);
  toolbar->addSeparator();
  toolbar->addAction(assemble_act_);
  auto_save_chk_ = new QCheckBox(QStringLiteral("Auto Save"), toolbar);
  auto_save_chk_->setToolTip(QStringLiteral("Automatically save files before assembling"));
  connect(auto_save_chk_, &QCheckBox::toggled, this,
          [this](bool checked) { prefs_.auto_save = checked; });
  toolbar->addWidget(auto_save_chk_);
  toolbar->addSeparator();
  toolbar->addAction(help_act_);
}

QMdiSubWindow* MainWindow::AddChildSubWindow(EditorChild* child) {
  ConnectChildSignals(child);
  QMdiSubWindow* sub = mdi_area_->addSubWindow(child);
  sub->setAttribute(Qt::WA_DeleteOnClose);
  if (prefs_.maximized_edit) {
    sub->showMaximized();
  } else {
    sub->show();
  }
  mdi_area_->setActiveSubWindow(sub);
  UpdateActionStates();
  return sub;
}

void MainWindow::ConnectChildSignals(EditorChild* child) {
  child->SetSaveHandler([this](EditorChild* c) { return SaveChild(c); });
  connect(child, &EditorChild::HelpRequested, this, &MainWindow::OpenContextHelp);
  connect(child, &EditorChild::FindNextRequested, this, &MainWindow::FindNext);
  connect(child, &EditorChild::ReloadRequested, this, [this](EditorChild* c) {
    c->ReloadFromDisk([this](const QString& msg) {
      return QMessageBox::question(this, QStringLiteral("Edit68K"), msg,
                                   QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
    });
  });
  connect(child, &EditorChild::StatusUpdated, this, &MainWindow::UpdateActionStates);
  connect(child->messages(), &QTreeWidget::itemDoubleClicked, this,
          [this, child](QTreeWidgetItem* item, int /*column*/) {
            if (!item)
              return;
            const QString inc_file = item->data(1, Qt::UserRole).toString();
            const int line_num = item->data(0, Qt::UserRole).toInt();
            if (!inc_file.isEmpty() && inc_file != child->current_file()) {
              QString resolved = inc_file;
              if (QFileInfo(resolved).isRelative() && child->has_name()) {
                resolved = QFileInfo(child->current_file()).dir().filePath(inc_file);
              }
              if (EditorChild* target = OpenFile(resolved)) {
                target->JumpToLine(line_num);
              }
            }
          });
}

EditorChild* MainWindow::NewSourceWindow() {
  ++untitled_counter_;
  const QString name = QStringLiteral("UNTITLED%1.X68").arg(untitled_counter_);
  auto* child = new EditorChild(this);
  const QFont font(prefs_.font_name, prefs_.font_size);
  child->InitNewSource(name, LoadTemplateText(), font, prefs_.tab_mode, prefs_.fixed_tab_size,
                       prefs_.auto_indent, prefs_.real_tabs, prefs_.theme);
  AddChildSubWindow(child);
  return child;
}

QMdiSubWindow* MainWindow::FindSubWindowForPath(const QString& path) const {
  const QString canonical = QFileInfo(path).canonicalFilePath();
  for (QMdiSubWindow* sub : mdi_area_->subWindowList()) {
    auto* child = qobject_cast<EditorChild*>(sub->widget());
    if (!child || !child->has_name())
      continue;
    const QString child_canon = QFileInfo(child->current_file()).canonicalFilePath();
    if ((!canonical.isEmpty() && child_canon == canonical) || child->current_file() == path) {
      return sub;
    }
  }
  return nullptr;
}

EditorChild* MainWindow::OpenFile(const QString& path) {
  if (path.isEmpty())
    return nullptr;
  if (QMdiSubWindow* existing = FindSubWindowForPath(path)) {
    mdi_area_->setActiveSubWindow(existing);
    return qobject_cast<EditorChild*>(existing->widget());
  }

  auto* child = new EditorChild(this);
  const QFont font(prefs_.font_name, prefs_.font_size);
  if (!child->LoadFromFile(path, font, prefs_.tab_mode, prefs_.fixed_tab_size, prefs_.auto_indent,
                           prefs_.real_tabs, prefs_.theme)) {
    delete child;
    return nullptr;
  }
  AddChildSubWindow(child);
  return child;
}

bool MainWindow::SaveChild(EditorChild* child) {
  if (!child)
    return false;
  if (!child->has_name() || child->current_file().isEmpty()) {
    return SaveChildAs(child);
  }
  const bool ok = child->SaveToFile(child->current_file());
  UpdateActionStates();
  return ok;
}

bool MainWindow::SaveChildAs(EditorChild* child, const QString& target_path) {
  if (!child)
    return false;
  QString path = target_path;
  if (path.isEmpty() && save_as_path_provider_) {
    path = save_as_path_provider_(child);
  }
  if (path.isEmpty()) {
    path =
        QFileDialog::getSaveFileName(this, QStringLiteral("Save File As"), child->current_file(),
                                     QStringLiteral("68000 Source file (*.X68);;Text file (*.TXT);;"
                                                    "All files (*.*)"));
  }
  if (path.isEmpty())
    return false;
  const bool ok = child->SaveToFile(path);
  UpdateActionStates();
  return ok;
}

bool MainWindow::CloseActiveFile() {
  QMdiSubWindow* sub = mdi_area_->activeSubWindow();
  if (!sub)
    return false;
  const bool closed = sub->close();
  UpdateActionStates();
  return closed;
}

bool MainWindow::ReloadActiveFile() {
  EditorChild* child = ActiveChild();
  if (!child || !child->has_name())
    return false;
  return child->ReloadFromDisk([this](const QString& msg) {
    return QMessageBox::question(this, QStringLiteral("Edit68K"), msg,
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
  });
}

EditorChild* MainWindow::ActiveChild() const {
  if (QMdiSubWindow* active = mdi_area_->activeSubWindow()) {
    return qobject_cast<EditorChild*>(active->widget());
  }
  const QList<QMdiSubWindow*> subs = mdi_area_->subWindowList();
  if (!subs.isEmpty()) {
    return qobject_cast<EditorChild*>(subs.back()->widget());
  }
  return nullptr;
}

std::vector<EditorChild*> MainWindow::AllChildren() const {
  std::vector<EditorChild*> out;
  for (QMdiSubWindow* sub : mdi_area_->subWindowList()) {
    if (auto* child = qobject_cast<EditorChild*>(sub->widget())) {
      out.push_back(child);
    }
  }
  return out;
}

MainWindow::AssembleOutcome MainWindow::AssembleActiveDocument() {
  AssembleOutcome outcome;
  EditorChild* child = ActiveChild();
  if (!child || !child->is_source()) {
    return outcome;
  }

  if (!child->has_name()) {
    if (!SaveChildAs(child)) {
      return outcome;
    }
  } else if (auto_save_chk_->isChecked() && child->is_modified()) {
    SaveChild(child);
  }

  outcome.source_path = child->current_file();
  const QFileInfo src_info(outcome.source_path);
  const QString base_path = src_info.dir().filePath(src_info.completeBaseName());
  outcome.list_path = base_path + QStringLiteral(".L68");
  outcome.srec_path = base_path + QStringLiteral(".S68");

  // If the listing file is currently open in an MDI window, close it first
  // (matching Edit68K/mainS.cpp AssembleSourceExecute).
  if (QMdiSubWindow* open_list = FindSubWindowForPath(outcome.list_path)) {
    open_list->close();
  }

  asm_::AssemblerOptions opts = prefs_.asm_options;
  opts.fixed_tabs = (child->tab_mode() == TabMode::kFixed);
  opts.tab_size = child->tab_size();

  asm_::Assembler assembler;
  assembler.SetOptions(opts);
  assembler.AddIncludePath(src_info.absolutePath().toStdString());
  assembler.ConfigureListing(
      QFileInfo(outcome.list_path).fileName().toStdString(),
      QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")).toStdString(),
      60, opts.cex);
  outcome.result = assembler.Assemble(child->editor()->toPlainText().toStdString());

  // Filter diagnostics depending on whether warnings are enabled.
  std::vector<asm_::AssemblyError> visible_diags;
  int error_count = 0;
  for (const asm_::AssemblyError& diag : outcome.result.errors) {
    if (diag.severity == asm_::ErrorSeverity::kWarning) {
      if (!opts.warnings) {
        continue;
      }
    } else {
      ++error_count;
    }
    visible_diags.push_back(diag);
  }
  child->SetDiagnostics(visible_diags);

  const std::string listing_text = assembler.GetListing();
  bool list_written = false;
  if (opts.generate_listing && !listing_text.empty()) {
    QFile lfile(outcome.list_path);
    if (lfile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
      lfile.write(QByteArray::fromStdString(listing_text));
      lfile.close();
      list_written = true;
    }
  }

  bool srec_written = false;
  if (opts.generate_srecord && error_count == 0 && !outcome.result.srecord.empty()) {
    QFile sfile(outcome.srec_path);
    if (sfile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
      sfile.write(QByteArray::fromStdString(outcome.result.srecord));
      sfile.close();
      srec_written = true;
    }
  }

  const int shown_warnings = opts.warnings ? outcome.result.warning_count : 0;
  const bool can_execute = (error_count == 0) && srec_written;
  const bool can_load_l68 = list_written;

  if (status_dialog_handler_) {
    outcome.choice = status_dialog_handler_(src_info.fileName(), shown_warnings, error_count,
                                            can_execute, can_load_l68);
  } else {
    AssemblerStatusDialog dlg(src_info.fileName(), shown_warnings, error_count, can_execute,
                              can_load_l68, this);
    dlg.exec();
    outcome.choice = dlg.choice();
  }

  if (outcome.choice == AssemblerStatusDialog::ActionChoice::kExecute && can_execute) {
    LaunchSimulator(outcome.srec_path);
  } else if (outcome.choice == AssemblerStatusDialog::ActionChoice::kLoadL68 && can_load_l68) {
    OpenFile(outcome.list_path);
  }

  return outcome;
}

bool MainWindow::LaunchSimulator(const QString& s68_path) {
  if (simulator_launcher_) {
    return simulator_launcher_(s68_path);
  }
  const QString app_dir = QCoreApplication::applicationDirPath();
  QString sim_bin = QDir(app_dir).filePath(QStringLiteral("easy68k-simulator"));
  if (!QFileInfo::exists(sim_bin)) {
    sim_bin = QDir(app_dir).filePath(QStringLiteral("../simulator/easy68k-simulator"));
  }
  if (!QFileInfo::exists(sim_bin)) {
    sim_bin = QStringLiteral("easy68k-simulator");
  }
  return QProcess::startDetached(sim_bin, {s68_path});
}

void MainWindow::SetPreferences(const EditorPreferences& prefs) {
  prefs_ = prefs;
  if (auto_save_chk_) {
    auto_save_chk_->setChecked(prefs_.auto_save);
  }
  if (EditorChild* active = ActiveChild()) {
    ApplyPreferencesToChild(active);
  }
  for (EditorChild* child : AllChildren()) {
    child->SetSyntaxTheme(prefs_.theme);
    child->SetAutoIndent(prefs_.auto_indent);
    child->SetRealTabs(prefs_.real_tabs);
  }
}

void MainWindow::ApplyPreferencesToChild(EditorChild* child) {
  if (!child)
    return;
  child->SetDocumentFont(QFont(prefs_.font_name, prefs_.font_size));
  child->SetTabSettings(prefs_.tab_mode, prefs_.fixed_tab_size);
  child->SetAutoIndent(prefs_.auto_indent);
  child->SetRealTabs(prefs_.real_tabs);
  child->SetSyntaxTheme(prefs_.theme);
}

QString MainWindow::LoadTemplateText() const {
  if (!template_path_.isEmpty()) {
    QFile file(template_path_);
    if (file.open(QIODevice::ReadOnly)) {
      const QByteArray raw = file.readAll();
      QString text = QString::fromLatin1(raw);
      text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
      return text;
    }
  }
  return QString::fromLatin1(kDefaultTemplate);
}

bool MainWindow::SaveTemplateText(const QString& text) {
  if (template_path_.isEmpty())
    return false;
  QFile file(template_path_);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
    return false;
  }
  QTextStream out(&file);
  out << text;
  file.close();
  return true;
}

void MainWindow::RenderChildToPrinter(EditorChild* child, QPrinter* printer) const {
  if (!child || !printer)
    return;
  QTextDocument doc;
  doc.setDefaultFont(child->DocumentFont());

  // Split pages on NEW_PAGE_MARKER (\f) just like Edit68K/mainS.cpp.
  const QString raw = child->editor()->toPlainText();
  const QStringList pages = raw.split(QLatin1Char('\f'));

  QPainter painter(printer);
  if (!painter.isActive())
    return;

  const QRectF page_rect = printer->pageLayout().paintRectPixels(printer->resolution());
  for (int i = 0; i < pages.size(); ++i) {
    if (i > 0) {
      printer->newPage();
    }
    QTextDocument page_doc;
    page_doc.setDefaultFont(child->DocumentFont());
    page_doc.setPlainText(pages[i]);
    if (!prefs_.print_black && child->is_source()) {
      M68kSyntaxHighlighter hl(&page_doc);
      hl.SetTheme(child->syntax_theme());
      hl.rehighlight();
    }
    page_doc.setPageSize(page_rect.size());
    page_doc.drawContents(&painter, QRectF(QPointF(0, 0), page_rect.size()));
  }
}

void MainWindow::ShowPrintDialog() {
  EditorChild* child = ActiveChild();
  if (!child)
    return;
  QPrinter printer(QPrinter::HighResolution);
  QPrintDialog dlg(&printer, this);
  if (dlg.exec() == QDialog::Accepted) {
    RenderChildToPrinter(child, &printer);
  }
}

void MainWindow::ShowPrintPreviewDialog() {
  EditorChild* child = ActiveChild();
  if (!child)
    return;
  QPrinter printer(QPrinter::HighResolution);
  QPrintPreviewDialog preview(&printer, this);
  connect(&preview, &QPrintPreviewDialog::paintRequested, this,
          [this, child](QPrinter* p) { RenderChildToPrinter(child, p); });
  preview.exec();
}

void MainWindow::ShowFindDialog() {
  if (!find_dialog_) {
    find_dialog_ = new EditorFindDialog(this);
    connect(find_dialog_, &EditorFindDialog::FindNextRequested, this,
            [this](const QString& text, bool whole_word, bool match_case) {
              last_find_text_ = text;
              last_whole_word_ = whole_word;
              last_match_case_ = match_case;
              FindNext();
            });
  }
  if (EditorChild* child = ActiveChild()) {
    const QString sel = child->editor()->textCursor().selectedText();
    if (!sel.isEmpty()) {
      find_dialog_->SetSearchText(sel);
    } else if (!last_find_text_.isEmpty()) {
      find_dialog_->SetSearchText(last_find_text_);
    }
  }
  find_dialog_->show();
  find_dialog_->raise();
  find_dialog_->activateWindow();
}

void MainWindow::FindNext() {
  EditorChild* child = ActiveChild();
  if (!child)
    return;
  if (last_find_text_.isEmpty()) {
    ShowFindDialog();
    return;
  }
  child->FindText(last_find_text_, last_whole_word_, last_match_case_);
}

void MainWindow::ShowReplaceDialog() {
  if (!replace_dialog_) {
    replace_dialog_ = new EditorReplaceDialog(this);
    connect(replace_dialog_, &EditorReplaceDialog::FindNextRequested, this,
            [this](const QString& text, bool whole_word, bool match_case) {
              last_find_text_ = text;
              last_whole_word_ = whole_word;
              last_match_case_ = match_case;
              FindNext();
            });
    connect(replace_dialog_, &EditorReplaceDialog::ReplaceRequested, this,
            [this](const QString& find_text, const QString& replace_text, bool whole_word,
                   bool match_case, bool replace_all) {
              last_find_text_ = find_text;
              last_replace_text_ = replace_text;
              last_whole_word_ = whole_word;
              last_match_case_ = match_case;
              if (EditorChild* child = ActiveChild()) {
                child->ReplaceOccurrences(find_text, replace_text, whole_word, match_case,
                                          replace_all);
              }
            });
  }
  if (EditorChild* child = ActiveChild()) {
    const QString sel = child->editor()->textCursor().selectedText();
    replace_dialog_->SetSearchAndReplaceText(!sel.isEmpty() ? sel : last_find_text_,
                                             last_replace_text_);
  }
  replace_dialog_->show();
  replace_dialog_->raise();
  replace_dialog_->activateWindow();
}

void MainWindow::ShowAssemblerOptionsDialog() {
  AssemblerOptionsDialog dlg(
      prefs_, LoadTemplateText(), {},
      [this](const QString& tmpl) { return SaveTemplateText(tmpl); }, this);
  connect(&dlg, &AssemblerOptionsDialog::HelpRequested, this, &MainWindow::OpenContextHelp);
  if (dlg.exec() == QDialog::Accepted) {
    SetPreferences(dlg.Preferences());
  }
}

void MainWindow::ShowEditorOptionsDialog() {
  EditorOptionsDialog::ActiveFileInfo info;
  if (EditorChild* child = ActiveChild()) {
    info.font_name = child->DocumentFont().family();
    info.font_size = child->DocumentFont().pointSize() > 0 ? child->DocumentFont().pointSize() : 10;
    info.tab_size = child->tab_size();
    info.tab_mode = child->tab_mode();
  } else {
    info.font_name = prefs_.font_name;
    info.font_size = prefs_.font_size;
    info.tab_size = prefs_.fixed_tab_size;
    info.tab_mode = prefs_.tab_mode;
  }

  EditorOptionsDialog dlg(prefs_, info, this);
  connect(&dlg, &EditorOptionsDialog::HelpRequested, this, &MainWindow::OpenContextHelp);
  if (dlg.exec() == QDialog::Accepted) {
    SetPreferences(dlg.UpdatedPreferences());
  }
}

void MainWindow::ShowChangeFontDialog() {
  EditorChild* child = ActiveChild();
  if (!child)
    return;
  AdjustFontDialog dlg(
      child->DocumentFont(), [child](const QFont& f) { child->SetDocumentFont(f); }, this);
  dlg.exec();
}

void MainWindow::OpenContextHelp(const QString& context) {
  if (!help_viewer_) {
    help_viewer_ = new gui::HelpViewer(this);
  }
  QString token = context.trimmed();
  if (token.isEmpty()) {
    if (EditorChild* child = ActiveChild()) {
      token = child->CurrentHelpToken();
    }
  }
  if (token.isEmpty()) {
    token = QStringLiteral("INTRO");
  }
  help_viewer_->ShowContext(token);
}

void MainWindow::ShowExamplesBrowser(const QString& select_id) {
  if (!examples_browser_) {
    examples_browser_ = new gui::ExamplesBrowserDialog(this);
    examples_browser_->SetOpenFileCallback(
        [this](const QString& source_path, const QString& /*readme_path*/) {
          if (!source_path.isEmpty()) {
            OpenFile(source_path);
          }
        });
  }
  if (!select_id.isEmpty()) {
    examples_browser_->SelectExampleById(select_id);
  }
  examples_browser_->show();
  examples_browser_->raise();
  examples_browser_->activateWindow();
}

void MainWindow::ShowAboutDialog() {
  QMessageBox::about(this, QStringLiteral("About Edit68K"),
                     QStringLiteral("<b>EASy68K Editor/Assembler (Edit68K)</b><br>"
                                    "Motorola 68000 Structured Assembly Language IDE."));
}

void MainWindow::UpdateActionStates() {
  EditorChild* child = ActiveChild();
  const bool has_child = (child != nullptr);
  const bool is_source = has_child && child->is_source();

  reload_act_->setEnabled(has_child && child->has_name());
  save_act_->setEnabled(is_source);
  save_as_act_->setEnabled(has_child);
  close_act_->setEnabled(has_child);
  print_act_->setEnabled(has_child);
  print_preview_act_->setEnabled(has_child);

  undo_act_->setEnabled(is_source);
  redo_act_->setEnabled(is_source);
  cut_act_->setEnabled(is_source);
  copy_act_->setEnabled(has_child);
  paste_act_->setEnabled(is_source);
  select_all_act_->setEnabled(has_child);
  find_act_->setEnabled(has_child);
  find_next_act_->setEnabled(has_child);
  replace_act_->setEnabled(is_source);
  comment_act_->setEnabled(is_source);
  uncomment_act_->setEnabled(is_source);
  indent_act_->setEnabled(is_source);
  outdent_act_->setEnabled(is_source);

  assemble_act_->setEnabled(is_source);
  change_font_act_->setEnabled(has_child);
  replace_tabs_act_->setEnabled(is_source);
}

void MainWindow::UpdateWindowMenu() {
  window_menu_->clear();
  window_menu_->addAction(cascade_act_);
  window_menu_->addAction(tile_horz_act_);
  window_menu_->addAction(tile_vert_act_);
  window_menu_->addAction(arrange_icons_act_);
  window_menu_->addAction(minimize_all_act_);

  const QList<QMdiSubWindow*> subs = mdi_area_->subWindowList();
  if (!subs.isEmpty()) {
    window_menu_->addSeparator();
    for (int i = 0; i < subs.size(); ++i) {
      QMdiSubWindow* sub = subs[i];
      auto* child = qobject_cast<EditorChild*>(sub->widget());
      if (!child)
        continue;
      QAction* act =
          window_menu_->addAction(QStringLiteral("&%1 %2").arg(i + 1).arg(child->windowTitle()));
      act->setCheckable(true);
      act->setChecked(sub == mdi_area_->activeSubWindow());
      connect(act, &QAction::triggered, this,
              [this, sub]() { mdi_area_->setActiveSubWindow(sub); });
    }
  }
}

void MainWindow::LoadSettings() {
  if (settings_path_.isEmpty() || !QFileInfo::exists(settings_path_)) {
    auto_save_chk_->setChecked(prefs_.auto_save);
    return;
  }

  QSettings s(settings_path_, QSettings::IniFormat);
  s.beginGroup(QStringLiteral("Assembler"));
  prefs_.asm_options.generate_listing =
      s.value(QStringLiteral("GenerateList"), prefs_.asm_options.generate_listing).toBool();
  prefs_.asm_options.generate_srecord =
      s.value(QStringLiteral("GenerateSRec"), prefs_.asm_options.generate_srecord).toBool();
  prefs_.auto_save = s.value(QStringLiteral("AutoSave"), prefs_.auto_save).toBool();
  prefs_.asm_options.warnings =
      s.value(QStringLiteral("ShowWarnings"), prefs_.asm_options.warnings).toBool();
  prefs_.asm_options.bitfield =
      s.value(QStringLiteral("BitfieldEnabled"), prefs_.asm_options.bitfield).toBool();
  prefs_.asm_options.cre = s.value(QStringLiteral("CrossRef"), prefs_.asm_options.cre).toBool();
  prefs_.asm_options.mex = s.value(QStringLiteral("MacEx"), prefs_.asm_options.mex).toBool();
  prefs_.asm_options.cex = s.value(QStringLiteral("ConstEx"), prefs_.asm_options.cex).toBool();
  prefs_.asm_options.sex = s.value(QStringLiteral("StrucEx"), prefs_.asm_options.sex).toBool();
  s.endGroup();

  s.beginGroup(QStringLiteral("Editor"));
  prefs_.font_name = s.value(QStringLiteral("FontName"), prefs_.font_name).toString();
  prefs_.font_size =
      std::clamp(s.value(QStringLiteral("FontSize"), prefs_.font_size).toInt(), 6, 72);
  const int tab_type =
      s.value(QStringLiteral("TabType"), prefs_.tab_mode == TabMode::kAssembly ? 0 : 1).toInt();
  prefs_.tab_mode = (tab_type == 0) ? TabMode::kAssembly : TabMode::kFixed;
  prefs_.fixed_tab_size =
      std::clamp(s.value(QStringLiteral("FixedTabSize"), prefs_.fixed_tab_size).toInt(), 1, 32);
  prefs_.maximized_edit = s.value(QStringLiteral("MaximizedEdit"), prefs_.maximized_edit).toBool();
  prefs_.auto_indent = s.value(QStringLiteral("AutoIndent"), prefs_.auto_indent).toBool();
  prefs_.real_tabs = s.value(QStringLiteral("RealTabs"), prefs_.real_tabs).toBool();
  prefs_.print_black = s.value(QStringLiteral("PrintBlack"), prefs_.print_black).toBool();
  s.endGroup();

  s.beginGroup(QStringLiteral("SyntaxHighlight"));
  prefs_.theme.disabled = s.value(QStringLiteral("Disabled"), prefs_.theme.disabled).toBool();
  prefs_.theme.preset =
      std::clamp(s.value(QStringLiteral("PresetIndex"), prefs_.theme.preset).toInt(), 0, 4);
  const QColor back(
      s.value(QStringLiteral("BackColor"), prefs_.theme.background.name()).toString());
  if (back.isValid()) {
    prefs_.theme.background = back;
  }
  LoadStyleFromSettings(s, QStringLiteral("Code"), prefs_.theme.code);
  LoadStyleFromSettings(s, QStringLiteral("Comment"), prefs_.theme.comment);
  LoadStyleFromSettings(s, QStringLiteral("Label"), prefs_.theme.label);
  LoadStyleFromSettings(s, QStringLiteral("Directive"), prefs_.theme.directive);
  LoadStyleFromSettings(s, QStringLiteral("Structured"), prefs_.theme.structure);
  LoadStyleFromSettings(s, QStringLiteral("Error"), prefs_.theme.error);
  LoadStyleFromSettings(s, QStringLiteral("String"), prefs_.theme.text);
  s.endGroup();

  s.beginGroup(QStringLiteral("Window"));
  const int w = std::max(300, s.value(QStringLiteral("Width"), width()).toInt());
  const int h = std::max(200, s.value(QStringLiteral("Height"), height()).toInt());
  int x = s.value(QStringLiteral("Left"), this->x()).toInt();
  int y = s.value(QStringLiteral("Top"), this->y()).toInt();
  if (QScreen* screen = QApplication::primaryScreen()) {
    const QRect avail = screen->availableGeometry();
    x = std::clamp(x, avail.left(), std::max(avail.left(), avail.right() - 100));
    y = std::clamp(y, avail.top(), std::max(avail.top(), avail.bottom() - 100));
  }
  setGeometry(x, y, w, h);
  if (s.value(QStringLiteral("Maximized"), false).toBool()) {
    setWindowState(windowState() | Qt::WindowMaximized);
  }
  s.endGroup();

  auto_save_chk_->setChecked(prefs_.auto_save);
}

void MainWindow::SaveSettings() const {
  if (settings_path_.isEmpty())
    return;
  QSettings s(settings_path_, QSettings::IniFormat);

  s.beginGroup(QStringLiteral("Assembler"));
  s.setValue(QStringLiteral("GenerateList"), prefs_.asm_options.generate_listing);
  s.setValue(QStringLiteral("GenerateSRec"), prefs_.asm_options.generate_srecord);
  s.setValue(QStringLiteral("AutoSave"), prefs_.auto_save);
  s.setValue(QStringLiteral("ShowWarnings"), prefs_.asm_options.warnings);
  s.setValue(QStringLiteral("BitfieldEnabled"), prefs_.asm_options.bitfield);
  s.setValue(QStringLiteral("CrossRef"), prefs_.asm_options.cre);
  s.setValue(QStringLiteral("MacEx"), prefs_.asm_options.mex);
  s.setValue(QStringLiteral("ConstEx"), prefs_.asm_options.cex);
  s.setValue(QStringLiteral("StrucEx"), prefs_.asm_options.sex);
  s.endGroup();

  s.beginGroup(QStringLiteral("Editor"));
  s.setValue(QStringLiteral("FontName"), prefs_.font_name);
  s.setValue(QStringLiteral("FontSize"), prefs_.font_size);
  s.setValue(QStringLiteral("TabType"), prefs_.tab_mode == TabMode::kAssembly ? 0 : 1);
  s.setValue(QStringLiteral("FixedTabSize"), prefs_.fixed_tab_size);
  s.setValue(QStringLiteral("MaximizedEdit"), prefs_.maximized_edit);
  s.setValue(QStringLiteral("AutoIndent"), prefs_.auto_indent);
  s.setValue(QStringLiteral("RealTabs"), prefs_.real_tabs);
  s.setValue(QStringLiteral("PrintBlack"), prefs_.print_black);
  s.endGroup();

  s.beginGroup(QStringLiteral("SyntaxHighlight"));
  s.setValue(QStringLiteral("Disabled"), prefs_.theme.disabled);
  s.setValue(QStringLiteral("PresetIndex"), prefs_.theme.preset);
  s.setValue(QStringLiteral("BackColor"), prefs_.theme.background.name(QColor::HexRgb));
  SaveStyleToSettings(s, QStringLiteral("Code"), prefs_.theme.code);
  SaveStyleToSettings(s, QStringLiteral("Comment"), prefs_.theme.comment);
  SaveStyleToSettings(s, QStringLiteral("Label"), prefs_.theme.label);
  SaveStyleToSettings(s, QStringLiteral("Directive"), prefs_.theme.directive);
  SaveStyleToSettings(s, QStringLiteral("Structured"), prefs_.theme.structure);
  SaveStyleToSettings(s, QStringLiteral("Error"), prefs_.theme.error);
  SaveStyleToSettings(s, QStringLiteral("String"), prefs_.theme.text);
  s.endGroup();

  s.beginGroup(QStringLiteral("Window"));
  s.setValue(QStringLiteral("Left"), x());
  s.setValue(QStringLiteral("Top"), y());
  s.setValue(QStringLiteral("Width"), width());
  s.setValue(QStringLiteral("Height"), height());
  s.setValue(QStringLiteral("Maximized"), isMaximized());
  s.endGroup();
  s.sync();
}

void MainWindow::closeEvent(QCloseEvent* event) {
  if (QMdiSubWindow* active = mdi_area_->activeSubWindow()) {
    prefs_.maximized_edit = active->isMaximized();
  }
  mdi_area_->closeAllSubWindows();
  if (!mdi_area_->subWindowList().isEmpty()) {
    event->ignore();
    return;
  }
  SaveSettings();
  event->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

void MainWindow::dropEvent(QDropEvent* event) {
  for (const QUrl& url : event->mimeData()->urls()) {
    const QString local = url.toLocalFile();
    if (!local.isEmpty()) {
      OpenFile(local);
    }
  }
  event->acceptProposedAction();
}

}  // namespace easym68k::editor
