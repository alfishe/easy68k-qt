// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "source_view.h"

#include <QFontDatabase>

#include "listing.h"

namespace easym68k::gui {

SourceView::SourceView(QWidget* parent) : QListWidget(parent) {
  setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));  // Options > Source font: 13.18
  setUniformItemSizes(true);
  setSelectionMode(QAbstractItemView::SingleSelection);
}

void SourceView::SetListing(const QStringList& lines, bool is_listing) {
  lines_ = lines;
  is_listing_ = is_listing;
  clear();
  addItems(lines);
}

// SIM68Ku.cpp highlight(): the first code line with the PC's address.
void SourceView::HighlightPc(uint32_t pc) {
  if (!is_listing_)
    return;
  const int row = LineOfAddress(lines_, pc & 0x00FFFFFF);
  if (row < 0)
    return;
  setCurrentRow(row);
  scrollToItem(item(row), QAbstractItemView::PositionAtCenter);
}

// SIM68Ku.cpp RunToCursorExecute(): a code line after the header (index > 2).
std::optional<uint32_t> SourceView::SelectedAddress() const {
  const int row = currentRow();
  if (!is_listing_ || row <= 2 || row >= lines_.size() || !IsInstructionLine(lines_[row]))
    return std::nullopt;
  return LineAddress(lines_[row]);
}

}  // namespace easym68k::gui
