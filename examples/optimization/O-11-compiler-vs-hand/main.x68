*-----------------------------------------------------------
* Title      : O-11 Compiler vs Hand Assembly (Sum + Max)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * 1) Compiler-style indexed loop -> D4 cycles
            MOVEQ   #30,D0
            TRAP    #15
            LEA     DATA(PC),A0
            MOVEQ   #0,D0
            MOVE.W  (A0),D1
            MOVEQ   #0,D2
.CLP:       CMPI.W  #8,D2
            BGE.S   .CDONE
            MOVE.W  D2,D3
            ADD.W   D3,D3
            MOVE.W  0(A0,D3.W),D6
            ADD.W   D6,D0
            CMP.W   D1,D6
            BLE.S   .CNEXT
            MOVE.W  D6,D1
.CNEXT:     ADDQ.W  #1,D2
            BRA.S   .CLP
.CDONE:     MOVE.L  D0,D6               ; save sum (300)
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D4               ; compiler-style cycles

            * 2) Hand-tuned (A0)+ / DBRA loop -> D5 cycles
            MOVEQ   #30,D0
            TRAP    #15
            LEA     DATA(PC),A0
            MOVE.W  (A0)+,D0            ; sum = data[0]
            MOVE.W  D0,D2               ; max = data[0]
            MOVEQ   #7-1,D3             ; remaining 7 elements
.HLP:       MOVE.W  (A0)+,D7
            ADD.W   D7,D0
            CMP.W   D2,D7
            BLE.S   .HNEXT
            MOVE.W  D7,D2
.HNEXT:     DBRA    D3,.HLP
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D5               ; hand-tuned cycles (< D4)

            MOVE.L  D6,D0               ; D0 = 300 (sum)
            MOVEQ   #0,D1
            MOVE.W  D2,D1               ; D1 = 89  (max)
            SIMHALT

DATA:       DC.W    12,45,7,89,23,56,3,65
            END     START
