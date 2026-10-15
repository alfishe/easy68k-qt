* L1-03 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #0,D2
        MOVEQ   #0,D3
        MOVE.B  #$80,D0
        MOVE.B  #$7F,D1
        CMP.B   D1,D0
        SLT     D2
        SHI     D3
        SIMHALT
        END     START
