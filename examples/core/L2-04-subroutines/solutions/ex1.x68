* L2-04 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #4,D0
        BSR.S   CUBE
        SIMHALT

CUBE:
        MOVE.W  D0,D1
        MULS.W  D1,D0
        MULS.W  D1,D0
        RTS
        END     START
