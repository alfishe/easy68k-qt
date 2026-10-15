// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Options > Fullscreen Options (FullscreenOptions.dfm / .cpp): the screen
// that shows the full screen output window, 1 = primary.

#ifndef EASY68K_GUI_SIMULATOR_FULLSCREEN_OPTIONS_DIALOG_H_
#define EASY68K_GUI_SIMULATOR_FULLSCREEN_OPTIONS_DIALOG_H_

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

namespace easym68k::gui {

class FullscreenOptionsDialog : public QDialog {
  Q_OBJECT

 public:
  // `monitor`: FullScreenMonitor (0 = primary); `screens`: the number of
  // screens.
  FullscreenOptionsDialog(int monitor, int screens, QWidget* parent = nullptr);

  int Monitor() const;  // the accepted FullScreenMonitor
  QLineEdit* ScreenNumber() const { return number_; }
  QPushButton* OkButton() const { return ok_; }

 private:
  int screens_;
  QLineEdit* number_ = nullptr;
  QPushButton* ok_ = nullptr;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_FULLSCREEN_OPTIONS_DIALOG_H_
