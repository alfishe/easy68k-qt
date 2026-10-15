// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// IFileIO for Sim68K-Qt (Trap #15 tasks 50-59): the headless StdFileIO file
// operations plus the task 58 file dialog (simIOu.cpp displayFileDialog()),
// shown on the GUI thread while the simulator thread waits.

#ifndef EASY68K_GUI_SIMULATOR_QT_FILE_IO_H_
#define EASY68K_GUI_SIMULATOR_QT_FILE_IO_H_

#include <functional>
#include <optional>
#include <QObject>
#include <QString>

#include "easym68k/sim/std_file_io.h"

namespace easym68k::gui {

class QtFileIO : public sim::StdFileIO {
 public:
  // An Open (save = false) or Save dialog: title, Qt name filter (empty for
  // none) and initial file name.
  struct DialogRequest {
    bool save = false;
    QString title;
    QString filter;
    QString path;
  };
  // Shows the dialog (called on the GUI thread); nullopt when cancelled.
  using DialogFn = std::function<std::optional<QString>(const DialogRequest&)>;

  // `memory`: the 68000 memory (16 MB). `gui_context`: an object of the GUI
  // thread. `before_dialog` runs first on the GUI thread (the original
  // cleared the I/O window key state: the dialog takes the key presses).
  QtFileIO(uint8_t* memory, QObject* gui_context, DialogFn dialog,
           std::function<void()> before_dialog = {});

  // Task 58. D1 mode 0 = Open, 1 = Save; A1 title, A2 extension list (both
  // optional, 0 = none), A3 file name buffer (initial name in, chosen name
  // out). D1 = 1 when a file was chosen, 0 when cancelled; other modes do
  // nothing. D0 = F_ERROR if the name would not fit in memory.
  void DisplayFileDialog(long* mode, long a1, long a2, long a3, short* result) override;

  // The 68000 memory, when it is created after this object.
  void SetMemory(uint8_t* memory) { memory_ = memory; }

  // The Qt filter for the original's extension list ("*.txt;*.dat").
  static QString FilterFor(const QString& extensions);

 private:
  QString ReadString(uint32_t address) const;

  uint8_t* memory_;
  QObject* gui_context_;
  DialogFn dialog_;
  std::function<void()> before_dialog_;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_QT_FILE_IO_H_
