* Arithmetic instructions
    ORG $1000
    ADD.W D0,D1
    ADD.L #$1234,D2
    SUB.B D3,D4
    SUB.W #$10,D5
    MULU.W D0,D1
    MULS.W #100,D2
    DIVU.W D3,D4
    DIVS.W #-5,D5
    END $1000
