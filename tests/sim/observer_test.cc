// Copyright 2024 EASy68K Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Event/observer tests (Task 9.5).

#include <gtest/gtest.h>

#include <cstdint>
#include <initializer_list>
#include <thread>
#include <vector>

#include "easym68k/sim/isim_observer.h"
#include "easym68k/sim/simulator.h"

namespace easym68k::sim {
namespace {

constexpr uint32_t kCode = 0x1000;
constexpr uint32_t kStack = 0x8000;
constexpr uint32_t kHandler = 0x3000;

class RecordingObserver : public ISimObserver {
 public:
  void OnEvent(const SimEvent& e) override { events.push_back(e); }

  int Count(SimEventType t) const {
    int n = 0;
    for (const auto& e : events)
      n += (e.type == t) ? 1 : 0;
    return n;
  }
  const SimEvent* First(SimEventType t) const {
    for (const auto& e : events)
      if (e.type == t)
        return &e;
    return nullptr;
  }

  std::vector<SimEvent> events;
};

class ObserverTest : public ::testing::Test {
 protected:
  ObserverTest() : sim_(M68kSimulator::Config{}) {}

  void Load(std::initializer_list<uint16_t> words) {
    Memory& m = sim_.GetMemory();
    m.Clear();
    m.WriteLong(0x0, kStack);
    m.WriteLong(0x4, kCode);
    uint32_t addr = kCode;
    for (uint16_t w : words) {
      m.WriteWord(addr, w);
      addr += 2;
    }
    sim_.ClearBreakpoints();
    sim_.Reset();
  }

