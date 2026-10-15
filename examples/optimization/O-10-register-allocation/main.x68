*-----------------------------------------------------------
* Title      : O-10 Register Allocation (Spills vs Registers)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * 1) Naive stack-spilling + full MOVEM -> D4 cycles
            MOVEQ   #30,D0
            TRAP    #15
            BSR.S   FIB10_NAIVE
            MOVE.L  D0,D6               ; 55
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D4               ; naive cycles

            * 2) Register-allocated leaf subroutine -> D5 cycles
            MOVEQ   #30,D0
            TRAP    #15
            BSR.S   FIB10_OPT
            MOVE.L  D0,D7               ; 55
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D5               ; optimised cycles (< D4)

            MOVE.L  D7,D0               ; D0 = 55
            SIMHALT

FIB10_NAIVE:
            LINK    A6,#-12
            MOVEM.L D2-D5,-(A7)
            MOVE.L  #0,-4(A6)
            MOVE.L  #1,-8(A6)
            MOVEQ   #9-1,D2
.NLP:       MOVE.L  -4(A6),D0
            ADD.L   -8(A6),D0
            MOVE.L  -8(A6),-4(A6)
            MOVE.L  D0,-8(A6)
            DBRA    D2,.NLP
            MOVE.L  -8(A6),D0
            MOVEM.L (A7)+,D2-D5
            UNLK    A6
            RTS

FIB10_OPT:
            MOVEQ   #0,D1               ; a = 0
            MOVEQ   #1,D0               ; b = 1
            MOVEQ   #9-1,D2             ; 9 steps from Fib(1) -> Fib(10)
.OLP:       ADD.L   D0,D1               ; a = a + b
            EXG     D0,D1               ; swap so D0 holds new Fib
            DBRA    D2,.OLP
            RTS

            END     START
