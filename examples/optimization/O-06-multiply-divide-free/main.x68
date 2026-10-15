*-----------------------------------------------------------
* Title      : O-06 Multiply/Divide-Free Arithmetic
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * 1) Naive MULU #320 + DIVU #8 -> D4 cycles
            MOVEQ   #30,D0
            TRAP    #15
            MOVE.W  #100,D2
            MOVE.W  D2,D1
            MULU    #320,D2
            DIVU    #8,D1
            ANDI.L  #$FFFF,D1
            ADD.L   D1,D2               ; D2 = 32012
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D4               ; naive cycles

            * 2) Shift-and-add: y*320 = (y<<6) + (y<<8), y/8 = y>>3 -> D5 cycles
            MOVEQ   #30,D0
            TRAP    #15
            MOVEQ   #100,D3
            MOVE.L  D3,D1
            LSR.L   #3,D1               ; y / 8 = 12
            LSL.L   #6,D3               ; y * 64 = 6400
            ADD.L   D3,D1               ; 6412
            LSL.L   #2,D3               ; y * 256 = 25600
            ADD.L   D1,D3               ; D3 = 32012
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D5               ; optimised cycles (< D4)

            MOVE.L  D3,D0               ; D0 = 32012
            SIMHALT
            END     START
