* L3-10 Exercise 1: Load PT_Z field from POINT_DATA into D0.L
* SPDX-License-Identifier: CC0-1.0
        INCLUDE 'defs.inc'
        ORG     $1000
START:
        LEA     POINT_DATA,A0
        * TODO: load PT_Z(A0) into D0.L
        SIMHALT

POINT_DATA:
        INCBIN  'payload.bin'
        END     START
