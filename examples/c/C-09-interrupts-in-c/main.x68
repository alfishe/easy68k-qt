*-----------------------------------------------------------
* Title      : C-09 Interrupts in C
* Description: Assembly ISR trampoline calling c_timer_isr()
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            LEA     $00FF0000,A7
            CLR.L   g_ticks

            * Enable exceptions (task 32, subtask 5)
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15

            * Install Level 2 autovector trampoline at $0068
            LEA     irq2_trampoline(PC),A0
            MOVE.L  A0,$0068

            * Enable auto-IRQ Level 2 (task 32, subtask 6, D2.B = $82)
            MOVEQ   #32,D0
            MOVEQ   #6,D1
            MOVE.B  #$82,D2
            MOVEQ   #50,D3
            TRAP    #15

            * Wait for interrupt
            STOP    #$2000

            * Disable auto-IRQ
            MOVEQ   #32,D0
            MOVEQ   #6,D1
            MOVEQ   #0,D2
            TRAP    #15

            * Read volatile g_ticks in a critical section (mask IRQs)
            MOVE.W  SR,D1
            ORI.W   #$0700,SR
            MOVE.L  g_ticks,D0
            MOVE.W  D1,SR
            SIMHALT

* Assembly trampoline saves C caller-saved registers (D0-D1/A0-A1)
irq2_trampoline:
            MOVEM.L D0-D1/A0-A1,-(A7)
            BSR.S   c_timer_isr
            MOVEM.L (A7)+,D0-D1/A0-A1
            RTE

* C function: void c_timer_isr(void)
c_timer_isr:
            ADDQ.L  #1,g_ticks
            RTS

            ORG     $2000
g_ticks:    DS.L    1
            END     _start
