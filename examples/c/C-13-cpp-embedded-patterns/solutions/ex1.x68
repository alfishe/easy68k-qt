*-----------------------------------------------------------
* Solution C-13-EX1: Construct Packet{$1234, $5678} at $2000 + Seg7 $3F
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            LEA     $2000,A0
            MOVE.W  #$1234,0(A0)
            MOVE.W  #$5678,2(A0)
            MOVE.B  #$3F,$00E00000
            MOVEQ   #0,D0
            MOVEQ   #0,D1
            MOVE.W  0(A0),D0
            MOVE.W  2(A0),D1
            ADD.L   D1,D0
            SIMHALT
            END     _start
