* L3-05 Exercise 1: Linear search for 26 in SORTED -> D0 = 3
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        LEA     SORTED,A0
        MOVEQ   #0,D0
        * TODO: scan SORTED until word == 26, incrementing D0 each step
        SIMHALT

SORTED  DC.W    5,12,19,26,33,47,58,71
        END     START
