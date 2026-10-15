*-----------------------------------------------------------
* Exercise C-08-EX1: Implement asm_abs32(4(A7)) -> D0
*-----------------------------------------------------------
            ORG     $1000
_start:
            MOVE.L  #-1234,-(A7)
            BSR.S   asm_abs32
            ADDQ.L  #4,A7
            SIMHALT

asm_abs32:
            * TODO: Read 4(A7) into D0 and negate if negative
            RTS
            END     _start
