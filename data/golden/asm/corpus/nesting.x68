* Assembler corpus: nested macros (with local labels, NARG and IFARG),
* nested INCLUDE files and long DC constants (OPT CEX continuation lines),
* compared with the original assembler in every option variant. CC0-1.0.
        ORG     $1000
INNER   MACRO
        IFARG   2
        ADD.\0  \1,\2
        ENDC
        IFNC    '\1',''
        MOVE.\0 \1,D7
        ENDC
LOOP\@  DBRA    D6,LOOP\@
        ENDM
MIDDLE  MACRO
        INNER.W \1,\2
        INNER.L #NARG,D5
        ENDM
OUTER   MACRO
        MIDDLE  \1,\2
        MIDDLE  #3,D0
        ENDM
START   OUTER   D1,D2
        OUTER   #$10
        INCLUDE inc_nest1.x68
TEXT    DC.B    'This is a long string constant that spans several listing lines',0
WORDS   DC.W    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24
LONGS   DC.L    $11111111,$22222222,$33333333,$44444444,$55555555,$66666666
BLOCK   DCB.W   20,$ABCD
        SIMHALT
        END     START
