*-----------------------------------------------------------
* Solution C-07-EX1: Print 42 in binary (task 15) and return 42
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            MOVEQ   #42,D1
            MOVEQ   #2,D2
            MOVEQ   #15,D0
            TRAP    #15
            MOVEQ   #42,D0
            SIMHALT
            END     _start
