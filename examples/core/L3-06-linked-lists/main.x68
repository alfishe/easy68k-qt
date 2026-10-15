*-----------------------------------------------------------
* Title      : L3-06 Linked Lists and Pointer Chasing
* Written by : EASy68K-Qt contributors
* Description: Traverse a 3-node singly linked list and sum values
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        LEA     NODE1,A0        ; head pointer
        MOVEQ   #0,D0           ; sum = 0
        MOVEQ   #0,D1           ; node count = 0

.LOOP:
        CMPA.L  #0,A0           ; is A0 == NULL?
        BEQ.S   .DONE
        ADD.L   0(A0),D0        ; sum += node->value
        ADDQ.L  #1,D1           ; count++
        MOVEA.L 4(A0),A0        ; A0 = node->next
        BRA.S   .LOOP
.DONE:
        SIMHALT

* Nodes placed non-contiguously to show pointer chasing
NODE1   DC.L    10,NODE2
NODE3   DC.L    30,0
NODE2   DC.L    20,NODE3

        END     START
