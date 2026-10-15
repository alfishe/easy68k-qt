* L3-07 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        LEA     $2100,A2
        MOVE.L  #100,-(A2)
        MOVE.L  #200,-(A2)
        MOVE.L  (A2)+,D0
        MOVE.L  (A2)+,D1
        SIMHALT
        END     START
