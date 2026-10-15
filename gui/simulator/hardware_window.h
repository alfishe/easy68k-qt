// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The hardware window ("EASy68K Hardware", hardwareu.dfm / hardwareu.cpp):
// eight 7-segment digits (one byte each, every other address from the
// 7-segment address; bits 0-7 = segments a-g and the point), eight LEDs
// (the bits of the LED byte), eight toggle switches and eight push buttons
// (bits of their bytes, a pressed button reads 0), the four device addresses,
// the IRQ 1-7 buttons, the automatic interrupts (a check box per level, the
// interval of the selected level), the Reset button and the memory map.
//
// While the program is stopped memory is read directly; while it runs the
// displays are refreshed from snapshots taken by the simulator thread, and
// the switches and buttons change memory through the session.

#ifndef EASY68K_GUI_SIMULATOR_HARDWARE_WINDOW_H_
#define EASY68K_GUI_SIMULATOR_HARDWARE_WINDOW_H_

#include <array>
#include <QAbstractButton>
#include <QByteArray>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QTimer>
#include <QWidget>

namespace easym68k::gui {

class SimSession;

class HardwareWindow : public QWidget {
  Q_OBJECT

 public:
  enum Device { kSeg7, kLeds, kSwitches, kButtons };
  enum MapRange { kRom, kRead, kProtected, kInvalid };

  explicit HardwareWindow(SimSession* session, QWidget* parent = nullptr);
  ~HardwareWindow() override;

  // hardwareu.cpp update(): reads the displayed bytes again.
  void Refresh();
  // Memory changed at `address` (updateIfNeeded()).
  void RefreshIfNeeded(uint32_t address);
  // A program was opened (initialize()): the switches are off.
  void Initialize();

  // Model of the window, for tests.
  int SegmentByte(int digit) const;  // digit 0 = rightmost; -1 = not shown
  int LedByte() const;               // -1 = not shown
  uint32_t Address(Device device) const { return loc_[device]; }
  QLineEdit* AddressField(Device device) const { return address_[device]; }
  bool AddressConflict(Device device) const;  // shown in red
  QAbstractButton* Switch(int bit) const { return switches_[bit]; }
  QAbstractButton* PushButton(int bit) const { return buttons_[bit]; }
  QAbstractButton* IrqButton(int level) const { return irq_[level - 1]; }
  QAbstractButton* ResetButton() const { return reset_; }
  QCheckBox* AutoIrqBox(int level) const { return auto_box_[level - 1]; }
  QComboBox* AutoIrqLevel() const { return auto_level_; }
  QLineEdit* IntervalField() const { return interval_; }
  QLabel* AutoLabel() const { return auto_label_; }
  QCheckBox* MapCheck(MapRange range) const { return map_check_[range]; }
  QLineEdit* MapStart(MapRange range) const { return map_start_[range]; }
  QLineEdit* MapEnd(MapRange range) const { return map_end_[range]; }

 signals:
  void MemoryChanged();        // a switch or button changed memory (program stopped)
  void NextWindowRequested();  // Ctrl+Tab (Break Points window)

 protected:
  void showEvent(QShowEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

 private:
  class SegmentPanel;
  class LedPanel;

  void BuildDisplays();
  void BuildAddresses();
  void BuildInterrupts();
  void BuildMemoryMap();
  void OnAddressChanged(Device device);
  void WriteByte(uint32_t address, uint8_t and_mask, uint8_t or_mask);
  void OnSnapshot(uint32_t address, const QByteArray& bytes);
  void ShowAutoIrqs();
  void ApplyAutoIrq(int level);
  void ShowMemoryMap();
  void ApplyMemoryMap();
  bool ReadByte(uint32_t address, uint8_t* value) const;

  SimSession* session_;
  SegmentPanel* segments_ = nullptr;
  LedPanel* leds_ = nullptr;
  std::array<QLineEdit*, 4> address_{};
  std::array<uint32_t, 4> loc_{};
  std::array<QAbstractButton*, 8> switches_{};
  std::array<QAbstractButton*, 8> buttons_{};
  uint8_t switch_bits_ = 0;  // the switches' positions
  std::array<QAbstractButton*, 7> irq_{};
  QAbstractButton* reset_ = nullptr;
  std::array<QCheckBox*, 7> auto_box_{};
  QComboBox* auto_level_ = nullptr;
  QLineEdit* interval_ = nullptr;
  QLabel* auto_label_ = nullptr;
  std::array<QCheckBox*, 4> map_check_{};
  std::array<QLineEdit*, 4> map_start_{};
  std::array<QLineEdit*, 4> map_end_{};
  bool showing_map_ = false;

  // Copies of the displayed bytes taken while the program runs.
  QTimer live_timer_;
  QByteArray seg_snapshot_;  // 16 bytes from the 7-segment address
  int led_snapshot_ = -1;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_HARDWARE_WINDOW_H_
