* L2-06 Exercise 1: Read stack arg at 8(A6) using LINK/UNLK and add 100
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVE.L  #23,-(SP)
        BSR.S   ADD_100
        ADDQ.L  #4,SP
        SIMHALT

ADD_100:
        * TODO: LINK A6,#-4, read 8(A6), add 100 into D0, UNLK A6, RTS
        RTS
        END     START
