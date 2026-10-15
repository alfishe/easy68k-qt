*-----------------------------------------------------------
* Title      : O-02 Instruction Selection (MOVEQ/ADDQ/SUBQ)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * Naive sequence: CLR.L + MOVE.W + ADDI.L #10 +ADDA.L #12
            MOVEQ   #30,D0
            TRAP    #15
            CLR.L   D0                  ; 6 cycles
            CLR.L   D1                  ; 6 cycles
            MOVE.W  #7,D1               ; 8 cycles
            ADDI.L  #10,D1              ; 16 cycles
            SUBI.L  #7,D1               ; 8 cycles (SUBQ)
            MOVE.L  D1,D2               ; 4 cycles
            MOVEQ   #31,D0              ; 4 cycles
            TRAP    #15
            MOVE.L  D1,D4               ; D4 = 52 cycles

            * Optimised sequence: MOVEQ + ADDQ/SUBQ (24 cycles with MOVEQ #31)
            MOVEQ   #30,D0
            TRAP    #15
            MOVEQ   #7,D3               ; 4 cycles
            ADDQ.L  #5,D3               ; 8 cycles
            SUBQ.L  #2,D3               ; 8 cycles
            MOVEQ   #31,D0              ; 4 cycles
            TRAP    #15
            MOVE.L  D1,D5               ; D5 = 24 cycles

            MOVE.L  D3,D0               ; D0 = 10
            SIMHALT
            END     START
