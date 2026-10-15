// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef EASY68K_GUI_EDITOR_EDITOR_CHILD_H_
#define EASY68K_GUI_EDITOR_EDITOR_CHILD_H_

#include <functional>
#include <QFont>
#include <QPlainTextEdit>
#include <QString>
#include <QWidget>
#include <vector>

#include "easym68k/asm/assembler.h"
#include "editor/syntax_highlighter.h"

class QCloseEvent;
class QKeyEvent;
class QLabel;
class QProgressBar;
class QSplitter;
class QTreeWidget;
class QTreeWidgetItem;

namespace easym68k::editor {

enum class TabMode {
  kAssembly = 0,
  kFixed = 1,
};

inline constexpr int kTab1 = 12;
inline constexpr int kTab2 = 20;
inline constexpr int kTab3 = 44;

int SpacesToNextTabStop(int col, TabMode mode, int fixed_tab_size);
int VisualColumnAt(const QString& line, int char_offset, TabMode mode, int fixed_tab_size);
QString ExpandTabsInText(const QString& text, int initial_col, TabMode mode, int fixed_tab_size);

class EditorChild;

class EditorTextEdit : public QPlainTextEdit {
  Q_OBJECT

 public:
  explicit EditorTextEdit(EditorChild* owner, QWidget* parent = nullptr);

 signals:
  void HelpRequested(const QString& context);
  void FindNextRequested();
  void ReloadRequested();
  void CommentRequested();
  void UncommentRequested();
  void StatusChanged();

 protected:
  void keyPressEvent(QKeyEvent* event) override;
  void contextMenuEvent(QContextMenuEvent* event) override;

 private:
  EditorChild* owner_ = nullptr;
};

// Represents one MDI child document window (Edit68K/textS.dfm TTextStuff and
// Edit68K/listS.dfm TListFile).
class EditorChild : public QWidget {
  Q_OBJECT

 public:
  enum class CloseDecision {
    kSave,
    kDiscard,
    kCancel,
  };

  explicit EditorChild(QWidget* parent = nullptr);

  void InitNewSource(const QString& untitled_name, const QString& template_text, const QFont& font,
                     TabMode tab_mode, int tab_size, bool auto_indent, bool real_tabs,
                     const SyntaxTheme& theme);

  bool LoadFromFile(const QString& path, const QFont& default_font, TabMode default_tab_mode,
                    int default_tab_size, bool auto_indent, bool real_tabs,
                    const SyntaxTheme& theme);

  bool SaveToFile(const QString& path);

  bool ReloadFromDisk(const std::function<bool(const QString&)>& confirm_discard);

  void CommentSelection();
  void UncommentSelection();
  void IndentSelection();
  void OutdentSelection();
  void ReplaceTabsInSelection();

  bool FindText(const QString& needle, bool whole_word, bool match_case);
  int ReplaceOccurrences(const QString& needle, const QString& replacement, bool whole_word,
                         bool match_case, bool replace_all);

  void SetDiagnostics(const std::vector<asm_::AssemblyError>& errors);
  void ClearErrorMessages();
  void JumpToLine(int one_based_line);

  QString CurrentHelpToken() const;

  void SetDocumentFont(const QFont& font);
  QFont DocumentFont() const { return editor_->font(); }

  void SetTabSettings(TabMode mode, int fixed_size);
  TabMode tab_mode() const { return tab_mode_; }
  int tab_size() const { return tab_size_; }

  void SetAutoIndent(bool enabled) { auto_indent_ = enabled; }
  bool auto_indent() const { return auto_indent_; }

  void SetRealTabs(bool enabled) { real_tabs_ = enabled; }
  bool real_tabs() const { return real_tabs_; }

  void SetSyntaxTheme(const SyntaxTheme& theme);
  const SyntaxTheme& syntax_theme() const { return highlighter_->theme(); }

  const QString& current_file() const { return current_file_; }
  bool has_name() const { return has_name_; }
  bool is_source() const { return is_source_; }
  bool is_modified() const { return editor_->document()->isModified(); }
  void set_modified(bool modified) { editor_->document()->setModified(modified); }

  EditorTextEdit* editor() const { return editor_; }
  QTreeWidget* messages() const { return messages_; }
  QProgressBar* progress_bar() const { return progress_bar_; }
  QLabel* status_pos_label() const { return status_pos_; }
  QLabel* status_mod_label() const { return status_mod_; }
  QLabel* status_mode_label() const { return status_mode_; }

  void SetClosePrompt(std::function<CloseDecision(const QString&)> close_prompt) {
    close_prompt_ = std::move(close_prompt);
  }
  void SetSaveHandler(std::function<bool(EditorChild*)> save_handler) {
    save_handler_ = std::move(save_handler);
  }

  void UpdateStatusBar();

 signals:
  void HelpRequested(const QString& context);
  void FindNextRequested();
  void ReloadRequested(EditorChild* child);
  void StatusUpdated();

 protected:
  void closeEvent(QCloseEvent* event) override;

 private:
  void PrefixSelectedLines(const QString& prefix);
  void UnprefixSelectedLines(const QString& prefix);
  void ApplyBackgroundAndTabDistance();
  void OnMessageDoubleClicked(QTreeWidgetItem* item, int column);

  QSplitter* splitter_ = nullptr;
  EditorTextEdit* editor_ = nullptr;
  QTreeWidget* messages_ = nullptr;
  QProgressBar* progress_bar_ = nullptr;
  QLabel* status_pos_ = nullptr;
  QLabel* status_mod_ = nullptr;
  QLabel* status_mode_ = nullptr;
  M68kSyntaxHighlighter* highlighter_ = nullptr;

  QString current_file_;
  bool has_name_ = false;
  bool is_source_ = true;
  TabMode tab_mode_ = TabMode::kFixed;
  int tab_size_ = 4;
  bool auto_indent_ = true;
  bool real_tabs_ = false;

  std::function<CloseDecision(const QString&)> close_prompt_;
  std::function<bool(EditorChild*)> save_handler_;
};

}  // namespace easym68k::editor

#endif  // EASY68K_GUI_EDITOR_EDITOR_CHILD_H_
