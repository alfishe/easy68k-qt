*-----------------------------------------------------------
* Solution P-05-EX1: Load saved task counters from TCB_A (111) and TCB_B (222)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     TCB_A(PC),A0
            MOVE.L  (A0)+,D0
            MOVE.L  (A0)+,D1
            MOVE.L  D0,D2
            ADD.L   D1,D2
            SIMHALT
TCB_A:      DC.L    111
TCB_B:      DC.L    222
            END     START
