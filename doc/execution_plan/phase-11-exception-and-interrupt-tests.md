## Phase 11: Exception and Interrupt Tests

### Task 11.1 — Exception Tests

New file: `tests/sim/exception_test.cc`

Test cases:
1. Bus error on unmapped memory read
2. Address error on odd-address word read
3. Privilege violation (MOVE to SR in user mode)
4. Illegal instruction exception
5. Divide by zero exception
6. CHK exception (Dn < 0, Dn > bound)
7. TRAPV exception (V flag set)
8. Trace exception (T bit set, single-step)
9. Line 1010 exception
10. Line 1111 exception
11. Exception stack frame: verify PC and SR are pushed in correct order on the supervisor stack
12. RTE restores both SR and PC
13. Double bus fault halts the simulator

**Quality Gate 11.1:** All 13 exception tests pass.

---

### Task 11.2 — Interrupt Tests

New file: (part of `exception_test.cc` or separate)

Test cases:
1. IRQ at level 5 with interrupt mask = 3 → exception generated
2. IRQ at level 5 with interrupt mask = 7 → no exception (masked)
3. Autovector interrupt → reads from vector $64+$68
4. Interrupt clears trace flag
5. Stop instruction wakes on pending interrupt

**Quality Gate 11.2:** All 5 interrupt tests pass.

---
