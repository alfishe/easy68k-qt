* Assembler corpus: every structured condition code, sizes and branch
* sizes, nested conjunctions, and expression operators/directive forms
* not used by structured.x68 / directives.x68. Compared with the original
* assembler. CC0-1.0.
        ORG     $1000
VAL1    EQU     10+2*3-4/2
VAL2    EQU     (1<<4)|(1>>0)&$FF^$0F
VAL3    EQU     -VAL1+%1010+@17
VAL4    SET     1
VAL4    SET     VAL4+1
CNT     EQU     $10
START   IF.B D0 <CC> D1 THEN
          NOP
        ENDI
        IF.W D0 <CS> D1 THEN.S
          NOP
        ENDI
        IF.L D0 <EQ> D1 THEN.L
          NOP
        ENDI
        IF D0 <NE> D1 THEN
          NOP
        ELSE.S
          NOP
        ENDI
        IF D0 <VC> D1 THEN
          NOP
        ELSE.L
          NOP
        ENDI
        IF D0 <VS> D1 OR D2 <PL> D3 OR D4 <MI> D5 THEN
          NOP
        ENDI
        IF D0 <GE> D1 AND D2 <LT> D3 AND D4 <GT> D5 THEN
          NOP
        ENDI
        IF D0 <LE> D1 OR D2 <HI> D3 AND D4 <LS> D5 THEN
          NOP
        ENDI
        IF <HS> THEN
          NOP
        ENDI
        IF <LO> THEN
          NOP
        ENDI
        IF <T> THEN
          NOP
        ENDI
        IF <F> THEN
          NOP
        ENDI
        WHILE.B (A0) <NE> #0 DO
          ADDQ.L  #1,A0
        ENDW
        WHILE <VC> DO.L
          NOP
        ENDW
        REPEAT
          NOP
        UNTIL.L D0 <GE> #CNT DO.S
        FOR.W D0 = #VAL1 TO #VAL1+5 BY #2 DO.L
          FOR.B D1 = #3 DOWNTO #1 DO
            NOP
          ENDF
        ENDF
        DBLOOP.S D3 = #CNT
          NOP
        UNLESS.L D0 <EQ> #0
* data and space forms
BYTES   DC.B    1,-1,$FF,'Hi',0
WORDS   DC.W    VAL1,VAL2,VAL3,VAL4,BYTES
LONGS   DC.L    START,-1,'ABCD'
        DCB.B   3,$AA
        DCB.W   2,$BBBB
        DCB.L   2,$CCCCCCCC
        DS.B    3
        DS.W    2
        DS.L    1
        DS.B    0
        DC.B    *-BYTES
        OFFSET  0
FIELD1  DS.W    1
FIELD2  DS.L    1
        ORG     *
        DC.W    FIELD2
        SIMHALT
        END     START
