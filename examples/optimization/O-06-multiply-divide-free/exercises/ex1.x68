*-----------------------------------------------------------
* Exercise O-06-EX1: Compute 123 * 10 = 1230 in D0 in <= 48 cycles
*-----------------------------------------------------------
            ORG     $1000
START:
            * Naive MULU (exceeds 48 cycles! Use shifts and adds: x*2 + x*8)
            MOVE.W  #123,D0
            MULU    #10,D0
            SIMHALT
            END     START
