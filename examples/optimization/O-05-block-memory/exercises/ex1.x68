*-----------------------------------------------------------
* Exercise O-05-EX1: Copy 32 bytes from BLK to $2000 in <= 190 cycles
*-----------------------------------------------------------
            ORG     $1000
START:
            * Naive byte loop (exceeds 190 cycles! Replace with MOVEM.L D0-D7)
            LEA     BLK(PC),A0
            LEA     $2000,A1
            MOVEQ   #32-1,D0
.LP:        MOVE.B  (A0)+,(A1)+
            DBRA    D0,.LP
            SIMHALT
BLK:        DC.L    $11111111,$22222222,$33333333,$44444444
            DC.L    $55555555,$66666666,$77777777,$88888888
            END     START
