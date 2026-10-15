*-----------------------------------------------------------
* Solution C-02-EX1: Sum high and low 16-bit words at $2000
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            MOVEQ   #0,D0
            MOVEQ   #0,D1
            MOVE.W  $2000,D0
            MOVE.W  $2002,D1
            ADD.L   D1,D0
            SIMHALT
            ORG     $2000
            DC.L    $ABCDEF01
            END     _start
