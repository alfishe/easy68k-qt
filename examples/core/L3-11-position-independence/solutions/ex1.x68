* L3-11 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #0,D0
        MOVEQ   #2,D1
        MOVE.W  TBL(PC,D1.W),D0
        SIMHALT

TBL     DC.W    150,250,350
        END     START
