*-----------------------------------------------------------
* Title      : L3-03 Strings: Length, Copy, and In-Place Uppercase
* Written by : EASy68K-Qt contributors
* Description: Compute strlen, strcpy to $2000, and uppercase in-place
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        * 1. Copy SRC to $2000
        LEA     SRC,A0
        LEA     $2000,A1
.COPY:
        MOVE.B  (A0)+,(A1)+
        BNE.S   .COPY

        * 2. Uppercase $2000 in-place and count length in D0
        LEA     $2000,A0
        MOVEQ   #0,D0
.UP:
        MOVE.B  (A0),D1
        BEQ.S   .DONE
        ADDQ.L  #1,D0
        CMP.B   #'a',D1
        BLT.S   .SKIP
        CMP.B   #'z',D1
        BGT.S   .SKIP
        SUBI.B  #$20,D1
        MOVE.B  D1,(A0)
.SKIP:
        ADDQ.L  #1,A0
        BRA.S   .UP
.DONE:
        LEA     $2000,A1
        MOVEQ   #14,D0          ; print uppercased string
        TRAP    #15
        MOVEQ   #11,D0          ; restore D0 = length (11)
        SIMHALT

SRC     DC.B    'm68k rocks!',0

        END     START
