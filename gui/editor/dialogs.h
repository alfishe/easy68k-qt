// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef EASY68K_GUI_EDITOR_DIALOGS_H_
#define EASY68K_GUI_EDITOR_DIALOGS_H_

#include <functional>
#include <QColor>
#include <QDialog>
#include <QFont>
#include <QString>

#include "easym68k/asm/assembler.h"
#include "editor/editor_child.h"
#include "editor/syntax_highlighter.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QTabWidget;

namespace easym68k::editor {

struct EditorPreferences {
  asm_::AssemblerOptions asm_options;
  bool auto_save = false;
  QString font_name = QStringLiteral("Courier New");
  int font_size = 10;
  TabMode tab_mode = TabMode::kFixed;
  int fixed_tab_size = 4;
  bool maximized_edit = true;
  bool auto_indent = true;
  bool real_tabs = false;
  bool print_black = false;
  SyntaxTheme theme;
};

// EASy68K Assembler Options + Template dialog (Edit68K/optionsS.dfm).
class AssemblerOptionsDialog : public QDialog {
  Q_OBJECT

 public:
  explicit AssemblerOptionsDialog(const EditorPreferences& prefs, const QString& template_text,
                                  std::function<bool(const QString&)> confirm_recommended = {},
                                  std::function<bool(const QString&)> save_template = {},
                                  QWidget* parent = nullptr);

  EditorPreferences Preferences() const;
  QString TemplateText() const;

  QTabWidget* tabs() const { return tabs_; }
  QCheckBox* gen_list_check() const { return gen_list_chk_; }
  QCheckBox* gen_srec_check() const { return gen_srec_chk_; }
  QCheckBox* auto_save_check() const { return auto_save_chk_; }
  QCheckBox* show_warnings_check() const { return show_warnings_chk_; }
  QCheckBox* bitfield_check() const { return bitfield_chk_; }
  QCheckBox* cross_ref_check() const { return cross_ref_chk_; }
  QCheckBox* mac_ex_check() const { return mac_ex_chk_; }
  QCheckBox* const_ex_check() const { return const_ex_chk_; }
  QCheckBox* struc_ex_check() const { return struc_ex_chk_; }
  QPlainTextEdit* template_editor() const { return template_edit_; }
  QPushButton* save_template_button() const { return save_template_btn_; }
  QPushButton* ok_button() const { return ok_btn_; }

 signals:
  void HelpRequested(const QString& context);

 private:
  void OnOkClicked();
  void OnSaveTemplateClicked();

  EditorPreferences base_prefs_;
  std::function<bool(const QString&)> confirm_recommended_;
  std::function<bool(const QString&)> save_template_;

  QTabWidget* tabs_ = nullptr;
  QCheckBox* gen_list_chk_ = nullptr;
  QCheckBox* gen_srec_chk_ = nullptr;
  QCheckBox* auto_save_chk_ = nullptr;
  QCheckBox* show_warnings_chk_ = nullptr;
  QCheckBox* bitfield_chk_ = nullptr;
  QCheckBox* cross_ref_chk_ = nullptr;
  QCheckBox* mac_ex_chk_ = nullptr;
  QCheckBox* const_ex_chk_ = nullptr;
  QCheckBox* struc_ex_chk_ = nullptr;
  QPlainTextEdit* template_edit_ = nullptr;
  QPushButton* save_template_btn_ = nullptr;
  QPushButton* ok_btn_ = nullptr;
  QPushButton* cancel_btn_ = nullptr;
};

// EASy68K Editor Options + Syntax Highlight dialog (Edit68K/editorOptions.dfm).
class EditorOptionsDialog : public QDialog {
  Q_OBJECT

 public:
  struct ActiveFileInfo {
    QString font_name = QStringLiteral("Courier New");
    int font_size = 10;
    int tab_size = 4;
    TabMode tab_mode = TabMode::kFixed;
  };

  explicit EditorOptionsDialog(const EditorPreferences& prefs, const ActiveFileInfo& active_info,
                               QWidget* parent = nullptr);

