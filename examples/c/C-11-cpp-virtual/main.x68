*-----------------------------------------------------------
* Title      : C-11 C++ Virtual Dispatch and Vtables
* Description: Annotated 68000 assembly for main.cpp
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            LEA     $00FF0000,A7
            BSR.S   main
            SIMHALT

main:
            * Construct Rect r(6, 7) at $2000: [0]=vptr, [4]=w(6), [6]=h(7)
            LEA     $2000,A0
            MOVE.L  #vtable_Rect,0(A0)
            MOVE.W  #6,4(A0)
            MOVE.W  #7,6(A0)

            * Construct Square sq(5) at $2008: [0]=vptr, [4]=side(5), [6]=pad(0)
            LEA     $2008,A0
            MOVE.L  #vtable_Square,0(A0)
            MOVE.W  #5,4(A0)
            CLR.W   6(A0)

            * call_area(&r) via virtual dispatch -> 42
            PEA     $2000
            BSR.S   call_area
            ADDQ.L  #4,A7
            MOVE.L  D0,D2               ; D2 = 42

            * call_area(&sq) via virtual dispatch -> 25
            PEA     $2008
            BSR.S   call_area
            ADDQ.L  #4,A7
            ADD.L   D2,D0               ; D0 = 42 + 25 = 67
            RTS

* int32_t call_area(const Shape* s):
*   A0 = s
*   A1 = *(void**)s        (load vptr from offset 0)
*   A1 = ((fn**)A1)[0]     (load slot 0 from vtable)
*   JSR (A1)
call_area:
            MOVEA.L 4(A7),A0            ; A0 = s (this)
            MOVEA.L 0(A0),A1            ; A1 = s->vptr
            MOVEA.L 0(A1),A1            ; A1 = vtable[0] (&area)
            MOVE.L  A0,-(A7)            ; push this
            JSR     (A1)                ; indirect virtual call!
            ADDQ.L  #4,A7
            RTS

Rect_area:
            MOVEA.L 4(A7),A0
            MOVE.W  4(A0),D0
            MULS.W  6(A0),D0
            RTS

Square_area:
            MOVEA.L 4(A7),A0
            MOVE.W  4(A0),D0
            MULS.W  D0,D0
            RTS

            DS.W    0
vtable_Rect:
            DC.L    Rect_area
vtable_Square:
            DC.L    Square_area

            END     _start
