#!/usr/bin/env python3
# Copyright 2024 EASy68K Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Block-level comparison of simulator traces for the instruction corpus.

The corpus programs (gen_sim_corpus.py) run one instruction case per fixed
64-byte slot starting at $10100; every slot re-initializes the registers.
This splits the oracle and port traces into slots, replaces absolute cycle
counts with per-slot deltas and reports the cases whose slots differ, so one
difference does not hide the rest of the program.

Usage: compare_sim_blocks.py <listing.L68> <ref.trace> <port.trace> [deviations]

A deviations file lists accepted differences, one per line:

    <case regex> TAB <ignore> TAB <reason>

<case regex> is matched against "<MNEMONIC-FILE>: <instruction text>".
<ignore> is a comma list of what to disregard in a matching case: a register
or field name (CYC, PC, A4, ...), a range (A0-A7), SR=<hex bits>, and
optional conditions: @fault (only cases that took an address/bus error) and
@V (only if the oracle's result has V set). A case is accepted if it matches
once those parts are masked out of both traces. Exit status 1 if any
unexplained difference remains.
"""

import os
import re
import sys

BASE = 0x10100
SLOT = 64


def cases_from_listing(path):
    """Map slot index -> instruction text (the line after B<k>)."""
    cases = {}
    pending = None
    for line in open(path, errors="replace"):
        m = re.search(r"\sB(\d+)\s+MOVE\s", line)
        if m:
            pending = int(m.group(1))
            continue
        if pending is not None and len(line) > 40 and line[:8].strip():
            text = line[40:].strip() if len(line) > 40 else ""
            # listing: address, code, line number, source
            parts = line.split(None, 0)
            src = re.sub(r"^\S+\s+(\S+\s+)*?\d+\s{2,}", "", line.rstrip())
            cases[pending] = src.strip()
            pending = None
    return cases


# Offset of B<k> in a slot: the prologue (MOVE #,SR; MOVE.L #,NEXT; LEA;
# MOVE USP; LEA; MOVEM.L) is 34 bytes. A slot's comparison starts at the state
# right after its MOVEM.L (all registers reloaded), so state carried over from
# the previous case does not count against this one.
B_OFFSET = 0x22


def blocks(trace_path):
    """Map slot index -> list of trace lines (cycle counts relative to B<k>)."""
    out = {}
    cur = -1
    started = False
    base_cyc = 0
    for line in open(trace_path):
        if not line.startswith("STEP"):
            continue
        f = dict(x.split("=", 1) for x in line.split()[1:])
        pc = int(f["PC"], 16)
        cyc = int(f["CYC"])
        if pc >= BASE and (pc - BASE) % SLOT == 0 and (pc - BASE) // SLOT > cur:
            cur = (pc - BASE) // SLOT
            started = False
            continue
        if cur < 0:
            continue
        if not started:
            if pc != BASE + cur * SLOT + B_OFFSET:
                continue
            started = True
            base_cyc = cyc
            out[cur] = []
        f["CYC"] = str(cyc - base_cyc)
        out[cur].append(" ".join(f"{k}={v}" for k, v in f.items()))
    return out


def parse_ignore(spec):
    fields, sr_mask, conds = set(), 0, set()
    for item in filter(None, (x.strip() for x in spec.split(","))):
        if item.startswith("@"):
            conds.add(item[1:])
        elif item.startswith("SR="):
            sr_mask |= int(item[3:], 16)
        elif re.fullmatch(r"([AD])(\d)-[AD](\d)", item):
            m = re.fullmatch(r"([AD])(\d)-[AD](\d)", item)
            fields.update(f"{m.group(1)}{i}" for i in range(int(m.group(2)), int(m.group(3)) + 1))
        else:
            fields.add(item)
    if "A7" in fields:
        fields.update(("US", "SS"))
    return fields, sr_mask, conds


def masked(lines, fields, sr_mask):
    out = []
    for line in lines:
        f = dict(x.split("=", 1) for x in line.split())
        for k in fields:
            f.pop(k, None)
        if sr_mask and "SR" in f:
            f["SR"] = f"{int(f['SR'], 16) & ~sr_mask:04X}"
        out.append(f)
    return out


def faulted(lines):
    # class 0 (address/bus error) frames are 14 bytes below SSTK ($8000)
    return any("SS=00007FF2" in x for x in lines)


def accepted_by(rule, r, p):
    fields, sr_mask, conds = rule
    if "fault" in conds and not faulted(r):
        return False
    if "V" in conds:
        first = next((a for a, b in zip(r, p) if a != b), None)
        if first is None:
            return False
        if not int(dict(x.split("=", 1) for x in first.split())["SR"], 16) & 2:
            return False
    return masked(r, fields, sr_mask) == masked(p, fields, sr_mask)


def main():
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    l68, ref_t, port_t = sys.argv[1:4]
    devs = []
    if len(sys.argv) > 4 and os.path.exists(sys.argv[4]):
        for line in open(sys.argv[4]):
            if line.strip() and not line.startswith(("#", "HASH ")):
                pat, ignore, reason = (line.rstrip("\n").split("\t") + ["", ""])[:3]
                devs.append((re.compile(pat), parse_ignore(ignore), reason))
    name = os.path.splitext(os.path.basename(l68))[0]
    cases = cases_from_listing(l68)
    rb, pb = blocks(ref_t), blocks(port_t)
    bad = 0
    accepted = 0
    for k in sorted(set(rb) | set(pb)):
        r, p = rb.get(k), pb.get(k)
        if r == p:
            continue
        key = f"{name}: {cases.get(k, '?')}"
        if r is not None and p is not None and any(
                d[0].search(key) and accepted_by(d[1], r, p) for d in devs):
            accepted += 1
            continue
        bad += 1
        print(f"DIFF slot {k}: {key}")
        if r is None or p is None:
            print("   missing in", "ref" if r is None else "port")
            continue
        for i in range(max(len(r), len(p))):
            a = r[i] if i < len(r) else "-"
            b = p[i] if i < len(p) else "-"
            if a != b:
                fa, fb = a.split(), b.split()
                print("   ref :", " ".join(x for x, y in zip(fa, fb) if x != y) or a)
                print("   port:", " ".join(y for x, y in zip(fa, fb) if x != y) or b)
                break
    print(f"{name}: {len(set(rb) | set(pb))} cases, {bad} different, {accepted} accepted deviations")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
