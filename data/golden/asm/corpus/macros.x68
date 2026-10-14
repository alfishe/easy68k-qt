* Macros: parameters, \@ labels, NARG, IFARG, MEXIT, nesting, size \0
        ORG     $1000
PUSH    MACRO
        MOVE.\0 \1,-(SP)
        ENDM
POP     MACRO
        MOVE.\0 (SP)+,\1
        ENDM
SWAPR   MACRO
        PUSH.L  \1
        MOVE.L  \2,\1
        POP.L   \2
        ENDM
DELAY   MACRO
        MOVE.W  #\1,D7
LOOP\@  DBRA    D7,LOOP\@
        ENDM
OPTARG  MACRO
        IFARG   2
        MOVE.L  \2,D0
        ENDC
        MOVE.L  \1,D1
        DC.W    NARG
        ENDM
EARLY   MACRO
        MOVEQ   #1,D0
        IFC     '\1','STOP'
        MEXIT
        ENDC
        MOVEQ   #2,D0
        ENDM
MANY    MACRO
        DC.B    \1,\2,\3,\4,\5,\6,\7,\8,\9,\A
        ENDM
START   PUSH    D0
        PUSH.L  A0
        POP.W   D1
        SWAPR   D2,D3
        DELAY   10
        DELAY   20
        OPTARG  #1
        OPTARG  #1,#2
        EARLY   GO
        EARLY   STOP
        MANY    1,2,3,4,5,6,7,8,9,10
        PUSH    <D0>
        SIMHALT
        END     START
