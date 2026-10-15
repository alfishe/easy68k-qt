*-----------------------------------------------------------
* Title      : P-05 Multitasking Mini-Kernel (TCBs + User/Supervisor + RTE)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
* TCB layout (16 bytes for lightweight task context):
*   +0: saved PC (long)
*   +4: saved SR (word) + pad (word)
*   +8: saved USP (long)
*  +12: saved D0 (long)

VEC_TRAP1   EQU     $0084

            ORG     $1000
START:
            * Install TRAP #1 kernel scheduler vector
            MOVE.L  #TRAP1_SCHED,VEC_TRAP1

            * Clear shared counters and kernel state
            CLR.L   CNT_TASK0
            CLR.L   CNT_TASK1
            CLR.L   SWITCH_CNT
            CLR.W   CUR_TASK

            * Initialize TCB0 (Task 0, User mode SR=$0000, USP=$6000)
            LEA     TCB0,A0
            MOVE.L  #TASK0_MAIN,0(A0)
            MOVE.W  #$0000,4(A0)
            MOVE.L  #$6000,8(A0)
            MOVE.L  #3,12(A0)           ; 3 time slices

            * Initialize TCB1 (Task 1, User mode SR=$0000, USP=$7000)
            LEA     TCB1,A0
            MOVE.L  #TASK1_MAIN,0(A0)
            MOVE.W  #$0000,4(A0)
            MOVE.L  #$7000,8(A0)
            MOVE.L  #3,12(A0)           ; 3 time slices

            * Launch Task 0 in User mode via RTE frame
            LEA     TCB0,A0
            MOVEA.L 8(A0),A1
            MOVE.L  A1,USP
            MOVE.L  12(A0),D0
            MOVE.L  0(A0),-(A7)         ; Push initial PC
            MOVE.W  4(A0),-(A7)         ; Push initial SR ($0000 = User mode)
            RTE

*--- User Task 0: adds 10 to CNT_TASK0 each slice, then yields via TRAP #1 ---
TASK0_MAIN:
            ADDI.L  #10,CNT_TASK0
            SUBQ.L  #1,D0
            TRAP    #1                  ; Yield to kernel scheduler
            BRA.S   TASK0_MAIN

*--- User Task 1: adds 7 to CNT_TASK1 each slice, then yields via TRAP #1 ---
TASK1_MAIN:
            ADDI.L  #7,CNT_TASK1
            SUBQ.L  #1,D0
            TRAP    #1                  ; Yield to kernel scheduler
            BRA.S   TASK1_MAIN

*--- Kernel TRAP #1 Scheduler (runs in Supervisor mode) ---
TRAP1_SCHED:
            ADDQ.L  #1,SWITCH_CNT
            CMPI.L  #6,SWITCH_CNT
            BGE.S   KERNEL_DONE

            * Save outgoing task state into its TCB
            TST.W   CUR_TASK
            BNE.S   .SAVE_T1
            LEA     TCB0,A0
            MOVE.W  #1,CUR_TASK
            LEA     TCB1,A1
            BRA.S   .DO_SWITCH
.SAVE_T1:
            LEA     TCB1,A0
            CLR.W   CUR_TASK
            LEA     TCB0,A1

.DO_SWITCH:
            MOVE.W  0(A7),4(A0)         ; Save outgoing SR
            MOVE.L  2(A7),0(A0)         ; Save outgoing PC
            MOVE.L  D0,12(A0)           ; Save outgoing D0
            MOVE.L  USP,A2
            MOVE.L  A2,8(A0)            ; Save outgoing USP

            * Restore incoming task from (A1)
            MOVEA.L 8(A1),A2
            MOVE.L  A2,USP              ; Restore incoming USP
            MOVE.L  12(A1),D0           ; Restore incoming D0
            MOVE.W  4(A1),0(A7)         ; Overwrite stacked SR
            MOVE.L  0(A1),2(A7)         ; Overwrite stacked PC
            RTE

KERNEL_DONE:
            MOVE.L  CNT_TASK0,D0        ; D0 = 30
            MOVE.L  CNT_TASK1,D1        ; D1 = 21
            MOVE.L  SWITCH_CNT,D2       ; D2 = 6
            SIMHALT

            ORG     $2000
CNT_TASK0:  DS.L    1
CNT_TASK1:  DS.L    1
SWITCH_CNT: DS.L    1
CUR_TASK:   DS.W    1
            DS.W    0
TCB0:       DS.B    16
TCB1:       DS.B    16
            END     START
