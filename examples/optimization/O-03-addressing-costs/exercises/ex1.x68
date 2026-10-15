*-----------------------------------------------------------
* Exercise O-03-EX1: Copy 8 words from SRC to $2000 in <= 200 cycles
*-----------------------------------------------------------
            ORG     $1000
START:
            * Naive indexed copy (exceeds 200 cycles! Replace with (A0)+,(A1)+)
            LEA     SRC(PC),A0
            LEA     $2000,A1
            MOVEQ   #0,D1
            MOVEQ   #8-1,D2
.LP:        MOVE.W  0(A0,D1.W),0(A1,D1.W)
            ADDQ.W  #2,D1
            DBRA    D2,.LP
            SIMHALT
SRC:        DC.W    11,22,33,44,55,66,77,88
            END     START
