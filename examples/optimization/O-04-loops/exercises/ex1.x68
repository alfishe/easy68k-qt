*-----------------------------------------------------------
* Exercise O-04-EX1: Sum 16 words at ARR into D0 in <= 200 cycles
*-----------------------------------------------------------
            ORG     $1000
START:
            * Naive loop (exceeds 200 cycles! Unroll 4x with DBRA)
            LEA     ARR(PC),A0
            MOVEQ   #0,D0
            MOVEQ   #0,D1
.LP:        CMPI.W  #16,D1
            BGE.S   .DONE
            ADD.W   (A0)+,D0
            ADDQ.W  #1,D1
            BRA.S   .LP
.DONE:      SIMHALT
ARR:        DC.W    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
            END     START
