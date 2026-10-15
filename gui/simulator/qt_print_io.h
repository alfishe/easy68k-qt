// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// IPrintIO for Sim68K-Qt (Trap #15 task 10): the original's page layout
// (CODE9.CPP printChar(), lineFeed(), nextX(), formFeed(), initPrint()):
// margins of 1/8 page width and 1/11 page height, one column per "W", one
// row per "Xp" in the printer font; control characters BS, FF, LF, CR, HT,
// VT. A form feed (or the bottom of the page) ends the print job.
//
// Characters are queued to the GUI thread, where the printer is painted.

#ifndef EASY68K_GUI_SIMULATOR_QT_PRINT_IO_H_
#define EASY68K_GUI_SIMULATOR_QT_PRINT_IO_H_

#include <functional>
#include <memory>
#include <QFont>
#include <QObject>
#include <QPagedPaintDevice>
#include <QPainter>

#include "easym68k/sim/iprint_io.h"

namespace easym68k::gui {

class QtPrintIO : public sim::IPrintIO {
 public:
  // Creates the printer for one print job on the GUI thread (a QPrinter with
  // the Printer Setup settings); null = no printer, characters are dropped.
  using DeviceFactory = std::function<std::unique_ptr<QPagedPaintDevice>()>;

  // `font`: Options > Printer Font (FontDialogPrinter, Courier New 10).
  QtPrintIO(QObject* gui_context, DeviceFactory factory, const QFont& font);
  ~QtPrintIO() override;

  void PrintChar(char ch) override;
  void FormFeed() override;

  // Ends an open print job (program end or reset). The original left a job
  // without a form feed open until the application closed.
  void EndJob();
  void SetFont(const QFont& font);

  static QFont DefaultFont();

 private:
  // GUI thread only.
  void DoPrintChar(char ch);
  void DoFormFeed();
  bool InitPrint();  // creates the device and the layout; false if none
  void LineFeed();
  void NextX();
  void FinishDocument();

  std::unique_ptr<QObject> receiver_;  // GUI-thread context for queued calls
  DeviceFactory factory_;
  QFont font_;

  std::unique_ptr<QPagedPaintDevice> device_;
  QPainter painter_;
  bool printing_ = false;
  int row_height_ = 0;
  int col_width_ = 0;
  int ascent_ = 0;
  int top_ = 0;
  int bottom_ = 0;
  int left_ = 0;
  int right_ = 0;
  int x_ = 0;
  int y_ = 0;
  // Position carried to the next page when LF, VT or a wrap ended the page
  // (the original restarted at the top-left margin).
  bool carry_ = false;
  int carry_x_ = 0;
  int carry_y_ = 0;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_QT_PRINT_IO_H_
