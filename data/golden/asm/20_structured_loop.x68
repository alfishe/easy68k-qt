* Structured loop structures
    ORG $1000
    .FOR D0 = #1 TO #10 BY #1
    NOP
    .ENDF
    .WHILE D1 <LT> #5 DO
    ADDQ.L #1,D1
    .ENDW
    .REPEAT
    SUBQ.L #1,D2
    .UNTIL D2 <EQ> #0
    END $1000
