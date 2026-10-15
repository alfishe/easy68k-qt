// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// SimSession: the Sim68K-Qt simulation session, the main window's model
// (SIM68Ku.cpp OpenFile(), Run/Step/Trace/Pause/Rewind/Reload, RUN.CPP
// runprog() messages, STARTSIM.CPP initSim(), hardwareu.cpp IRQprocess()).
//
// It owns the simulator, all Trap #15 back ends and a worker thread. The
// program executes on the worker; the GUI thread issues commands and gets
// StateChanged()/Message()/ExecutionStopped(). While IsRunning() the GUI must
// not touch the simulator; otherwise Simulator() may be read and edited.
// Commands that need the program stopped (Open, Close, Rewind, Reload)
// stop it first and complete when the worker has returned.

#ifndef EASY68K_GUI_SIMULATOR_SIM_SESSION_H_
#define EASY68K_GUI_SIMULATOR_SIM_SESSION_H_

#include <functional>
#include <memory>
#include <optional>
#include <QObject>
#include <QPagedPaintDevice>
#include <QString>
#include <QStringList>
#include <QThread>

#include "easym68k/sim/isim_observer.h"
#include "easym68k/sim/simulator.h"
#include "easym68k/sim/std_network_io.h"

namespace easym68k::gui {

class SimIoScreen;
class QtTextIO;
class QtGraphicsIO;
class QtPeripheralIO;
class QtSimulatorEnv;
class QtFileIO;
class QtPrintIO;
class QtSoundIO;
class QtSerialIO;

class SimSession : public QObject {
  Q_OBJECT

 public:
  enum class RunState {
    kNoProgram,  // nothing loaded
    kReady,      // loaded, not executing: Run/Step/Trace available
    kRunning,    // the worker executes (Run, Run To, Step Over, Trace)
    kStopped,    // a STOP instruction waits for an interrupt
    kHalted,     // halted (SIMHALT, task 9, error): Rewind/Reload to restart
  };
  Q_ENUM(RunState)

  // Services provided by the GUI (main window).
  struct Services {
    // Task 58 file dialog (GUI thread); nullopt = cancelled.
    std::function<std::optional<QString>(bool save, const QString& title, const QString& filter,
                                         const QString& path)>
        file_dialog;
    // Creates the printer for a print job (GUI thread); null = no printer.
    std::function<std::unique_ptr<QPagedPaintDevice>()> printer;
  };

  explicit SimSession(Services services = {}, QObject* parent = nullptr);
  ~SimSession() override;

  // ---- Program -----------------------------------------------------------------
  // SIM68Ku.cpp OpenFile(): clears memory to $FF, loads the S-record file and
  // its .L68 listing (start address, *[sim68k] commands). Breakpoints are
  // cleared first. Returns false if the S-record file could not be loaded.
  bool Open(const QString& s68_path);
  bool OpenData(const QString& s68_path);  // load more S-records
  bool Reload();                           // Open(current file), breakpoints kept
  void Close();                            // forget the program
  void Rewind();                           // initSim(): PC = start address

  QString ProgramPath() const { return program_path_; }
  const QStringList& Listing() const { return listing_; }
  bool HasListing() const { return has_listing_; }

  // ---- Execution -----------------------------------------------------------------
  void Run();
  void RunTo(uint32_t address);  // Run To Cursor
  void StepOver();
  void TraceInto();
  void Pause();
  RunState State() const { return state_; }
  bool IsRunning() const { return state_ == RunState::kRunning; }

  // ---- Options ---------------------------------------------------------------------
  void SetExceptionsEnabled(bool enabled);
  bool ExceptionsEnabled() const;
  void SetBitFieldEnabled(bool enabled);
  bool BitFieldEnabled() const;

  // ---- Components (GUI thread) -----------------------------------------------------
  sim::M68kSimulator& Simulator() { return *sim_; }  // only while !IsRunning()
  SimIoScreen& Screen() { return *screen_; }
  QtSimulatorEnv& Env() { return *env_; }
  QtPeripheralIO& Peripheral() { return *peripheral_; }
  QtPrintIO& Print() { return *print_; }
  QtSoundIO& Sound() { return *sound_; }

  // An interrupt request from the hardware window, timers, mouse or keyboard
  // (hardwareu.cpp IRQprocess()): ignored while no program is active; wakes a
  // program waiting after STOP.
  void Interrupt(int level);

 signals:
  void StateChanged(easym68k::gui::SimSession::RunState state);
  void Message(const QString& text);  // a line for the message pane
  void MessagesCleared();             // a program was opened
  void OptionsChanged();              // exceptions / bit field changed by the program
  void ProgramLoaded();
  void ProgramClosed();
  // Execution stopped (or the program was rewound/loaded): refresh views.
  void ExecutionStopped();

 private:
  // kResume: run started by an interrupt (IRQprocess(): no pass-counter
  // reset).
  enum class Command { kRun, kRunTo, kStepOver, kTrace, kResume };
  class Observer;

  void InitSim();  // STARTSIM.CPP initSim() for the back ends and the CPU
  void Start(Command command, uint32_t address = 0);
  void OnFinished(Command command, sim::SimResult result);
  void ReportStop(Command command, sim::SimResult result);
  void SetState(RunState state);
  void WhenStopped(std::function<void()> action);  // after the worker returned
  bool LoadProgram(const QString& path, bool keep_breakpoints);

  Services services_;
  std::unique_ptr<SimIoScreen> screen_;
  std::unique_ptr<QtTextIO> text_io_;
  std::unique_ptr<QtGraphicsIO> graphics_io_;
  std::unique_ptr<QtPeripheralIO> peripheral_;
  std::unique_ptr<QtSimulatorEnv> env_;
  std::unique_ptr<QtFileIO> file_io_;
  std::unique_ptr<QtPrintIO> print_;
  std::unique_ptr<QtSoundIO> sound_;
  std::unique_ptr<QtSerialIO> serial_;
  std::unique_ptr<sim::StdNetworkIO> network_;
  std::unique_ptr<sim::M68kSimulator> sim_;
  std::unique_ptr<Observer> observer_;

  QThread thread_;
  QObject* worker_ = nullptr;  // lives in thread_

  RunState state_ = RunState::kNoProgram;
  QString program_path_;
  QStringList listing_;
  bool has_listing_ = false;
  bool trace_ = false;          // the original's `trace` (stepping, stopped)
  bool stop_reported_ = false;  // "STOP instruction" shown once (stopInstruction)
  bool hardware_enabled_ = false;
  std::function<void()> after_stop_;
};

}  // namespace easym68k::gui

#endif  // EASY68K_GUI_SIMULATOR_SIM_SESSION_H_
