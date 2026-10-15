* L1-04 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #5,D0
        MOVEQ   #1,D1
        MOVEQ   #1,D2
        MOVEQ   #2,D3
        SUB.L   D3,D1
        SUBX.L  D2,D0
        SIMHALT
        END     START
