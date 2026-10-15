*-----------------------------------------------------------
* Title      : S-08 Exceptions and Vector Table
* Description: TRAP #0, CHK, TRAPV, and Divide-by-Zero handlers
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * Enable exception processing via Trap #15 task 32 subtask 5
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15

            * Install vectors: Vec 5 ($14), Vec 6 ($18), Vec 7 ($1C), Vec 32 ($80)
            LEA     ISR_DIV0(PC),A0
            MOVE.L  A0,$0014
            LEA     ISR_CHK(PC),A0
            MOVE.L  A0,$0018
            LEA     ISR_TRAPV(PC),A0
            MOVE.L  A0,$001C
            LEA     ISR_TRAP0(PC),A0
            MOVE.L  A0,$0080

            MOVEQ   #0,D7               ; bitmask of handled exceptions

            * 1) Trigger TRAP #0
            TRAP    #0

            * 2) Trigger CHK (D1 = 25 > upper bound 10)
            MOVEQ   #25,D1
            CHK     #10,D1

            * 3) Trigger TRAPV ($7F + 1 sets V=1)
            MOVE.B  #$7F,D1
            ADDQ.B  #1,D1
            TRAPV

            * 4) Trigger Divide by Zero
            MOVEQ   #100,D1
            MOVEQ   #0,D2
            DIVU    D2,D1

            SIMHALT

ISR_TRAP0:  BSET    #0,D7
            RTE
ISR_CHK:    BSET    #1,D7
            RTE
ISR_TRAPV:  BSET    #2,D7
            RTE
ISR_DIV0:   BSET    #3,D7
            RTE

            END     START
