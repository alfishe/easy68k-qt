*-----------------------------------------------------------
* Title      : L3-10 Multi-File Projects: INCLUDE, INCBIN, and OFFSET
* Written by : EASy68K-Qt contributors
* Description: Use INCLUDE + OFFSET struct definitions and INCBIN data
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        INCLUDE 'defs.inc'

        ORG     $1000
START:
        LEA     POINT_DATA,A0
        MOVEQ   #0,D0
        MOVE.W  PT_X(A0),D0     ; 10 ($000A)
        ADD.W   PT_Y(A0),D0     ; + 20 ($0014) = 30
        ADD.L   PT_Z(A0),D0     ; + 40 ($00000028) = 70
        MOVEQ   #PT_SIZE,D1     ; struct size = 8 bytes
        SIMHALT

POINT_DATA:
        INCBIN  'payload.bin'

        END     START
