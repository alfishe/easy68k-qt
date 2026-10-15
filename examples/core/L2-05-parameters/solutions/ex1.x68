* L2-05 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVE.L  #$DEADBEEF,D2
        MOVEQ   #6,D0
        MOVEQ   #9,D1
        BSR.S   SUM_DOUBLE
        SIMHALT

SUM_DOUBLE:
        MOVE.L  D2,-(SP)
        MOVE.L  D0,D2
        ADD.L   D1,D2
        ADD.L   D2,D2
        MOVE.L  D2,D0
        MOVE.L  (SP)+,D2
        RTS
        END     START
