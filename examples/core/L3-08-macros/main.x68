*-----------------------------------------------------------
* Title      : L3-08 Assembler Macros: MACRO, ENDM, and \@ Labels
* Written by : EASy68K-Qt contributors
* Description: Define a CLAMP macro using \0 size, \1..\3 args, and \@ labels
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
* Clamp register \1 between low bound \2 and high bound \3
CLAMP   MACRO
        CMP.\0  \2,\1
        BGE.S   .LOW_OK\@
        MOVE.\0 \2,\1
        BRA.S   .END\@
.LOW_OK\@:
        CMP.\0  \3,\1
        BLE.S   .END\@
        MOVE.\0 \3,\1
.END\@:
        ENDM

        ORG     $1000
START:
        MOVE.L  #150,D0
        CLAMP.L D0,#0,#100      ; D0 clamped to 100

        MOVEQ   #-5,D1
        CLAMP.L D1,#0,#100      ; D1 clamped to 0

        MOVEQ   #42,D2
        CLAMP.L D2,#0,#100      ; D2 stays 42
        SIMHALT

        END     START
