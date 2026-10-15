// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "editor/editor_child.h"

#include <algorithm>
#include <cctype>
#include <QAbstractItemView>
#include <QAction>
#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QFile>
#include <QFileInfo>
#include <QFontMetricsF>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QPalette>
#include <QProgressBar>
#include <QSplitter>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace easym68k::editor {

int SpacesToNextTabStop(int col, TabMode mode, int fixed_tab_size) {
  const int safe_size = std::max(1, fixed_tab_size);
  if (mode == TabMode::kAssembly) {
    if (col < kTab1)
      return kTab1 - col;
    if (col < kTab2)
      return kTab2 - col;
    if (col < kTab3)
      return kTab3 - col;
  }
  return safe_size - (col % safe_size);
}

int VisualColumnAt(const QString& line, int char_offset, TabMode mode, int fixed_tab_size) {
  const int limit = std::clamp(char_offset, 0, static_cast<int>(line.size()));
  int col = 0;
  for (int i = 0; i < limit; ++i) {
    if (line[i] == QLatin1Char('\t')) {
      col += SpacesToNextTabStop(col, mode, fixed_tab_size);
    } else {
      ++col;
    }
  }
  return col;
}

QString ExpandTabsInText(const QString& text, int initial_col, TabMode mode, int fixed_tab_size) {
  QString out;
  out.reserve(text.size());
  int col = std::max(0, initial_col);
  for (int i = 0; i < text.size(); ++i) {
    const QChar ch = text[i];
    if (ch == QLatin1Char('\t')) {
      const int spaces = SpacesToNextTabStop(col, mode, fixed_tab_size);
      out.append(QString(spaces, QLatin1Char(' ')));
      col += spaces;
    } else if (ch == QLatin1Char('\n') || ch == QLatin1Char('\r')) {
      out.append(ch);
      col = 0;
    } else {
      out.append(ch);
      ++col;
    }
  }
  return out;
}

EditorTextEdit::EditorTextEdit(EditorChild* owner, QWidget* parent)
    : QPlainTextEdit(parent), owner_(owner) {
  setLineWrapMode(QPlainTextEdit::NoWrap);
}

void EditorTextEdit::keyPressEvent(QKeyEvent* event) {
  const Qt::KeyboardModifiers mods = event->modifiers();
  const int key = event->key();

  // Prevent Ctrl+Backspace / Ctrl+Delete / Ctrl+E as in Edit68K/textS.cpp.
  if (mods == Qt::ControlModifier && key == Qt::Key_E) {
    event->accept();
    return;
  }

  if (key == Qt::Key_F1) {
    if (owner_ != nullptr) {
      emit HelpRequested(owner_->CurrentHelpToken());
    }
    event->accept();
    return;
  }

  if (key == Qt::Key_F3) {
    emit FindNextRequested();
    event->accept();
    return;
  }

  if (key == Qt::Key_Insert && mods == Qt::NoModifier) {
    setOverwriteMode(!overwriteMode());
    emit StatusChanged();
    event->accept();
    return;
  }

  if ((mods & (Qt::ControlModifier | Qt::ShiftModifier)) ==
      (Qt::ControlModifier | Qt::ShiftModifier)) {
    if (key == Qt::Key_Greater || key == Qt::Key_Period) {
      if (owner_ != nullptr)
        owner_->IndentSelection();
      event->accept();
      return;
    }
    if (key == Qt::Key_Less || key == Qt::Key_Comma) {
      if (owner_ != nullptr)
        owner_->OutdentSelection();
      event->accept();
      return;
    }
  }

  if (key == Qt::Key_Tab && (mods == Qt::NoModifier) && owner_ != nullptr && !owner_->real_tabs()) {
    QTextCursor cursor = textCursor();
    const QString line_text = cursor.block().text();
    const int col =
        VisualColumnAt(line_text, cursor.positionInBlock(), owner_->tab_mode(), owner_->tab_size());
    const int spaces = SpacesToNextTabStop(col, owner_->tab_mode(), owner_->tab_size());
    cursor.insertText(QString(spaces, QLatin1Char(' ')));
    setTextCursor(cursor);
    emit StatusChanged();
    event->accept();
    return;
  }

  if ((key == Qt::Key_Return || key == Qt::Key_Enter) && (mods == Qt::NoModifier) &&
      owner_ != nullptr && owner_->auto_indent()) {
    QTextCursor cursor = textCursor();
    const QString line_text = cursor.block().text();
    const int max_lead = std::min(cursor.positionInBlock(), static_cast<int>(line_text.size()));
    int lead_len = 0;
    while (lead_len < max_lead &&
           (line_text[lead_len] == QLatin1Char(' ') || line_text[lead_len] == QLatin1Char('\t'))) {
      ++lead_len;
    }
    const QString leading = line_text.left(lead_len);
    cursor.beginEditBlock();
    cursor.insertBlock();
    if (!leading.isEmpty()) {
      cursor.insertText(leading);
    }
    cursor.endEditBlock();
    setTextCursor(cursor);
    emit StatusChanged();
    event->accept();
    return;
  }

  QPlainTextEdit::keyPressEvent(event);
  emit StatusChanged();
}

