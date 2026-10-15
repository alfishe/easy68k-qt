*-----------------------------------------------------------
* Solution S-15-EX1: Pack TCP port 4096 ($1000) parameter word
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVE.L  #$10000001,D0
            SIMHALT
            END     START
