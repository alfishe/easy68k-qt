// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Sim68K-Qt: the EASy68K 68000 simulator.

#include <QApplication>

#include "mainwindow.h"

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("Sim68K"));
  QApplication::setOrganizationName(QStringLiteral("EASy68K"));
  easym68k::gui::MainWindow window;
  window.show();
  window.ApplyArguments(QApplication::arguments().mid(1));
  return QApplication::exec();
}
