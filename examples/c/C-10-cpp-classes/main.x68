*-----------------------------------------------------------
* Title      : C-10 Freestanding C++17 Classes and RAII
* Description: Annotated 68000 assembly for main.cpp
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            LEA     $00FF0000,A7
            BSR.S   main
            SIMHALT

main:
            LINK    A6,#-16
            * ScopedLed guard(0xA5, 0x5A): writes $A5 to LEDs on entry
            MOVE.B  #$A5,$00E00010
            MOVE.B  #$5A,-2(A6)         ; guard.exit_val_ = $5A

            * Vec2D a(12, 5) at -6(A6), b(3, 4) at -10(A6), c(5, 6) at -14(A6)
            MOVE.W  #12,-6(A6)
            MOVE.W  #5,-4(A6)
            MOVE.W  #3,-10(A6)
            MOVE.W  #4,-8(A6)
            MOVE.W  #5,-14(A6)
            MOVE.W  #6,-12(A6)

            * a.manhattan(&a) -> 17
            PEA     -6(A6)              ; push this = &a
            BSR.S   Vec2D_manhattan
            ADDQ.L  #4,A7
            MOVE.L  D0,D2               ; D2 = 17

            * b.dot(&b, &c) -> 3*5 + 4*6 = 39
            PEA     -14(A6)             ; push &c
            PEA     -10(A6)             ; push this = &b
            BSR.S   Vec2D_dot
            ADDQ.L  #8,A7
            ADD.L   D2,D0               ; D0 = 17 + 39 = 56

            * ~ScopedLed() runs automatically at scope exit
            MOVE.B  -2(A6),$00E00010    ; writes $5A to $E00010
            UNLK    A6
            RTS

* int32_t Vec2D::manhattan(const Vec2D* this)
Vec2D_manhattan:
            MOVEA.L 4(A7),A0            ; A0 = this
            MOVE.W  0(A0),D0            ; this->x_
            EXT.L   D0
            MOVE.W  2(A0),D1            ; this->y_
            EXT.L   D1
            ADD.L   D1,D0
            RTS

* int32_t Vec2D::dot(const Vec2D* this, const Vec2D* other)
Vec2D_dot:
            MOVEA.L 4(A7),A0            ; A0 = this
            MOVEA.L 8(A7),A1            ; A1 = other
            MOVE.W  0(A0),D0
            MULS.W  0(A1),D0            ; x_ * other.x_
            MOVE.W  2(A0),D1
            MULS.W  2(A1),D1            ; y_ * other.y_
            ADD.L   D1,D0
            RTS

            END     _start
