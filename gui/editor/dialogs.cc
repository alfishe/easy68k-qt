// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "editor/dialogs.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace easym68k::editor {

// ---------------------------------------------------------------------------
// AssemblerOptionsDialog (Edit68K/optionsS.dfm)
// ---------------------------------------------------------------------------

AssemblerOptionsDialog::AssemblerOptionsDialog(
    const EditorPreferences& prefs, const QString& template_text,
    std::function<bool(const QString&)> confirm_recommended,
    std::function<bool(const QString&)> save_template, QWidget* parent)
    : QDialog(parent),
      base_prefs_(prefs),
      confirm_recommended_(std::move(confirm_recommended)),
      save_template_(std::move(save_template)) {
  setWindowTitle(QStringLiteral("EASy68K Options"));
  resize(370, 320);

  auto* root = new QVBoxLayout(this);
  tabs_ = new QTabWidget(this);

  // Tab 1: Assembler
  auto* asm_tab = new QWidget(tabs_);
  auto* asm_layout = new QVBoxLayout(asm_tab);

  auto* general_row = new QHBoxLayout();
  auto* general_col1 = new QVBoxLayout();
  gen_list_chk_ = new QCheckBox(QStringLiteral("Generate Listing File*"), asm_tab);
  gen_list_chk_->setToolTip(QStringLiteral("Generates listing file and symbol table for Sim68K"));
  gen_srec_chk_ = new QCheckBox(QStringLiteral("Save S-Record*"), asm_tab);
  gen_srec_chk_->setToolTip(QStringLiteral("Generates 68000 code for Sim68K"));
  auto_save_chk_ = new QCheckBox(QStringLiteral("Auto Save Checked"), asm_tab);
  auto_save_chk_->setToolTip(QStringLiteral("Check the Auto Save files box upon starting Edit68K"));
  general_col1->addWidget(gen_list_chk_);
  general_col1->addWidget(gen_srec_chk_);
  general_col1->addWidget(auto_save_chk_);

  auto* general_col2 = new QVBoxLayout();
  show_warnings_chk_ = new QCheckBox(QStringLiteral("Show Warnings"), asm_tab);
  show_warnings_chk_->setToolTip(QStringLiteral("Show assembler warnings after assembling"));
  bitfield_chk_ = new QCheckBox(QStringLiteral("Enable Bitfield Instructions*"), asm_tab);
  bitfield_chk_->setToolTip(
      QStringLiteral("Enable 68020 type bit field instructions for simulation inside Sim68K"));
  general_col2->addWidget(show_warnings_chk_);
  general_col2->addWidget(bitfield_chk_);
  general_col2->addStretch(1);

  general_row->addLayout(general_col1);
  general_row->addLayout(general_col2);
  asm_layout->addLayout(general_row);

  auto* list_group = new QGroupBox(QStringLiteral("Listing File Options"), asm_tab);
  auto* list_layout = new QVBoxLayout(list_group);
  cross_ref_chk_ = new QCheckBox(QStringLiteral("Add Cross Reference Table"), list_group);
  mac_ex_chk_ = new QCheckBox(QStringLiteral("Expand Macros*"), list_group);
  const_ex_chk_ = new QCheckBox(QStringLiteral("Expand Constants*"), list_group);
  struc_ex_chk_ = new QCheckBox(QStringLiteral("Expand Structured Statements*"), list_group);
  list_layout->addWidget(cross_ref_chk_);
  list_layout->addWidget(mac_ex_chk_);
  list_layout->addWidget(const_ex_chk_);
  list_layout->addWidget(struc_ex_chk_);
  asm_layout->addWidget(list_group);

  auto* rec_lbl = new QLabel(QStringLiteral("* Recommended if Running in Sim68K"), asm_tab);
  asm_layout->addWidget(rec_lbl);
  asm_layout->addStretch(1);

  tabs_->addTab(asm_tab, QStringLiteral("Assembler"));

  // Tab 2: Template
  auto* tmpl_tab = new QWidget(tabs_);
  auto* tmpl_layout = new QVBoxLayout(tmpl_tab);
  auto* tmpl_lbl = new QLabel(QStringLiteral("Template for New Files"), tmpl_tab);
  template_edit_ = new QPlainTextEdit(tmpl_tab);
  template_edit_->setFont(QFont(QStringLiteral("Courier New"), 9));
  template_edit_->setLineWrapMode(QPlainTextEdit::NoWrap);
  template_edit_->setPlainText(template_text);
  save_template_btn_ = new QPushButton(QStringLiteral("Save Template"), tmpl_tab);
  tmpl_layout->addWidget(tmpl_lbl);
  tmpl_layout->addWidget(template_edit_, 1);
  tmpl_layout->addWidget(save_template_btn_, 0, Qt::AlignRight);

  tabs_->addTab(tmpl_tab, QStringLiteral("Template"));
  root->addWidget(tabs_, 1);

  // Bottom buttons
  auto* btn_row = new QHBoxLayout();
  btn_row->addStretch(1);
  ok_btn_ = new QPushButton(QStringLiteral("&OK"), this);
  ok_btn_->setDefault(true);
  cancel_btn_ = new QPushButton(QStringLiteral("&Cancel"), this);
  btn_row->addWidget(ok_btn_);
  btn_row->addWidget(cancel_btn_);
  root->addLayout(btn_row);

  // Populate from prefs
  gen_list_chk_->setChecked(prefs.asm_options.generate_listing);
  gen_srec_chk_->setChecked(prefs.asm_options.generate_srecord);
  auto_save_chk_->setChecked(prefs.auto_save);
  show_warnings_chk_->setChecked(prefs.asm_options.warnings);
  bitfield_chk_->setChecked(prefs.asm_options.bitfield);
  cross_ref_chk_->setChecked(prefs.asm_options.cre);
  mac_ex_chk_->setChecked(prefs.asm_options.mex);
  const_ex_chk_->setChecked(prefs.asm_options.cex);
  struc_ex_chk_->setChecked(prefs.asm_options.sex);

  auto sync_list_enabled = [this](bool checked) {
    cross_ref_chk_->setEnabled(checked);
    mac_ex_chk_->setEnabled(checked);
    const_ex_chk_->setEnabled(checked);
    struc_ex_chk_->setEnabled(checked);
  };
  sync_list_enabled(gen_list_chk_->isChecked());
  connect(gen_list_chk_, &QCheckBox::toggled, this, sync_list_enabled);
  connect(save_template_btn_, &QPushButton::clicked, this,
          &AssemblerOptionsDialog::OnSaveTemplateClicked);
  connect(ok_btn_, &QPushButton::clicked, this, &AssemblerOptionsDialog::OnOkClicked);
  connect(cancel_btn_, &QPushButton::clicked, this, &QDialog::reject);
}

