* L2-05 Exercise 1: Compute D0 = (D0 + D1) * 2 while preserving D2
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVE.L  #$DEADBEEF,D2
        MOVEQ   #6,D0
        MOVEQ   #9,D1
        BSR.S   SUM_DOUBLE
        SIMHALT

SUM_DOUBLE:
        * TODO: save D2, compute D0 = (D0 + D1) * 2 using D2, restore D2
        RTS
        END     START
