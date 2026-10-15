*-----------------------------------------------------------
* Solution S-16-EX1: Measure a single NOP with tasks 30 and 31
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #30,D0
            TRAP    #15
            NOP
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D0
            SIMHALT
            END     START
