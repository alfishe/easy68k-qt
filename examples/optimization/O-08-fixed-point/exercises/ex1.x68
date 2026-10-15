*-----------------------------------------------------------
* Exercise O-08-EX1: Extract integer part of Q16.16 $002A8000 (42) in <= 32 cycles
*-----------------------------------------------------------
            ORG     $1000
START:
            * Naive ASR.L #16 (exceeds 32 cycles! Replace with SWAP + EXT.L)
            MOVE.L  #$002A8000,D0
            MOVEQ   #16,D1
            ASR.L   D1,D0
            SIMHALT
            END     START
