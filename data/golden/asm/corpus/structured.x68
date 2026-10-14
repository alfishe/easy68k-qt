* Structured assembly: IF/ELSE/ENDI, WHILE, REPEAT/UNTIL, FOR, DBLOOP/UNLESS
        ORG     $1000
START   MOVE.W  #5,D0
        IF.W D0 <GT> #3 THEN
          MOVEQ   #1,D1
        ELSE
          MOVEQ   #2,D1
        ENDI
        IF <EQ> THEN.S
          NOP
        ENDI
        IF.B D0 <LT> D1 OR.B D2 <GE> D3 THEN
          NOP
        ENDI
        IF D0 <NE> #0 AND D1 <HI> D2 THEN.L
          NOP
        ENDI
        WHILE D0 <GT> #0 DO
          SUBQ.W  #1,D0
        ENDW
        WHILE.L (A0)+ <NE> (A1)+ DO.S
          NOP
        ENDW
        REPEAT
          ADDQ.W  #1,D0
        UNTIL D0 <EQ> #10 DO
        REPEAT
          NOP
        UNTIL <MI>
        FOR D0 = #1 TO #10 DO
          NOP
        ENDF
        FOR.L D1 = #100 DOWNTO #0 BY #5 DO.S
          NOP
        ENDF
        FOR.B D2 = D3 TO D4 BY D5 DO
          NOP
        ENDF
        DBLOOP D6 = #4
          NOP
        UNLESS
        DBLOOP D7 = D6
          NOP
        UNLESS <F>
        DBLOOP D5 = #3
          NOP
        UNLESS.W D0 <EQ> D1
        IF D0 <EQ> D1 THEN
          IF D2 <EQ> D3 THEN
            WHILE <CS> DO
              NOP
            ENDW
          ENDI
        ENDI
        IF D0 <GT> D1
          NOP
        ENDI
        SIMHALT
        END     START