EditorPreferences AssemblerOptionsDialog::Preferences() const {
  EditorPreferences out = base_prefs_;
  out.asm_options.generate_listing = gen_list_chk_->isChecked();
  out.asm_options.generate_srecord = gen_srec_chk_->isChecked();
  out.auto_save = auto_save_chk_->isChecked();
  out.asm_options.warnings = show_warnings_chk_->isChecked();
  out.asm_options.bitfield = bitfield_chk_->isChecked();
  out.asm_options.cre = cross_ref_chk_->isChecked();
  out.asm_options.mex = mac_ex_chk_->isChecked();
  out.asm_options.cex = const_ex_chk_->isChecked();
  out.asm_options.sex = struc_ex_chk_->isChecked();
  return out;
}

QString AssemblerOptionsDialog::TemplateText() const {
  return template_edit_->toPlainText();
}

void AssemblerOptionsDialog::OnOkClicked() {
  if (!gen_list_chk_->isChecked() || !gen_srec_chk_->isChecked() || !mac_ex_chk_->isChecked() ||
      !const_ex_chk_->isChecked() || !struc_ex_chk_->isChecked()) {
    const QString msg = QStringLiteral(
        "Unchecking recommended options (*) may prevent the proper operation "
        "of other features and is NOT recommended. Do you want to continue?");
    bool proceed = true;
    if (confirm_recommended_) {
      proceed = confirm_recommended_(msg);
    } else {
      proceed = (QMessageBox::question(this, QStringLiteral("EASy68K Options"), msg,
                                       QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes);
    }
    if (!proceed) {
      return;
    }
  }
  accept();
}

void AssemblerOptionsDialog::OnSaveTemplateClicked() {
  if (save_template_) {
    save_template_(template_edit_->toPlainText());
  }
}

// ---------------------------------------------------------------------------
// EditorOptionsDialog (Edit68K/editorOptions.dfm)
// ---------------------------------------------------------------------------

EditorOptionsDialog::EditorOptionsDialog(const EditorPreferences& prefs,
                                         const ActiveFileInfo& active_info, QWidget* parent)
    : QDialog(parent), prefs_(prefs), temp_theme_(prefs.theme) {
  setWindowTitle(QStringLiteral("Editor Options"));
  resize(390, 380);

  auto* root = new QVBoxLayout(this);
  tabs_ = new QTabWidget(this);

  // Tab 1: General
  auto* gen_tab = new QWidget(tabs_);
  auto* gen_layout = new QVBoxLayout(gen_tab);

  auto* font_group = new QGroupBox(QStringLiteral("Default Font Settings"), gen_tab);
  auto* font_layout = new QGridLayout(font_group);
  font_layout->addWidget(new QLabel(QStringLiteral("Font:"), font_group), 0, 0);
  font_combo_ = new QComboBox(font_group);
  font_layout->addWidget(font_combo_, 0, 1);
  font_layout->addWidget(new QLabel(QStringLiteral("Size:"), font_group), 0, 2);
  size_combo_ = new QComboBox(font_group);
  font_layout->addWidget(size_combo_, 0, 3);
  font_layout->addWidget(new QLabel(QStringLiteral("Current file:"), font_group), 1, 0);
  current_font_lbl_ = new QLabel(active_info.font_name, font_group);
  font_layout->addWidget(current_font_lbl_, 1, 1);
  current_size_lbl_ = new QLabel(QString::number(active_info.font_size), font_group);
  font_layout->addWidget(current_size_lbl_, 1, 3);
  gen_layout->addWidget(font_group);

  auto* tab_group = new QGroupBox(QStringLiteral("Default Tab Settings"), gen_tab);
  auto* tab_layout = new QGridLayout(tab_group);
  assembly_tabs_radio_ = new QRadioButton(QStringLiteral("Assembly  (12, 20, 44)"), tab_group);
  fixed_tabs_radio_ = new QRadioButton(QStringLiteral("Fixed:"), tab_group);
  fixed_tab_spin_ = new QSpinBox(tab_group);
  fixed_tab_spin_->setRange(1, 32);
  fixed_tab_spin_->setValue(prefs.fixed_tab_size);
  tab_layout->addWidget(assembly_tabs_radio_, 0, 0, 1, 3);
  tab_layout->addWidget(fixed_tabs_radio_, 1, 0);
  tab_layout->addWidget(fixed_tab_spin_, 1, 1);
  tab_layout->addWidget(new QLabel(QStringLiteral("Current file:"), tab_group), 2, 0);
  current_tab_type_lbl_ =
      new QLabel(active_info.tab_mode == TabMode::kAssembly ? QStringLiteral("Assembly")
                                                            : QStringLiteral("Fixed"),
                 tab_group);
  current_tab_size_lbl_ =
      new QLabel(active_info.tab_mode == TabMode::kAssembly ? QString()
                                                            : QString::number(active_info.tab_size),
                 tab_group);
  tab_layout->addWidget(current_tab_type_lbl_, 2, 1);
  tab_layout->addWidget(current_tab_size_lbl_, 2, 2);
  gen_layout->addWidget(tab_group);

  auto* note_lbl =
      new QLabel(QStringLiteral("NOTE: Font and Tab settings are saved in the source file. "
                                "Changes made here will affect the active file and any "
                                "New files created."),
                 gen_tab);
  note_lbl->setWordWrap(true);
  gen_layout->addWidget(note_lbl);

  auto* chk_row = new QHBoxLayout();
  auto_indent_chk_ = new QCheckBox(QStringLiteral("Auto Indent"), gen_tab);
  real_tabs_chk_ = new QCheckBox(QStringLiteral("Real Tabs"), gen_tab);
  print_black_chk_ = new QCheckBox(QStringLiteral("Print Black and White"), gen_tab);
  chk_row->addWidget(auto_indent_chk_);
  chk_row->addWidget(real_tabs_chk_);
  chk_row->addWidget(print_black_chk_);
  gen_layout->addLayout(chk_row);
  gen_layout->addStretch(1);

  tabs_->addTab(gen_tab, QStringLiteral("General"));

  // Tab 2: Syntax Highlight
  auto* hl_tab = new QWidget(tabs_);
  auto* hl_layout = new QVBoxLayout(hl_tab);

  auto* top_hl_row = new QHBoxLayout();
  element_list_ = new QListWidget(hl_tab);
  element_list_->addItems({
      QStringLiteral("Code"),
      QStringLiteral("Comment"),
      QStringLiteral("Label"),
      QStringLiteral("Directive"),
      QStringLiteral("Structured"),
      QStringLiteral("StructureError"),
      QStringLiteral("String"),
      QStringLiteral("Background"),
  });
  top_hl_row->addWidget(element_list_, 1);

  auto* right_hl_col = new QVBoxLayout();
  auto* preset_row = new QHBoxLayout();
  preset_row->addWidget(new QLabel(QStringLiteral("Pre-sets:"), hl_tab));
  preset_combo_ = new QComboBox(hl_tab);
  preset_combo_->addItems({
      QStringLiteral("Color 1"),
      QStringLiteral("Color 2"),
      QStringLiteral("Mono"),
      QStringLiteral("Custom"),
      QStringLiteral("Disabled"),
  });
  preset_row->addWidget(preset_combo_, 1);
  right_hl_col->addLayout(preset_row);

  auto* color_btn = new QPushButton(QStringLiteral("Choose Color..."), hl_tab);
  right_hl_col->addWidget(color_btn);

  auto* attr_group = new QGroupBox(QStringLiteral("Text Attributes"), hl_tab);
  auto* attr_layout = new QVBoxLayout(attr_group);
  bold_chk_ = new QCheckBox(QStringLiteral("&Bold"), attr_group);
  italic_chk_ = new QCheckBox(QStringLiteral("&Italic"), attr_group);
  underline_chk_ = new QCheckBox(QStringLiteral("&Underline"), attr_group);
  attr_layout->addWidget(bold_chk_);
  attr_layout->addWidget(italic_chk_);
  attr_layout->addWidget(underline_chk_);
  right_hl_col->addWidget(attr_group);
  right_hl_col->addStretch(1);

  top_hl_row->addLayout(right_hl_col, 1);
  hl_layout->addLayout(top_hl_row);

  // Preview panel
  preview_panel_ = new QWidget(hl_tab);
  preview_panel_->setAutoFillBackground(true);
  auto* prev_grid = new QGridLayout(preview_panel_);
  const QStringList preview_samples = {
      QStringLiteral("MOVE.L D0,D1"),  QStringLiteral("* Comment"),  QStringLiteral("START"),
      QStringLiteral("DC.B"),          QStringLiteral("ENDI"),       QStringLiteral("Error"),
      QStringLiteral("'Text String'"), QStringLiteral("Background"),
  };
  for (int i = 0; i < 8; ++i) {
    preview_labels_[i] = new QLabel(preview_samples[i], preview_panel_);
    prev_grid->addWidget(preview_labels_[i], i / 2, i % 2);
  }
  hl_layout->addWidget(preview_panel_);

  tabs_->addTab(hl_tab, QStringLiteral("Syntax Highlight"));
  root->addWidget(tabs_, 1);

  // Bottom buttons
  auto* btn_row = new QHBoxLayout();
  btn_row->addStretch(1);
  ok_btn_ = new QPushButton(QStringLiteral("OK"), this);
  ok_btn_->setDefault(true);
  cancel_btn_ = new QPushButton(QStringLiteral("Cancel"), this);
  help_btn_ = new QPushButton(QStringLiteral("Help"), this);
  btn_row->addWidget(ok_btn_);
  btn_row->addWidget(cancel_btn_);
  btn_row->addWidget(help_btn_);
  root->addLayout(btn_row);

  // Populate font list
  const QStringList families = QFontDatabase::families();
  font_combo_->addItems(families);
  if (font_combo_->findText(prefs.font_name, Qt::MatchFixedString) < 0) {
    font_combo_->insertItem(0, prefs.font_name);
  }
  font_combo_->setCurrentIndex(font_combo_->findText(prefs.font_name, Qt::MatchFixedString));
  PopulateSizesForFont(prefs.font_name, prefs.font_size);

  if (prefs.tab_mode == TabMode::kAssembly) {
    assembly_tabs_radio_->setChecked(true);
  } else {
    fixed_tabs_radio_->setChecked(true);
  }
  auto_indent_chk_->setChecked(prefs.auto_indent);
  real_tabs_chk_->setChecked(prefs.real_tabs);
  print_black_chk_->setChecked(prefs.print_black);

  preset_combo_->setCurrentIndex(std::clamp(temp_theme_.preset, 0, 4));
  element_list_->setCurrentRow(0);
  OnElementChanged(0);
  UpdatePreviewLabels();

  connect(font_combo_, &QComboBox::currentTextChanged, this, [this](const QString& family) {
    const int cur_size = size_combo_->currentText().toInt();
    PopulateSizesForFont(family, cur_size > 0 ? cur_size : 10);
  });
  connect(element_list_, &QListWidget::currentRowChanged, this,
          &EditorOptionsDialog::OnElementChanged);
  connect(preset_combo_, &QComboBox::currentIndexChanged, this, &EditorOptionsDialog::SelectPreset);
  connect(bold_chk_, &QCheckBox::toggled, this, &EditorOptionsDialog::OnStyleCheckToggled);
  connect(italic_chk_, &QCheckBox::toggled, this, &EditorOptionsDialog::OnStyleCheckToggled);
  connect(underline_chk_, &QCheckBox::toggled, this, &EditorOptionsDialog::OnStyleCheckToggled);
  connect(color_btn, &QPushButton::clicked, this, [this]() {
    const int row = element_list_->currentRow();
    QColor initial = Qt::black;
    if (row == 7) {
      initial = temp_theme_.background;
    } else if (const TextStyle* style = StyleForElement(row)) {
      initial = style->color;
    }
    const QColor chosen = QColorDialog::getColor(initial, this);
    if (chosen.isValid()) {
      SetSelectedElementColor(chosen);
    }
  });
  connect(ok_btn_, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancel_btn_, &QPushButton::clicked, this, &QDialog::reject);
  connect(help_btn_, &QPushButton::clicked, this,
          [this]() { emit HelpRequested(QStringLiteral("OPTIONS")); });
}

void EditorOptionsDialog::PopulateSizesForFont(const QString& font_name, int preferred_size) {
  size_combo_->clear();
  QList<int> sizes = QFontDatabase::pointSizes(font_name);
  if (sizes.isEmpty()) {
    sizes = {8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72};
  }
  if (!sizes.contains(preferred_size) && preferred_size > 0) {
    sizes.append(preferred_size);
    std::sort(sizes.begin(), sizes.end());
  }
  int target_idx = 0;
  for (int i = 0; i < sizes.size(); ++i) {
    size_combo_->addItem(QString::number(sizes[i]));
    if (sizes[i] == preferred_size) {
      target_idx = i;
    }
  }
  size_combo_->setCurrentIndex(target_idx);
}

TextStyle* EditorOptionsDialog::StyleForElement(int row) {
  switch (row) {
    case 0:
      return &temp_theme_.code;
    case 1:
      return &temp_theme_.comment;
    case 2:
      return &temp_theme_.label;
    case 3:
      return &temp_theme_.directive;
    case 4:
      return &temp_theme_.structure;
    case 5:
      return &temp_theme_.error;
    case 6:
      return &temp_theme_.text;
    default:
      return nullptr;
  }
}

void EditorOptionsDialog::SelectElement(int index) {
  element_list_->setCurrentRow(index);
}

void EditorOptionsDialog::OnElementChanged(int row) {
  in_element_change_ = true;
  if (const TextStyle* style = StyleForElement(row)) {
    bold_chk_->setEnabled(true);
    italic_chk_->setEnabled(true);
    underline_chk_->setEnabled(true);
    bold_chk_->setChecked(style->bold);
    italic_chk_->setChecked(style->italic);
    underline_chk_->setChecked(style->underline);
  } else {
    bold_chk_->setChecked(false);
    italic_chk_->setChecked(false);
    underline_chk_->setChecked(false);
    bold_chk_->setEnabled(false);
    italic_chk_->setEnabled(false);
    underline_chk_->setEnabled(false);
  }
  in_element_change_ = false;
}

void EditorOptionsDialog::SetSelectedElementColor(const QColor& color) {
  const int row = element_list_->currentRow();
  if (row == 7) {
    temp_theme_.background = color;
  } else if (TextStyle* style = StyleForElement(row)) {
    style->color = color;
  }
  temp_theme_.preset = SyntaxTheme::kCustom;
  temp_theme_.disabled = false;
  if (preset_combo_->currentIndex() != SyntaxTheme::kCustom) {
    preset_combo_->blockSignals(true);
    preset_combo_->setCurrentIndex(SyntaxTheme::kCustom);
    preset_combo_->blockSignals(false);
  }
  UpdatePreviewLabels();
}

void EditorOptionsDialog::OnStyleCheckToggled() {
  if (in_element_change_) {
    return;
  }
  TextStyle* style = StyleForElement(element_list_->currentRow());
  if (!style) {
    return;
  }
  style->bold = bold_chk_->isChecked();
  style->italic = italic_chk_->isChecked();
  style->underline = underline_chk_->isChecked();
  temp_theme_.preset = SyntaxTheme::kCustom;
  temp_theme_.disabled = false;
  if (preset_combo_->currentIndex() != SyntaxTheme::kCustom) {
    preset_combo_->blockSignals(true);
    preset_combo_->setCurrentIndex(SyntaxTheme::kCustom);
    preset_combo_->blockSignals(false);
  }
  UpdatePreviewLabels();
}

void EditorOptionsDialog::SelectPreset(int preset_index) {
  if (preset_index == SyntaxTheme::kCustom) {
    temp_theme_.preset = SyntaxTheme::kCustom;
    temp_theme_.disabled = false;
  } else {
    temp_theme_ = SyntaxTheme::FromPreset(preset_index, temp_theme_.background);
  }
  if (preset_combo_->currentIndex() != preset_index) {
    preset_combo_->blockSignals(true);
    preset_combo_->setCurrentIndex(preset_index);
    preset_combo_->blockSignals(false);
  }
  OnElementChanged(element_list_->currentRow());
  UpdatePreviewLabels();
}

void EditorOptionsDialog::UpdatePreviewLabels() {
  QPalette pal = preview_panel_->palette();
  pal.setColor(QPalette::Window, temp_theme_.background);
  preview_panel_->setPalette(pal);

  const TextStyle* styles[7] = {
      &temp_theme_.code,      &temp_theme_.comment, &temp_theme_.label, &temp_theme_.directive,
      &temp_theme_.structure, &temp_theme_.error,   &temp_theme_.text,
  };
  for (int i = 0; i < 7; ++i) {
    QFont f = preview_labels_[i]->font();
    f.setBold(styles[i]->bold);
    f.setItalic(styles[i]->italic);
    f.setUnderline(styles[i]->underline);
    preview_labels_[i]->setFont(f);
    QPalette lpal = preview_labels_[i]->palette();
    lpal.setColor(QPalette::WindowText, styles[i]->color);
    preview_labels_[i]->setPalette(lpal);
  }
  QPalette bg_pal = preview_labels_[7]->palette();
  bg_pal.setColor(QPalette::WindowText, temp_theme_.code.color);
  preview_labels_[7]->setPalette(bg_pal);
}

EditorPreferences EditorOptionsDialog::UpdatedPreferences() const {
  EditorPreferences out = prefs_;
  out.font_name = font_combo_->currentText();
  const int sz = size_combo_->currentText().toInt();
  if (sz > 0) {
    out.font_size = sz;
  }
  out.fixed_tab_size = std::clamp(fixed_tab_spin_->value(), 1, 32);
  out.tab_mode = assembly_tabs_radio_->isChecked() ? TabMode::kAssembly : TabMode::kFixed;
  out.auto_indent = auto_indent_chk_->isChecked();
  out.real_tabs = real_tabs_chk_->isChecked();
  out.print_black = print_black_chk_->isChecked();
  out.theme = temp_theme_;
  return out;
}

// ---------------------------------------------------------------------------
// AssemblerStatusDialog (Edit68K/assembleS.dfm)
// ---------------------------------------------------------------------------

AssemblerStatusDialog::AssemblerStatusDialog(const QString& source_filename, int warnings,
                                             int errors, bool can_execute, bool can_load_l68,
                                             QWidget* parent)
    : QDialog(parent), warnings_(warnings), errors_(errors) {
  setWindowTitle(QStringLiteral("Assembler Status"));
  resize(260, 150);

  auto* root = new QVBoxLayout(this);
  auto* file_lbl = new QLabel(source_filename, this);
  file_lbl->setAlignment(Qt::AlignCenter);
  root->addWidget(file_lbl);

  auto* counts_grid = new QGridLayout();
  counts_grid->addWidget(new QLabel(QStringLiteral("Warnings:"), this), 0, 0, Qt::AlignRight);
  warnings_lbl_ = new QLabel(QString::number(warnings), this);
  counts_grid->addWidget(warnings_lbl_, 0, 1, Qt::AlignLeft);

  counts_grid->addWidget(new QLabel(QStringLiteral("Errors:"), this), 1, 0, Qt::AlignRight);
  errors_lbl_ = new QLabel(QString::number(errors), this);
  counts_grid->addWidget(errors_lbl_, 1, 1, Qt::AlignLeft);
  root->addLayout(counts_grid);

  auto* btn_row = new QHBoxLayout();
  execute_btn_ = new QPushButton(QStringLiteral("&Execute"), this);
  execute_btn_->setToolTip(QStringLiteral("Run program in simulator"));
  execute_btn_->setEnabled(can_execute);

  load_l68_btn_ = new QPushButton(QStringLiteral("&Load L68"), this);
  load_l68_btn_->setToolTip(QStringLiteral("Open Listing File"));
  load_l68_btn_->setEnabled(can_load_l68);

  close_btn_ = new QPushButton(QStringLiteral("&Close"), this);
  close_btn_->setDefault(true);

  btn_row->addWidget(execute_btn_);
  btn_row->addWidget(load_l68_btn_);
  btn_row->addWidget(close_btn_);
  root->addLayout(btn_row);

  connect(execute_btn_, &QPushButton::clicked, this, [this]() {
    choice_ = ActionChoice::kExecute;
    accept();
  });
  connect(load_l68_btn_, &QPushButton::clicked, this, [this]() {
    choice_ = ActionChoice::kLoadL68;
    accept();
  });
  connect(close_btn_, &QPushButton::clicked, this, [this]() {
    choice_ = ActionChoice::kClose;
    accept();
  });
}

// ---------------------------------------------------------------------------
// EditorFindDialog (Edit68K/findDialogS.dfm)
// ---------------------------------------------------------------------------

EditorFindDialog::EditorFindDialog(QWidget* parent) : QDialog(parent) {
  setWindowTitle(QStringLiteral("Find"));
  resize(340, 120);

  auto* root = new QHBoxLayout(this);
  auto* left_col = new QVBoxLayout();
  auto* find_row = new QHBoxLayout();
  find_row->addWidget(new QLabel(QStringLiteral("Fi&nd what:"), this));
  find_edit_ = new QLineEdit(this);
  find_row->addWidget(find_edit_, 1);
  left_col->addLayout(find_row);

  whole_word_chk_ = new QCheckBox(QStringLiteral("Match &whole word only"), this);
  match_case_chk_ = new QCheckBox(QStringLiteral("Match &case"), this);
  left_col->addWidget(whole_word_chk_);
  left_col->addWidget(match_case_chk_);
  left_col->addStretch(1);
  root->addLayout(left_col, 1);

  auto* right_col = new QVBoxLayout();
  find_next_btn_ = new QPushButton(QStringLiteral("&Find Next"), this);
  find_next_btn_->setDefault(true);
  find_next_btn_->setEnabled(false);
  cancel_btn_ = new QPushButton(QStringLiteral("Cancel"), this);
  right_col->addWidget(find_next_btn_);
  right_col->addWidget(cancel_btn_);
  right_col->addStretch(1);
  root->addLayout(right_col);

  connect(find_edit_, &QLineEdit::textChanged, this,
          [this](const QString& text) { find_next_btn_->setEnabled(!text.isEmpty()); });
  connect(find_next_btn_, &QPushButton::clicked, this, [this]() {
    emit FindNextRequested(find_edit_->text(), whole_word_chk_->isChecked(),
                           match_case_chk_->isChecked());
  });
  connect(cancel_btn_, &QPushButton::clicked, this, &QDialog::close);
}

void EditorFindDialog::SetSearchText(const QString& text) {
  find_edit_->setText(text);
  find_edit_->selectAll();
}

QString EditorFindDialog::SearchText() const {
  return find_edit_->text();
}

bool EditorFindDialog::WholeWord() const {
  return whole_word_chk_->isChecked();
}

bool EditorFindDialog::MatchCase() const {
  return match_case_chk_->isChecked();
}

// ---------------------------------------------------------------------------
// EditorReplaceDialog (Edit68K/replaceDialogS.dfm)
// ---------------------------------------------------------------------------

EditorReplaceDialog::EditorReplaceDialog(QWidget* parent) : QDialog(parent) {
  setWindowTitle(QStringLiteral("Find and Replace"));
  resize(380, 150);

  auto* root = new QHBoxLayout(this);
  auto* left_col = new QVBoxLayout();
  auto* form = new QFormLayout();
  find_edit_ = new QLineEdit(this);
  replace_edit_ = new QLineEdit(this);
  form->addRow(QStringLiteral("Fi&nd what:"), find_edit_);
  form->addRow(QStringLiteral("Re&place with:"), replace_edit_);
  left_col->addLayout(form);

  whole_word_chk_ = new QCheckBox(QStringLiteral("Match &whole word only"), this);
  match_case_chk_ = new QCheckBox(QStringLiteral("Match &case"), this);
  left_col->addWidget(whole_word_chk_);
  left_col->addWidget(match_case_chk_);
  left_col->addStretch(1);
  root->addLayout(left_col, 1);

  auto* right_col = new QVBoxLayout();
  find_next_btn_ = new QPushButton(QStringLiteral("&Find Next"), this);
  find_next_btn_->setDefault(true);
  find_next_btn_->setEnabled(false);
  replace_btn_ = new QPushButton(QStringLiteral("&Replace"), this);
  replace_btn_->setEnabled(false);
  replace_all_btn_ = new QPushButton(QStringLiteral("Replace &All"), this);
  replace_all_btn_->setEnabled(false);
  cancel_btn_ = new QPushButton(QStringLiteral("Cancel"), this);
  right_col->addWidget(find_next_btn_);
  right_col->addWidget(replace_btn_);
  right_col->addWidget(replace_all_btn_);
  right_col->addWidget(cancel_btn_);
  right_col->addStretch(1);
  root->addLayout(right_col);

  connect(find_edit_, &QLineEdit::textChanged, this, [this](const QString& text) {
    const bool has_text = !text.isEmpty();
    find_next_btn_->setEnabled(has_text);
    replace_btn_->setEnabled(has_text);
    replace_all_btn_->setEnabled(has_text);
  });
  connect(find_next_btn_, &QPushButton::clicked, this, [this]() {
    emit FindNextRequested(find_edit_->text(), whole_word_chk_->isChecked(),
                           match_case_chk_->isChecked());
  });
  connect(replace_btn_, &QPushButton::clicked, this, [this]() {
    emit ReplaceRequested(find_edit_->text(), replace_edit_->text(), whole_word_chk_->isChecked(),
                          match_case_chk_->isChecked(), false);
  });
  connect(replace_all_btn_, &QPushButton::clicked, this, [this]() {
    emit ReplaceRequested(find_edit_->text(), replace_edit_->text(), whole_word_chk_->isChecked(),
                          match_case_chk_->isChecked(), true);
  });
  connect(cancel_btn_, &QPushButton::clicked, this, &QDialog::close);
}

void EditorReplaceDialog::SetSearchAndReplaceText(const QString& find_text,
                                                  const QString& replace_text) {
  find_edit_->setText(find_text);
  replace_edit_->setText(replace_text);
  find_edit_->selectAll();
}

QString EditorReplaceDialog::SearchText() const {
  return find_edit_->text();
}

QString EditorReplaceDialog::ReplaceText() const {
  return replace_edit_->text();
}

bool EditorReplaceDialog::WholeWord() const {
  return whole_word_chk_->isChecked();
}

bool EditorReplaceDialog::MatchCase() const {
  return match_case_chk_->isChecked();
}

// ---------------------------------------------------------------------------
// AdjustFontDialog (Edit68K/fontForm1.dfm)
// ---------------------------------------------------------------------------

AdjustFontDialog::AdjustFontDialog(const QFont& current_font,
                                   std::function<void(const QFont&)> apply_font, QWidget* parent)
    : QDialog(parent), apply_font_(std::move(apply_font)) {
  setWindowTitle(QStringLiteral("Adjust Font"));
  resize(300, 140);

  auto* root = new QVBoxLayout(this);
  auto* grid = new QGridLayout();
  grid->addWidget(new QLabel(QStringLiteral("Font:"), this), 0, 0);
  grid->addWidget(new QLabel(QStringLiteral("Size:"), this), 0, 1);

  current_font_edit_ = new QLineEdit(current_font.family(), this);
  current_font_edit_->setReadOnly(true);
  current_size_edit_ = new QLineEdit(
      QString::number(current_font.pointSize() > 0 ? current_font.pointSize() : 10), this);
  current_size_edit_->setReadOnly(true);
  grid->addWidget(current_font_edit_, 1, 0);
  grid->addWidget(current_size_edit_, 1, 1);

  font_combo_ = new QComboBox(this);
  size_combo_ = new QComboBox(this);
  grid->addWidget(font_combo_, 2, 0);
  grid->addWidget(size_combo_, 2, 1);
  root->addLayout(grid);

  auto* btn_row = new QHBoxLayout();
  btn_row->addStretch(1);
  change_btn_ = new QPushButton(QStringLiteral("&Change"), this);
  change_btn_->setDefault(true);
  close_btn_ = new QPushButton(QStringLiteral("C&lose"), this);
  btn_row->addWidget(change_btn_);
  btn_row->addWidget(close_btn_);
  root->addLayout(btn_row);

  const QStringList families = QFontDatabase::families();
  font_combo_->addItems(families);
  if (font_combo_->findText(current_font.family(), Qt::MatchFixedString) < 0) {
    font_combo_->insertItem(0, current_font.family());
  }
  font_combo_->setCurrentIndex(font_combo_->findText(current_font.family(), Qt::MatchFixedString));
  const int initial_pt = current_font.pointSize() > 0 ? current_font.pointSize() : 10;
  PopulateSizes(current_font.family(), initial_pt);

  connect(font_combo_, &QComboBox::currentTextChanged, this, [this](const QString& family) {
    current_font_edit_->setText(family);
    const int pt = size_combo_->currentText().toInt();
    PopulateSizes(family, pt > 0 ? pt : 10);
  });
  connect(size_combo_, &QComboBox::currentTextChanged, this,
          [this](const QString& sz) { current_size_edit_->setText(sz); });
  connect(change_btn_, &QPushButton::clicked, this, &AdjustFontDialog::OnChangeClicked);
  connect(close_btn_, &QPushButton::clicked, this, &QDialog::accept);
}

void AdjustFontDialog::PopulateSizes(const QString& font_name, int current_size) {
  size_combo_->clear();
  QList<int> sizes = QFontDatabase::pointSizes(font_name);
  if (sizes.isEmpty()) {
    sizes = {8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26, 28, 36, 48, 72};
  }
  if (!sizes.contains(current_size) && current_size > 0) {
    sizes.append(current_size);
    std::sort(sizes.begin(), sizes.end());
  }
  int target = 0;
  for (int i = 0; i < sizes.size(); ++i) {
    size_combo_->addItem(QString::number(sizes[i]));
    if (sizes[i] == current_size) {
      target = i;
    }
  }
  size_combo_->setCurrentIndex(target);
}

void AdjustFontDialog::OnChangeClicked() {
  const QString family = font_combo_->currentText();
  const int pt = size_combo_->currentText().toInt();
  current_font_edit_->setText(family);
  if (pt > 0) {
    current_size_edit_->setText(QString::number(pt));
  }
  if (apply_font_) {
    apply_font_(QFont(family, pt > 0 ? pt : 10));
  }
}

}  // namespace easym68k::editor
