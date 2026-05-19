* Macro using NARG and \0 parameter
ARGCOUNT: MACRO
    DC.W NARG
    IFARG \1
    MOVE.W \1,D0
    ENDC
    ENDM
    ORG $1000
    ARGCOUNT
    ARGCOUNT D1
    END $1000
