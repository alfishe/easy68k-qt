* Macro with standard parameters
ADD2: MACRO
    ADD.W \1,\2
    ENDM
    ORG $1000
    ADD2 D0,D1
    ADD2 #10,D2
    END $1000
