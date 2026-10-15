// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The register panel of the Sim68K main window (SIM68Ku.dfm GroupBox1
// "Registers"; SIM68Ku.cpp scrshow(), loadRegs(), regKeyPress(),
// regSRKeyPress(), regA7KeyUp(), regUSKeyUp(), regA8KeyUp(),
// ClearCyclesClick()): D0-D7, A0-A7 (A7 = the active stack pointer), PC,
// SS, US as 8 hex digits, SR as 16 binary digits under "T S  INT   XNZVC",
// and the cycle counter with Clear Cycles. Every field can be edited while
// the program is not running; edits are applied at once.

#ifndef EASY68K_GUI_SIMULATOR_REGISTER_WIDGET_H_
#define EASY68K_GUI_SIMULATOR_REGISTER_WIDGET_H_

#include <array>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include "easym68k/sim/cpu_state.h"

namespace easym68k::gui {

class RegisterWidget : public QGroupBox {
  Q_OBJECT

 public:
  explicit RegisterWidget(QWidget* parent = nullptr);

  // SIM68Ku.cpp scrshow(): shows the state (also the state the edits start
  // from).
  void Show(const sim::CpuState& state);
  // Editing is allowed only while the program is not running.
  void SetEditable(bool editable);

  // Field access, for tests.
  QLineEdit* DataField(int n) const { return d_[n]; }
  QLineEdit* AddressField(int n) const { return a_[n]; }
  QLineEdit* PcField() const { return pc_; }
  QLineEdit* SrField() const { return sr_; }
  QLineEdit* UsField() const { return us_; }
  QLineEdit* SsField() const { return ss_; }
  QString CyclesText() const { return cycles_->text(); }
  QPushButton* ClearCyclesButton() const { return clear_cycles_; }

 signals:
  void Edited(const easym68k::sim::CpuState& state);  // a field changed
  void ClearCycles();
  void AddressDoubleClicked(uint32_t address);  // regDblClick: memory window
  void PcDoubleClicked();                       // regPCDblClick: highlight

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  QLineEdit* HexField(int row, int column, const QString& name);
  void OnEdited(QLineEdit* field);
  static uint32_t Value(const QLineEdit* field);

  sim::CpuState state_{};
  std::array<QLineEdit*, 8> d_{};
  std::array<QLineEdit*, 8> a_{};
  QLineEdit* pc_ = nullptr;
  QLineEdit* sr_ = nullptr;
  QLineEdit* us_ = nullptr;
  QLineEdit* ss_ = nullptr;
  QLabel* cycles_ = nullptr;
  QPushButton* clear_cycles_ = nullptr;
  bool showing_ = false;  // Show() in progress: no Edited()
};

}  // namespace easym68k::gui

Q_DECLARE_METATYPE(easym68k::sim::CpuState)

#endif  // EASY68K_GUI_SIMULATOR_REGISTER_WIDGET_H_
