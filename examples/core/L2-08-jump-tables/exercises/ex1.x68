* L2-08 Exercise 1: Dispatch CASE_SUB (index 1) for 20 - 8 = 12
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #1,D0
        MOVEQ   #20,D1
        MOVEQ   #8,D2
        * TODO: dispatch via JTABLE
        SIMHALT

CASE_ADD:
        ADD.L   D2,D1
        SIMHALT
CASE_SUB:
        SUB.L   D2,D1
        SIMHALT

JTABLE  DC.L    CASE_ADD,CASE_SUB
        END     START
