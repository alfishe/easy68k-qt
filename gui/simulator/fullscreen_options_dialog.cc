// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "fullscreen_options_dialog.h"

#include <QLabel>

namespace easym68k::gui {

FullscreenOptionsDialog::FullscreenOptionsDialog(int monitor, int screens, QWidget* parent)
    : QDialog(parent), screens_(screens) {
  setWindowTitle(QStringLiteral("Fullscreen Options"));
  setFixedSize(259, 105);
  auto* label = new QLabel(QStringLiteral("Screen to use for fullscreen output window.  Primary "
                                          "screen = 1  Secondary screens = 2 and up."),
                           this);
  label->setGeometry(8, 0, 241, 32);
  label->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
  label->setWordWrap(true);
  number_ = new QLineEdit(this);
  number_->setGeometry(104, 40, 49, 21);
  ok_ = new QPushButton(QStringLiteral("&Ok"), this);
  ok_->setGeometry(8, 72, 97, 25);
  ok_->setDefault(true);
  auto* cancel = new QPushButton(QStringLiteral("&Cancel"), this);
  cancel->setGeometry(152, 72, 97, 25);
  // txtScreenNumberChange(): Ok only for an existing screen.
  connect(number_, &QLineEdit::textChanged, this, [this](const QString& text) {
    bool ok = false;
    const int n = text.trimmed().toInt(&ok);
    ok_->setEnabled(ok && n >= 1 && n <= screens_);
  });
  connect(ok_, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
  number_->setText(QString::number(monitor + 1));  // FormShow()
  number_->setFocus();
}

int FullscreenOptionsDialog::Monitor() const {
  return number_->text().trimmed().toInt() - 1;
}

}  // namespace easym68k::gui
