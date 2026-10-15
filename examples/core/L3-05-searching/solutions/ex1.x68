* L3-05 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        LEA     SORTED,A0
        MOVEQ   #0,D0
.LOOP:
        CMP.W   #26,(A0)+
        BEQ.S   .DONE
        ADDQ.L  #1,D0
        BRA.S   .LOOP
.DONE:
        SIMHALT

SORTED  DC.W    5,12,19,26,33,47,58,71
        END     START
