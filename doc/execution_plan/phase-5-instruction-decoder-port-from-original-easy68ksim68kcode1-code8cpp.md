## Phase 5: Instruction Decoder — Port from Original (EASy68K/Sim68K/CODE1-CODE8.CPP)

### Task 5.1 — Implement Hierarchical Decoder

New file: `src/sim/decode.cc`

Primary source: `EASy68K/Sim68K/CODE1.CPP`–`CODE8.CPP` — the original instruction dispatch functions (`line0()` through `lineF()`) which use bit-field switch/case dispatch.
Secondary reference: `EASy68K-qt/src/core/simulator/instruction_decoder.cc` for the complete list of mask/value/handler entries. Do NOT copy the linear search pattern.

Implement two-level dispatch:

```cpp
SimResult M68kSimulator::ExecuteInstruction(uint16_t opcode) {
  int group = (opcode >> 12) & 0xF;

  switch (group) {
    case 0x0: return DispatchGroup0(opcode);  // ORI/ANDI/SUBI/ADDI/EORI/CMPI/BTST/BCHG/BCLR/BSET/MOVEP
    case 0x1: return OpMove(opcode);          // MOVE.B
    case 0x2: return DispatchMoveOrMovea(opcode); // MOVE.L / MOVEA.L
    case 0x3: return DispatchMoveOrMovea(opcode); // MOVE.W / MOVEA.W
    case 0x4: return DispatchGroup4(opcode);  // CHK/LEA/CLR/NEG/NEGX/NOT/TST/EXT/NBCD/SWAP/PEA/MOVEM/JSR/JMP/...
    case 0x5: return DispatchGroup5(opcode);  // ADDQ/SUBQ/Scc/DBcc
    case 0x6: return DispatchGroup6(opcode);  // BRA/BSR/Bcc
    case 0x7: return OpMoveq(opcode);         // MOVEQ
    case 0x8: return DispatchGroup8(opcode);  // OR/DIVU/DIVS/SBCD
    case 0x9: return DispatchGroup9(opcode);  // SUB/SUBA/SUBX
    case 0xA: return OpLine1010(opcode);      // Line A
    case 0xB: return DispatchGroupB(opcode);  // CMP/CMPA/CMPM/EOR
    case 0xC: return DispatchGroupC(opcode);  // AND/MULU/MULS/ABCD/EXG
    case 0xD: return DispatchGroupD(opcode);  // ADD/ADDA/ADDX
    case 0xE: return DispatchGroupE(opcode);  // ASd/LSd/ROXd/ROd
    case 0xF: return DispatchSimhalt(opcode); // SIMHALT or Line F
  }
  return SimResult::kBadInstruction;
}
```

Each `DispatchGroupN` function contains a secondary switch or if-chain for sub-opcodes within that group, checking specific bit patterns.

**Special case — SIMHALT:** Opcode `0xFFFF` matches group 0xF. Check `opcode == 0xFFFF` first → return `SimResult::kHalted`. Any other Line-F opcode → `SimResult::kLine1111`.

**Quality Gate 5.1:**
- `decode_test.cc` passes: every opcode in the EASy68K-qt instruction table maps to the correct handler
- SIMHALT (`0xFFFF`) returns `SimResult::kHalted`
- Unknown opcodes return `SimResult::kBadInstruction`
- Decoder is O(1) — no linear search through a table

---

### Task 5.2 — Decoder Tests

New file: `tests/sim/decode_test.cc`

1. `NopDispatches` — opcode `0x4E71` → handler for NOP
2. `RtsDispatches` — opcode `0x4E75` → handler for RTS
3. `MoveqDispatches` — opcode `0x7042` → handler for MOVEQ
4. `SimhaltDispatches` — opcode `0xFFFF` → returns `SimResult::kHalted`
5. `IllegalDispatches` — opcode `0x4AFC` → handler for ILLEGAL
6. `LineADispatches` — opcode `0xA000` → returns `SimResult::kLine1010`
7. `LineFDispatches` — opcode `0xF000` (not SIMHALT) → returns `SimResult::kLine1111`
8. `OriToCcr` — opcode `0x003C` dispatches correctly (not confused with generic ORI)
9. `ExgDnDn` — opcode `0xC140` dispatches correctly
10. `ExgAnAn` — opcode `0xC148` dispatches correctly
11. `MovemRegister` — opcode `0x4880` dispatches correctly
12. `BtstDynamic` — opcode `0x0100` dispatches correctly (not confused with ORI)

**Quality Gate 5.2:** All 12 decode tests pass.

---
