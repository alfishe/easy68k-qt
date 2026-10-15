*-----------------------------------------------------------
* Title      : S-12 Memory Map Protection
* Description: MEMORY ROM, READ, PROTECTED, INVALID and Vector 2
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            MEMORY  ROM $2000,$20FF
            MEMORY  READ $2100,$21FF
            MEMORY  PROTECTED $2200,$22FF
            MEMORY  INVALID $2300,$23FF

            ORG     $1000
START:
            * Enable exceptions (task 32, subtask 5)
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15

            * Install Bus Error handler at Vector 2 ($0008)
            LEA     BUS_ERR_ISR(PC),A0
            MOVE.L  A0,$0008

            * 1) Attempt to overwrite ROM at $2000 (silently ignored)
            MOVE.L  #$DEADBEEF,$2000
            MOVE.L  $2000,D4            ; D4 remains $12345678

            * 2) Supervisor access to PROTECTED at $2200 succeeds
            MOVE.L  #$CAFE0001,$2200
            MOVE.L  $2200,D5

            * 3) Access INVALID at $2300 -> triggers Vector 2 Bus Error!
            MOVEQ   #0,D7
            LEA     $2300,A0
            MOVE.W  (A0),D0

            SIMHALT

BUS_ERR_ISR:
            ADDQ.L  #2,A7               ; skip info word
            MOVE.L  (A7)+,D6            ; faulting address ($00002300)
            ADDQ.L  #2,A7               ; skip opcode word
            ADDQ.L  #1,D7               ; count bus error
            RTE

            ORG     $2000
ROM_CONST:  DC.L    $12345678

            END     START