void EditorTextEdit::contextMenuEvent(QContextMenuEvent* event) {
  QMenu menu(this);
  QAction* undo_act = menu.addAction(QStringLiteral("&Undo"));
  undo_act->setEnabled(document()->isUndoAvailable());
  connect(undo_act, &QAction::triggered, this, &QPlainTextEdit::undo);

  QAction* redo_act = menu.addAction(QStringLiteral("&Redo"));
  redo_act->setEnabled(document()->isRedoAvailable());
  connect(redo_act, &QAction::triggered, this, &QPlainTextEdit::redo);

  menu.addSeparator();
  QAction* cut_act = menu.addAction(QStringLiteral("Cu&t"));
  cut_act->setEnabled(textCursor().hasSelection());
  connect(cut_act, &QAction::triggered, this, &QPlainTextEdit::cut);

  QAction* copy_act = menu.addAction(QStringLiteral("&Copy"));
  copy_act->setEnabled(textCursor().hasSelection());
  connect(copy_act, &QAction::triggered, this, &QPlainTextEdit::copy);

  QAction* paste_act = menu.addAction(QStringLiteral("&Paste"));
  connect(paste_act, &QAction::triggered, this, &QPlainTextEdit::paste);

  QAction* select_all_act = menu.addAction(QStringLiteral("Select &All"));
  connect(select_all_act, &QAction::triggered, this, &QPlainTextEdit::selectAll);

  menu.addSeparator();
  QAction* comment_act = menu.addAction(QStringLiteral("C&omment Selection"));
  connect(comment_act, &QAction::triggered, this, &EditorTextEdit::CommentRequested);

  QAction* uncomment_act = menu.addAction(QStringLiteral("Unco&mment Selection"));
  connect(uncomment_act, &QAction::triggered, this, &EditorTextEdit::UncommentRequested);

  menu.addSeparator();
  QAction* reload_act = menu.addAction(QStringLiteral("&Reload File"));
  reload_act->setEnabled(owner_ != nullptr && owner_->has_name());
  connect(reload_act, &QAction::triggered, this, &EditorTextEdit::ReloadRequested);

  menu.exec(event->globalPos());
}

