* L3-06 Exercise 1: Bypass NODE2 so NODE1->next = NODE3 and sum -> D0 = 40
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        LEA     NODE1,A0
        * TODO: set 4(A0) to #NODE3, then sum the list into D0
        SIMHALT

NODE1   DC.L    10,NODE2
NODE2   DC.L    20,NODE3
NODE3   DC.L    30,0
        END     START
