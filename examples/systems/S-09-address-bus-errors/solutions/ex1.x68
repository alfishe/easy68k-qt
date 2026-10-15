*-----------------------------------------------------------
* Solution S-09-EX1: Recover from odd longword access at $3003
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15
            LEA     HANDLER(PC),A0
            MOVE.L  A0,$000C
            LEA     $3003,A1
            MOVE.L  (A1),D1
            SIMHALT

HANDLER:    ADDQ.L  #2,A7               ; skip info word
            MOVE.L  (A7)+,D0            ; pop faulting address ($00003003)
            ADDQ.L  #2,A7               ; skip IR word
            RTE
            END     START