EditorChild::EditorChild(QWidget* parent) : QWidget(parent) {
  auto* root_layout = new QVBoxLayout(this);
  root_layout->setContentsMargins(0, 0, 0, 0);
  root_layout->setSpacing(0);

  splitter_ = new QSplitter(Qt::Vertical, this);
  editor_ = new EditorTextEdit(this, splitter_);
  highlighter_ = new M68kSyntaxHighlighter(editor_->document());

  messages_ = new QTreeWidget(splitter_);
  messages_->setColumnCount(2);
  messages_->setHeaderLabels({QStringLiteral("Line "), QStringLiteral("Error Message")});
  messages_->setRootIsDecorated(false);
  messages_->setSelectionBehavior(QAbstractItemView::SelectRows);
  messages_->header()->setStretchLastSection(true);
  messages_->header()->resizeSection(0, 72);
  messages_->setContextMenuPolicy(Qt::CustomContextMenu);
  messages_->setVisible(false);

  splitter_->addWidget(editor_);
  splitter_->addWidget(messages_);
  splitter_->setStretchFactor(0, 4);
  splitter_->setStretchFactor(1, 1);

  root_layout->addWidget(splitter_, 1);

  auto* status_row = new QHBoxLayout();
  status_row->setContentsMargins(6, 2, 6, 2);
  status_row->setSpacing(12);

  status_pos_ = new QLabel(QStringLiteral("1: 1"), this);
  status_pos_->setMinimumWidth(80);
  status_mod_ = new QLabel(QString(), this);
  status_mod_->setMinimumWidth(70);
  status_mode_ = new QLabel(QStringLiteral("Insert"), this);
  status_mode_->setMinimumWidth(60);
  progress_bar_ = new QProgressBar(this);
  progress_bar_->setRange(0, 100);
  progress_bar_->setValue(0);
  progress_bar_->setTextVisible(false);
  progress_bar_->setMaximumWidth(140);
  progress_bar_->setMaximumHeight(14);
  progress_bar_->setVisible(false);

  status_row->addWidget(status_pos_);
  status_row->addWidget(status_mod_);
  status_row->addWidget(status_mode_);
  status_row->addStretch(1);
  status_row->addWidget(progress_bar_);
  root_layout->addLayout(status_row);

  connect(editor_, &QPlainTextEdit::cursorPositionChanged, this, &EditorChild::UpdateStatusBar);
  connect(editor_->document(), &QTextDocument::modificationChanged, this,
          [this](bool) { UpdateStatusBar(); });
  connect(editor_, &EditorTextEdit::StatusChanged, this, &EditorChild::UpdateStatusBar);
  connect(editor_, &EditorTextEdit::HelpRequested, this, &EditorChild::HelpRequested);
  connect(editor_, &EditorTextEdit::FindNextRequested, this, &EditorChild::FindNextRequested);
  connect(editor_, &EditorTextEdit::CommentRequested, this, &EditorChild::CommentSelection);
  connect(editor_, &EditorTextEdit::UncommentRequested, this, &EditorChild::UncommentSelection);
  connect(editor_, &EditorTextEdit::ReloadRequested, this, [this] { emit ReloadRequested(this); });

  connect(messages_, &QTreeWidget::itemDoubleClicked, this, &EditorChild::OnMessageDoubleClicked);
  connect(messages_, &QTreeWidget::itemActivated, this, &EditorChild::OnMessageDoubleClicked);
  connect(messages_, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
    QMenu menu(messages_);
    QAction* clear = menu.addAction(QStringLiteral("&Clear Error Messages"));
    connect(clear, &QAction::triggered, this, &EditorChild::ClearErrorMessages);
    menu.exec(messages_->viewport()->mapToGlobal(pos));
  });
}

void EditorChild::InitNewSource(const QString& untitled_name, const QString& template_text,
                                const QFont& font, TabMode tab_mode, int tab_size, bool auto_indent,
                                bool real_tabs, const SyntaxTheme& theme) {
  current_file_ = untitled_name;
  has_name_ = false;
  is_source_ = true;
  tab_mode_ = tab_mode;
  tab_size_ = std::clamp(tab_size, 1, 32);
  auto_indent_ = auto_indent;
  real_tabs_ = real_tabs;

  setWindowTitle(QFileInfo(current_file_).fileName());
  editor_->setFont(font);
  highlighter_->SetEnabled(true);
  SetSyntaxTheme(theme);

  QString normalized = template_text;
  normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
  normalized.replace(QLatin1Char('\r'), QLatin1Char('\n'));
  editor_->setPlainText(normalized);
  if (!real_tabs_) {
    editor_->selectAll();
    ReplaceTabsInSelection();
    QTextCursor c = editor_->textCursor();
    c.movePosition(QTextCursor::Start);
    editor_->setTextCursor(c);
  }
  editor_->document()->clearUndoRedoStacks();
  editor_->document()->setModified(false);
  ClearErrorMessages();
  UpdateStatusBar();
}

