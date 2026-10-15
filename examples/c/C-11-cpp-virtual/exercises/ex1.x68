*-----------------------------------------------------------
* Exercise C-11-EX1: Virtual call through vptr at $2000
*-----------------------------------------------------------
            ORG     $1000
_start:
            LEA     $2000,A0
            MOVE.L  #vtable_Node,0(A0)
            MOVE.L  #10,4(A0)
            * TODO: Load vptr from 0(A0), load slot 0 from vtable, push A0,
            * and JSR to compute 3 * 10 = 30 in D0
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
