// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// EASyBIN-Qt main window (EASyBIN/Form1u.dfm, Form1u.h, Form1u.cpp, fileIO.cpp):
// - Open S-Record (.S68/.S3/.S19) and Binary (.BIN) files, plus drag-and-drop
// - Save S-Record (From..To with Start Address) and Binary (First Address +
//   Length, split into 1, 2, or 4 files)
// - Color-coded hex + ASCII memory viewer/editor with Row/Page spin buttons,
//   mouse wheel, keyboard navigation, Copy Range, Fill Range, and New

#ifndef EASY68K_GUI_EASYBIN_MAINWINDOW_H_
#define EASY68K_GUI_EASYBIN_MAINWINDOW_H_

#include <cstdint>
#include <functional>
#include <QAction>
#include <QButtonGroup>
#include <QColor>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QRadioButton>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QWidget>

#include "easym68k/sim/memory.h"

namespace easym68k::gui {
class HelpViewer;
}

namespace easym68k::easybin {

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  static constexpr const char* kTitle = "EASyBIN v2.5.0";
  static constexpr int kHexLeft = 10;
  static constexpr int kHexRight = 56;
  static constexpr int kAscLeft = 58;
  static constexpr int kAscRight = 73;

  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

  void ApplyArguments(const QStringList& args);

  // File operations (callable directly in headless tests).
  bool OpenSRecordFile(const QString& path);
  int OpenBinaryFile(const QString& path);
  bool DropFile(const QString& path);
  bool SaveBinaryTo(const QString& path);
  bool SaveSRecordTo(const QString& path);
  void NewEnvironment(bool confirm = false);

  // Memory range operations.
  void CopyRange();
  void FillRange();

  // Split selection: index 0 -> 1 file (split 0), 1 -> 2 files (split 2),
  // 2 -> 4 files (split 4).
  void SetSplitIndex(int index);
  int Split() const { return split_; }

  // Navigation and editing helpers.
  void ShowAddress(uint32_t address);
  uint32_t TopAddress() const { return mem_addr_; }
  int Rows() const;
  QString RowText(int row) const;
  QColor ByteColor(uint32_t address) const;
  int CursorRow() const { return row_; }
  int CursorColumn() const { return col_; }
  void PlaceCursor(int row, int col);

  // Accessors for tests.
  sim::Memory& GetMemory() { return memory_; }
  const sim::Memory& GetMemory() const { return memory_; }
  QLineEdit* StartAddressField() const { return start_address_; }
  QLabel* S68EndAddressLabel() const { return s68_end_address_; }
  QLabel* S68LengthLabel() const { return s68_length_; }
  QLineEdit* OutputFirstAddressField() const { return output_first_address_; }
  QLineEdit* OutputLengthField() const { return output_length_; }
  QLineEdit* AddressField() const { return address_; }
  QLineEdit* FromField() const { return from_; }
  QLineEdit* ToField() const { return to_; }
  QLineEdit* BytesField() const { return bytes_; }
  QLineEdit* FillByteField() const { return fill_byte_; }
  QLabel* SplitLabel(int index) const;
  QAction* NewAction() const { return new_action_; }
  QAction* OpenSRecordAction() const { return open_srec_action_; }
  QAction* OpenBinaryAction() const { return open_bin_action_; }
  QAction* SaveSRecordAction() const { return save_srec_action_; }
  QAction* SaveBinaryAction() const { return save_bin_action_; }
  QAction* HelpAction() const { return help_action_; }
  QAction* AboutAction() const { return about_action_; }
  void ShowHelp(const QString& context);
  gui::HelpViewer* HelpView() const { return help_viewer_; }

  void SetNotify(std::function<void(const QString&)> notify) { notify_ = std::move(notify); }
  const QStringList& Notifications() const { return notifications_; }

  static QString SplitFilePath(const QString& path, int part);

 protected:
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  int RowHeight() const;
  int ColumnWidth() const;
  int HeaderTop() const;
  int HeaderHeight() const;
  int CanvasTop() const;
  void SetTop(int64_t address);
  void ScrollRows(int rows);
  void GotoRC(int row, int col);
  void ErasePrompt();
  void TypeKey(char key);
  bool LoadSrec(const QString& path);
  int LoadBinary(const QString& path, int split);
  void Notify(const QString& message);

  void OnOpenSRecord();
  void OnOpenBinary();
  void OnSaveSRecord();
  void OnSaveBinary();
  void OnAbout();
  void OnHelp();

  sim::Memory memory_;
  std::function<void(const QString&)> notify_;
  QStringList notifications_;

  QWidget* header_ = nullptr;
  QWidget* side_ = nullptr;
  gui::HelpViewer* help_viewer_ = nullptr;

  QAction* new_action_ = nullptr;
  QAction* open_srec_action_ = nullptr;
  QAction* open_bin_action_ = nullptr;
  QAction* save_srec_action_ = nullptr;
  QAction* save_bin_action_ = nullptr;
  QAction* exit_action_ = nullptr;
  QAction* help_action_ = nullptr;
  QAction* about_action_ = nullptr;

  QLineEdit* start_address_ = nullptr;
  QLabel* s68_end_address_ = nullptr;
  QLabel* s68_length_ = nullptr;
  QPushButton* open_srec_btn_ = nullptr;

  QButtonGroup* split_group_ = nullptr;
  QRadioButton* split_1_radio_ = nullptr;
  QRadioButton* split_2_radio_ = nullptr;
  QRadioButton* split_4_radio_ = nullptr;

  QLineEdit* output_first_address_ = nullptr;
  QLineEdit* output_length_ = nullptr;
  QLabel* split_labels_[4] = {nullptr, nullptr, nullptr, nullptr};
  QPushButton* save_bin_btn_ = nullptr;

  QLineEdit* address_ = nullptr;
  QLineEdit* from_ = nullptr;
  QLineEdit* to_ = nullptr;
  QLineEdit* bytes_ = nullptr;
  QPushButton* copy_btn_ = nullptr;
  QLineEdit* fill_byte_ = nullptr;
  QPushButton* fill_btn_ = nullptr;

  uint32_t mem_addr_ = 0;
  uint32_t start_addr_ = 0;
  uint32_t end_addr_ = 0;
  uint32_t length_ = 0;
  int split_ = 0;  // 0, 2, or 4
  int row_ = 0;
  int col_ = kHexLeft;
  bool cursor_shown_ = false;
  bool prompt_visible_ = false;
  QTimer prompt_timer_;
};

}  // namespace easym68k::easybin

#endif  // EASY68K_GUI_EASYBIN_MAINWINDOW_H_
