*-----------------------------------------------------------
* Title      : O-06 Naive MULU #320 and DIVU #8
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVE.W  #100,D0
            MOVE.W  D0,D1
            MULU    #320,D0             ; ~54 cycles
            DIVU    #8,D1               ; ~140 cycles
            ANDI.L  #$FFFF,D1
            ADD.L   D1,D0               ; 32012
            SIMHALT
            END     START
