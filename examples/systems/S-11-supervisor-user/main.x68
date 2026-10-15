*-----------------------------------------------------------
* Title      : S-11 Supervisor vs User Mode
* Description: USP/SSP separation, Vector 8 privilege fault, TRAP
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * Enable exceptions (task 32, subtask 5)
            MOVEQ   #32,D0
            MOVEQ   #5,D1
            TRAP    #15

            * Install Vector 8 (Privilege Violation, $0020) and Vector 32 (TRAP #0, $0080)
            LEA     PRIV_ISR(PC),A0
            MOVE.L  A0,$0020
            LEA     SYSCALL_ISR(PC),A0
            MOVE.L  A0,$0080

            * Configure User Stack Pointer (USP = $00FE0000)
            LEA     $00FE0000,A0
            MOVE.L  A0,USP

            MOVEQ   #0,D5               ; privilege violation counter
            MOVEQ   #0,D6               ; syscall counter

            * Drop from Supervisor mode to User mode (clear S bit 13)
            ANDI.W  #$DFFF,SR

            * In User mode: A7 is now USP ($00FE0000)
            MOVE.L  #$ABCD1234,-(A7)    ; pushed at $00FDFFFC on USP

            * Attempt privileged instruction in User mode -> triggers Vector 8!
            ORI.W   #$2000,SR           ; 4-byte instruction skipped by PRIV_ISR

            * Legally request elevation via TRAP #0 system call
            TRAP    #0

            * Now back in Supervisor mode! Read what user pushed on USP
            MOVE.L  USP,A1
            MOVE.L  (A1),D4             ; D4 = $ABCD1234
            SIMHALT

PRIV_ISR:
            ADDQ.L  #1,D5               ; record privilege violation
            ADDQ.L  #4,2(A7)            ; skip 4-byte ORI.W #$2000,SR
            RTE

SYSCALL_ISR:
            ADDQ.L  #1,D6
            ORI.W   #$2000,(A7)         ; set S bit in stacked SR so RTE stays Supervisor
            RTE

            END     START
