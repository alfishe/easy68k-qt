* Disjoint memory blocks using ORG
    ORG $1000
    NOP
    ORG $2000
    MOVEQ #1,D0
    ORG $3000
    RTS
    END $1000