bool EditorChild::LoadFromFile(const QString& path, const QFont& default_font,
                               TabMode default_tab_mode, int default_tab_size, bool auto_indent,
                               bool real_tabs, const SyntaxTheme& theme) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }
  QByteArray raw = file.readAll();
  file.close();

  QString text = QString::fromLatin1(raw);
  text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
  text.replace(QLatin1Char('\r'), QLatin1Char('\n'));

  const QString ext = QFileInfo(path).suffix().toUpper();
  is_source_ = (ext != QStringLiteral("L68") && ext != QStringLiteral("S68"));
  current_file_ = path;
  has_name_ = true;
  tab_mode_ = default_tab_mode;
  tab_size_ = std::clamp(default_tab_size, 1, 32);
  auto_indent_ = auto_indent;
  real_tabs_ = real_tabs;

  QFont doc_font = default_font;
  QStringList lines = text.split(QLatin1Char('\n'));

  if (is_source_) {
    // Strip trailing "*~Font name~...~", "*~Font size~...~",
    // "*~Tab type~...~", "*~Tab size~...~" metadata lines (Edit68K/textS.cpp
    // LoadFile()).
    bool stripped_any = false;
    int inspect_limit = std::max(0, static_cast<int>(lines.size()) - 7);
    for (int i = static_cast<int>(lines.size()) - 1; i >= inspect_limit; --i) {
      const QString line = lines[i];
      if (line.isEmpty() && i == static_cast<int>(lines.size()) - 1) {
        lines.removeAt(i);
        continue;
      }
      if (line.startsWith(QStringLiteral("*~Tab size~")) && line.endsWith(QLatin1Char('~'))) {
        const QString val = line.mid(11, line.size() - 12);
        const int parsed = val.toInt();
        if (parsed > 0)
          tab_size_ = std::clamp(parsed, 1, 32);
        lines.removeAt(i);
        stripped_any = true;
      } else if (line.startsWith(QStringLiteral("*~Tab type~")) &&
                 line.endsWith(QLatin1Char('~'))) {
        const QString val = line.mid(11, line.size() - 12);
        tab_mode_ = (val == QStringLiteral("0")) ? TabMode::kAssembly : TabMode::kFixed;
        lines.removeAt(i);
        stripped_any = true;
      } else if (line.startsWith(QStringLiteral("*~Font size~")) &&
                 line.endsWith(QLatin1Char('~'))) {
        const QString val = line.mid(12, line.size() - 13);
        const int parsed = val.toInt();
        if (parsed > 0)
          doc_font.setPointSize(parsed);
        lines.removeAt(i);
        stripped_any = true;
      } else if (line.startsWith(QStringLiteral("*~Font name~")) &&
                 line.endsWith(QLatin1Char('~'))) {
        const QString val = line.mid(12, line.size() - 13).trimmed();
        if (!val.isEmpty())
          doc_font.setFamily(val);
        lines.removeAt(i);
        stripped_any = true;
      }
    }
    // SaveFont() prepends a leading '\n' before "*~Font name~"; remove that
    // extra empty line if we stripped metadata lines.
    if (stripped_any && !lines.isEmpty() && lines.back().isEmpty()) {
      lines.removeLast();
    }
  }

  setWindowTitle(QFileInfo(current_file_).fileName());
  editor_->setFont(doc_font);
  highlighter_->SetEnabled(is_source_);
  SetSyntaxTheme(theme);

  editor_->setPlainText(lines.join(QLatin1Char('\n')));
  QTextCursor cursor = editor_->textCursor();
  cursor.movePosition(QTextCursor::Start);
  editor_->setTextCursor(cursor);
  editor_->document()->clearUndoRedoStacks();
  editor_->document()->setModified(false);
  ClearErrorMessages();
  UpdateStatusBar();
  return true;
}

