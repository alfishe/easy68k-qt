* Structured loop structures
    ORG $1000
    FOR.L D0 = #1 TO #10 BY #1 DO
    NOP
    ENDF
    WHILE.L D1 <LT> #5 DO
    ADDQ.L #1,D1
    ENDW
    REPEAT
    SUBQ.L #1,D2
    UNTIL.L D2 <EQ> #0 DO
    END $1000