  EditorPreferences UpdatedPreferences() const;

  void SelectElement(int index);
  void SetSelectedElementColor(const QColor& color);
  void SelectPreset(int preset_index);

  QTabWidget* tabs() const { return tabs_; }
  QComboBox* font_combo() const { return font_combo_; }
  QComboBox* size_combo() const { return size_combo_; }
  QSpinBox* fixed_tab_spin() const { return fixed_tab_spin_; }
  QRadioButton* assembly_tabs_radio() const { return assembly_tabs_radio_; }
  QRadioButton* fixed_tabs_radio() const { return fixed_tabs_radio_; }
  QCheckBox* auto_indent_check() const { return auto_indent_chk_; }
  QCheckBox* real_tabs_check() const { return real_tabs_chk_; }
  QCheckBox* print_black_check() const { return print_black_chk_; }
  QComboBox* preset_combo() const { return preset_combo_; }
  QListWidget* element_list() const { return element_list_; }
  QCheckBox* bold_check() const { return bold_chk_; }
  QCheckBox* italic_check() const { return italic_chk_; }
  QCheckBox* underline_check() const { return underline_chk_; }
  QPushButton* ok_button() const { return ok_btn_; }
  QPushButton* help_button() const { return help_btn_; }

 signals:
  void HelpRequested(const QString& context);

 private:
  void PopulateSizesForFont(const QString& font_name, int preferred_size);
  void OnElementChanged(int row);
  void OnStyleCheckToggled();
  void UpdatePreviewLabels();
  TextStyle* StyleForElement(int row);

  EditorPreferences prefs_;
  SyntaxTheme temp_theme_;
  bool in_element_change_ = false;

  QTabWidget* tabs_ = nullptr;
  QComboBox* font_combo_ = nullptr;
  QComboBox* size_combo_ = nullptr;
  QSpinBox* fixed_tab_spin_ = nullptr;
  QRadioButton* assembly_tabs_radio_ = nullptr;
  QRadioButton* fixed_tabs_radio_ = nullptr;
  QCheckBox* auto_indent_chk_ = nullptr;
  QCheckBox* real_tabs_chk_ = nullptr;
  QCheckBox* print_black_chk_ = nullptr;

  QLabel* current_font_lbl_ = nullptr;
  QLabel* current_size_lbl_ = nullptr;
  QLabel* current_tab_size_lbl_ = nullptr;
  QLabel* current_tab_type_lbl_ = nullptr;

  QComboBox* preset_combo_ = nullptr;
  QListWidget* element_list_ = nullptr;
  QCheckBox* bold_chk_ = nullptr;
  QCheckBox* italic_chk_ = nullptr;
  QCheckBox* underline_chk_ = nullptr;
  QWidget* preview_panel_ = nullptr;
  QLabel* preview_labels_[8] = {};

  QPushButton* ok_btn_ = nullptr;
  QPushButton* cancel_btn_ = nullptr;
  QPushButton* help_btn_ = nullptr;
};

// Assembler Status dialog (Edit68K/assembleS.dfm).
class AssemblerStatusDialog : public QDialog {
  Q_OBJECT

 public:
  enum class ActionChoice {
    kClose,
    kExecute,
    kLoadL68,
  };

  AssemblerStatusDialog(const QString& source_filename, int warnings, int errors, bool can_execute,
                        bool can_load_l68, QWidget* parent = nullptr);

  int warnings() const { return warnings_; }
  int errors() const { return errors_; }
  ActionChoice choice() const { return choice_; }

  QPushButton* execute_button() const { return execute_btn_; }
  QPushButton* load_l68_button() const { return load_l68_btn_; }
  QPushButton* close_button() const { return close_btn_; }
  QLabel* warnings_label() const { return warnings_lbl_; }
  QLabel* errors_label() const { return errors_lbl_; }

