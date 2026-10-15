*-----------------------------------------------------------
* Title      : O-12 Naive 32-Bit Software Multiply Loop (__mulsi3)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     ARR,A0
            MOVEQ   #0,D0
            MOVEQ   #0,D1
.LP:        CMPI.L  #4,D1
            BGE.S   .DONE
            MOVE.L  D1,D2
            LSL.L   #2,D2
            MOVE.L  0(A0,D2.L),D3
            BSR.S   MULSI3_BY_3
            ADD.L   D3,D0
            ADDQ.L  #1,D1
            BRA.S   .LP
.DONE:      SIMHALT

MULSI3_BY_3:
            MOVE.L  D3,D4
            MULU    #3,D3
            SWAP    D4
            MULU    #3,D4
            SWAP    D4
            CLR.W   D4
            ADD.L   D4,D3
            RTS

ARR:        DC.L    10,20,30,40
            END     START
