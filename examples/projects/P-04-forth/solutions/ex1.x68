*-----------------------------------------------------------
* Solution P-04-EX1: Implement W_OVER ( a b -- a b a ) and compute 6 10 OVER * + = 66
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
NEXT        MACRO
            MOVEA.L (A4)+,A0
            JMP     (A0)
            ENDM

            ORG     $1000
START:
            LEA     $4000,A5
            LEA     PROG(PC),A4
            NEXT

W_LIT:      MOVE.L  (A4)+,-(A5)
            NEXT

W_OVER:     MOVE.L  4(A5),-(A5)         ; Copy second item (a) to top of stack
            NEXT

W_MUL:      MOVE.L  (A5)+,D0
            MOVE.L  (A5),D1
            MULS.W  D0,D1
            MOVE.L  D1,(A5)
            NEXT

W_ADD:      MOVE.L  (A5)+,D0
            ADD.L   D0,(A5)
            NEXT

W_HALT:     MOVE.L  (A5)+,D0
            SIMHALT

            DS.W    0
PROG:       DC.L    W_LIT, 6, W_LIT, 10, W_OVER, W_MUL, W_ADD, W_HALT
            END     START