 private:
  int warnings_ = 0;
  int errors_ = 0;
  ActionChoice choice_ = ActionChoice::kClose;
  QLabel* warnings_lbl_ = nullptr;
  QLabel* errors_lbl_ = nullptr;
  QPushButton* execute_btn_ = nullptr;
  QPushButton* load_l68_btn_ = nullptr;
  QPushButton* close_btn_ = nullptr;
};

// Find dialog (Edit68K/findDialogS.dfm).
class EditorFindDialog : public QDialog {
  Q_OBJECT

 public:
  explicit EditorFindDialog(QWidget* parent = nullptr);

  void SetSearchText(const QString& text);
  QString SearchText() const;
  bool WholeWord() const;
  bool MatchCase() const;

  QLineEdit* find_edit() const { return find_edit_; }
  QCheckBox* whole_word_check() const { return whole_word_chk_; }
  QCheckBox* match_case_check() const { return match_case_chk_; }
  QPushButton* find_next_button() const { return find_next_btn_; }

 signals:
  void FindNextRequested(const QString& text, bool whole_word, bool match_case);

 private:
  QLineEdit* find_edit_ = nullptr;
  QCheckBox* whole_word_chk_ = nullptr;
  QCheckBox* match_case_chk_ = nullptr;
  QPushButton* find_next_btn_ = nullptr;
  QPushButton* cancel_btn_ = nullptr;
};

// Find and Replace dialog (Edit68K/replaceDialogS.dfm).
class EditorReplaceDialog : public QDialog {
  Q_OBJECT

 public:
  explicit EditorReplaceDialog(QWidget* parent = nullptr);

  void SetSearchAndReplaceText(const QString& find_text, const QString& replace_text);
  QString SearchText() const;
  QString ReplaceText() const;
  bool WholeWord() const;
  bool MatchCase() const;

  QLineEdit* find_edit() const { return find_edit_; }
  QLineEdit* replace_edit() const { return replace_edit_; }
  QCheckBox* whole_word_check() const { return whole_word_chk_; }
  QCheckBox* match_case_check() const { return match_case_chk_; }
  QPushButton* find_next_button() const { return find_next_btn_; }
  QPushButton* replace_button() const { return replace_btn_; }
  QPushButton* replace_all_button() const { return replace_all_btn_; }

 signals:
  void FindNextRequested(const QString& text, bool whole_word, bool match_case);
  void ReplaceRequested(const QString& find_text, const QString& replace_text, bool whole_word,
                        bool match_case, bool replace_all);

 private:
  QLineEdit* find_edit_ = nullptr;
  QLineEdit* replace_edit_ = nullptr;
  QCheckBox* whole_word_chk_ = nullptr;
  QCheckBox* match_case_chk_ = nullptr;
  QPushButton* find_next_btn_ = nullptr;
  QPushButton* replace_btn_ = nullptr;
  QPushButton* replace_all_btn_ = nullptr;
  QPushButton* cancel_btn_ = nullptr;
};

// Adjust Font dialog for the active document (Edit68K/fontForm1.dfm).
class AdjustFontDialog : public QDialog {
  Q_OBJECT

 public:
  explicit AdjustFontDialog(const QFont& current_font,
                            std::function<void(const QFont&)> apply_font = {},
                            QWidget* parent = nullptr);

  QComboBox* font_combo() const { return font_combo_; }
  QComboBox* size_combo() const { return size_combo_; }
  QPushButton* change_button() const { return change_btn_; }
  QPushButton* close_button() const { return close_btn_; }

 private:
  void PopulateSizes(const QString& font_name, int current_size);
  void OnChangeClicked();

  std::function<void(const QFont&)> apply_font_;
  QLineEdit* current_font_edit_ = nullptr;
  QLineEdit* current_size_edit_ = nullptr;
  QComboBox* font_combo_ = nullptr;
  QComboBox* size_combo_ = nullptr;
  QPushButton* change_btn_ = nullptr;
  QPushButton* close_btn_ = nullptr;
};

}  // namespace easym68k::editor

#endif  // EASY68K_GUI_EDITOR_DIALOGS_H_
