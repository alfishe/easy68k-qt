* L2-07 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #6,D0
        BSR.S   TRI
        SIMHALT

TRI:
        CMP.W   #1,D0
        BLE.S   .BASE
        MOVE.W  D0,-(SP)
        SUBQ.W  #1,D0
        BSR.S   TRI
        MOVE.W  (SP)+,D1
        EXT.L   D1
        ADD.L   D1,D0
        RTS
.BASE:
        EXT.L   D0
        RTS
        END     START