  M68kSimulator sim_;
  RecordingObserver obs_;
};

TEST_F(ObserverTest, AddRemoveObserver) {
  Load({0x4E71, 0x4E71});
  sim_.AddObserver(&obs_);
  sim_.Step();
  sim_.RemoveObserver(&obs_);
  sim_.Step();
  EXPECT_EQ(obs_.Count(SimEventType::kInstructionExecuted), 1);
}

TEST_F(ObserverTest, InstructionExecutedEvent) {
  Load({0x4E71});
  sim_.AddObserver(&obs_);
  sim_.Step();
  ASSERT_EQ(obs_.events.size(), 1u);
  EXPECT_EQ(obs_.events[0].type, SimEventType::kInstructionExecuted);
  EXPECT_EQ(obs_.events[0].result, SimResult::kOk);
}

TEST_F(ObserverTest, EventHasCorrectPc) {
  Load({0x4E71, 0x7005});
  sim_.AddObserver(&obs_);
  sim_.Step();
  sim_.Step();
  ASSERT_EQ(obs_.events.size(), 2u);
  EXPECT_EQ(obs_.events[0].pc, kCode);
  EXPECT_EQ(obs_.events[1].pc, kCode + 2);
}

TEST_F(ObserverTest, BreakpointHitEvent) {
  Load({0x4E71, 0x4E71, 0x4E71});
  sim_.AddBreakpoint(kCode + 4);
  sim_.AddObserver(&obs_);
  sim_.Run();
  const SimEvent* bp = obs_.First(SimEventType::kBreakpointHit);
  ASSERT_NE(bp, nullptr);
  EXPECT_EQ(bp->pc, kCode + 4);
  EXPECT_EQ(obs_.Count(SimEventType::kInstructionExecuted), 2);
}

TEST_F(ObserverTest, ExceptionRaisedEvent) {
  Load({0x4AFC});  // ILLEGAL
  sim_.GetMemory().WriteLong(0x10, kHandler);
  sim_.AddObserver(&obs_);
  sim_.Step();
  ASSERT_EQ(obs_.events.size(), 2u);
  EXPECT_EQ(obs_.events[0].type, SimEventType::kInstructionExecuted);
  EXPECT_EQ(obs_.events[0].result, SimResult::kBadInstruction);
  EXPECT_EQ(obs_.events[1].type, SimEventType::kExceptionRaised);
  EXPECT_EQ(obs_.events[1].pc, kCode);
  EXPECT_EQ(obs_.events[1].vector, static_cast<int>(ExceptionVector::kIllegalInst));
}

TEST_F(ObserverTest, HaltedEvent) {
  Load({0xFFFF, 0xFFFF});  // SIMHALT
  sim_.AddObserver(&obs_);
  sim_.Step();
  const SimEvent* h = obs_.First(SimEventType::kHalted);
  ASSERT_NE(h, nullptr);
  EXPECT_EQ(h->pc, kCode);
  EXPECT_EQ(h->result, SimResult::kHalted);
  EXPECT_EQ(obs_.Count(SimEventType::kStopped), 0);
}

TEST_F(ObserverTest, StoppedEvent) {
  Load({0x4E72, 0x2000});  // STOP #$2000
  sim_.AddObserver(&obs_);
  sim_.Step();
  EXPECT_EQ(obs_.Count(SimEventType::kStopped), 1);
  EXPECT_EQ(obs_.Count(SimEventType::kHalted), 0);
}

TEST_F(ObserverTest, ResumedEvent) {
  Load({0x4E72, 0x2000, 0x4E71});  // STOP #$2000 (mask 0)
  sim_.GetMemory().WriteLong((24 + 5) * 4, kHandler);
  sim_.AddObserver(&obs_);
  sim_.Step();
  ASSERT_TRUE(sim_.State().stopped);
  sim_.Interrupt(5);
  sim_.Step();  // delivers kInterruptRequested + kResumed, then services IRQ
  EXPECT_EQ(obs_.Count(SimEventType::kResumed), 1);
  const SimEvent* irq = obs_.First(SimEventType::kExceptionRaised);
  ASSERT_NE(irq, nullptr);
  EXPECT_EQ(irq->interrupt_level, 5);
  EXPECT_EQ(irq->vector, 24 + 5);
}

TEST_F(ObserverTest, InterruptRequestedEvent) {
  Load({0x4E71});
  sim_.State().SetInterruptMask(7);  // masked: requested but not serviced
  sim_.AddObserver(&obs_);
  sim_.Interrupt(5);
  EXPECT_TRUE(obs_.events.empty());  // delivered on the simulator thread
  sim_.Step();
  const SimEvent* req = obs_.First(SimEventType::kInterruptRequested);
  ASSERT_NE(req, nullptr);
  EXPECT_EQ(req->interrupt_level, 5);
  EXPECT_EQ(obs_.Count(SimEventType::kExceptionRaised), 0);
}

TEST_F(ObserverTest, InterruptFromAnotherThreadDeliveredOnSimThread) {
  Load({0x4E71});
  sim_.State().SetInterruptMask(7);
  sim_.AddObserver(&obs_);
  std::thread t([this] { sim_.Interrupt(3); });
  t.join();
  EXPECT_TRUE(obs_.events.empty());
  sim_.Step();
  EXPECT_EQ(obs_.Count(SimEventType::kInterruptRequested), 1);
}

TEST_F(ObserverTest, MultipleObservers) {
  RecordingObserver a, b, c;
  Load({0x4E71});
  sim_.AddObserver(&a);
  sim_.AddObserver(&b);
  sim_.AddObserver(&c);
  sim_.Step();
  EXPECT_EQ(a.events.size(), 1u);
  EXPECT_EQ(b.events.size(), 1u);
  EXPECT_EQ(c.events.size(), 1u);
}

TEST_F(ObserverTest, DuplicateRegistrationIgnored) {
  Load({0x4E71});
  sim_.AddObserver(&obs_);
  sim_.AddObserver(&obs_);
  sim_.Step();
  EXPECT_EQ(obs_.events.size(), 1u);
}

TEST_F(ObserverTest, RemoveObserverDuringRun) {
  Load({0x4E71, 0x4E71, 0x4E71});
  sim_.AddObserver(&obs_);
  sim_.Step();
  sim_.RemoveObserver(&obs_);
  sim_.Step();
  sim_.AddObserver(&obs_);
  sim_.Step();
  EXPECT_EQ(obs_.Count(SimEventType::kInstructionExecuted), 2);
}

// An observer that unregisters itself and another observer from OnEvent().
class SelfRemovingObserver : public ISimObserver {
 public:
  SelfRemovingObserver(M68kSimulator* sim, ISimObserver* other) : sim_(sim), other_(other) {}
  void OnEvent(const SimEvent&) override {
    ++calls;
    sim_->RemoveObserver(this);
    sim_->RemoveObserver(other_);
  }
  int calls = 0;

 private:
  M68kSimulator* sim_;
  ISimObserver* other_;
};

TEST_F(ObserverTest, RemoveObserverDuringIterationIsSafe) {
  Load({0x4E71, 0x4E71});
  RecordingObserver later;
  SelfRemovingObserver remover(&sim_, &later);
  sim_.AddObserver(&remover);
  sim_.AddObserver(&later);  // removed before it is reached
  sim_.Step();
  sim_.Step();
  EXPECT_EQ(remover.calls, 1);
  EXPECT_TRUE(later.events.empty());
}

TEST_F(ObserverTest, NullObserverIgnored) {
  Load({0x4E71});
  sim_.AddObserver(nullptr);
  sim_.RemoveObserver(nullptr);
  EXPECT_EQ(sim_.Step(), SimResult::kOk);
}

TEST_F(ObserverTest, EventsRequestedWithoutObserversAreDropped) {
  Load({0x4E71, 0x4E71});
  sim_.State().SetInterruptMask(7);
  sim_.Interrupt(4);
  sim_.Step();  // drained with no observer registered
  sim_.AddObserver(&obs_);
  sim_.Step();
  EXPECT_EQ(obs_.Count(SimEventType::kInterruptRequested), 0);
}

}  // namespace
}  // namespace easym68k::sim
