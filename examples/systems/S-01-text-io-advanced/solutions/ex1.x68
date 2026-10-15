*-----------------------------------------------------------
* Solution S-01-EX1: Clear screen and set/read cursor
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #11,D0
            MOVE.W  #$FF00,D1
            TRAP    #15
            MOVEQ   #11,D0
            MOVE.W  #$0502,D1
            TRAP    #15
            MOVEQ   #11,D0
            MOVE.W  #$00FF,D1
            TRAP    #15
            MOVEQ   #0,D0
            MOVE.W  D1,D0
            SIMHALT
            END     START
