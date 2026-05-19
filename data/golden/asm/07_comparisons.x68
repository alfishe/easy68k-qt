* Comparison and Scc instructions
    ORG $1000
    CMP.W D0,D1
    CMP.L #$2000,D2
    TST.B D3
    TST.L A0
    SEQ D0
    SNE D1
    SGT D2
    SLT D3
    END $1000
