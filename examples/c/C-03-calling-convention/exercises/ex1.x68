*-----------------------------------------------------------
* Exercise C-03-EX1: clamp_val(150, 0, 100) -> 100
*-----------------------------------------------------------
            ORG     $1000
_start:
            MOVE.L  #100,-(A7)
            MOVE.L  #0,-(A7)
            MOVE.L  #150,-(A7)
            BSR.S   clamp_val
            LEA     12(A7),A7
            SIMHALT

clamp_val:
            * TODO: Implement clamp_val(x=8(A6), lo=12(A6), hi=16(A6)) -> D0
            RTS
            END     _start
