*-----------------------------------------------------------
* Title      : C-04 Reading Compiler Output (-O0 vs -O2)
* Description: Compares -O0 stack-local codegen vs -O2 register codegen
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            LEA     $00FF0000,A7

            * Measure -O0 version
            MOVEQ   #30,D0
            TRAP    #15
            MOVE.L  #6,-(A7)
            BSR.S   sum_sq_O0
            ADDQ.L  #4,A7
            MOVE.L  D0,D4               ; result = 91
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D2               ; D2 = -O0 cycles

            * Measure -O2 version
            MOVEQ   #30,D0
            TRAP    #15
            MOVE.L  #6,-(A7)
            BSR.S   sum_sq_O2
            ADDQ.L  #4,A7
            MOVE.L  D0,D5               ; result = 91
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,D3               ; D3 = -O2 cycles (< D2)

            MOVE.L  D5,D0               ; return 91 in D0
            SIMHALT

* GCC -O0 style: frame pointer + stack locals -4(A6)=sum, -6(A6)=i
sum_sq_O0:
            LINK    A6,#-8
            CLR.L   -4(A6)              ; sum = 0
            MOVE.W  #1,-6(A6)           ; i = 1
.L_O0_chk:  MOVE.W  -6(A6),D0
            CMP.W   10(A6),D0
            BGT.S   .L_O0_done
            MOVE.W  -6(A6),D0
            MULS.W  -6(A6),D0
            ADD.L   D0,-4(A6)
            ADDQ.W  #1,-6(A6)
            BRA.S   .L_O0_chk
.L_O0_done: MOVE.L  -4(A6),D0
            UNLK    A6
            RTS

* GCC -O2 style: no frame pointer, all values in registers + DBRA
sum_sq_O2:
            MOVE.W  6(A7),D1            ; n = 6
            MOVEQ   #0,D0               ; sum = 0
            SUBQ.W  #1,D1               ; DBRA count = n - 1
.L_O2_lp:   MOVE.W  D1,D6
            ADDQ.W  #1,D6               ; k = D1 + 1
            MULS.W  D6,D6
            ADD.L   D6,D0
            DBRA    D1,.L_O2_lp
            RTS

            END     _start