bool EditorChild::SaveToFile(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
    return false;
  }

  QTextStream out(&file);
  out << editor_->toPlainText();

  // Edit68K/mainS.cpp SaveFont(): append font & tab trailer to .X68 source
  // files.
  const QString ext = QFileInfo(path).suffix().toUpper();
  if (is_source_ && ext == QStringLiteral("X68")) {
    const QFont f = editor_->font();
    const int font_size = f.pointSize() > 0 ? f.pointSize() : 10;
    const int tab_type_int = (tab_mode_ == TabMode::kAssembly) ? 0 : 1;
    out << "\n"
        << "*~Font name~" << f.family() << "~\n"
        << "*~Font size~" << font_size << "~\n"
        << "*~Tab type~" << tab_type_int << "~\n"
        << "*~Tab size~" << tab_size_ << "~\n";
  }
  file.close();

  current_file_ = path;
  has_name_ = true;
  setWindowTitle(QFileInfo(current_file_).fileName());
  editor_->document()->setModified(false);
  UpdateStatusBar();
  return true;
}

bool EditorChild::ReloadFromDisk(const std::function<bool(const QString&)>& confirm_discard) {
  if (!has_name_ || current_file_.isEmpty())
    return false;
  if (is_modified() && confirm_discard) {
    const QString msg = QStringLiteral(
                            "%1 is modified. Do you want to discard changes and "
                            "reopen?")
                            .arg(QFileInfo(current_file_).fileName());
    if (!confirm_discard(msg)) {
      return false;
    }
  }
  return LoadFromFile(current_file_, editor_->font(), tab_mode_, tab_size_, auto_indent_,
                      real_tabs_, highlighter_->theme());
}

void EditorChild::PrefixSelectedLines(const QString& prefix) {
  QTextCursor cursor = editor_->textCursor();
  const int sel_start = cursor.selectionStart();
  int sel_end = cursor.selectionEnd();
  const QTextBlock start_block = editor_->document()->findBlock(sel_start);
  QTextBlock end_block =
      editor_->document()->findBlock(sel_end > sel_start ? sel_end - 1 : sel_start);

  cursor.beginEditBlock();
  for (QTextBlock block = start_block; block.isValid(); block = block.next()) {
    QTextCursor line_cursor(block);
    line_cursor.movePosition(QTextCursor::StartOfBlock);
    line_cursor.insertText(prefix);
    if (block == end_block)
      break;
  }
  cursor.endEditBlock();
  UpdateStatusBar();
}

void EditorChild::UnprefixSelectedLines(const QString& prefix) {
  QTextCursor cursor = editor_->textCursor();
  const int sel_start = cursor.selectionStart();
  const int sel_end = cursor.selectionEnd();
  const QTextBlock start_block = editor_->document()->findBlock(sel_start);
  const QTextBlock end_block =
      editor_->document()->findBlock(sel_end > sel_start ? sel_end - 1 : sel_start);

  cursor.beginEditBlock();
  for (QTextBlock block = start_block; block.isValid(); block = block.next()) {
    if (block.text().startsWith(prefix)) {
      QTextCursor line_cursor(block);
      line_cursor.movePosition(QTextCursor::StartOfBlock);
      line_cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, prefix.size());
      line_cursor.removeSelectedText();
    }
    if (block == end_block)
      break;
  }
  cursor.endEditBlock();
  UpdateStatusBar();
}

void EditorChild::CommentSelection() {
  PrefixSelectedLines(QStringLiteral("*"));
}

void EditorChild::UncommentSelection() {
  UnprefixSelectedLines(QStringLiteral("*"));
}

void EditorChild::IndentSelection() {
  PrefixSelectedLines(QStringLiteral(" "));
}

void EditorChild::OutdentSelection() {
  UnprefixSelectedLines(QStringLiteral(" "));
}

void EditorChild::ReplaceTabsInSelection() {
  QTextCursor cursor = editor_->textCursor();
  if (!cursor.hasSelection())
    return;

  const int sel_start = cursor.selectionStart();
  const QTextBlock start_block = editor_->document()->findBlock(sel_start);
  const int initial_col = sel_start - start_block.position();
  QString selected = cursor.selectedText();
  selected.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
  const QString expanded = ExpandTabsInText(selected, initial_col, tab_mode_, tab_size_);
  if (expanded != selected) {
    cursor.insertText(expanded);
    editor_->setTextCursor(cursor);
  }
}

