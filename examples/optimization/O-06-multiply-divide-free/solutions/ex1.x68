*-----------------------------------------------------------
* Solution O-06-EX1: Compute 123 * 10 = 1230 in D0 in <= 48 cycles
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #123,D0
            ADD.L   D0,D0               ; x * 2
            MOVE.L  D0,D1
            LSL.L   #2,D0               ; x * 8
            ADD.L   D1,D0               ; x * 10 = 1230
            SIMHALT
            END     START
