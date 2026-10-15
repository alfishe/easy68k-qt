*-----------------------------------------------------------
* Title      : L2-03 EASy68K Structured Control Flow
* Written by : EASy68K-Qt contributors
* Description: Demonstrate FOR/ENDF, WHILE/ENDW, and IF/ELSE/ENDI
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        MOVEQ   #0,D0
        FOR.L D1 = #1 TO #5 DO.S
            ADD.L   D1,D0
        ENDF

        IF.L D0 <EQ> #15 THEN.S
            MOVEQ   #1,D2
        ELSE.S
            MOVEQ   #0,D2
        ENDI

        MOVEQ   #3,D3
        WHILE.L D3 <GT> #0 DO.S
            ADDQ.L  #2,D0
            SUBQ.L  #1,D3
        ENDW
        SIMHALT

        END     START
