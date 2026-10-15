// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "log_window.h"

#include <algorithm>
#include <QApplication>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QStyle>

#include "easym68k/sim/types.h"

namespace easym68k::gui {

namespace {

// MemFromKeyPress(): hex digits, upper case; other keys beep.
QLineEdit* HexEdit(QWidget* parent, int digits, const QRect& geometry) {
  auto* e = new QLineEdit(parent);
  e->setGeometry(geometry);
  e->setMaxLength(digits);
  e->setValidator(new QRegularExpressionValidator(
      QRegularExpression(QStringLiteral("[0-9A-Fa-f]{0,%1}").arg(digits)), e));
  QObject::connect(e, &QLineEdit::inputRejected, e, [] { QApplication::beep(); });
  QObject::connect(e, &QLineEdit::textEdited, e, [e](const QString& t) {
    if (t != t.toUpper()) {
      const int c = e->cursorPosition();
      e->setText(t.toUpper());
      e->setCursorPosition(c);
    }
  });
  return e;
}

// ELogOpenBtnClick() / OLogOpenBtnClick(): a save dialog for the file.
QPushButton* BrowseButton(QWidget* parent, QLineEdit* name, const QString& default_name,
                          const QRect& geometry) {
  auto* b = new QPushButton(parent);
  b->setGeometry(geometry);
  b->setIcon(parent->style()->standardIcon(QStyle::SP_DialogOpenButton));
  QObject::connect(b, &QPushButton::clicked, parent, [parent, name, default_name] {
    const QString file = QFileDialog::getSaveFileName(
        parent, QString(), name->text().isEmpty() ? default_name : name->text(),
        QStringLiteral("Text File (*.txt)"));
    if (!file.isEmpty())
      name->setText(file);
  });
  return b;
}

}  // namespace

LogOutputDialog::LogOutputDialog(const SimLog::Settings& settings, QWidget* parent)
    : QDialog(parent) {
  setWindowTitle(QStringLiteral("Log Output"));
  setFixedSize(297, 419);
  inform_ = [this](const QString& text) {
    QMessageBox::information(this, QStringLiteral("Information"), text);
  };

  (new QLabel(QStringLiteral("Execution Log File Name:"), this))->setGeometry(16, 16, 200, 13);
  exec_name_ = new QLineEdit(settings.exec_path, this);
  exec_name_->setGeometry(16, 32, 241, 21);
  BrowseButton(this, exec_name_, QStringLiteral("EASy68kELog.txt"), QRect(256, 32, 25, 25));

  auto* exec_box = new QGroupBox(QStringLiteral("Execution Log Type"), this);
  exec_box->setGeometry(16, 72, 209, 121);
  static const char* const kExecTypes[4] = {"Disabled", "Instruction only",
                                            "Instruction and Registers",
                                            "Instruction, Registers and Memory"};
  for (int i = 0; i < 4; i++) {
    exec_type_[i] = new QRadioButton(QString::fromLatin1(kExecTypes[i]), exec_box);
    exec_type_[i]->setGeometry(8, 20 + i * 24, 197, 20);
  }

  mem_range_ = new QGroupBox(QStringLiteral("Memory Range to Log"), this);
  mem_range_->setGeometry(16, 200, 209, 65);
  (new QLabel(QStringLiteral("From:"), mem_range_))->setGeometry(16, 32, 30, 13);
  mem_from_ = HexEdit(mem_range_, 8, QRect(48, 28, 72, 21));
  mem_from_->setText(settings.mem_from);
  (new QLabel(QStringLiteral("Bytes"), mem_range_))->setGeometry(134, 32, 32, 13);
  mem_bytes_ = HexEdit(mem_range_, 2, QRect(168, 28, 25, 21));
  mem_bytes_->setText(settings.mem_bytes);
  // MemFromExit(): an address outside memory is reported.
  connect(mem_from_, &QLineEdit::editingFinished, this, [this] {
    const uint32_t addr = mem_from_->text().toUInt(nullptr, 16);
    if (addr >= sim::kMemorySize && inform_)
      inform_(QStringLiteral("Invalid Address"));
  });

  (new QLabel(QStringLiteral("Output Log File Name:"), this))->setGeometry(16, 280, 200, 13);
  output_name_ = new QLineEdit(settings.output_path, this);
  output_name_->setGeometry(16, 296, 241, 21);
  BrowseButton(this, output_name_, QStringLiteral("EASy68kOLog.txt"), QRect(256, 296, 25, 25));

  auto* output_box = new QGroupBox(QStringLiteral("Output Log Type"), this);
  output_box->setGeometry(16, 328, 177, 81);
  output_type_[0] = new QRadioButton(QStringLiteral("Disabled"), output_box);
  output_type_[0]->setGeometry(8, 20, 160, 20);
  output_type_[1] = new QRadioButton(QStringLiteral("Text Only"), output_box);
  output_type_[1]->setGeometry(8, 44, 160, 20);

  auto* ok = new QPushButton(QStringLiteral("&OK"), this);
  ok->setGeometry(208, 336, 75, 25);
  ok->setDefault(true);
  connect(ok, &QPushButton::clicked, this, &QDialog::accept);
  auto* cancel = new QPushButton(QStringLiteral("&Cancel"), this);
  cancel->setGeometry(208, 384, 75, 25);
  connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

  // ELogTypeClick(): the range only for "Instruction, Registers and Memory".
  for (int i = 0; i < 4; i++) {
    connect(exec_type_[i], &QRadioButton::toggled, this,
            [this] { mem_range_->setVisible(exec_type_[SimLog::kExecMemory]->isChecked()); });
  }
  exec_type_[std::clamp(settings.exec_type, 0, 3)]->setChecked(true);
  output_type_[std::clamp(settings.output_type, 0, 1)]->setChecked(true);
  mem_range_->setVisible(settings.exec_type == SimLog::kExecMemory);
}

SimLog::Settings LogOutputDialog::Settings() const {
  SimLog::Settings s;
  for (int i = 0; i < 4; i++) {
    if (exec_type_[i]->isChecked())
      s.exec_type = i;
  }
  s.output_type = output_type_[1]->isChecked() ? SimLog::kOutputText : SimLog::kOutputDisabled;
  s.exec_path = exec_name_->text();
  s.output_path = output_name_->text();
  s.mem_from = mem_from_->text();
  s.mem_bytes = mem_bytes_->text();
  return s;
}

LogFileExistsDialog::LogFileExistsDialog(const QString& message, QWidget* parent)
    : QDialog(parent) {
  setWindowTitle(QStringLiteral("Log File Options"));
  setFixedSize(331, 99);
  auto* label = new QLabel(message, this);
  label->setGeometry(0, 16, 329, 20);
  label->setAlignment(Qt::AlignCenter);
  struct Button {
    const char* text;
    int x;
    SimLog::Exists choice;
  };
  for (const Button& b : {Button{"Replace", 16, SimLog::Exists::kReplace},
                          Button{"Append", 128, SimLog::Exists::kAppend},
                          Button{"Cancel", 240, SimLog::Exists::kCancel}}) {
    auto* button = new QPushButton(QString::fromLatin1(b.text), this);
    button->setGeometry(b.x, 64, 75, 25);
    button->setObjectName(QString::fromLatin1(b.text));
    const SimLog::Exists choice = b.choice;
    connect(button, &QPushButton::clicked, this, [this, choice] {
      choice_ = choice;
      choice == SimLog::Exists::kCancel ? reject() : accept();
    });
  }
}

}  // namespace easym68k::gui
