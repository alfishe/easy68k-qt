// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "qt_simulator_env.h"

#include <algorithm>
#include <chrono>
#include <QTime>

#include "gui_thread.h"
#include "sim_io_screen.h"

namespace easym68k::gui {

QtSimulatorEnv::QtSimulatorEnv(SimIoScreen* screen, IrqFn irq, QObject* parent)
    : QObject(parent), screen_(screen), irq_(std::move(irq)) {
  for (int i = 0; i < 7; i++) {
    timers_[i] = new QTimer(this);
    timers_[i]->setInterval(auto_irq_[i].interval_ms);
    connect(timers_[i], &QTimer::timeout, this, [this, i] {
      if (irq_)
        irq_(i + 1);
    });
  }
}

QtSimulatorEnv::~QtSimulatorEnv() {
  CancelDelay();
}

// CODE9.CPP task 8 (gettime): hundredths of a second since midnight.
uint32_t QtSimulatorEnv::GetTimeHundredths() {
  return static_cast<uint32_t>(QTime::currentTime().msecsSinceStartOfDay() / 10);
}

// CODE9.CPP task 23 looped Sleep(10) n times; one timed wait here.
void QtSimulatorEnv::DelayHundredths(unsigned int n) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(10) * static_cast<int64_t>(n);
  std::unique_lock<std::mutex> lock(mu_);
  while (delay_cv_.wait_until(lock, deadline, [this] { return delay_cancelled_ || poked_; })) {
    if (delay_cancelled_)
      return;
    poked_ = false;
    lock.unlock();
    if (wait_hook_)
      wait_hook_();
    lock.lock();
  }
}

void QtSimulatorEnv::Poke() {
  {
    std::lock_guard<std::mutex> lock(mu_);
    poked_ = true;
  }
  delay_cv_.notify_all();
}

void QtSimulatorEnv::CancelDelay() {
  {
    std::lock_guard<std::mutex> lock(mu_);
    delay_cancelled_ = true;
  }
  delay_cv_.notify_all();
}

void QtSimulatorEnv::ResumeDelay() {
  std::lock_guard<std::mutex> lock(mu_);
  delay_cancelled_ = false;
}

void QtSimulatorEnv::ShowHardware() {
  emit ShowHardwareRequested();
}

uint32_t QtSimulatorEnv::GetSeg7Address() {
  return GetHardwareAddresses().seg7;
}
uint32_t QtSimulatorEnv::GetLEDAddress() {
  return GetHardwareAddresses().leds;
}
uint32_t QtSimulatorEnv::GetSwitchAddress() {
  return GetHardwareAddresses().switches;
}
uint32_t QtSimulatorEnv::GetPushButtonAddress() {
  return GetHardwareAddresses().buttons;
}

void QtSimulatorEnv::SetHardwareAddresses(const HardwareAddresses& addresses) {
  std::lock_guard<std::mutex> lock(mu_);
  addresses_ = addresses;
}

QtSimulatorEnv::HardwareAddresses QtSimulatorEnv::GetHardwareAddresses() const {
  std::lock_guard<std::mutex> lock(mu_);
  return addresses_;
}

uint32_t QtSimulatorEnv::GetVersion() {
  return kVersion;
}

void QtSimulatorEnv::SetExceptionsEnabled(bool enable) {
  emit ExceptionsEnabledChanged(enable);
}

void QtSimulatorEnv::SetWindowSize(unsigned short width, unsigned short height) {
  screen_->SetWindowSize(width, height);
}

void QtSimulatorEnv::GetWindowSize(unsigned short* width, unsigned short* height) {
  int w = 0, h = 0;
  screen_->GetWindowSize(&w, &h);
  *width = static_cast<unsigned short>(w);
  *height = static_cast<unsigned short>(h);
}

void QtSimulatorEnv::SetupWindow(bool fullscreen) {
  emit FullScreenRequested(fullscreen);
}

void QtSimulatorEnv::SetAutoIRQ(uint8_t irq_config, uint32_t interval) {
  PostToThread(this, [this, irq_config, interval] { ApplyAutoIrq(irq_config, interval); });
}

// hardwareu.cpp setAutoIRQ(): checking a box starts its timer only while
// the automatic interrupts are on (CheckBoxNClick). Code 0 unchecks every
// level; unlike the original it does not also turn the automatic mode off,
// which left levels enabled afterwards by the program idle until the next
// run.
void QtSimulatorEnv::ApplyAutoIrq(uint8_t irq_config, uint32_t interval) {
  {
    std::lock_guard<std::mutex> lock(mu_);
    const int level = irq_config & 0x7F;
    if (irq_config == 0) {
      for (auto& a : auto_irq_)
        a.checked = false;
    } else if (level >= 1 && level <= 7 && (irq_config & 0x80) == 0) {
      auto_irq_[level - 1].checked = false;
    } else if (level >= 1 && level <= 7) {
      auto_irq_[level - 1].checked = true;
      auto_irq_[level - 1].interval_ms = static_cast<int>(std::min<uint32_t>(interval, 99999999));
    }
  }
  for (int i = 1; i <= 7; i++)
    UpdateTimer(i);
  emit AutoIrqChanged();
}

void QtSimulatorEnv::SetHardwareEnabled(bool enabled) {
  PostToThread(this, [this, enabled] {
    bool changed = false;
    {
      std::lock_guard<std::mutex> lock(mu_);
      changed = auto_irq_on_ != enabled;
      auto_irq_on_ = enabled;
    }
    for (int i = 1; i <= 7; i++)
      UpdateTimer(i);
    if (changed)
      emit AutoIrqEnabledChanged(enabled);
  });
}

bool QtSimulatorEnv::AutoIrqEnabled() const {
  std::lock_guard<std::mutex> lock(mu_);
  return auto_irq_on_;
}

void QtSimulatorEnv::ClearAutoIrqs() {
  SetHardwareEnabled(false);
  PostToThread(this, [this] {
    {
      std::lock_guard<std::mutex> lock(mu_);
      for (auto& a : auto_irq_)
        a.checked = false;
    }
    for (int i = 1; i <= 7; i++)
      UpdateTimer(i);
    emit AutoIrqChanged();
  });
}

void QtSimulatorEnv::AutoIrqOff() {
  SetHardwareEnabled(false);
}

void QtSimulatorEnv::SetAutoIrq(int level, const AutoIrq& config) {
  if (level < 1 || level > 7)
    return;
  PostToThread(this, [this, level, config] {
    {
      std::lock_guard<std::mutex> lock(mu_);
      auto_irq_[level - 1] = config;
    }
    UpdateTimer(level);
    emit AutoIrqChanged();
  });
}

QtSimulatorEnv::AutoIrq QtSimulatorEnv::GetAutoIrq(int level) const {
  std::lock_guard<std::mutex> lock(mu_);
  return level >= 1 && level <= 7 ? auto_irq_[level - 1] : AutoIrq{};
}

bool QtSimulatorEnv::AutoIrqRunning(int level) const {
  return level >= 1 && level <= 7 && timers_[level - 1]->isActive();
}

// A TTimer with interval 0 never fires.
void QtSimulatorEnv::UpdateTimer(int level) {
  AutoIrq config;
  bool on = false;
  {
    std::lock_guard<std::mutex> lock(mu_);
    config = auto_irq_[level - 1];
    on = auto_irq_on_;
  }
  QTimer* t = timers_[level - 1];
  t->setInterval(config.interval_ms);
  if (on && config.checked && config.interval_ms > 0)
    t->start();
  else
    t->stop();
}

}  // namespace easym68k::gui
