*-----------------------------------------------------------
* Title      : O-01 Measuring First (Naive vs Optimised Cycles)
* Description: Measures naive (D2) vs optimised (D3) 8-longword sum
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * 1) Measure naive indexed loop -> cycles in D2
            MOVEQ   #30,D0
            TRAP    #15
            BSR.S   SUM_NAIVE
            MOVE.L  D0,D4               ; save naive sum (360)
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D2               ; D2 = naive cycles

            * 2) Measure optimised (A0)+ / DBRA loop -> cycles in D3
            MOVEQ   #30,D0
            TRAP    #15
            BSR.S   SUM_OPT
            MOVE.L  D0,D5               ; save opt sum (360)
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D3               ; D3 = optimised cycles (< D2)

            MOVE.L  D5,D0               ; D0 = 360
            SIMHALT

SUM_NAIVE:
            MOVE.L  #0,D0
            MOVE.W  #0,D1
.NLP:       CMPI.W  #8,D1
            BGE.S   .NDONE
            LEA     ARR(PC),A0
            MOVE.W  D1,D6
            LSL.W   #2,D6
            ADD.L   0(A0,D6.W),D0
            ADDI.W  #1,D1
            BRA.S   .NLP
.NDONE:     RTS

SUM_OPT:
            LEA     ARR(PC),A0
            MOVEQ   #0,D0
            MOVEQ   #8-1,D1
.OLP:       ADD.L   (A0)+,D0
            DBRA    D1,.OLP
            RTS

ARR:        DC.L    10,20,30,40,50,60,70,80
            END     START
