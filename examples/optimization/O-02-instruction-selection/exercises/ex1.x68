*-----------------------------------------------------------
* Exercise O-02-EX1: Compute D0 = (100 + 8) + (7 - 3) = 112 in <= 36 cycles
*-----------------------------------------------------------
            ORG     $1000
START:
            * Naive (60 cycles — replace with MOVEQ/ADDQ/SUBQ!)
            MOVE.L  #100,D0
            MOVE.L  #7,D1
            ADDI.L  #8,D0
            SUBI.L  #3,D1
            ADD.L   D1,D0
            SIMHALT
            END     START
