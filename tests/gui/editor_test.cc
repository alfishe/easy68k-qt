// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <gtest/gtest.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTreeWidget>

#include "editor/dialogs.h"
#include "editor/editor_child.h"
#include "editor/mainwindow.h"
#include "editor/syntax_highlighter.h"
#include "help/help_viewer.h"

namespace easym68k::editor {
namespace {

TEST(EditorTest, ClassifyLineCoversLabelsInstructionsMovemStringsAndComments) {
  // Full-line comment starting with '*'
  const auto full_comment = M68kSyntaxHighlighter::ClassifyLine(QStringLiteral("* Full comment"));
  ASSERT_EQ(full_comment.size(), 1);
  EXPECT_EQ(full_comment[0].kind, M68kSyntaxHighlighter::SpanKind::kComment);

  // Label + MOVEM instruction + operand + comment
  // (highlightBlock sets the entire line to kCode first, and ClassifyLine emits
  // non-code spans; for MOVEM, operands are skipped as code so the kComment
  // span starts at "save regs" instead of at the operand!)
  const QString movem_line = QStringLiteral("SAVE    MOVEM.L D0-D7/A0-A6,-(SP)  save regs");
  const auto spans = M68kSyntaxHighlighter::ClassifyLine(movem_line);
  ASSERT_EQ(spans.size(), 2);
  EXPECT_EQ(spans[0].kind, M68kSyntaxHighlighter::SpanKind::kLabel);
  EXPECT_EQ(movem_line.mid(spans[0].start, spans[0].length), QStringLiteral("SAVE"));
  EXPECT_EQ(spans[1].kind, M68kSyntaxHighlighter::SpanKind::kComment);
  EXPECT_EQ(movem_line.mid(spans[1].start, spans[1].length), QStringLiteral("save regs"));

  // Directive + string literal + comment
  const QString dc_line = QStringLiteral("MSG     DC.B    'Hello',0 ; text");
  const auto dc_spans = M68kSyntaxHighlighter::ClassifyLine(dc_line);
  bool has_directive = false;
  bool has_string = false;
  for (const auto& s : dc_spans) {
    if (s.kind == M68kSyntaxHighlighter::SpanKind::kDirective) {
      has_directive = true;
    } else if (s.kind == M68kSyntaxHighlighter::SpanKind::kText) {
      has_string = true;
      EXPECT_EQ(dc_line.mid(s.start, s.length), QStringLiteral("'Hello'"));
    }
  }
  EXPECT_TRUE(has_directive);
  EXPECT_TRUE(has_string);

  // Valid structured control statement vs invalid structured statement
  const auto valid_if =
      M68kSyntaxHighlighter::ClassifyLine(QStringLiteral("        IF.B D0 <EQ> #1 THEN.S"));
  ASSERT_FALSE(valid_if.isEmpty());
  EXPECT_EQ(valid_if[0].kind, M68kSyntaxHighlighter::SpanKind::kStructure);

  const auto invalid_if =
      M68kSyntaxHighlighter::ClassifyLine(QStringLiteral("        IF.B D0 == #1"));
  ASSERT_FALSE(invalid_if.isEmpty());
  EXPECT_EQ(invalid_if[0].kind, M68kSyntaxHighlighter::SpanKind::kError);
}

TEST(EditorTest, TabStopsHandleAssemblyAndFixedModesCorrectly) {
  // Assembly tab stops are at 0-based columns 12, 20, 44, and columns >= 44
  // fall back to fixed_tab_size instead of returning <= 0.
  EXPECT_EQ(SpacesToNextTabStop(0, TabMode::kAssembly, 8), 12);
  EXPECT_EQ(SpacesToNextTabStop(5, TabMode::kAssembly, 8), 7);
  EXPECT_EQ(SpacesToNextTabStop(12, TabMode::kAssembly, 8), 8);
  EXPECT_EQ(SpacesToNextTabStop(20, TabMode::kAssembly, 8), 24);
  EXPECT_GT(SpacesToNextTabStop(50, TabMode::kAssembly, 8), 0);

  // Fixed tab mode
  EXPECT_EQ(SpacesToNextTabStop(0, TabMode::kFixed, 4), 4);
  EXPECT_EQ(SpacesToNextTabStop(1, TabMode::kFixed, 4), 3);
  EXPECT_EQ(SpacesToNextTabStop(4, TabMode::kFixed, 4), 4);

  const QString expanded =
      ExpandTabsInText(QStringLiteral("START\tMOVE.L\tD0,D1\tComment"), 0, TabMode::kAssembly, 8);
  EXPECT_FALSE(expanded.contains(QLatin1Char('\t')));
  EXPECT_EQ(expanded.indexOf(QStringLiteral("MOVE.L")), kTab1);
  EXPECT_EQ(expanded.indexOf(QStringLiteral("D0,D1")), kTab2);
  EXPECT_EQ(expanded.indexOf(QStringLiteral("Comment")), kTab3);
}

TEST(EditorTest, SaveAndLoadX68PreservesMetadataWithoutAccumulatingBlankLines) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const QString file_path = temp_dir.filePath(QStringLiteral("roundtrip.X68"));

