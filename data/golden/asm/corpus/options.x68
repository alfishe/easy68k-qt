* OPT directives and listing control
        OPT     MEX,SEX,CEX
        ORG     $1000
INC2    MACRO
        ADDQ.W  #2,\1
        ENDM
START   INC2    D0
        IF D0 <EQ> #4 THEN
          NOP
        ENDI
        DC.L    1,2,3,4,5,6,7,8
        DC.B    'A long constant expansion string'
        OPT     NOMEX,NOSEX,NOCEX
        INC2    D1
        IF D0 <EQ> #4 THEN
          NOP
        ENDI
        DC.L    1,2,3,4,5,6,7,8
        NOLIST
        NOP
        DC.W    $1234
        LIST
        PAGE
        NOP
        OPT     NOWAR
        ORG     $1001
        OPT     WAR
        ORG     $1101
        OPT     BIT
        OPT     CRE
        END     START
