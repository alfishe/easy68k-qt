*-----------------------------------------------------------
* Solution C-05-EX1: Reverse 4 words at $2000 in-place
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            LEA     $2000,A0
            LEA     $2006,A1
.lp:        CMPA.L  A1,A0
            BHS.S   .done
            MOVE.W  (A0),D1
            MOVE.W  (A1),(A0)+
            MOVE.W  D1,(A1)
            SUBQ.L  #2,A1
            BRA.S   .lp
.done:      MOVEQ   #0,D0
            MOVE.W  $2000,D0
            SIMHALT
            ORG     $2000
            DC.W    10,20,30,40
            END     _start
