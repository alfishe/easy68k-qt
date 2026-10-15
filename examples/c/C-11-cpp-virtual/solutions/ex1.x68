*-----------------------------------------------------------
* Solution C-11-EX1: Virtual call through vptr at $2000
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            LEA     $2000,A0
            MOVE.L  #vtable_Node,0(A0)
            MOVE.L  #10,4(A0)
            MOVEA.L 0(A0),A1
            MOVEA.L 0(A1),A1
            MOVE.L  A0,-(A7)
            JSR     (A1)
            ADDQ.L  #4,A7
            SIMHALT

Node_eval:
            MOVEA.L 4(A7),A0
            MOVE.L  4(A0),D0
            MOVE.L  D0,D1
            ADD.L   D0,D0
            ADD.L   D1,D0
            RTS

vtable_Node:
            DC.L    Node_eval
            END     _start
