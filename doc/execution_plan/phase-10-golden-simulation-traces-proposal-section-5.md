## Phase 10: Golden Simulation Traces (Proposal Section 5)

### Task 10.1 — Implement Golden Trace Comparison

New file: `tests/sim/golden_trace_test.cc`

Load a golden execution trace (JSON) and compare step-by-step:

```cpp
TEST_P(GoldenTraceTest, ExecutionTraceMatches) {
  LoadSRecordIntoSimulator(GetParam().s68_path);
  for (const auto& expected : GetParam().trace_steps) {
    SimResult result = sim.Step();
    ASSERT_EQ(result, SimResult::kOk);
    EXPECT_EQ(sim.State().pc, expected.pc);
    EXPECT_EQ(sim.State().d, expected.d);
    EXPECT_EQ(sim.State().a, expected.a);
    EXPECT_EQ(sim.State().sr, expected.sr);
    // Verify memory writes
    for (const auto& write : expected.memory_writes) {
      auto actual = sim.GetMemory().Read(write.address, write.size);
      EXPECT_EQ(actual.value, write.value);
    }
  }
}
```

**Quality Gate 10.1:** All 10+ golden simulation trace tests pass with 100% register and memory parity.

---
