* L1-04 Exercise 1: 64-bit subtraction D0:D1 -= D2:D3
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #5,D0
        MOVEQ   #1,D1
        MOVEQ   #1,D2
        MOVEQ   #2,D3
        * TODO: subtract D2:D3 from D0:D1 using SUB.L and SUBX.L
        SIMHALT
        END     START
