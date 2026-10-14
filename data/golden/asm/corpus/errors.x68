* Error and warning paths
        ORG     $1000
START   NOP
        BOGUS   D0
        MOVE.X  D0,D1
        ADDQ    #9,D0
        DC.W    1/0
        DCB.B   -1,0
        BRA.S   START+1000
DUP     NOP
DUP     NOP
        EQU     5
NOLABEL EQU
        MOVE.W  $1234.W,D0
        DC.L    'TOOLONG'
        DC.W    $123456789
        FAIL    Custom failure message
        ORG     FWD
FWD     NOP
        MOVEQ   #300,D0
        TRAP    #16
        LINK    A6,#$10000
        INCLUDE missing_file.x68
        ENDM
        MOVE.L  D0,D1,D2
        END
