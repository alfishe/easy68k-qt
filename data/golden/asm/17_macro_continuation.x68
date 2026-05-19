* Macro using continuation lines
COMPLEX: MACRO
    ADD.W \1,\2
    ADD.W \3,\4
    ENDM
    ORG $1000
    COMPLEX D0,D1,
& D2,D3
    END $1000