bool EditorChild::FindText(const QString& needle, bool whole_word, bool match_case) {
  if (needle.isEmpty())
    return false;

  QTextDocument::FindFlags flags;
  if (whole_word)
    flags |= QTextDocument::FindWholeWords;
  if (match_case)
    flags |= QTextDocument::FindCaseSensitively;

  QTextCursor cursor = editor_->textCursor();
  if (!cursor.hasSelection()) {
    cursor.movePosition(QTextCursor::Start);
  } else {
    cursor.setPosition(cursor.selectionEnd());
  }

  QTextCursor found = editor_->document()->find(needle, cursor, flags);
  if (!found.isNull()) {
    editor_->setTextCursor(found);
    UpdateStatusBar();
    return true;
  }

  // Clear selection on reaching end of document (matching Edit68K/mainS.cpp
  // FindDialogFind: Active->SourceText->SelLength = 0).
  QTextCursor clear_cursor = editor_->textCursor();
  clear_cursor.clearSelection();
  editor_->setTextCursor(clear_cursor);
  UpdateStatusBar();
  return false;
}

int EditorChild::ReplaceOccurrences(const QString& needle, const QString& replacement,
                                    bool whole_word, bool match_case, bool replace_all) {
  if (needle.isEmpty())
    return 0;

  if (!editor_->textCursor().hasSelection()) {
    if (!FindText(needle, whole_word, match_case)) {
      return 0;
    }
  }

  int replaced_count = 0;
  QTextCursor edit_cursor = editor_->textCursor();
  edit_cursor.beginEditBlock();
  do {
    QTextCursor cur = editor_->textCursor();
    if (!cur.hasSelection())
      break;
    const int start_pos = cur.selectionStart();
    cur.insertText(replacement);
    ++replaced_count;

    // Position after the replaced text and search for the next match.
    QTextDocument::FindFlags flags;
    if (whole_word)
      flags |= QTextDocument::FindWholeWords;
    if (match_case)
      flags |= QTextDocument::FindCaseSensitively;

    QTextCursor search_from = editor_->textCursor();
    search_from.setPosition(start_pos + replacement.size());
    QTextCursor next = editor_->document()->find(needle, search_from, flags);
    if (!next.isNull()) {
      editor_->setTextCursor(next);
    } else {
      QTextCursor end_cur = editor_->textCursor();
      end_cur.clearSelection();
      editor_->setTextCursor(end_cur);
      break;
    }
  } while (replace_all && editor_->textCursor().hasSelection());
  edit_cursor.endEditBlock();
  UpdateStatusBar();
  return replaced_count;
}

void EditorChild::SetDiagnostics(const std::vector<asm_::AssemblyError>& errors) {
  messages_->clear();
  for (const asm_::AssemblyError& err : errors) {
    auto* item = new QTreeWidgetItem(messages_);
    item->setText(0, QString::number(err.line_number));
    item->setText(1, QString::fromStdString(err.message).trimmed());
    item->setData(0, Qt::UserRole, err.line_number);
    item->setData(1, Qt::UserRole, QString::fromStdString(err.file));
  }
  messages_->setVisible(!errors.empty());
}

void EditorChild::ClearErrorMessages() {
  messages_->clear();
  messages_->setVisible(false);
}

void EditorChild::JumpToLine(int one_based_line) {
  const int block_idx = std::max(0, one_based_line - 1);
  const QTextBlock block = editor_->document()->findBlockByNumber(block_idx);
  if (!block.isValid())
    return;

  QTextCursor cursor(block);
  cursor.movePosition(QTextCursor::StartOfBlock);
  cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
  editor_->setTextCursor(cursor);
  editor_->centerCursor();
  editor_->setFocus();
  UpdateStatusBar();
}

