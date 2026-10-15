*-----------------------------------------------------------
* Title      : C-03 m68k C Calling Convention
* Description: Annotated 68000 assembly for main.c
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            LEA     $00FF0000,A7
            MOVE.L  #$11223344,D2       ; sentinel in callee-saved D2
            BSR.S   main
            SIMHALT

main:
            LINK    A6,#0
            MOVE.L  #30,-(A7)           ; arg 3: c = 30
            MOVE.L  #20,-(A7)           ; arg 2: b = 20
            MOVE.L  #10,-(A7)           ; arg 1: a = 10
            BSR.S   weighted_sum
            LEA     12(A7),A7           ; caller cleans 3 longwords
            UNLK    A6
            RTS

weighted_sum:
            LINK    A6,#0
            MOVE.L  D2,-(A7)            ; save callee-saved D2
            MOVE.L  8(A6),D0            ; a
            ADD.L   D0,D0               ; a * 2
            MOVE.L  12(A6),D1           ; b
            MOVE.L  D1,D2
            ADD.L   D1,D1
            ADD.L   D2,D1               ; b * 3
            ADD.L   D1,D0
            MOVE.L  16(A6),D1           ; c
            LSL.L   #2,D1               ; c * 4
            ADD.L   D1,D0               ; return in D0 = 200
            MOVE.L  (A7)+,D2            ; restore D2
            UNLK    A6
            RTS

            END     _start
