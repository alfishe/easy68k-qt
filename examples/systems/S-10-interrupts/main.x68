*-----------------------------------------------------------
* Title      : S-10 Interrupts and Autovectors
* Description: Level 2 autovector ISR, auto-IRQ, and STOP wake
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * Enable exceptions (task 32, subtask 5)
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15

            * Install Level 2 autovector ISR at Vector 26 ($0068)
            LEA     IRQ2_ISR(PC),A0
            MOVE.L  A0,$0068

            MOVEQ   #0,D6               ; ISR counter

            * Enable auto-IRQ Level 2 every 50 ms (task 32, subtask 6, D2=$82)
            MOVEQ   #32,D0
            MOVEQ   #6,D1
            MOVE.B  #$82,D2
            MOVEQ   #50,D3
            TRAP    #15

            * Wait for interrupt with mask 0 (supervisor mode SR = $2000)
            STOP    #$2000

            * Disable all auto-IRQs (task 32, subtask 6, D2=0)
            MOVEQ   #32,D0
            MOVEQ   #6,D1
            MOVEQ   #0,D2
            TRAP    #15

            SIMHALT

IRQ2_ISR:
            ADDQ.L  #1,D6
            RTE

            END     START
