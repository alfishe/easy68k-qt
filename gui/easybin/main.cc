// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// EASyBIN-Qt: binary file creation and editing utility for EASy68K.

#include <QApplication>

#include "easybin/mainwindow.h"

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("EASyBIN"));
  QApplication::setOrganizationName(QStringLiteral("EASy68K"));
  easym68k::easybin::MainWindow window;
  window.show();
  window.ApplyArguments(QApplication::arguments().mid(1));
  return QApplication::exec();
}
