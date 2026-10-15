*-----------------------------------------------------------
* Exercise O-07-EX1: Compute 13^2 = 169 in D0 in <= 48 cycles
*-----------------------------------------------------------
            ORG     $1000
START:
            * Naive MULU (exceeds 48 cycles! Use SQ_TAB lookup instead)
            MOVEQ   #13,D0
            MULU    D0,D0
            SIMHALT

SQ_TAB:     DC.W    0,1,4,9,16,25,36,49,64,81,100,121,144,169,196,225
            END     START
