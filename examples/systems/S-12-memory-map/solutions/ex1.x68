*-----------------------------------------------------------
* Solution S-12-EX1: Catch write to MEMORY READ range at $2100
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            MEMORY  READ $2100,$21FF
            ORG     $1000
START:
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15
            LEA     BUS_ISR(PC),A0
            MOVE.L  A0,$0008
            LEA     $2100,A0
            MOVE.W  #1,(A0)
            SIMHALT

BUS_ISR:    ADDQ.L  #2,A7
            MOVE.L  (A7)+,D0
            ADDQ.L  #2,A7
            RTE
            END     START
