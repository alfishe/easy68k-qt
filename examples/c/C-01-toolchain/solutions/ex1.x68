*-----------------------------------------------------------
* Solution C-01-EX1: crt0 .bss clear + main return 123
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            CLR.L   g_zero
            MOVE.L  g_start,D0
            ADD.L   g_zero,D0
            ADD.L   #23,D0
            SIMHALT
            ORG     $2000
g_start:    DC.L    100
g_zero:     DS.L    1
            END     _start
