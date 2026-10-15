*-----------------------------------------------------------
* Title      : L3-01 All 68000 Addressing Modes Side by Side
* Written by : EASy68K-Qt contributors
* Description: Exercise register, indirect, postinc, predec, disp, index, PC-rel
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        MOVEQ   #0,D0           ; accumulation register
        LEA     TABLE,A0        ; A0 -> TABLE (10, 20, 30, 40)

        ADD.W   (A0),D0         ; (An) indirect:         +10 -> 10
        ADD.W   (A0)+,D0        ; (An)+ post-increment:  +10 -> 20 (A0 -> TABLE+2)
        ADD.W   (A0)+,D0        ; (An)+ post-increment:  +20 -> 40 (A0 -> TABLE+4)
        ADD.W   -(A0),D0        ; -(An) pre-decrement:   +20 -> 60 (A0 -> TABLE+2)
        ADD.W   2(A0),D0        ; d16(An) displacement:  +30 -> 90

        MOVEQ   #6,D1           ; byte index 6 -> 4th word (40)
        LEA     TABLE,A0
        ADD.W   0(A0,D1.W),D0   ; d8(An,Xn) indexed:     +40 -> 130

        ADD.W   TABLE(PC),D0    ; d16(PC) PC-relative:   +10 -> 140
        SIMHALT

TABLE   DC.W    10,20,30,40

        END     START
