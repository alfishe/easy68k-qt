// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "listing.h"

namespace easym68k::gui {

namespace {

QChar At(const QString& s, int column) {  // 1-based, ' ' past the end
  return column - 1 < s.size() ? s.at(column - 1) : QLatin1Char(' ');
}

}  // namespace

bool IsInstructionLine(const QString& line) {
  return line.startsWith(QLatin1String("00")) && At(line, 11) != QLatin1Char(' ') &&
         At(line, 9) != QLatin1Char('=') && At(line, 11) != QLatin1Char('=');
}

std::optional<uint32_t> LineAddress(const QString& line) {
  if (line.size() < 8)
    return std::nullopt;
  bool ok = false;
  const uint32_t value = line.left(8).toUInt(&ok, 16);
  if (!ok)
    return std::nullopt;
  return value;
}

// SIM68Ku.cpp OpenFile(). The original read the line after the last line
// for a *[sim68k]break on the last line; such a command is ignored here.
ListingCommands ParseListing(const QStringList& lines) {
  ListingCommands c;
  if (!lines.isEmpty()) {
    const QString first = lines.first().left(8);
    if (first.size() == 8 && first >= QLatin1String("00000000") &&
        first < QLatin1String("01000000")) {
      if (auto a = LineAddress(first))
        c.start_address = a;
    }
  }
  for (int i = 0; i < lines.size(); i++) {
    const QString command = lines[i].mid(40).toLower();
    if (command.startsWith(QLatin1String("*[sim68k]break")) && i + 1 < lines.size()) {
      const QString& next = lines[i + 1];
      if (i + 1 > 2 && IsInstructionLine(next)) {
        if (auto a = LineAddress(next))
          c.breakpoints.push_back(*a);
      }
    }
    if (command.startsWith(QLatin1String("*[sim68k]bitfield")))
      c.bitfield = true;
    if (command.startsWith(QLatin1String("*[sim68k]simhalt_off")))
      c.simhalt_off = true;
  }
  return c;
}

int LineOfAddress(const QStringList& lines, uint32_t address) {
  for (int i = 0; i < lines.size(); i++) {
    if (!IsInstructionLine(lines[i]))
      continue;
    if (auto a = LineAddress(lines[i]); a && *a == address)
      return i;
  }
  return -1;
}

}  // namespace easym68k::gui
