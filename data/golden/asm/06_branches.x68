* Branch and jump instructions
    ORG $1000
START:
    BRA START
    BSR SUBR
    BEQ START
    BNE START
    BGT START
    BLT START
    JMP START
    JSR SUBR
SUBR:
    RTS
    END START
