// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The listing view of the Sim68K main window (SIM68Ku.dfm HeadingsLbl,
// breakP, ListBox1, ScrollBar1; SIM68Ku.cpp ListBox1DrawItem(),
// breakPPaint(), breakPMouseDown(), ClearAllPCBreakpoints1Click(),
// highlight(), ListBox1DblClick(), ScrollBar1Change()):
//  - the column heading, scrolled left/right with the listing;
//  - the breakpoint gutter: a green dot on each code line, red when it has
//    a PC breakpoint; a click toggles it; the popup menu clears all;
//  - the .L68 lines (tabs expanded to 8 columns) with the PC line selected;
//  - double click on a code line sets the PC to its address.

#ifndef EASY68K_GUI_SIMULATOR_SOURCE_VIEW_H_
#define EASY68K_GUI_SIMULATOR_SOURCE_VIEW_H_

#include <cstdint>
#include <functional>
#include <optional>
#include <QListWidget>
#include <QStringList>
#include <QWidget>

namespace easym68k::gui {

class SourceView : public QWidget {
  Q_OBJECT

 public:
  static constexpr char kHeading[] =
      " Address  --------Code---------   Line -----------Source----------->>";
  static constexpr int kHeadingLimit = 41;  // HEADING_LIMIT: heading scroll stop

  explicit SourceView(QWidget* parent = nullptr);

  // `is_listing`: false for the "no .L68 file" text (no code lines).
  void SetListing(const QStringList& lines, bool is_listing);
  // Breakpoint state for the gutter.
  void SetBreakpointQuery(std::function<bool(uint32_t)> has_breakpoint);
  void SetListFont(const QFont& font);  // Options > Source Window Font
  void RefreshBreakpoints();

  // SIM68Ku.cpp highlight(): selects the last code line with the PC's
  // address (any line with it when the program is halted) and scrolls it
  // into view; leaves the selection if there is none.
  void HighlightPc(uint32_t pc, bool halted);
  // The address of the selected code line (Run To Cursor), if any.
  std::optional<uint32_t> SelectedAddress() const;
  // The address of a code line (index > 2), if any.
  std::optional<uint32_t> CodeLineAddress(int row) const;

  QListWidget* List() const { return list_; }
  QWidget* Gutter() const { return gutter_; }
  int RowAt(int y) const;  // listing row at a gutter/listing y, -1 if none

 signals:
  void BreakpointToggled(uint32_t address);
  void ClearAllBreakpoints();
  void PcRequested(uint32_t address);  // double click on a code line

 private:
  class GutterWidget;
  class HeadingWidget;

  QStringList lines_;  // as in the file (tabs kept), for addresses
  bool is_listing_ = false;
  std::function<bool(uint32_t)> has_breakpoint_;
  QListWidget* list_ = nullptr;
  QWidget* gutter_ = nullptr;
  QWidget* heading_ = nullptr;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_SOURCE_VIEW_H_
