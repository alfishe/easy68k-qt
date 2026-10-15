* L1-05 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #50,D0
        DIVU.W  #6,D0
        MOVE.L  D0,D1
        ANDI.L  #$0000FFFF,D0
        SWAP    D1
        ANDI.L  #$0000FFFF,D1
        SIMHALT
        END     START
