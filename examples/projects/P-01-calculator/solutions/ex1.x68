*-----------------------------------------------------------
* Solution P-01-EX1: Add 'n' (negate) operator and evaluate "15 3 * n 50 + ="
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     INBUF,A1
            MOVEQ   #2,D0
            TRAP    #15

            LEA     INBUF,A0
            LEA     RPN_STK,A3
.TOKEN_LP:
            MOVE.B  (A0)+,D2
            BEQ.W   .FINISH
            CMP.B   #' ',D2
            BEQ.S   .TOKEN_LP
            CMP.B   #'0',D2
            BLT.S   .CHECK_OP
            CMP.B   #'9',D2
            BGT.S   .CHECK_OP
            MOVEQ   #0,D3
.DIGIT_LP:
            SUB.B   #'0',D2
            EXT.W   D2
            EXT.L   D2
            MULU.W  #10,D3
            ADD.L   D2,D3
            MOVE.B  (A0),D2
            CMP.B   #'0',D2
            BLT.S   .PUSH_NUM
            CMP.B   #'9',D2
            BGT.S   .PUSH_NUM
            ADDQ.L  #1,A0
            BRA.S   .DIGIT_LP
.PUSH_NUM:
            MOVE.L  D3,(A3)+
            BRA.S   .TOKEN_LP
.CHECK_OP:
            CMP.B   #'+',D2
            BEQ.S   .OP_ADD
            CMP.B   #'*',D2
            BEQ.S   .OP_MUL
            CMP.B   #'n',D2
            BEQ.S   .OP_NEG
            CMP.B   #'=',D2
            BEQ.S   .OP_EQ
            BRA.S   .TOKEN_LP
.OP_ADD:
            MOVE.L  -(A3),D4
            ADD.L   D4,-4(A3)
            BRA.S   .TOKEN_LP
.OP_MUL:
            MOVE.L  -(A3),D4
            MOVE.L  -(A3),D5
            MULS.W  D4,D5
            MOVE.L  D5,(A3)+
            BRA.S   .TOKEN_LP
.OP_NEG:
            NEG.L   -4(A3)
            BRA.W   .TOKEN_LP
.OP_EQ:
            MOVE.L  -4(A3),D1
            MOVEQ   #3,D0
            TRAP    #15
            LEA     CRLF(PC),A1
            MOVEQ   #14,D0
            TRAP    #15
            BRA.W   .TOKEN_LP
.FINISH:
            MOVE.L  -4(A3),D0
            SIMHALT
CRLF:       DC.B    13,10,0
            DS.W    0
INBUF:      DS.B    84
            ORG     $3000
RPN_STK:    DS.L    32
            END     START
