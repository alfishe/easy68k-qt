* Structured IF/ELSE/ENDI
    ORG $1000
    .IF D0 <EQ> #0
    NOP
    .ELSE
    MOVEQ #1,D0
    .ENDI
    END $1000
