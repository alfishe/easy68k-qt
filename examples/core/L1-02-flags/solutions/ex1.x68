* L1-02 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #0,D0
        MOVEQ   #0,D1
        SUBQ.B  #1,D0
        MOVE.W  SR,D1
        ANDI.W  #$001F,D1
        SIMHALT
        END     START
