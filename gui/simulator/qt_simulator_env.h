// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// ISimulatorEnv for Sim68K-Qt (Trap #15 tasks 8, 23, 32, 33): time, delay,
// hardware window addresses and automatic interrupts (hardwareu.cpp
// setAutoIRQ(), IRQ1timer..IRQ7timer), the output window size (SimIoScreen)
// and requests to the main window (show hardware, full screen, exceptions
// option), signalled for the GUI.
//
// The automatic interrupt timers run on the thread that owns this object
// (the GUI thread) and call `irq` (the hardware window's IRQprocess()).

#ifndef EASY68K_GUI_SIMULATOR_QT_SIMULATOR_ENV_H_
#define EASY68K_GUI_SIMULATOR_QT_SIMULATOR_ENV_H_

#include <array>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <QObject>
#include <QTimer>

#include "easym68k/sim/isimulator_env.h"

namespace easym68k::gui {

class SimIoScreen;

class QtSimulatorEnv : public QObject, public sim::ISimulatorEnv {
  Q_OBJECT

 public:
  using IrqFn = std::function<void(int level)>;
  static constexpr uint32_t kVersion = 0x00051001;  // def.h VERSION

  // Hardware window device addresses (defaults as Sim68K).
  struct HardwareAddresses {
    uint32_t seg7 = 0xE00000;
    uint32_t leds = 0xE00010;
    uint32_t switches = 0xE00012;
    uint32_t buttons = 0xE00014;
  };

  // One automatic interrupt (hardware window check box and interval).
  struct AutoIrq {
    bool checked = false;
    int interval_ms = 1000;  // TTimer default
  };

  QtSimulatorEnv(SimIoScreen* screen, IrqFn irq, QObject* parent = nullptr);
  ~QtSimulatorEnv() override;

  // ---- ISimulatorEnv (simulator thread) ---------------------------------------
  uint32_t GetTimeHundredths() override;
  // Waits n/100 s; returns early on CancelDelay() (the original stopped
  // waiting when the run ended).
  void DelayHundredths(unsigned int n) override;
  void ShowHardware() override;
  uint32_t GetSeg7Address() override;
  uint32_t GetLEDAddress() override;
  uint32_t GetSwitchAddress() override;
  uint32_t GetPushButtonAddress() override;
  // Task 32.6: 0 = all off; $01-$07 = that level off; $81-$87 = that level
  // on with `interval` ms. Other values only refresh the display.
  void SetAutoIRQ(uint8_t irq_config, uint32_t interval) override;
  uint32_t GetVersion() override;
  void SetExceptionsEnabled(bool enable) override;
  void SetWindowSize(unsigned short width, unsigned short height) override;
  void GetWindowSize(unsigned short* width, unsigned short* height) override;
  void SetupWindow(bool fullscreen) override;

  // ---- GUI ----------------------------------------------------------------
  void SetHardwareAddresses(const HardwareAddresses& addresses);
  HardwareAddresses GetHardwareAddresses() const;
  // Hardware window enabled (program loaded and not halted): the checked
  // automatic interrupts run (hardwareu.cpp enable()/disable()).
  void SetHardwareEnabled(bool enabled);
  // Program start (STARTSIM.CPP autoIRQoff()): timers stop until enabled.
  void AutoIrqOff();
  void SetAutoIrq(int level, const AutoIrq& config);  // check box / interval
  AutoIrq GetAutoIrq(int level) const;
  bool AutoIrqRunning(int level) const;
  void CancelDelay();  // stop/reset: wakes and skips delays until ResumeDelay()
  void ResumeDelay();

 signals:
  void ShowHardwareRequested();
  void FullScreenRequested(bool fullscreen);
  void ExceptionsEnabledChanged(bool enabled);
  void AutoIrqChanged();  // check boxes or intervals changed

 private:
  void ApplyAutoIrq(uint8_t irq_config, uint32_t interval);
  void UpdateTimer(int level);

  SimIoScreen* screen_;
  IrqFn irq_;
  mutable std::mutex mu_;
  std::condition_variable delay_cv_;
  bool delay_cancelled_ = false;
  HardwareAddresses addresses_;
  std::array<AutoIrq, 7> auto_irq_{};
  bool auto_irq_on_ = false;
  std::array<QTimer*, 7> timers_{};
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_QT_SIMULATOR_ENV_H_
