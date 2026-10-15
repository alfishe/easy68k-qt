*-----------------------------------------------------------
* Title      : C-08 Mixing C and 68000 Assembly
* Description: C calling asm_popcount32 and asm calling c_double
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            LEA     $00FF0000,A7
            BSR.S   main
            SIMHALT

main:
            MOVE.L  D2,-(A7)
            MOVE.L  #$F0F055AA,-(A7)
            BSR.S   asm_popcount32
            ADDQ.L  #4,A7
            MOVE.L  D0,D2               ; D2 = 16

            MOVE.L  #21,-(A7)
            BSR.S   asm_call_c_double
            ADDQ.L  #4,A7
            ADD.L   D2,D0               ; D0 = 16 + 42 = 58
            MOVE.L  (A7)+,D2
            RTS

* uint32_t asm_popcount32(uint32_t x) — leaf function using scratch D0,D1
asm_popcount32:
            MOVE.L  4(A7),D1
            MOVEQ   #0,D0
.pc_lp:     TST.L   D1
            BEQ.S   .pc_done
.pc_k:      TST.L   D1
            BEQ.S   .pc_done
            ADDQ.L  #1,D0
            MOVE.L  D1,-(A7)
            SUBQ.L  #1,D1
            AND.L   (A7)+,D1            ; D1 &= (D1 - 1) clears lowest set bit
            BRA.S   .pc_k
.pc_done:   RTS

* int32_t asm_call_c_double(int32_t x) — calls C function c_double(x)
asm_call_c_double:
            MOVE.L  4(A7),-(A7)
            BSR.S   c_double
            ADDQ.L  #4,A7
            RTS

* int32_t c_double(int32_t x)
c_double:
            MOVE.L  4(A7),D0
            ADD.L   D0,D0
            RTS

            END     _start
