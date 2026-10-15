*-----------------------------------------------------------
* Title      : L0-05 Reading the .L68 Listing and S-Records
* Written by : EASy68K-Qt contributors
* Description: Inspect EQU constants, instruction sizes, and symbol table
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
COUNT   EQU     5
BUFFER  EQU     $2000

        ORG     $1000
START:
        MOVEQ   #COUNT,D0       ; 2 bytes: 7005
        LEA     BUFFER,A0       ; 6 bytes: 41F9 00002000
        MOVE.W  #$CAFE,(A0)     ; 4 bytes: 30BC CAFE
        SIMHALT

        END     START
