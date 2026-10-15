* L1-03 Exercise 1: Compare $80 and $7F signed (SLT) and unsigned (SHI)
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #0,D2
        MOVEQ   #0,D3
        MOVE.B  #$80,D0
        MOVE.B  #$7F,D1
        * TODO: CMP.B D1,D0 then SLT D2 and SHI D3
        SIMHALT
        END     START
