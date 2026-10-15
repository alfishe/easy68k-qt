*-----------------------------------------------------------
* Exercise P-05-EX1: Load saved task counters from TCB_A (111) and TCB_B (222)
*-----------------------------------------------------------
            ORG     $1000
START:
            * TODO: Load D0 = 111 from TCB_A, D1 = 222 from TCB_B, and D2 = D0 + D1 (333)
            SIMHALT
TCB_A:      DC.L    111
TCB_B:      DC.L    222
            END     START
