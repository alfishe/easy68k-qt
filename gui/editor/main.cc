// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QFileInfo>

#include "editor/mainwindow.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("Edit68K"));
  QApplication::setOrganizationName(QStringLiteral("EASy68K"));

  easym68k::editor::MainWindow window;
  bool opened_any = false;
  const QStringList args = app.arguments();
  for (int i = 1; i < args.size(); ++i) {
    if (QFileInfo::exists(args[i])) {
      if (window.OpenFile(args[i])) {
        opened_any = true;
      }
    }
  }
  if (!opened_any) {
    window.NewSourceWindow();
  }
  window.show();
  return app.exec();
}
