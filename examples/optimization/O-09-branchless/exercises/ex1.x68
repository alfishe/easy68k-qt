*-----------------------------------------------------------
* Exercise O-09-EX1: Branchless (D1 < 100) -> D0 = 1 in <= 32 cycles
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #40,D1
            MOVEQ   #0,D0
            * TODO: Set D0 = 1 if D1 < 100 else 0 using CMPI.W, SLT, NEG.B
            SIMHALT
            END     START
