*-----------------------------------------------------------
* Solution O-09-EX1: Branchless (D1 < 100) -> D0 = 1 in <= 32 cycles
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #40,D1
            MOVEQ   #0,D0
            CMPI.W  #100,D1
            SLT     D0
            NEG.B   D0
            SIMHALT
            END     START
