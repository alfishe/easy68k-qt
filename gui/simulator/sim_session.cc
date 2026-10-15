// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "sim_session.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <utility>

#include "easym68k/sim/srecord_loader.h"
#include "listing.h"
#include "qt_file_io.h"
#include "qt_graphics_io.h"
#include "qt_peripheral_io.h"
#include "qt_print_io.h"
#include "qt_serial_io.h"
#include "qt_simulator_env.h"
#include "qt_sound_io.h"
#include "qt_text_io.h"
#include "sim_io_screen.h"

namespace easym68k::gui {

using sim::SimResult;

// Worker-thread observer: a STOP instruction ends the run (RUN.CPP: halt
// until an interrupt); the halt event gives the faulting instruction.
class SimSession::Observer : public sim::ISimObserver {
 public:
  Observer(sim::M68kSimulator* sim, SimSession* session) : sim_(sim), session_(session) {}
  void OnEvent(const sim::SimEvent& e) override {
    if (e.type == sim::SimEventType::kInstructionExecuted)
      session_->ServiceWorker();
    if (e.type == sim::SimEventType::kStopped)
      sim_->RequestStop();
    if (e.type == sim::SimEventType::kHalted || e.type == sim::SimEventType::kStopped) {
      halt_pc = e.pc;
      halt_result = e.result;
    }
  }
  uint32_t halt_pc = 0;
  SimResult halt_result = SimResult::kOk;

