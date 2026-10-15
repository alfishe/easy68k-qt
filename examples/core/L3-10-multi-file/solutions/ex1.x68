* L3-10 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        INCLUDE 'defs.inc'
        ORG     $1000
START:
        LEA     POINT_DATA,A0
        MOVE.L  PT_Z(A0),D0
        SIMHALT

POINT_DATA:
        INCBIN  'payload.bin'
        END     START
