*-----------------------------------------------------------
* Solution C-03-EX1: clamp_val(150, 0, 100) -> 100
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            MOVE.L  #100,-(A7)
            MOVE.L  #0,-(A7)
            MOVE.L  #150,-(A7)
            BSR.S   clamp_val
            LEA     12(A7),A7
            SIMHALT

clamp_val:
            LINK    A6,#0
            MOVE.L  8(A6),D0
            CMP.L   12(A6),D0
            BGE.S   .chk_hi
            MOVE.L  12(A6),D0
            BRA.S   .done
.chk_hi:    CMP.L   16(A6),D0
            BLE.S   .done
            MOVE.L  16(A6),D0
.done:      UNLK    A6
            RTS
            END     _start
