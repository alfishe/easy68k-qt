*-----------------------------------------------------------
* Solution C-10-EX1: Counter_add(this=4(A7), delta=8(A7))
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            MOVE.L  #100,$2000
            MOVE.L  #30,-(A7)
            PEA     $2000
            BSR.S   Counter_add
            ADDQ.L  #8,A7
            MOVE.L  #12,-(A7)
            PEA     $2000
            BSR.S   Counter_add
            ADDQ.L  #8,A7
            MOVE.L  $2000,D0
            SIMHALT

Counter_add:
            MOVEA.L 4(A7),A0
            MOVE.L  8(A7),D0
            ADD.L   D0,(A0)
            RTS
            END     _start
