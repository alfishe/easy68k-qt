* L3-06 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        LEA     NODE1,A0
        MOVE.L  #NODE3,4(A0)
        MOVEQ   #0,D0
.LOOP:
        CMPA.L  #0,A0
        BEQ.S   .DONE
        ADD.L   0(A0),D0
        MOVEA.L 4(A0),A0
        BRA.S   .LOOP
.DONE:
        SIMHALT

NODE1   DC.L    10,NODE2
NODE2   DC.L    20,NODE3
NODE3   DC.L    30,0
        END     START
