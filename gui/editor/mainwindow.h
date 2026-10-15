// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef EASY68K_GUI_EDITOR_MAINWINDOW_H_
#define EASY68K_GUI_EDITOR_MAINWINDOW_H_

#include <functional>
#include <QMainWindow>
#include <QString>
#include <vector>

#include "easym68k/asm/assembler.h"
#include "editor/dialogs.h"
#include "editor/editor_child.h"

class QAction;
class QCheckBox;
class QCloseEvent;
class QDragEnterEvent;
class QDropEvent;
class QMdiArea;
class QMdiSubWindow;
class QMenu;
class QPrinter;

namespace easym68k::gui {
class ExamplesBrowserDialog;
class HelpViewer;
}  // namespace easym68k::gui

namespace easym68k::editor {

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(const QString& settings_path = QString(), QWidget* parent = nullptr);
  ~MainWindow() override;

  // Document lifecycle
  EditorChild* NewSourceWindow();
  EditorChild* OpenFile(const QString& path);
  bool SaveChild(EditorChild* child);
  bool SaveChildAs(EditorChild* child, const QString& target_path = QString());
  bool CloseActiveFile();
  bool ReloadActiveFile();

  // Active child accessors
  EditorChild* ActiveChild() const;
  std::vector<EditorChild*> AllChildren() const;
  QMdiArea* mdi_area() const { return mdi_area_; }

  // Assemble workflow
  struct AssembleOutcome {
    asm_::AssemblyResult result;
    QString source_path;
    QString list_path;
    QString srec_path;
    AssemblerStatusDialog::ActionChoice choice = AssemblerStatusDialog::ActionChoice::kClose;
  };
  AssembleOutcome AssembleActiveDocument();

  // Preferences & persistence
  const EditorPreferences& preferences() const { return prefs_; }
  void SetPreferences(const EditorPreferences& prefs);
  void ApplyPreferencesToChild(EditorChild* child);

  void LoadSettings();
  void SaveSettings() const;
  QString settings_file_path() const { return settings_path_; }

  // Template management
  QString LoadTemplateText() const;
  bool SaveTemplateText(const QString& text);
  void SetTemplateFilePath(const QString& path) { template_path_ = path; }
  const QString& template_file_path() const { return template_path_; }

  // Printing helper (callable directly in headless tests)
  void RenderChildToPrinter(EditorChild* child, QPrinter* printer) const;

  // Help & dialogs
  void OpenContextHelp(const QString& context = QString());
  void ShowExamplesBrowser(const QString& select_id = QString());
  gui::HelpViewer* help_viewer() const { return help_viewer_; }
  gui::ExamplesBrowserDialog* examples_browser() const { return examples_browser_; }
  EditorFindDialog* find_dialog() const { return find_dialog_; }
  EditorReplaceDialog* replace_dialog() const { return replace_dialog_; }
  QCheckBox* auto_save_checkbox() const { return auto_save_chk_; }

  // Injectable hooks for deterministic headless tests
  void SetStatusDialogHandler(std::function<AssemblerStatusDialog::ActionChoice(
                                  const QString& source_file, int warnings, int errors,
                                  bool can_execute, bool can_load_l68)>
                                  handler) {
    status_dialog_handler_ = std::move(handler);
  }
  void SetSimulatorLauncher(std::function<bool(const QString& s68_path)> cb) {
    simulator_launcher_ = std::move(cb);
  }
  void SetSaveAsPathProvider(std::function<QString(EditorChild*)> provider) {
    save_as_path_provider_ = std::move(provider);
  }

