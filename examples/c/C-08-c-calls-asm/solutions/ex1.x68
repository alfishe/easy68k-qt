*-----------------------------------------------------------
* Solution C-08-EX1: Implement asm_abs32(4(A7)) -> D0
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            MOVE.L  #-1234,-(A7)
            BSR.S   asm_abs32
            ADDQ.L  #4,A7
            SIMHALT

asm_abs32:
            MOVE.L  4(A7),D0
            BPL.S   .pos
            NEG.L   D0
.pos:       RTS
            END     _start
