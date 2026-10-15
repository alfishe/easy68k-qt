*-----------------------------------------------------------
* Title      : S-09 Address and Bus Errors
* Description: Odd word access, Group 0 frame inspection, recovery
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * Enable exception processing (task 32, subtask 5)
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15

            * Install Address Error handler at Vector 3 ($000C)
            LEA     ADDR_ERR_ISR(PC),A0
            MOVE.L  A0,$000C

            * Deliberately read a word from an odd address ($2001)
            LEA     $2001,A0
            MOVE.W  (A0),D0             ; faults! Handler recovers to next line

            MOVEQ   #1,D4               ; confirms recovery succeeded
            SIMHALT

* Group 0 stack frame on entry:
*   0(A7).W = info word
*   2(A7).L = faulting address ($00002001)
*   6(A7).W = opcode ($3010)
*   8(A7).W = saved SR
*  10(A7).L = saved PC
ADDR_ERR_ISR:
            MOVEQ   #0,D5
            MOVE.W  (A7)+,D5            ; pop info word
            MOVE.L  (A7)+,D6            ; pop faulting address ($00002001)
            MOVEQ   #0,D7
            MOVE.W  (A7)+,D7            ; pop opcode ($3010)
            RTE                         ; pop SR and PC to resume

            END     START
