* Move instructions
    ORG $1000
    MOVE.W D0,D1
    MOVE.L D2,A0
    MOVE.B #$42,D3
    MOVEQ #5,D4
    MOVEQ #-10,D5
    MOVE.W D0,CCR
    MOVE.W SR,D1
    MOVE.W D2,SR
    END $1000
