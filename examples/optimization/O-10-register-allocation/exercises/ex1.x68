*-----------------------------------------------------------
* Exercise O-10-EX1: Compute Fib(10) = 55 in D0 in <= 180 cycles
*-----------------------------------------------------------
            ORG     $1000
START:
            * Naive stack-spilling loop (exceeds 180 cycles! Keep in D0,D1,D2)
            LINK    A6,#-8
            MOVE.L  #0,-4(A6)
            MOVE.L  #1,-8(A6)
            MOVE.W  #9-1,D2
.LP:        MOVE.L  -4(A6),D0
            ADD.L   -8(A6),D0
            MOVE.L  -8(A6),-4(A6)
            MOVE.L  D0,-8(A6)
            DBRA    D2,.LP
            MOVE.L  -8(A6),D0
            UNLK    A6
            SIMHALT
            END     START
