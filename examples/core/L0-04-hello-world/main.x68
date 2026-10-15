*-----------------------------------------------------------
* Title      : L0-04 Hello World with Trap #15 Text I/O
* Written by : EASy68K-Qt contributors
* Description: Print strings using LEA and Trap #15 tasks 14 and 13
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        LEA     PART1,A1        ; A1 = address of PART1
        MOVEQ   #14,D0          ; task 14: display null-terminated string (no CRLF)
        TRAP    #15

        LEA     PART2,A1        ; A1 = address of PART2
        MOVEQ   #13,D0          ; task 13: display null-terminated string with CRLF
        TRAP    #15

        SIMHALT

PART1   DC.B    'Hello, ',0
PART2   DC.B    '68000 World!',0

        END     START
