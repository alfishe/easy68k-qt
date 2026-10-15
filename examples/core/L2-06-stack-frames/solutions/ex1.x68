* L2-06 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVE.L  #23,-(SP)
        BSR.S   ADD_100
        ADDQ.L  #4,SP
        SIMHALT

ADD_100:
        LINK    A6,#-4
        MOVE.L  8(A6),D0
        ADDI.L  #100,D0
        UNLK    A6
        RTS
        END     START
