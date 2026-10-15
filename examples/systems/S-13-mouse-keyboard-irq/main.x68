*-----------------------------------------------------------
* Title      : S-13 Mouse and Keyboard Interrupts
* Description: Peripheral IRQs (tasks 60-62) and MouseRead
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * Enable exceptions (task 32, subtask 5)
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15

            * Install Level 1 (Mouse, $0064) and Level 2 (Keyboard, $0068) ISRs
            LEA     MOUSE_ISR(PC),A0
            MOVE.L  A0,$0064
            LEA     KEY_ISR(PC),A0
            MOVE.L  A0,$0068

            * Enable Mouse IRQ on Level 1 for button down (D1.W = $0101, task 60)
            MOVEQ   #60,D0
            MOVE.W  #$0101,D1
            TRAP    #15

            * Enable Key IRQ on Level 2 for key down (D1.W = $0201, task 62)
            MOVEQ   #62,D0
            MOVE.W  #$0201,D1
            TRAP    #15

            * Query current mouse state via task 61 (mode 0)
            MOVEQ   #61,D0
            MOVEQ   #0,D1
            TRAP    #15
            MOVE.L  D0,D4               ; buttons (0 when idle)

            * Disable peripheral IRQs cleanly before halting
            MOVEQ   #60,D0
            MOVEQ   #0,D1
            TRAP    #15
            MOVEQ   #62,D0
            MOVEQ   #0,D1
            TRAP    #15

            MOVEQ   #1,D3
            SIMHALT

MOUSE_ISR:
            MOVEM.L D0-D1,-(A7)
            MOVEQ   #61,D0
            MOVEQ   #2,D1               ; read last down position
            TRAP    #15
            MOVEM.L (A7)+,D0-D1
            RTE

KEY_ISR:
            RTE

            END     START
