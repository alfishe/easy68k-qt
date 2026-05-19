* Conditional assembly IF/ELSE/ENDC
MODE: EQU 1
    ORG $1000
    IFNE MODE
    MOVEQ #1,D0
    ELSE
    MOVEQ #2,D0
    ENDC
    END $1000
