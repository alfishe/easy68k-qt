// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// ISimObserver — simulator event notifications (Task 9.5).
// Not present in the original (whose run loop updated VCL forms directly);
// this lets a GUI, CLI or test harness follow execution without touching the
// core.
//
// Threading: OnEvent() is always called on the thread executing Step()/Run().
// Interrupt() may be called from any thread, so kInterruptRequested and
// kResumed are recorded there and delivered at the start of the next Step().
// AddObserver()/RemoveObserver() must be called from the simulator thread or
// while the simulator is not running; removing an observer from inside
// OnEvent() is safe.

#ifndef EASY68K_SIM_ISIM_OBSERVER_H_
#define EASY68K_SIM_ISIM_OBSERVER_H_

#include <cstdint>

#include "easym68k/sim/types.h"

namespace easym68k::sim {

enum class SimEventType {
  kInstructionExecuted,  // an instruction was executed (any result)
  kBreakpointHit,        // Run() stopped at a PC breakpoint
  kExceptionRaised,      // exception processing started (incl. interrupts)
  kInterruptRequested,   // Interrupt(level) was called
  kHalted,               // simulator halted (SIMHALT, task 9, error, ...)
  kStopped,              // STOP instruction
  kResumed,              // woke from STOP on an interrupt
};

struct SimEvent {
  SimEventType type{};
  uint32_t pc{};                      // instruction address (kInstructionExecuted,
                                      // kExceptionRaised, kHalted, kStopped) or current PC
  SimResult result = SimResult::kOk;  // instruction result where applicable, else kOk
  int interrupt_level = 0;            // kInterruptRequested / interrupt kExceptionRaised
  int vector = -1;                    // exception vector number for kExceptionRaised
};

class ISimObserver {
 public:
  virtual ~ISimObserver() = default;
  virtual void OnEvent(const SimEvent& event) = 0;
};

}  // namespace easym68k::sim

#endif  // EASY68K_SIM_ISIM_OBSERVER_H_
