* L0-06 Exercise 1: Call ADD_TEN three times so D0 = D1 = 35
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #5,D0
        BSR.S   ADD_TEN
        BSR.S   ADD_TEN
        * TODO: call ADD_TEN a third time
        MOVE.L  D0,D1
        SIMHALT

ADD_TEN:
        ADDQ.L  #8,D0
        ADDQ.L  #2,D0
        RTS
        END     START
