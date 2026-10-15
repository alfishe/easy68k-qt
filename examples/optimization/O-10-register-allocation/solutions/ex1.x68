*-----------------------------------------------------------
* Solution O-10-EX1: Compute Fib(10) = 55 in D0 in <= 180 cycles
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #1,D1
            MOVEQ   #1,D0
            MOVEQ   #4-1,D2
.LP:        ADD.L   D0,D1
            ADD.L   D1,D0
            DBRA    D2,.LP
            SIMHALT
            END     START