  EditorChild child;
  const QFont initial_font(QStringLiteral("Courier New"), 12);
  child.InitNewSource(QStringLiteral("UNTITLED1.X68"),
                      QStringLiteral("START   ORG $1000\n        END START"), initial_font,
                      TabMode::kAssembly, 8, true, false,
                      SyntaxTheme::FromPreset(SyntaxTheme::kColor1));
  ASSERT_TRUE(child.SaveToFile(file_path));

  // First load
  EditorChild loaded1;
  ASSERT_TRUE(loaded1.LoadFromFile(file_path, QFont(QStringLiteral("Monospace"), 9),
                                   TabMode::kFixed, 4, true, false,
                                   SyntaxTheme::FromPreset(SyntaxTheme::kColor1)));
  EXPECT_EQ(loaded1.DocumentFont().pointSize(), 12);
  EXPECT_EQ(loaded1.tab_mode(), TabMode::kAssembly);
  EXPECT_EQ(loaded1.tab_size(), 8);
  const QString text_after_load1 = loaded1.editor()->toPlainText();
  EXPECT_EQ(text_after_load1, QStringLiteral("START   ORG $1000\n        END START"));

  // Save again and reload: text must be identical (no extra blank lines!).
  ASSERT_TRUE(loaded1.SaveToFile(file_path));
  EditorChild loaded2;
  ASSERT_TRUE(loaded2.LoadFromFile(file_path, QFont(QStringLiteral("Monospace"), 9),
                                   TabMode::kFixed, 4, true, false,
                                   SyntaxTheme::FromPreset(SyntaxTheme::kColor1)));
  EXPECT_EQ(loaded2.editor()->toPlainText(), text_after_load1);
}

TEST(EditorTest, CommentUncommentIndentOutdentAndFindReplaceWork) {
  EditorChild child;
  child.InitNewSource(QStringLiteral("UNTITLED1.X68"),
                      QStringLiteral("    MOVE.L D0,D1\n    ADD.L D1,D2\n    RTS"),
                      QFont(QStringLiteral("Courier New"), 10), TabMode::kFixed, 4, true, false,
                      SyntaxTheme::FromPreset(SyntaxTheme::kColor1));

  child.editor()->selectAll();
  child.CommentSelection();
  EXPECT_TRUE(child.editor()->toPlainText().startsWith(
      QStringLiteral("*    MOVE.L D0,D1\n*    ADD.L D1,D2")));

  child.editor()->selectAll();
  child.UncommentSelection();
  EXPECT_TRUE(child.editor()->toPlainText().startsWith(
      QStringLiteral("    MOVE.L D0,D1\n    ADD.L D1,D2")));

  child.editor()->selectAll();
  child.IndentSelection();
  EXPECT_TRUE(child.editor()->toPlainText().startsWith(QStringLiteral("     MOVE.L D0,D1")));

  child.editor()->selectAll();
  child.OutdentSelection();
  EXPECT_TRUE(child.editor()->toPlainText().startsWith(QStringLiteral("    MOVE.L D0,D1")));

  // Find and Replace
  QTextCursor c = child.editor()->textCursor();
  c.movePosition(QTextCursor::Start);
  child.editor()->setTextCursor(c);
  EXPECT_TRUE(child.FindText(QStringLiteral("D1"), true, true));
  EXPECT_EQ(child.editor()->textCursor().selectedText(), QStringLiteral("D1"));

  const int replaced =
      child.ReplaceOccurrences(QStringLiteral("D1"), QStringLiteral("D7"), true, true, true);
  EXPECT_EQ(replaced, 2);
  EXPECT_TRUE(child.editor()->toPlainText().contains(QStringLiteral("MOVE.L D0,D7")));
  EXPECT_TRUE(child.editor()->toPlainText().contains(QStringLiteral("ADD.L D7,D2")));
}

TEST(EditorTest, AutoIndentCopiesLeadingSpacesOnReturnKey) {
  EditorChild child;
  child.InitNewSource(QStringLiteral("UNTITLED1.X68"), QStringLiteral("        MOVE.L D0,D1"),
                      QFont(QStringLiteral("Courier New"), 10), TabMode::kFixed, 4,
                      /*auto_indent=*/true, /*real_tabs=*/false,
                      SyntaxTheme::FromPreset(SyntaxTheme::kColor1));

  QTextCursor cursor = child.editor()->textCursor();
  cursor.movePosition(QTextCursor::End);
  child.editor()->setTextCursor(cursor);

  QKeyEvent enter_press(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
  QApplication::sendEvent(child.editor(), &enter_press);

  const QStringList lines = child.editor()->toPlainText().split(QLatin1Char('\n'));
  ASSERT_EQ(lines.size(), 2);
  EXPECT_EQ(lines[1], QStringLiteral("        "));
}

TEST(EditorTest, AssemblerAndEditorOptionsDialogsUpdatePreferences) {
  EditorPreferences prefs;
  bool warned = false;
  bool template_saved = false;
  QString saved_template;

  AssemblerOptionsDialog asm_dlg(
      prefs, QStringLiteral("START ORG $1000"),
      [&](const QString&) {
        warned = true;
        return true;
      },
      [&](const QString& text) {
        template_saved = true;
        saved_template = text;
        return true;
      });

  // Uncheck a recommended option (*) and click OK -> triggers confirmation
  asm_dlg.gen_srec_check()->setChecked(false);
  asm_dlg.template_editor()->setPlainText(QStringLiteral("NEW TEMPLATE"));
  asm_dlg.save_template_button()->click();
  EXPECT_TRUE(template_saved);
  EXPECT_EQ(saved_template, QStringLiteral("NEW TEMPLATE"));

  asm_dlg.ok_button()->click();
  EXPECT_TRUE(warned);
  EXPECT_EQ(asm_dlg.result(), QDialog::Accepted);
  EXPECT_FALSE(asm_dlg.Preferences().asm_options.generate_srecord);

  // EditorOptionsDialog
  EditorOptionsDialog::ActiveFileInfo active_info;
  EditorOptionsDialog ed_dlg(prefs, active_info);
  ed_dlg.assembly_tabs_radio()->setChecked(true);
  ed_dlg.fixed_tab_spin()->setValue(6);
  ed_dlg.print_black_check()->setChecked(true);
  ed_dlg.SelectPreset(SyntaxTheme::kColor2);
  EXPECT_EQ(ed_dlg.UpdatedPreferences().theme.preset, SyntaxTheme::kColor2);

  // Toggling Bold on element 0 (Code) switches preset to Custom (3)
  ed_dlg.SelectElement(0);
  ed_dlg.bold_check()->setChecked(true);
  const EditorPreferences updated = ed_dlg.UpdatedPreferences();
  EXPECT_EQ(updated.tab_mode, TabMode::kAssembly);
  EXPECT_EQ(updated.fixed_tab_size, 6);
  EXPECT_TRUE(updated.print_black);
  EXPECT_EQ(updated.theme.preset, SyntaxTheme::kCustom);
  EXPECT_TRUE(updated.theme.code.bold);
}

TEST(EditorTest, MainWindowAssembleGeneratesL68AndS68AndSupportsIncludeAndSimulatorLaunch) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const QString settings_file = temp_dir.filePath(QStringLiteral("edit68k.ini"));

  // Write an included file in the same directory to verify A8 (include search path).
  const QString inc_path = temp_dir.filePath(QStringLiteral("defs.X68"));
  {
    QFile inc_file(inc_path);
    ASSERT_TRUE(inc_file.open(QIODevice::WriteOnly | QIODevice::Text));
    inc_file.write("VAL EQU $42\n");
  }

  const QString src_path = temp_dir.filePath(QStringLiteral("prog.X68"));
  {
    QFile src_file(src_path);
    ASSERT_TRUE(src_file.open(QIODevice::WriteOnly | QIODevice::Text));
    src_file.write(
        "        ORG     $1000\n"
        "        INCLUDE 'defs.X68'\n"
        "START   MOVE.B  #VAL,D0\n"
        "        SIMHALT\n"
        "        END     START\n");
  }

  MainWindow win(settings_file);
  EditorChild* child = win.OpenFile(src_path);
  ASSERT_NE(child, nullptr);

  QString launched_s68;
  win.SetSimulatorLauncher([&](const QString& s68) {
    launched_s68 = s68;
    return true;
  });

  // First assemble: choose Load L68
  win.SetStatusDialogHandler(
      [](const QString&, int warnings, int errors, bool can_exec, bool can_load) {
        EXPECT_EQ(warnings, 0);
        EXPECT_EQ(errors, 0);
        EXPECT_TRUE(can_exec);
        EXPECT_TRUE(can_load);
        return AssemblerStatusDialog::ActionChoice::kLoadL68;
      });
  const auto outcome1 = win.AssembleActiveDocument();
  EXPECT_TRUE(outcome1.result.success);
  EXPECT_TRUE(QFileInfo::exists(outcome1.list_path));
  EXPECT_TRUE(QFileInfo::exists(outcome1.srec_path));
  // Active child should now be the opened .L68 listing window!
  ASSERT_NE(win.ActiveChild(), nullptr);
  EXPECT_FALSE(win.ActiveChild()->is_source());
  EXPECT_EQ(QFileInfo(win.ActiveChild()->current_file()).suffix().toUpper(), QStringLiteral("L68"));

  // Activate the source window again and assemble with Execute
  win.OpenFile(src_path);
  ASSERT_EQ(win.ActiveChild(), child);
  win.SetStatusDialogHandler([](const QString&, int, int, bool, bool) {
    return AssemblerStatusDialog::ActionChoice::kExecute;
  });
  const auto outcome2 = win.AssembleActiveDocument();
  EXPECT_EQ(launched_s68, outcome2.srec_path);
}

TEST(EditorTest, AssembleErrorsPopulateMessagesAndDoubleClickJumpsToErrorLine) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const QString src_path = temp_dir.filePath(QStringLiteral("broken.X68"));
  {
    QFile src_file(src_path);
    ASSERT_TRUE(src_file.open(QIODevice::WriteOnly | QIODevice::Text));
    src_file.write(
        "START   ORG     $1000\n"
        "        MOVE.L  D0,D1\n"
        "        BAD_INSTR_XYZ #1,D0\n"
        "        END     START\n");
  }

