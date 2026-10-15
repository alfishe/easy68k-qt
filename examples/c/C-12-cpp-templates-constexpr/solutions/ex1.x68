*-----------------------------------------------------------
* Solution C-12-EX1: Fold fib_ct(10)=55 + (5<<3)=40 -> 95 in D0
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            MOVEQ   #5,D0
            ASL.L   #3,D0
            ADD.L   #55,D0
            SIMHALT
            END     _start
