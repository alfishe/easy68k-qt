// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The .L68 listing as Sim68K uses it (SIM68Ku.cpp OpenFile(), isInstruction()):
// the program start address on the first line and the *[sim68k] commands.

#ifndef EASY68K_GUI_SIMULATOR_LISTING_H_
#define EASY68K_GUI_SIMULATOR_LISTING_H_

#include <cstdint>
#include <optional>
#include <QString>
#include <QStringList>
#include <vector>

namespace easym68k::gui {

// A code line: "00" address, data in column 11, not DS/DC ('=' in column 9)
// nor EQU/SET ('=' in column 11) (columns 1-based, as the original).
bool IsInstructionLine(const QString& line);

// The 8-digit hex address at the start of a line, if any.
std::optional<uint32_t> LineAddress(const QString& line);

struct ListingCommands {
  std::optional<uint32_t> start_address;  // first line, 00000000-00FFFFFF
  std::vector<uint32_t> breakpoints;      // *[sim68k]break: the next code line
  bool bitfield = false;                  // *[sim68k]bitfield
  bool simhalt_off = false;               // *[sim68k]simhalt_off
};

// Scans a listing (case-insensitive commands at column 41).
ListingCommands ParseListing(const QStringList& lines);

// The listing line of `address` (the first code line with that address), or
// -1.
int LineOfAddress(const QStringList& lines, uint32_t address);

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_LISTING_H_
