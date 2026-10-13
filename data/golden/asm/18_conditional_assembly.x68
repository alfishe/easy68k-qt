* Conditional assembly IFNE/IFEQ/ENDC
MODE: EQU 1
    ORG $1000
    IFNE MODE
    MOVEQ #1,D0
    ENDC
    IFEQ MODE
    MOVEQ #2,D0
    ENDC
    END $1000
