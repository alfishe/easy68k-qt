* L0-06 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #5,D0
        BSR.S   ADD_TEN
        BSR.S   ADD_TEN
        BSR.S   ADD_TEN
        MOVE.L  D0,D1
        SIMHALT

ADD_TEN:
        ADDQ.L  #8,D0
        ADDQ.L  #2,D0
        RTS
        END     START