  // Menu actions exposed for tests
  QAction* new_action() const { return new_act_; }
  QAction* open_action() const { return open_act_; }
  QAction* examples_action() const { return examples_act_; }
  QAction* reload_action() const { return reload_act_; }
  QAction* save_action() const { return save_act_; }
  QAction* save_as_action() const { return save_as_act_; }
  QAction* close_action() const { return close_act_; }
  QAction* print_action() const { return print_act_; }
  QAction* print_preview_action() const { return print_preview_act_; }
  QAction* undo_action() const { return undo_act_; }
  QAction* redo_action() const { return redo_act_; }
  QAction* cut_action() const { return cut_act_; }
  QAction* copy_action() const { return copy_act_; }
  QAction* paste_action() const { return paste_act_; }
  QAction* select_all_action() const { return select_all_act_; }
  QAction* find_action() const { return find_act_; }
  QAction* find_next_action() const { return find_next_act_; }
  QAction* replace_action() const { return replace_act_; }
  QAction* comment_action() const { return comment_act_; }
  QAction* uncomment_action() const { return uncomment_act_; }
  QAction* indent_action() const { return indent_act_; }
  QAction* outdent_action() const { return outdent_act_; }
  QAction* assemble_action() const { return assemble_act_; }
  QAction* asm_options_action() const { return asm_options_act_; }
  QAction* editor_options_action() const { return editor_options_act_; }
  QAction* change_font_action() const { return change_font_act_; }
  QAction* replace_tabs_action() const { return replace_tabs_act_; }
  QAction* help_action() const { return help_act_; }

 protected:
  void closeEvent(QCloseEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;

 private:
  void BuildActionsAndMenus();
  void ConnectChildSignals(EditorChild* child);
  QMdiSubWindow* AddChildSubWindow(EditorChild* child);
  QMdiSubWindow* FindSubWindowForPath(const QString& path) const;
  void UpdateActionStates();
  void UpdateWindowMenu();
  bool LaunchSimulator(const QString& s68_path);

  void ShowFindDialog();
  void FindNext();
  void ShowReplaceDialog();
  void ShowAssemblerOptionsDialog();
  void ShowEditorOptionsDialog();
  void ShowChangeFontDialog();
  void ShowPrintDialog();
  void ShowPrintPreviewDialog();
  void ShowAboutDialog();

  QString settings_path_;
  QString template_path_;
  EditorPreferences prefs_;
  int untitled_counter_ = 0;

  QString last_find_text_;
  QString last_replace_text_;
  bool last_whole_word_ = false;
  bool last_match_case_ = false;

  QMdiArea* mdi_area_ = nullptr;
  QMenu* window_menu_ = nullptr;
  QCheckBox* auto_save_chk_ = nullptr;

  EditorFindDialog* find_dialog_ = nullptr;
  EditorReplaceDialog* replace_dialog_ = nullptr;
  gui::HelpViewer* help_viewer_ = nullptr;
  gui::ExamplesBrowserDialog* examples_browser_ = nullptr;

  std::function<AssemblerStatusDialog::ActionChoice(const QString&, int, int, bool, bool)>
      status_dialog_handler_;
  std::function<bool(const QString&)> simulator_launcher_;
  std::function<QString(EditorChild*)> save_as_path_provider_;

  // Actions
  QAction* new_act_ = nullptr;
  QAction* open_act_ = nullptr;
  QAction* examples_act_ = nullptr;
  QAction* reload_act_ = nullptr;
  QAction* save_act_ = nullptr;
  QAction* save_as_act_ = nullptr;
  QAction* close_act_ = nullptr;
  QAction* print_act_ = nullptr;
  QAction* print_setup_act_ = nullptr;
  QAction* print_preview_act_ = nullptr;
  QAction* exit_act_ = nullptr;

  QAction* undo_act_ = nullptr;
  QAction* redo_act_ = nullptr;
  QAction* cut_act_ = nullptr;
  QAction* copy_act_ = nullptr;
  QAction* paste_act_ = nullptr;
  QAction* select_all_act_ = nullptr;
  QAction* find_act_ = nullptr;
  QAction* find_next_act_ = nullptr;
  QAction* replace_act_ = nullptr;
  QAction* comment_act_ = nullptr;
  QAction* uncomment_act_ = nullptr;
  QAction* indent_act_ = nullptr;
  QAction* outdent_act_ = nullptr;

  QAction* assemble_act_ = nullptr;

  QAction* asm_options_act_ = nullptr;
  QAction* editor_options_act_ = nullptr;
  QAction* change_font_act_ = nullptr;
  QAction* replace_tabs_act_ = nullptr;

  QAction* cascade_act_ = nullptr;
  QAction* tile_horz_act_ = nullptr;
  QAction* tile_vert_act_ = nullptr;
  QAction* arrange_icons_act_ = nullptr;
  QAction* minimize_all_act_ = nullptr;

  QAction* help_act_ = nullptr;
  QAction* about_act_ = nullptr;
};

}  // namespace easym68k::editor

#endif  // EASY68K_GUI_EDITOR_MAINWINDOW_H_
