*-----------------------------------------------------------
* Solution O-02-EX1: Compute D0 = (100 + 8) + (7 - 3) = 112 in <= 36 cycles
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #100,D0
            MOVEQ   #7,D1
            ADDQ.L  #8,D0
            SUBQ.L  #3,D1
            ADD.L   D1,D0
            SIMHALT
            END     START