  MainWindow win(temp_dir.filePath(QStringLiteral("edit68k.ini")));
  EditorChild* child = win.OpenFile(src_path);
  ASSERT_NE(child, nullptr);

  win.SetStatusDialogHandler([](const QString&, int, int errors, bool can_exec, bool) {
    EXPECT_GT(errors, 0);
    EXPECT_FALSE(can_exec);
    return AssemblerStatusDialog::ActionChoice::kClose;
  });
  const auto outcome = win.AssembleActiveDocument();
  EXPECT_FALSE(outcome.result.success);
  ASSERT_GT(child->messages()->topLevelItemCount(), 0);

  // Double-clicking the error item jumps the editor cursor to line 3.
  QTreeWidgetItem* err_item = child->messages()->topLevelItem(0);
  emit child->messages()->itemDoubleClicked(err_item, 0);
  EXPECT_EQ(child->editor()->textCursor().blockNumber() + 1, 3);
}

TEST(EditorTest, ContextSensitiveHelpOpensKeywordInHelpViewer) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  MainWindow win(temp_dir.filePath(QStringLiteral("edit68k.ini")));
  EditorChild* child = win.NewSourceWindow();
  ASSERT_NE(child, nullptr);

  child->editor()->setPlainText(QStringLiteral("        MOVE.L D0,D1"));
  QTextCursor c = child->editor()->textCursor();
  c.setPosition(9);  // Inside "MOVE"
  child->editor()->setTextCursor(c);
  EXPECT_EQ(child->CurrentHelpToken(), QStringLiteral("MOVE"));

  win.OpenContextHelp();
  ASSERT_NE(win.help_viewer(), nullptr);
  EXPECT_TRUE(win.help_viewer()->current_page().endsWith(QStringLiteral("Reference/68ks4d.htm")));
}