 private:
  sim::M68kSimulator* sim_;
  SimSession* session_;
};

namespace {

// The text shown when a .L68 file is missing (SIM68Ku.cpp OpenFile()).
QStringList NoListingText() {
  return {QStringLiteral("A matching .L68 file was not found. The .L68 file"),
          QStringLiteral("is used to provide source level debugging."),
          QString(),
          QStringLiteral("Use EASy68K to assemble the source file to create"),
          QStringLiteral("a properly formatted .L68 file and make sure the"),
          QStringLiteral(".L68 and .S68 files are in the same directory."),
          QString(),
          QStringLiteral("You may run the program without an .L68 file but"),
          QStringLiteral("source level debugging will not be available.")};
}

QString ListingPathFor(const QString& s68) {
  const QFileInfo info(s68);
  const QString base = info.dir().filePath(info.completeBaseName());
  for (const char* ext : {".L68", ".l68"}) {
    const QString candidate = base + QLatin1String(ext);
    if (QFileInfo::exists(candidate))
      return candidate;
  }
  return QString();
}

// RUN.CPP exec_inst(), exceptions off: the halt message for a result.
QString HaltMessage(SimResult r, uint32_t pc) {
  const char* text = nullptr;
  switch (r) {
    case SimResult::kBadInstruction:
      text = "Illegal instruction found at location %4x. Execution halted";
      break;
    case SimResult::kPrivilegeViolation:
      text = "supervisor privilege violation at location %4x. Execution halted";
      break;
    case SimResult::kChkException:
      text = "CHK exception occurred at location %4x. Execution halted";
      break;
    case SimResult::kStopInstruction:
      text = "STOP instruction executed at location %4x. Execution halted";
      break;
    case SimResult::kTrapException:
      text = "TRAP exception occurred at location %4x. Execution halted";
      break;
    case SimResult::kTrapVException:
      text = "TRAPV exception occurred at location %4x. Execution halted";
      break;
    case SimResult::kDivideByZero:
      text = "Divide by zero occurred at location %4x. Execution halted";
      break;
    case SimResult::kTraceException:
      text = "Trace exception occurred at location %4x. Execution halted";
      break;
    case SimResult::kLine1010:
      text = "Line 1010 Emulator exception occurred at location %4x. Execution halted";
      break;
    case SimResult::kLine1111:
      text = "Line 1111 Emulator exception occurred at location %4x. Execution halted";
      break;
    default:
      return QString();
  }
  return QString::asprintf(text, pc);
}

}  // namespace

SimSession::SimSession(Services services, QObject* parent)
    : QObject(parent), services_(std::move(services)) {
  screen_ = std::make_unique<SimIoScreen>();
  text_io_ = std::make_unique<QtTextIO>(screen_.get());
  graphics_io_ = std::make_unique<QtGraphicsIO>(screen_.get());
  peripheral_ = std::make_unique<QtPeripheralIO>([this](int level) { Interrupt(level); });
  screen_->SetKeyEventHook([this](bool down) { peripheral_->KeyEvent(down); });
  env_ = std::make_unique<QtSimulatorEnv>(screen_.get(), [this](int level) { Interrupt(level); });
  file_io_ = std::make_unique<QtFileIO>(
      nullptr, this,
      [this](const QtFileIO::DialogRequest& r) -> std::optional<QString> {
        if (!services_.file_dialog)
          return std::nullopt;
        return services_.file_dialog(r.save, r.title, r.filter, r.path);
      },
      [this] { screen_->ClearKeys(); });
  print_ = std::make_unique<QtPrintIO>(this, services_.printer, QtPrintIO::DefaultFont());
  sound_ = std::make_unique<QtSoundIO>(this, QtSoundIO::EffectVoiceFactory());
  serial_ = std::make_unique<QtSerialIO>();
  network_ = std::make_unique<sim::StdNetworkIO>();

  sim::M68kSimulator::Config config;
  config.text_io = text_io_.get();
  config.file_io = file_io_.get();
  config.serial_io = serial_.get();
  config.network_io = network_.get();
  config.graphics_io = graphics_io_.get();
  config.sound_io = sound_.get();
  config.peripheral_io = peripheral_.get();
  config.sim_env = env_.get();
  config.print_io = print_.get();
  config.exceptions_enabled = false;  // Sim68K default (Options > Enable Exceptions)
  sim_ = std::make_unique<sim::M68kSimulator>(config);
  file_io_->SetMemory(sim_->GetMemory().RawData());
  observer_ = std::make_unique<Observer>(sim_.get(), this);
  sim_->AddObserver(observer_.get());

  connect(env_.get(), &QtSimulatorEnv::ExceptionsEnabledChanged, this,
          [this](bool) { emit OptionsChanged(); });
  // A program waiting for input or a delay still serves the GUI's requests.
  screen_->SetWaitHook([this] { ServiceWorker(); });
  env_->SetWaitHook([this] { ServiceWorker(); });
  const auto hw = env_->GetHardwareAddresses();
  SetHardwareAddresses(hw.seg7, hw.leds, hw.switches, hw.buttons);

  worker_ = new QObject;
  worker_->moveToThread(&thread_);
  thread_.setObjectName(QStringLiteral("Sim68K"));
  thread_.start();
}

SimSession::~SimSession() {
  sim_->RequestStop();
  screen_->CancelInput();
  env_->CancelDelay();
  thread_.quit();
  thread_.wait();
  delete worker_;
  sim_->RemoveObserver(observer_.get());
}

// ---------------------------------------------------------------------------
// Program
// ---------------------------------------------------------------------------

// STARTSIM.CPP initSim(): output window, input, sounds, files, network,
// mouse/key IRQs and printing are reset; the CPU is initialised (PC = start
// address); the hardware is enabled.
void SimSession::InitSim() {
  screen_->ResetForRun();
  env_->ResumeDelay();
  env_->AutoIrqOff();
  peripheral_->Reset();
  sound_->ResetSounds();
  network_->Reset();
  short result = 0;
  file_io_->CloseAll(&result);
  print_->EndJob();
  sim_->ClearPendingIrqs();
  sim_->InitProgram();
  trace_ = false;
  stop_reported_ = false;
  SetHardwareEnabledFlag(true);  // Hardware->enable(): automatic IRQs run again
  env_->SetHardwareEnabled(true);
}

bool SimSession::Open(const QString& s68_path) {
  if (IsRunning()) {
    WhenStopped([this, s68_path] { LoadProgram(s68_path, false); });
    Pause();
    return true;
  }
  return LoadProgram(s68_path, false);
}

bool SimSession::Reload() {
  if (program_path_.isEmpty() || IsRunning())
    return false;
  return LoadProgram(program_path_, true);
}

// SIM68Ku.cpp OpenFile(). Breakpoints are cleared on every open, including
// drag and drop (the original cleared them only for File > Open, even when
// the dialog was cancelled); Reload keeps them.
bool SimSession::LoadProgram(const QString& path, bool keep_breakpoints) {
  if (!keep_breakpoints)
    ClearBreakpoints();
  InitSim();
  auto& memory = sim_->GetMemory();
  std::memset(memory.RawData(), 0xFF, memory.Size());                  // erase memory to $FF
  memory.RawData()[env_->GetSwitchAddress() & sim::kAddressMask] = 0;  // toggle switches
  sim_->SetSimhaltEnabled(true);
  env_->ClearAutoIrqs();  // Hardware->initialize()
  emit MessagesCleared();
  listing_.clear();
  has_listing_ = false;

  sim::SRecordLoader loader;
  const auto r = loader.LoadFromFile(QFile::encodeName(path).toStdString(), memory);
  if (!r.success) {
    emit Message(QStringLiteral("error: cannot open file %1").arg(path));
    program_path_.clear();
    SetState(RunState::kNoProgram);
    emit ProgramClosed();
    return false;
  }
  if (r.has_start_address)
    sim_->SetStartAddress(r.start_address);
  for (const std::string& text : r.messages)
    emit Message(QString::fromLatin1(text.c_str()));  // loadSrec()'s lines
  ApplyS0MemoryMap(r.s0_texts);

  const QString listing_path = ListingPathFor(path);
  QFile listing(listing_path);
  if (!listing_path.isEmpty() && listing.open(QIODevice::ReadOnly)) {
    const QString text = QString::fromLatin1(listing.readAll());
    listing_ = text.split(QLatin1Char('\n'));
    for (QString& line : listing_)
      if (line.endsWith(QLatin1Char('\r')))
        line.chop(1);
    if (!listing_.isEmpty() && listing_.last().isEmpty())
      listing_.removeLast();
    has_listing_ = true;
    const ListingCommands c = ParseListing(listing_);
    if (c.start_address)
      sim_->SetStartAddress(*c.start_address);
    for (uint32_t a : c.breakpoints)
      SetBreakpoint(a, true);
    if (c.bitfield && !sim_->IsBitFieldEnabled()) {
      sim_->SetBitFieldEnabled(true);
      emit OptionsChanged();
    }
    if (c.simhalt_off)
      sim_->SetSimhaltEnabled(false);
  } else {
    emit Message(QStringLiteral("Unable to open .L68 file."));
    listing_ = NoListingText();
  }
  sim_->InitProgram();  // PC = start address
  program_path_ = path;
  SetState(RunState::kReady);
  emit ProgramLoaded();
  emit ExecutionStopped();
  return true;
}

// SIM68Ku.cpp OpenDataExecute(): more S-records into memory. The PC is
// kept (the original's loader also set it from the data file's S7-S9
// record).
bool SimSession::OpenData(const QString& s68_path) {
  if (IsRunning())
    return false;
  sim::SRecordLoader loader;
  const auto r = loader.LoadFromFile(QFile::encodeName(s68_path).toStdString(), sim_->GetMemory());
  if (!r.success) {
    emit Message(QStringLiteral("error: cannot open file %1").arg(s68_path));
    return false;
  }
  for (const std::string& text : r.messages)
    emit Message(QString::fromLatin1(text.c_str()));  // loadSrec()'s lines
  ApplyS0MemoryMap(r.s0_texts);
  emit ExecutionStopped();  // memory changed
  return true;
}

// SIM68Ku.cpp CloseExecute(): the listing and breakpoints go; memory and
// the file name (for Reload) stay.
void SimSession::Close() {
  if (IsRunning()) {
    WhenStopped([this] { Close(); });
    Pause();
    return;
  }
  listing_.clear();
  has_listing_ = false;
  InitSim();
  ClearBreakpoints();
  if (state_ != RunState::kNoProgram)
    SetState(RunState::kReady);
  emit ProgramClosed();
  emit ExecutionStopped();
}

// SIM68Ku.cpp RewindExecute(): PC = start address, initSim().
void SimSession::Rewind() {
  if (IsRunning()) {
    WhenStopped([this] { Rewind(); });
    Pause();
    return;
  }
  if (state_ == RunState::kNoProgram)
    return;
  InitSim();
  SetState(RunState::kReady);
  emit ExecutionStopped();
}

// ---------------------------------------------------------------------------
// Execution
// ---------------------------------------------------------------------------

void SimSession::Run() {
  Start(Command::kRun);
}
void SimSession::RunTo(uint32_t address) {
  Start(Command::kRunTo, address);
}
void SimSession::StepOver() {
  Start(Command::kStepOver);
}
void SimSession::TraceInto() {
  Start(Command::kTrace);
}

void SimSession::Start(Command command, uint32_t address) {
  if (state_ != RunState::kReady && state_ != RunState::kStopped)
    return;
  switch (command) {
    case Command::kRun:
    case Command::kRunTo:
      sim_->ResetBreakPassCounts();  // BreaksFrm->resetDebug()
      trace_ = false;
      env_->SetHardwareEnabled(true);  // Hardware->autoIRQon()
      break;
    case Command::kStepOver:
    case Command::kTrace:
      trace_ = true;
      break;
    case Command::kResume:
      trace_ = false;
      break;
  }
  observer_->halt_pc = 0;
  observer_->halt_result = SimResult::kOk;
  screen_->ResumeInput();
  env_->ResumeDelay();
  sim_->ClearStopRequest();
  ServiceWorker();
  SetState(RunState::kRunning);
  QMetaObject::invokeMethod(
      worker_,
      [this, command, address] {
        SimResult result = SimResult::kOk;
        switch (command) {
          case Command::kRun:
          case Command::kResume:
            result = sim_->Run();
            break;
          case Command::kRunTo:
            result = sim_->RunTo(address);
            break;
          case Command::kStepOver:
            result = sim_->StepOver();
            break;
          case Command::kTrace:
            result = sim_->Step();
            break;
        }
        QMetaObject::invokeMethod(
            this, [this, command, result] { OnFinished(command, result); }, Qt::QueuedConnection);
      },
      Qt::QueuedConnection);
}

// SIM68Ku.cpp PauseExecute(): stops a run; a program waiting for input or
// a delay is released.
void SimSession::Pause() {
  trace_ = true;
  if (IsRunning()) {
    sim_->RequestStop();
    screen_->CancelInput();
    env_->CancelDelay();
    return;
  }
  env_->SetHardwareEnabled(false);  // Hardware->autoIRQoff()
}

void SimSession::WhenStopped(std::function<void()> action) {
  after_stop_ = std::move(action);
}

// RUN.CPP runprog() after an instruction stopped the run.
void SimSession::OnFinished(Command command, SimResult result) {
  ServiceWorker();  // requests the worker did not reach
  const auto& cpu = sim_->State();
  if (cpu.stopped && ExceptionsEnabled()) {
    // STOP: waits for an interrupt; the automatic IRQs keep running unless
    // the program is being traced.
    if (trace_)
      env_->SetHardwareEnabled(false);
    SetState(RunState::kStopped);
  } else if (cpu.halted) {
    SetHardwareEnabledFlag(false);  // haltSimulator(): Hardware->disable()
    env_->SetHardwareEnabled(false);
    SetState(RunState::kHalted);
  } else {
    trace_ = true;
    env_->SetHardwareEnabled(false);  // autoIRQoff()
    SetState(RunState::kReady);
  }
  ReportStop(command, result);
  emit ExecutionStopped();
  if (after_stop_) {
    auto action = std::move(after_stop_);
    after_stop_ = nullptr;
    action();
  }
}

// The message pane lines of RUN.CPP runprog()/exec_inst() and UTILS.CPP.
void SimSession::ReportStop(Command /*command*/, SimResult result) {
  const auto& cpu = sim_->State();
  if (cpu.stopped && ExceptionsEnabled()) {
    if (!stop_reported_) {
      stop_reported_ = true;
      emit Message(QStringLiteral("STOP instruction. Execution halted"));
    }
    return;
  }
  if (cpu.halted) {
    const uint32_t pc = observer_->halt_pc;
    const SimResult r = observer_->halt_result != SimResult::kOk ? observer_->halt_result : result;
    if (r == SimResult::kAddressError || r == SimResult::kBusError) {
      const auto& fault = sim_->GetMemory().GetFault();
      emit Message(QString::asprintf(r == SimResult::kAddressError
                                         ? "Address Error: Instruction at %4x accessing address %4x"
                                         : "Bus Error: Instruction at %4x accessing address %4x",
                                     pc, fault.address));
      emit Message(QStringLiteral("Execution halted"));
    } else if (const QString text = HaltMessage(r, pc); !text.isEmpty()) {
      emit Message(text);
    }
    return;
  }
  stop_reported_ = false;
  if (sim_->HasBreakpoint(cpu.pc) || sim_->AdvancedBreakpointHit())
    emit Message(QString::asprintf("break point at %04x", cpu.pc));
}

void SimSession::SetState(RunState state) {
  if (state_ == state)
    return;
  state_ = state;
  emit StateChanged(state);
}

// hardwareu.cpp IRQprocess(): with the hardware enabled the level is
// requested; a program not being traced (or stopped by STOP) runs.
void SimSession::Interrupt(int level) {
  if (!hardware_enabled_ || state_ == RunState::kNoProgram || state_ == RunState::kHalted)
    return;
  sim_->Interrupt(level);
  if (state_ == RunState::kStopped) {
    // After STOP the request waits while every pending level is masked.
    const int mask = sim_->State().GetInterruptMask();
    const int int_mask = (0xFF80 >> (7 - mask)) | 0x40;
    if ((sim_->GetPendingIrqs() & int_mask) == 0)
      return;
  }
  RunOnSimulator([](sim::M68kSimulator& sim) { sim.State().cycles += 44; });  // interrupt time
  if (state_ == RunState::kRunning || trace_)
    return;
  Start(Command::kResume);
}

void SimSession::HardwareReset() {
  if (!hardware_enabled_ || state_ == RunState::kNoProgram || state_ == RunState::kHalted)
    return;
  RunOnSimulator([](sim::M68kSimulator& sim) {
    sim.HardwareReset();
    sim.State().cycles += 44;  // IRQprocess(0)
  });
  if (IsRunning())
    return;
  if (state_ == RunState::kStopped)
    SetState(RunState::kReady);
  emit ExecutionStopped();  // scrshow()
  if (!trace_)
    Start(Command::kResume);
}

void SimSession::SetHardwareEnabledFlag(bool enabled) {
  if (hardware_enabled_ == enabled)
    return;
  hardware_enabled_ = enabled;
  emit HardwareEnabledChanged(enabled);
}

void SimSession::SetHardwareAddresses(uint32_t seg7, uint32_t leds, uint32_t switches,
                                      uint32_t buttons) {
  QtSimulatorEnv::HardwareAddresses a;
  a.seg7 = seg7;
  a.leds = leds;
  a.switches = switches;
  a.buttons = buttons;
  env_->SetHardwareAddresses(a);
  RunOnSimulator([a](sim::M68kSimulator& sim) {
    sim.SetHardwareAddresses(a.seg7, a.leds, a.switches, a.buttons);
  });
}

void SimSession::SetMemoryMap(const sim::M68kSimulator::MemoryMapConfig& map) {
  memory_map_ = map;
  RunOnSimulator([map](sim::M68kSimulator& sim) { sim.ConfigureMemoryMap(map); });
}

// SIMOPS2.CPP loadSrec(): after the EASy68K header record, S0 records
// "      ROM", "     READ", "PROTECTED" or "  INVALID" with "ssssss,eeeeee"
// enable that range of the memory map; other ranges are kept.
void SimSession::ApplyS0MemoryMap(const std::vector<std::string>& s0_texts) {
  static const QString kHeader = QStringLiteral("S0 = 68KPROG   20CREATED BY EASY68K");
  bool easy68k = false;
  bool changed = false;
  sim::M68kSimulator::MemoryMapConfig map = memory_map_;
  for (const std::string& text : s0_texts) {
    const QString str = QString::fromLatin1(text.c_str());
    if (!easy68k) {
      easy68k = str == kHeader;
      continue;
    }
    bool ok_start = false, ok_end = false;
    const uint32_t start = str.mid(15, 6).toUInt(&ok_start, 16);
    const uint32_t end = str.mid(22, 6).toUInt(&ok_end, 16);
    if (!ok_start || !ok_end)
      continue;
    const QString kind = str.mid(5, 9);
    sim::M68kSimulator::MemoryMapConfig::Range* range = nullptr;
    if (kind == QLatin1String("      ROM"))
      range = &map.rom;
    else if (kind == QLatin1String("     READ"))
      range = &map.read;
    else if (kind == QLatin1String("PROTECTED"))
      range = &map.protected_;
    else if (kind == QLatin1String("  INVALID"))
      range = &map.invalid;
    if (range == nullptr)
      continue;
    *range = {true, start, end};
    changed = true;
  }
  if (!changed)
    return;
  SetMemoryMap(map);
  emit MemoryMapChanged();
}

// ---------------------------------------------------------------------------
// PC breakpoints
// ---------------------------------------------------------------------------

bool SimSession::HasBreakpoint(uint32_t address) const {
  return breakpoints_.count(address & sim::kAddressMask) != 0;
}

std::vector<uint32_t> SimSession::Breakpoints() const {
  return std::vector<uint32_t>(breakpoints_.begin(), breakpoints_.end());
}

void SimSession::SetBreakpoint(uint32_t address, bool enabled) {
  address &= sim::kAddressMask;
  if (enabled == HasBreakpoint(address))
    return;
  if (enabled)
    breakpoints_.insert(address);
  else
    breakpoints_.erase(address);
  QueueBreakpoint(address, enabled, false);
  emit BreakpointsChanged();
}

void SimSession::ToggleBreakpoint(uint32_t address) {
  SetBreakpoint(address, !HasBreakpoint(address));
}

void SimSession::ClearBreakpoints() {
  breakpoints_.clear();
  QueueBreakpoint(0, false, true);
  emit BreakpointsChanged();
}

void SimSession::QueueBreakpoint(uint32_t address, bool enabled, bool clear_all) {
  {
    std::lock_guard<std::mutex> lock(bp_mu_);
    bp_queue_.push_back({address, enabled, clear_all});
  }
  bp_pending_.store(true, std::memory_order_release);
  if (!IsRunning())
    ApplyBreakpointQueue();
}

void SimSession::ApplyBreakpointQueue() {
  std::vector<BpChange> changes;
  {
    std::lock_guard<std::mutex> lock(bp_mu_);
    changes.swap(bp_queue_);
    bp_pending_.store(false, std::memory_order_release);
  }
  for (const BpChange& c : changes) {
    if (c.clear_all)
      sim_->ClearBreakpoints();
    else if (c.enabled)
      sim_->AddBreakpoint(c.address);
    else
      sim_->RemoveBreakpoint(c.address);
  }
}

// ---------------------------------------------------------------------------
// Memory snapshot
// ---------------------------------------------------------------------------

void SimSession::RequestMemorySnapshot(uint32_t address, uint32_t size) {
  const std::pair<uint32_t, uint32_t> request(address & sim::kAddressMask,
                                              std::min<uint32_t>(size, sim::kMemorySize));
  {
    std::lock_guard<std::mutex> lock(snap_mu_);
    if (std::find(snap_requests_.begin(), snap_requests_.end(), request) == snap_requests_.end())
      snap_requests_.push_back(request);
  }
  snap_pending_.store(true, std::memory_order_release);
  if (!IsRunning())
    TakeSnapshot();
  else
    NotifyWorker();
}

void SimSession::TakeSnapshot() {
  std::vector<std::pair<uint32_t, uint32_t>> requests;
  {
    std::lock_guard<std::mutex> lock(snap_mu_);
    requests.swap(snap_requests_);
    snap_pending_.store(false, std::memory_order_release);
  }
  const uint8_t* m = sim_->GetMemory().RawData();
  for (const auto& [address, size] : requests) {
    QByteArray bytes(static_cast<qsizetype>(size), '\0');
    for (uint32_t i = 0; i < size; i++)
      bytes[static_cast<qsizetype>(i)] = static_cast<char>(m[(address + i) & sim::kAddressMask]);
    QMetaObject::invokeMethod(
        this, [this, address = address, bytes] { emit MemorySnapshot(address, bytes); },
        Qt::QueuedConnection);
  }
}

// ---------------------------------------------------------------------------
// Simulator tasks
// ---------------------------------------------------------------------------

void SimSession::RunOnSimulator(std::function<void(sim::M68kSimulator&)> fn) {
  {
    std::lock_guard<std::mutex> lock(task_mu_);
    tasks_.push_back(std::move(fn));
  }
  tasks_pending_.store(true, std::memory_order_release);
  if (!IsRunning())
    RunTasks();
  else
    NotifyWorker();
}

void SimSession::RunTasks() {
  std::vector<std::function<void(sim::M68kSimulator&)>> tasks;
  {
    std::lock_guard<std::mutex> lock(task_mu_);
    tasks.swap(tasks_);
    tasks_pending_.store(false, std::memory_order_release);
  }
  for (auto& task : tasks)
    task(*sim_);
}

void SimSession::ServiceWorker() {
  if (bp_pending_.load(std::memory_order_acquire))
    ApplyBreakpointQueue();
  if (tasks_pending_.load(std::memory_order_acquire))
    RunTasks();
  if (snap_pending_.load(std::memory_order_acquire))
    TakeSnapshot();
}

void SimSession::NotifyWorker() {
  screen_->Poke();
  env_->Poke();
}

// ---------------------------------------------------------------------------
// Options
// ---------------------------------------------------------------------------

void SimSession::SetExceptionsEnabled(bool enabled) {
  sim_->SetExceptionsEnabled(enabled);
}
bool SimSession::ExceptionsEnabled() const {
  return sim_->IsExceptionsEnabled();
}
void SimSession::SetBitFieldEnabled(bool enabled) {
  sim_->SetBitFieldEnabled(enabled);
}
bool SimSession::BitFieldEnabled() const {
  return sim_->IsBitFieldEnabled();
}

}  // namespace easym68k::gui
