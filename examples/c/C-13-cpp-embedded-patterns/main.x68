*-----------------------------------------------------------
* Title      : C-13 Embedded C++ Patterns (Placement new + CRTP)
* Description: Annotated 68000 assembly for main.cpp
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
_start:
            LEA     $00FF0000,A7
            BSR.S   main
            SIMHALT

main:
            * Placement new at $2000: DeviceState(0x68000C13, 0xC3, 0x07)
            LEA     $2000,A0
            MOVE.L  #$68000C13,0(A0)    ; dev->magic
            MOVE.B  #$C3,4(A0)          ; dev->led_val
            MOVE.B  #$07,5(A0)          ; dev->seg7_val

            * CRTP drivers inline directly to single MOVE.B instructions!
            MOVE.B  5(A0),$00E00000     ; Seg7Driver::write_impl ($07)
            MOVE.B  4(A0),$00E00010     ; LedDriver::write_impl  ($C3)

            MOVEQ   #0,D0
            MOVE.B  4(A0),D0            ; return $C3 (195)
            RTS

            END     _start
