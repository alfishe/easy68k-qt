#!/usr/bin/env python3
# Copyright 2024 EASy68K Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Instruction matrix golden data from the ORIGINAL EASy68K assembler.

Generates instruction x addressing-mode x size cases and assembles them with
the reference oracle (easy68k-ref-asm). Every case is one source line
assembled at $1000 with three symbols in scope: BK (defined before the case:
a backward reference), RL (a REG register list) and FW (defined after the
case: a forward reference). The
oracle assembles all cases of one mnemonic in a single file, each preceded by
"ORG $1000"; the expected object bytes and diagnostic of every line come from
its listing and messages.

Output: <out-dir>/<MNEMONIC>.tsv, one case per line:
  <source>\t<hex object bytes>\t<diagnostic>
where <diagnostic> is "-" or "ERROR: ..." / "WARNING: ..." (the original
reports at most one message, the most severe, per line).

Usage: gen_instruction_matrix.py <easy68k-ref-asm> <out-dir>
"""

import os
import re
import subprocess
import sys
import tempfile

# Mnemonics assembled through the original's flavor table (parseFlag true),
# including aliases (BHS/BLO/DBRA/SHS/...) so the lookup is exercised too.
MNEMONICS = """
ABCD ADD ADDA ADDI ADDQ ADDX AND ANDI ASL ASR
BCC BCHG BCLR BCS BEQ BFCHG BFCLR BFEXTS BFEXTU BFFFO BFINS BFSET BFTST
BGE BGT BHI BHS BLE BLO BLS BLT BMI BNE BPL BRA BSET BSR BTST BVC BVS
CHK CLR CMP CMPA CMPI CMPM
DBCC DBCS DBEQ DBF DBGE DBGT DBHI DBHS DBLE DBLO DBLS DBLT DBMI DBNE DBPL
DBRA DBT DBVC DBVS DIVS DIVU EOR EORI EXG EXT ILLEGAL JMP JSR LEA LINK
LSL LSR MOVE MOVEA MOVEP MOVEQ MULS MULU NBCD NEG NEGX NOP NOT OR ORI PEA
RESET ROL ROR ROXL ROXR RTE RTR RTS SBCD SCC SCS SEQ SF SGE SGT SHI SHS
SLE SLO SLS SLT SMI SNE SPL ST STOP SUB SUBA SUBI SUBQ SUBX SVC SVS SWAP
TAS TRAP TRAPV TST UNLK MOVEM
""".split()

MOVEM_LISTS = ["D0", "A6", "D0-D7", "D0/A0", "D0-D3/A0-A2", "A0-A7/D0-D7",
               "D7-D0", "SP", "D1/D1", "RL", "BK", "UNDEF"]
MOVEM_EAS = ["D1", "A2", "(A3)", "(A4)+", "-(A5)", "$10(A6)",
             "$10(A6,D7.W)", "$1234", "$12345678", "FW", "*+$10(PC)",
             "*(PC,D0.W)", "#1", "SR"]

SIZES = ["", ".B", ".W", ".L", ".S"]

# One representative operand per addressing mode, plus value classes that
# change the encoding (quick/byte/word/long immediates, short/long absolute,
# backward/forward symbols).
FULL_OPERANDS = [
    "D1", "A2", "SP", "(A3)", "(A4)+", "-(A5)", "$10(A6)", "-2(A0)",
    "$10(A6,D7.W)", "$12(A0,A1.L)", "(A2,D3)", "$1234", "$12345678",
    "$1234.L", "$1234.W", "$8000", "BK", "FW", "*+$10(PC)", "*(PC,D0.W)", "BK(PC)",
    "#1", "#8", "#9", "#0", "#-1", "#$80", "#$1234", "#$12345678",
    "#$12345678.L", "#BK", "#FW", "#UNDEF", "UNDEF", "SR", "CCR", "USP",
]
# Operand product for two-operand instructions (unsized and .W).
PAIR_OPERANDS = [
    "D1", "A2", "(A3)", "(A4)+", "-(A5)", "$10(A6)", "$10(A6,D7.W)",
    "$1234", "$12345678", "FW", "*+$10(PC)", "#1", "#$80", "#$1234",
    "#$12345678", "SR", "CCR", "USP",
]
# Smaller product for the other size suffixes.
SMALL_PAIR_OPERANDS = ["D1", "A2", "(A3)", "$10(A6)", "$1234", "FW", "#1",
                       "#$1234", "CCR", "SR"]

BRANCH_TARGETS = ["*+2", "*+4", "*+$80", "*+$82", "*-$7E", "*-$80",
                  "*+$8000", "*", "BK", "FW", "$1000", "$12345678", "D1",
                  "(A0)", "#4"]
BITFIELD_EAS = ["D1", "(A3)", "$10(A6)", "$10(A6,D7.W)", "$1234", "FW",
                "*+$10(PC)", "(A4)+", "-(A5)", "A2", "#1"]
BITFIELD_FIELDS = ["{0:8}", "{31:32}", "{D2:D3}", "{4:D5}", "{D6:1}",
                   "{32:1}", "{0:0}", "{BK:1}"]

# Instructions without a destination operand: their operand product is
# reduced to the small set (only to check that a second operand is refused).
NO_DEST = {"ILLEGAL", "NOP", "RESET", "RTE", "RTR", "RTS", "TRAPV", "CLR",
           "JMP", "JSR", "NBCD", "NEG", "NEGX", "NOT", "PEA", "SWAP", "TAS",
           "TRAP", "TST", "UNLK", "EXT", "STOP", "SCC", "SCS", "SEQ", "SF",
           "SGE", "SGT", "SHI", "SHS", "SLE", "SLO", "SLS", "SLT", "SMI",
           "SNE", "SPL", "ST", "SVC", "SVS"}

BRANCHES = {"BCC", "BCS", "BEQ", "BGE", "BGT", "BHI", "BHS", "BLE", "BLO",
            "BLS", "BLT", "BMI", "BNE", "BPL", "BRA", "BSR", "BVC", "BVS"}


def cases_for(mnemonic):
    out = []
    if mnemonic == "MOVEM":
        for size in SIZES:
            m = mnemonic + size
            out.append(m)
            for rl in MOVEM_LISTS:
                out.append(f"{m} {rl}")
                for ea in MOVEM_EAS:
                    out.append(f"{m} {rl},{ea}")
                    out.append(f"{m} {ea},{rl}")
        return out
    if mnemonic.startswith("BF"):
        for size in ["", ".B", ".L", ".S"]:
            for ea in BITFIELD_EAS:
                for fld in BITFIELD_FIELDS:
                    if mnemonic in ("BFEXTS", "BFEXTU", "BFFFO"):
                        out.append(f"{mnemonic}{size} {ea}{fld},D4")
                    elif mnemonic == "BFINS":
                        out.append(f"{mnemonic}{size} D4,{ea}{fld}")
                    else:
                        out.append(f"{mnemonic}{size} {ea}{fld}")
            out.append(f"{mnemonic}{size} D1")
        return out
    for size in SIZES:
        m = mnemonic + size
        out.append(m)  # no operands
        if mnemonic in BRANCHES:
            out.extend(f"{m} {t}" for t in BRANCH_TARGETS)
            continue
        if mnemonic.startswith("DB"):
            out.extend(f"{m} D2,{t}" for t in BRANCH_TARGETS)
            out.append(f"{m} A2,*")
            out.append(f"{m} #1,*")
            continue
        out.extend(f"{m} {a}" for a in FULL_OPERANDS)
        pair = PAIR_OPERANDS if size in ("", ".W") else SMALL_PAIR_OPERANDS
        if mnemonic in NO_DEST:
            pair = SMALL_PAIR_OPERANDS if size == "" else []
        for a in pair:
            for b in pair:
                out.append(f"{m} {a},{b}")
        # Every remaining operand once as source and once as destination.
        for a in FULL_OPERANDS if pair else []:
            if a not in pair:
                out.append(f"{m} {a},D1")
                out.append(f"{m} {a},A2")
                out.append(f"{m} D1,{a}")
                out.append(f"{m} #$12,{a}")
        out.append(f"{m} D1,D2,D3")  # too many operands
    out.append(f"{mnemonic}.X D1,D2")  # invalid size code
    out.append(f"{mnemonic}.X #1")
    return out


LIST_RE = re.compile(r"^([0-9A-F]{8})  ((?:[0-9A-F]{4,8} )*)\s*(\d+)\s")
DIAG_RE = re.compile(r"^line (\d+): (.*?)\s*$")


def run_oracle(oracle, cases):
    """Returns [(hex, diagnostic)] per case."""
    lines = ["BK EQU $1F00", "RL REG D2-D4/A3"]
    case_line = {}
    for i, c in enumerate(cases):
        lines.append(" ORG $1000")
        lines.append(" " + c)
        case_line[len(lines)] = i  # 1-based line number of the case
    lines.append("FW EQU $1F00")
    lines.append(" END $1000")
    with tempfile.TemporaryDirectory() as d:
        src = os.path.join(d, "m.x68")
        with open(src, "w") as f:
            f.write("\r\n".join(lines) + "\r\n")
        p = subprocess.run([oracle, src], capture_output=True, text=True,
                           errors="replace")
        diags = {}
        for ln in (p.stderr + p.stdout).splitlines():
            m = DIAG_RE.match(ln)
            if m:
                diags[int(m.group(1))] = re.sub(r"\s+\S*m\.x68$", "",
                                                m.group(2))
        hexes = {}
        with open(os.path.join(d, "m.L68"), errors="replace") as f:
            for ln in f:
                m = LIST_RE.match(ln)
                if m:
                    hexes[int(m.group(3))] = m.group(2).replace(" ", "")
    res = [None] * len(cases)
    for line_no, i in case_line.items():
        res[i] = (hexes.get(line_no, ""), diags.get(line_no, "-"))
    return res


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    oracle, out_dir = sys.argv[1], sys.argv[2]
    os.makedirs(out_dir, exist_ok=True)
    total = 0
    for mn in MNEMONICS:
        cases = cases_for(mn)
        res = run_oracle(oracle, cases)
        with open(os.path.join(out_dir, mn + ".tsv"), "w") as f:
            for c, (hx, dg) in zip(cases, res):
                f.write(f"{c}\t{hx}\t{dg}\n")
        total += len(cases)
    print(f"{total} cases in {len(MNEMONICS)} files")


if __name__ == "__main__":
    main()
