// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The memory window ("68000 Memory", Memory1.dfm / Memory1.cpp): rows of
// 16 bytes in hex and ASCII from the Address row; Row/Page spin buttons,
// mouse wheel and keys scroll; a click places the cursor in the hex or
// ASCII area and typing edits memory; From/To/Bytes with Copy, Fill and
// Save; Live shows memory while the program runs.
//
// Memory is read directly only while the program is stopped. While it runs
// the window shows its last image, or with Live a copy of the visible rows
// taken by the simulator thread every 100 ms (the original repainted the
// rows around each write). Edits are possible only while stopped.

#ifndef EASY68K_GUI_SIMULATOR_MEMORY_WINDOW_H_
#define EASY68K_GUI_SIMULATOR_MEMORY_WINDOW_H_

#include <functional>
#include <QByteArray>
#include <QCheckBox>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

#include "easym68k/sim/memory.h"

namespace easym68k::gui {

class MemoryWindow : public QWidget {
  Q_OBJECT

 public:
  // Memory1.cpp column layout (character columns).
  static constexpr int kHexLeft = 10;
  static constexpr int kHexRight = 56;
  static constexpr int kAscLeft = 58;
  static constexpr int kAscRight = 73;

  struct Access {
    std::function<bool()> readable;  // program stopped: memory may be read/written
    std::function<void(uint32_t address, uint32_t size)> request_snapshot;
  };

  MemoryWindow(sim::Memory* memory, Access access, QWidget* parent = nullptr);
  ~MemoryWindow() override;

  // Shows the row of `address` at the top (register double click, regDblClick()).
  void ShowAddress(uint32_t address);
  // A copy of memory taken by the simulator thread (Live).
  void OnSnapshot(uint32_t address, const QByteArray& bytes);
  // The program was stopped or memory changed elsewhere: repaint.
  void Refresh();

  // Model of the display, for tests.
  uint32_t TopAddress() const { return mem_addr_; }
  int Rows() const;
  QString RowText(int row) const;  // "00001000: 00 01 ... 0F 0123456789ABCDEF"
  int CursorRow() const { return row_; }
  int CursorColumn() const { return col_; }
  QLineEdit* AddressField() const { return address_; }
  QLineEdit* FromField() const { return from_; }
  QLineEdit* ToField() const { return to_; }
  QLineEdit* BytesField() const { return bytes_; }
  QCheckBox* LiveBox() const { return live_; }
  void PlaceCursor(int row, int col);  // as a mouse click at that cell
  void Copy();
  void Fill();
  bool SaveTo(const QString& path);  // the Save button without the dialog
  // Message boxes (replaceable for tests).
  void SetNotify(std::function<void(const QString&)> notify) { notify_ = std::move(notify); }

 signals:
  void MemoryEdited(uint32_t address);  // a byte was changed (hardware/stack refresh)
  void NextWindowRequested();           // Ctrl+Tab (stack window)

 protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  int RowHeight() const;
  int ColumnWidth() const;
  int HeaderHeight() const;
  bool Readable() const { return !access_.readable || access_.readable(); }
  bool ByteAt(uint32_t address, uint8_t* value) const;  // false if not available
  void SetTop(int64_t address);
  void ScrollRows(int rows);
  void GotoRC(int row, int col);
  void TypeKey(char key);
  void OnSave();

  sim::Memory* memory_;
  Access access_;
  std::function<void(const QString&)> notify_;
  QWidget* header_ = nullptr;
  QWidget* side_ = nullptr;
  QLineEdit* address_ = nullptr;
  QLineEdit* from_ = nullptr;
  QLineEdit* to_ = nullptr;
  QLineEdit* bytes_ = nullptr;
  QCheckBox* live_ = nullptr;
  uint32_t mem_addr_ = 0;  // memAddr: top row, multiple of 16
  int row_ = 0;
  int col_ = kHexLeft;
  bool cursor_shown_ = false;  // a click placed the cursor
  bool prompt_visible_ = false;
  QTimer prompt_timer_;  // 500 ms
  QTimer live_timer_;    // 100 ms while running with Live
  uint32_t snap_address_ = 0;
  QByteArray snapshot_;
  QPixmap last_image_;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_MEMORY_WINDOW_H_
