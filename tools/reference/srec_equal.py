#!/usr/bin/env python3
# Copyright 2024 EASy68K Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""srec_equal.py A.S68 B.S68: exit 0 if both load the same memory image.

Bytes equal to $FF are ignored (Sim68K erases memory to $FF before loading)
and the S0 header and S8/S9/S7 end records must match. Used where the port
writes SIMHALT's second $FFFF word, which the original drops (accepted
deviation), so the record text differs but the program does not.
"""
import sys


def load(path):
    image, header, end = {}, None, None
    for line in open(path, errors="replace"):
        line = line.strip()
        if line.startswith("S0"):
            header = line
        elif line[:2] in ("S7", "S8", "S9"):
            end = line
        elif line[:2] in ("S1", "S2", "S3"):
            alen = 2 * (int(line[1]) + 1)
            count = int(line[2:4], 16)
            addr = int(line[4:4 + alen], 16)
            for i in range(count - alen // 2 - 1):
                b = int(line[4 + alen + 2 * i:6 + alen + 2 * i], 16)
                if b == 0xFF:
                    image.pop(addr + i, None)
                else:
                    image[addr + i] = b
    return image, header, end


sys.exit(0 if load(sys.argv[1]) == load(sys.argv[2]) else 1)
