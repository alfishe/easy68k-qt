*-----------------------------------------------------------
* Title      : O-12 Writing 68000-Friendly C (32-bit vs 16-bit C)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * 1) Naive 32-bit C loop calling software __mulsi3 -> D4 cycles
            MOVEQ   #30,D0
            TRAP    #15
            LEA     ARR32(PC),A0
            MOVEQ   #0,D2
            MOVEQ   #0,D1
.NLP:       CMPI.L  #4,D1
            BGE.S   .NDONE
            MOVE.L  D1,D6
            LSL.L   #2,D6
            MOVE.L  0(A0,D6.L),D3
            BSR.S   MULSI3_BY_3
            ADD.L   D3,D2
            ADDQ.L  #1,D1
            BRA.S   .NLP
.NDONE:     MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D4               ; 32-bit C cycles

            * 2) 68000-friendly C (int16_t*, countdown DBRA, MULS.W) -> D5 cycles
            MOVEQ   #30,D0
            TRAP    #15
            LEA     ARR16(PC),A0
            MOVEQ   #0,D3
            MOVEQ   #4-1,D1
.OLP:       MOVE.W  (A0)+,D6
            MULS.W  #3,D6
            ADD.L   D6,D3
            DBRA    D1,.OLP
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D5               ; 68000-friendly C cycles (< D4)

            MOVE.L  D3,D0               ; D0 = 300
            SIMHALT

MULSI3_BY_3:
            MOVE.L  D3,D7
            MULU    #3,D3
            SWAP    D7
            MULU    #3,D7
            SWAP    D7
            CLR.W   D7
            ADD.L   D7,D3
            RTS

ARR32:      DC.L    10,20,30,40
ARR16:      DC.W    10,20,30,40
            END     START