void EditorChild::OnMessageDoubleClicked(QTreeWidgetItem* item, int /*column*/) {
  if (item == nullptr)
    return;
  const QString inc_file = item->data(1, Qt::UserRole).toString();
  if (!inc_file.isEmpty() && inc_file != current_file_) {
    return;
  }
  bool ok = false;
  const int line_num = item->data(0, Qt::UserRole).toInt(&ok);
  if (ok && line_num > 0) {
    JumpToLine(line_num);
  }
}

QString EditorChild::CurrentHelpToken() const {
  const QTextCursor cursor = editor_->textCursor();
  if (cursor.hasSelection()) {
    QString sel = cursor.selectedText();
    sel.replace(QChar::ParagraphSeparator, QLatin1Char(' '));
    sel = sel.trimmed();
    if (!sel.isEmpty())
      return sel;
  }

  const QString line = cursor.block().text();
  const int pos = cursor.positionInBlock();
  if (line.isEmpty() || pos < 0 || pos >= line.size() || line[pos].isSpace()) {
    return QStringLiteral("INTRO");
  }

  auto is_word_char = [](QChar c) { return c.isLetterOrNumber(); };
  if (!is_word_char(line[pos])) {
    return QStringLiteral("INTRO");
  }

  int start = pos;
  while (start > 0 && is_word_char(line[start - 1])) {
    --start;
  }
  int end = pos + 1;
  while (end < line.size() && is_word_char(line[end])) {
    ++end;
  }
  return line.mid(start, end - start);
}

void EditorChild::SetDocumentFont(const QFont& font) {
  editor_->setFont(font);
  ApplyBackgroundAndTabDistance();
}

void EditorChild::SetTabSettings(TabMode mode, int fixed_size) {
  tab_mode_ = mode;
  tab_size_ = std::clamp(fixed_size, 1, 32);
  ApplyBackgroundAndTabDistance();
  UpdateStatusBar();
}

void EditorChild::SetSyntaxTheme(const SyntaxTheme& theme) {
  highlighter_->SetTheme(theme);
  ApplyBackgroundAndTabDistance();
}

void EditorChild::ApplyBackgroundAndTabDistance() {
  QPalette pal = editor_->palette();
  pal.setColor(QPalette::Base, highlighter_->theme().background);
  pal.setColor(QPalette::Text, highlighter_->theme().code.color);
  editor_->setPalette(pal);

  const QFontMetricsF fm(editor_->font());
  const qreal space_w = std::max(1.0, fm.horizontalAdvance(QLatin1Char(' ')));
  editor_->setTabStopDistance(space_w * std::max(1, tab_size_));
}

void EditorChild::UpdateStatusBar() {
  const QTextCursor cursor = editor_->textCursor();
  const int line_num = cursor.blockNumber() + 1;
  const int col_num =
      VisualColumnAt(cursor.block().text(), cursor.positionInBlock(), tab_mode_, tab_size_) + 1;
  if (is_source_) {
    status_pos_->setText(QStringLiteral("%1: %2").arg(line_num).arg(col_num));
  } else {
    status_pos_->setText(QStringLiteral("ln %1  col %2").arg(line_num).arg(col_num));
  }
  status_mod_->setText(is_modified() ? QStringLiteral("Modified") : QString());
  status_mode_->setText(editor_->overwriteMode() ? QStringLiteral("Replace")
                                                 : QStringLiteral("Insert"));
  emit StatusUpdated();
}

void EditorChild::closeEvent(QCloseEvent* event) {
  if (is_modified() && close_prompt_) {
    const QString msg = QStringLiteral(
                            "File modified, do you wish to save '%1' before "
                            "closing?")
                            .arg(QFileInfo(current_file_).fileName());
    const CloseDecision decision = close_prompt_(msg);
    if (decision == CloseDecision::kCancel) {
      event->ignore();
      return;
    }
    if (decision == CloseDecision::kSave && save_handler_) {
      if (!save_handler_(this)) {
        event->ignore();
        return;
      }
    }
  }
  event->accept();
}

}  // namespace easym68k::editor
