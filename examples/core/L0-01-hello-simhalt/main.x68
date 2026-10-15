*-----------------------------------------------------------
* Title      : L0-01 Program Skeleton: ORG, START, END, SIMHALT
* Written by : EASy68K-Qt contributors
* Description: Minimal runnable 68000 program demonstrating ORG, NOP, SIMHALT
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        NOP                     ; 2-byte no-op instruction (4 cycles)
        SIMHALT                 ; halt simulator

        END     START
