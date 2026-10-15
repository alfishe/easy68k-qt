// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "qt_file_io.h"

#include <algorithm>
#include <QFile>
#include <utility>

#include "easym68k/sim/types.h"
#include "gui_thread.h"

namespace easym68k::gui {

QtFileIO::QtFileIO(uint8_t* memory, QObject* gui_context, DialogFn dialog,
                   std::function<void()> before_dialog)
    : memory_(memory),
      gui_context_(gui_context),
      dialog_(std::move(dialog)),
      before_dialog_(std::move(before_dialog)) {}

// At most 255 characters, the address wrapped to 24 bits.
QString QtFileIO::ReadString(uint32_t address) const {
  QByteArray bytes;
  for (int i = 0; i < 255; i++) {
    const char ch = static_cast<char>(memory_[(address + i) & sim::kAddressMask]);
    if (ch == '\0')
      break;
    bytes.append(ch);
  }
  return QFile::decodeName(bytes);
}

// The original set the Windows filter "ext|ext": the list is both the
// description and the patterns (separated by ';').
QString QtFileIO::FilterFor(const QString& extensions) {
  if (extensions.isEmpty())
    return QString();
  QString patterns = extensions;
  patterns.replace(QLatin1Char(';'), QLatin1Char(' '));
  return QStringLiteral("%1 (%2)").arg(extensions, patterns.simplified());
}

// simIOu.cpp displayFileDialog(). Only the chosen name and its terminator
// are written to A3 (the original's strncpy also zero-filled the rest of a
// 256-byte buffer); the Save dialog returns its own name (the original read
// the Open dialog's).
void QtFileIO::DisplayFileDialog(long* mode, long a1, long a2, long a3, short* result) {
  *result = sim::kFileSuccess;
  if (*mode != 0 && *mode != 1)
    return;
  DialogRequest request;
  request.save = *mode == 1;
  if (a1 != 0)
    request.title = ReadString(static_cast<uint32_t>(a1));
  if (a2 != 0)
    request.filter = FilterFor(ReadString(static_cast<uint32_t>(a2)));
  const auto path_address = static_cast<uint32_t>(a3) & sim::kAddressMask;
  request.path = ReadString(path_address);

  const std::optional<QString> chosen =
      CallInThread<std::optional<QString>>(gui_context_, [this, &request] {
        if (before_dialog_)
          before_dialog_();
        return dialog_ ? dialog_(request) : std::optional<QString>();
      });
  if (!chosen) {
    *mode = 0;  // cancelled
    return;
  }
  QByteArray name = QFile::encodeName(*chosen).left(255);
  if (path_address + static_cast<uint32_t>(name.size()) > sim::kMemorySize) {
    *result = sim::kFileError;  // would run past the 68000 memory
    return;
  }
  std::copy(name.cbegin(), name.cend(), memory_ + path_address);
  if (path_address + name.size() < sim::kMemorySize)
    memory_[path_address + name.size()] = 0;
  *mode = 1;
}

}  // namespace easym68k::gui
