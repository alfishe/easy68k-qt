*-----------------------------------------------------------
* Exercise O-11-EX1: Compute sum (D0=300) and max (D1=89) in <= 220 cycles
*-----------------------------------------------------------
            ORG     $1000
START:
            * Naive indexed loop (exceeds 220 cycles! Use (A0)+ and DBRA)
            LEA     DATA(PC),A0
            MOVEQ   #0,D0
            MOVE.W  (A0),D1
            MOVEQ   #0,D2
.LP:        CMPI.W  #8,D2
            BGE.S   .DONE
            MOVE.W  D2,D3
            ADD.W   D3,D3
            MOVE.W  0(A0,D3.W),D4
            ADD.W   D4,D0
            CMP.W   D1,D4
            BLE.S   .NEXT
            MOVE.W  D4,D1
.NEXT:      ADDQ.W  #1,D2
            BRA.S   .LP
.DONE:      SIMHALT
DATA:       DC.W    12,45,7,89,23,56,3,65
            END     START
