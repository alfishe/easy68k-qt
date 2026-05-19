* Simple macro
MYMAC: MACRO
    NOP
    ENDM
    ORG $1000
    MYMAC
    END $1000
