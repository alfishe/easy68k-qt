*-----------------------------------------------------------
* Title      : O-04 Loop Optimisation (Top-Tested vs 4x Unrolled DBRA)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * 1) Naive top-tested loop -> D4 cycles
            MOVEQ   #30,D0
            TRAP    #15
            LEA     ARR(PC),A0
            MOVEQ   #0,D2
            MOVEQ   #0,D1
.NLP:       CMPI.W  #16,D1
            BGE.S   .NDONE
            ADD.W   (A0)+,D2
            ADDQ.W  #1,D1
            BRA.S   .NLP
.NDONE:     MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D4               ; naive cycles

            * 2) 4x unrolled DBRA loop -> D5 cycles
            MOVEQ   #30,D0
            TRAP    #15
            LEA     ARR(PC),A0
            MOVEQ   #0,D3
            MOVEQ   #4-1,D1             ; 16 / 4 = 4 iterations
.OLP:       ADD.W   (A0)+,D3
            ADD.W   (A0)+,D3
            ADD.W   (A0)+,D3
            ADD.W   (A0)+,D3
            DBRA    D1,.OLP
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D5               ; optimised cycles (< D4)

            MOVE.L  D3,D0               ; D0 = 136
            SIMHALT

ARR:        DC.W    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
            END     START
