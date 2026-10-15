* L2-08 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #1,D0
        MOVEQ   #20,D1
        MOVEQ   #8,D2
        LSL.W   #2,D0
        LEA     JTABLE,A0
        MOVEA.L 0(A0,D0.W),A0
        JMP     (A0)

CASE_ADD:
        ADD.L   D2,D1
        SIMHALT
CASE_SUB:
        SUB.L   D2,D1
        SIMHALT

JTABLE  DC.L    CASE_ADD,CASE_SUB
        END     START
