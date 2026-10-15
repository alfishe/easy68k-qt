*-----------------------------------------------------------
* Exercise O-01-EX1: Sum 8 longwords at ARR into D0 in <= 220 cycles
*-----------------------------------------------------------
            ORG     $1000
START:
            * Naive implementation (exceeds 220 cycles! Optimise with (A0)+ and DBRA)
            MOVE.L  #0,D0
            MOVE.W  #0,D1
.LP:        CMPI.W  #8,D1
            BGE     .DONE
            LEA     ARR,A0
            MOVE.W  D1,D4
            LSL.W   #2,D4
            ADD.L   0(A0,D4.W),D0
            ADDI.W  #1,D1
            BRA     .LP
.DONE:      SIMHALT

ARR:        DC.L    10,20,30,40,50,60,70,80
            END     START
