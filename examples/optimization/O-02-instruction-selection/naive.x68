*-----------------------------------------------------------
* Title      : O-02 Naive Instruction Selection
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            CLR.L   D0                  ; 6 cycles
            CLR.L   D1                  ; 6 cycles
            MOVE.W  #7,D1               ; 8 cycles
            ADDI.L  #10,D1              ; 16 cycles
            SUBI.L  #7,D1               ; 16 cycles
            MOVE.L  D1,D0               ; 4 cycles
            SIMHALT
            END     START
