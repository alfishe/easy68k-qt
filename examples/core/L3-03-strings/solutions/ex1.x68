* L3-03 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        LEA     STR,A0
        MOVEQ   #0,D0
.LOOP:
        TST.B   (A0)+
        BEQ.S   .DONE
        ADDQ.L  #1,D0
        BRA.S   .LOOP
.DONE:
        SIMHALT

STR     DC.B    'EASy68K',0
        END     START
