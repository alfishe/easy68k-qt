* Shift and rotate instructions
    ORG $1000
    ASL.B #2,D0
    ASR.W D1,D2
    LSL.L #8,D3
    LSR.W D4,D5
    ROL.B #1,D6
    ROR.L D0,D1
    END $1000
