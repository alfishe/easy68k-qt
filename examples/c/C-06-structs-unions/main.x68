*-----------------------------------------------------------
* Title      : C-06 Structs, Padding, and OFFSET Views
* Description: Annotated 68000 assembly for main.c
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            OFFSET  0
SPR_ID:     DS.B    1
            DS.B    1                   ; 1-byte even alignment pad
SPR_X:      DS.W    1                   ; offset 2
SPR_Y:      DS.W    1                   ; offset 4
SPR_COLOR:  DS.L    1                   ; offset 6
SPR_SIZEOF: DS.W    0                   ; 10 bytes

            ORG     $1000
_start:
            LEA     $00FF0000,A7
            BSR.S   main
            SIMHALT

main:
            LEA     $2000,A0
            CLR.W   (A0)                ; clear id + pad byte
            MOVE.B  #7,SPR_ID(A0)
            MOVE.W  #120,SPR_X(A0)
            MOVE.W  #80,SPR_Y(A0)
            MOVE.L  #$00FF8000,SPR_COLOR(A0)
            MOVEQ   #SPR_SIZEOF,D0      ; return sizeof(struct Sprite) = 10
            RTS

            END     _start
