*-----------------------------------------------------------
* Title      : L3-09 Conditional Assembly (IFEQ, IFNE, ENDC)
* Written by : EASy68K-Qt contributors
* Description: Compile-time feature toggles using IFNE and IFEQ
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
DEBUG       EQU     1
FAST_SHIFT  EQU     1

        ORG     $1000
START:
        MOVEQ   #9,D0

        IFNE    FAST_SHIFT
        LSL.L   #3,D0           ; D0 = 9 * 8 = 72
        ENDC

        IFEQ    FAST_SHIFT
        MULU.W  #8,D0
        ENDC

        IFNE    DEBUG
        MOVEQ   #1,D7           ; debug build marker in D7
        ENDC
        SIMHALT

        END     START
