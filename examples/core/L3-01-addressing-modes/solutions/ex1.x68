* L3-01 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        LEA     LONGS,A0
        MOVEQ   #8,D1
        MOVE.L  0(A0,D1.W),D0
        SIMHALT

LONGS   DC.L    100,200,300,400
        END     START
