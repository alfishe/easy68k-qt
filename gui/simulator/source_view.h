// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The listing view of the Sim68K main window (SIM68Ku.dfm ListBox1): the
// .L68 lines with the line of the PC highlighted (SIM68Ku.cpp highlight()).
// The breakpoint gutter and owner drawing are Task 13.4.

#ifndef EASY68K_GUI_SIMULATOR_SOURCE_VIEW_H_
#define EASY68K_GUI_SIMULATOR_SOURCE_VIEW_H_

#include <cstdint>
#include <optional>
#include <QListWidget>
#include <QStringList>

namespace easym68k::gui {

class SourceView : public QListWidget {
  Q_OBJECT

 public:
  explicit SourceView(QWidget* parent = nullptr);

  void SetListing(const QStringList& lines, bool is_listing);
  void HighlightPc(uint32_t pc);  // selects the line of the PC, if any
  // The address of the selected line when it is a code line (Run To Cursor).
  std::optional<uint32_t> SelectedAddress() const;

 private:
  QStringList lines_;
  bool is_listing_ = false;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_SOURCE_VIEW_H_
