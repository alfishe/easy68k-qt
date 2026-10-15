*-----------------------------------------------------------
* Solution O-08-EX1: Extract integer part of Q16.16 $002A8000 (42) in <= 32 cycles
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVE.L  #$002A8000,D0
            SWAP    D0
            EXT.L   D0
            SIMHALT
            END     START
