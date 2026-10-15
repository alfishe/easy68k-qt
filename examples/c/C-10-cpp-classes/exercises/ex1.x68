*-----------------------------------------------------------
* Exercise C-10-EX1: Counter_add(this=4(A7), delta=8(A7))
*-----------------------------------------------------------
            ORG     $1000
_start:
            MOVE.L  #100,$2000          ; Counter c(100) at $2000
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
            * TODO: Add 8(A7) (delta) to the longword pointed to by 4(A7) (this)
            RTS
            END     _start