TEST(EditorTest, SettingsRoundTripPersistsPreferences) {
  QTemporaryDir temp_dir;
  ASSERT_TRUE(temp_dir.isValid());
  const QString ini_path = temp_dir.filePath(QStringLiteral("edit68k.ini"));

  {
    MainWindow win1(ini_path);
    EditorPreferences prefs = win1.preferences();
    prefs.auto_save = true;
    prefs.font_size = 14;
    prefs.tab_mode = TabMode::kAssembly;
    prefs.fixed_tab_size = 6;
    prefs.print_black = true;
    prefs.asm_options.bitfield = false;
    prefs.theme = SyntaxTheme::FromPreset(SyntaxTheme::kColor2);
    win1.SetPreferences(prefs);
    win1.SaveSettings();
  }

  MainWindow win2(ini_path);
  EXPECT_TRUE(win2.preferences().auto_save);
  EXPECT_EQ(win2.preferences().font_size, 14);
  EXPECT_EQ(win2.preferences().tab_mode, TabMode::kAssembly);
  EXPECT_EQ(win2.preferences().fixed_tab_size, 6);
  EXPECT_TRUE(win2.preferences().print_black);
  EXPECT_FALSE(win2.preferences().asm_options.bitfield);
  EXPECT_EQ(win2.preferences().theme.preset, SyntaxTheme::kColor2);
}

}  // namespace
}  // namespace easym68k::editor
