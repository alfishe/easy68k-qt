* Directives: DC/DS/DCB, EQU/SET/REG, ORG/SECTION/OFFSET, expressions
COUNT   EQU     5
MASK    EQU     $F0F0
VAL     SET     1
VAL     SET     VAL+2
SAVED   REG     D0-D3/A0/A5-A6
        ORG     $1000
START   DC.B    1,2,3,'A','It''s',0
        DC.W    COUNT,MASK,START,-1,$1234
        DC.L    START,END_OF,$12345678,'ABCD'
        DC.B    'This is a long string that will not fit in the listing columns',0
        DS.B    3
        DS.W    2
        DS.L    1
BLOCK   DCB.B   5,$AA
        DCB.W   3,$BEEF
        DCB.L   2,START
EXPR    DC.W    2+3*4,(2+3)*4,100/7,100\7,1<<4,256>>2,$FF&$0F,$F0!$0F,$FF^$0F,~0
        DC.L    -COUNT,--COUNT,%1010,@17,'AB'<<16
        DC.W    VAL,*-START
        SECTION 1
        ORG     $2000
SEC1    DC.W    1
        SECTION 0
        DC.W    2
        OFFSET  0
FIELD1  DS.L    1
FIELD2  DS.W    2
FIELD3  DS.B    1
        ORG     $3000
        MOVEM.L SAVED,-(SP)
        MOVEM.L (SP)+,SAVED
        LEA     FIELD2(A0),A1
        MOVE.W  #FIELD3,D0
END_OF  DC.B    0
        SIMHALT
        END     START
