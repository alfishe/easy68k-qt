// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "find_dialog.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QVBoxLayout>

namespace easym68k::gui {

FindDialog::FindDialog(QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::WindowStaysOnTopHint) {
  setWindowTitle(tr("Find"));
  resize(272, 98);

  auto* root = new QVBoxLayout(this);
  auto* top_row = new QHBoxLayout();
  auto* label = new QLabel(tr("Fi&nd what:"), this);
  find_text_ = new QLineEdit(this);
  label->setBuddy(find_text_);
  top_row->addWidget(label);
  top_row->addWidget(find_text_, 1);
  root->addLayout(top_row);

  auto* btn_col = new QVBoxLayout();
  find_next_btn_ = new QPushButton(tr("Find  (F3)"), this);
  find_next_btn_->setEnabled(false);
  find_next_btn_->setDefault(false);
  cancel_btn_ = new QPushButton(tr("Cancel"), this);
  btn_col->addWidget(find_next_btn_, 0, Qt::AlignRight);
  btn_col->addWidget(cancel_btn_, 0, Qt::AlignRight);
  root->addLayout(btn_col);

  connect(find_text_, &QLineEdit::textChanged, this,
          &FindDialog::OnTextChanged);
  connect(find_next_btn_, &QPushButton::clicked, this,
          &FindDialog::OnFindNextClicked);
  connect(cancel_btn_, &QPushButton::clicked, this, &QWidget::close);
}

void FindDialog::OnTextChanged(const QString& text) {
  const bool has_text = !text.isEmpty();
  find_next_btn_->setEnabled(has_text);
  find_next_btn_->setDefault(has_text);
  find_next_ = false;  // search from top
}

void FindDialog::OnFindNextClicked() {
  if (find_text_->text().isEmpty()) {
    return;
  }
  emit FindRequested(find_text_->text(), find_next_);
  find_next_ = true;  // search for next
}

void FindDialog::showEvent(QShowEvent* event) {
  QDialog::showEvent(event);
  const bool has_text = !find_text_->text().isEmpty();
  find_next_btn_->setEnabled(has_text);
  find_next_btn_->setDefault(has_text);
  if (has_text) {
    find_next_btn_->setFocus();
  } else {
    find_text_->setFocus();
  }
  find_next_ = false;  // search from top
}

void FindDialog::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_F3 && !find_text_->text().isEmpty()) {
    emit FindRequested(find_text_->text(), true);
    find_next_ = true;
    event->accept();
    return;
  }
  QDialog::keyPressEvent(event);
}

}  // namespace easym68k::gui
