*-----------------------------------------------------------
* Title      : L0-03 Memory Layout, DC/DS, and Big-Endian Byte Order
* Written by : EASy68K-Qt contributors
* Description: Demonstrate big-endian byte order and DC/DS directives
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        MOVE.L  INITVAL,D0      ; D0 = $AABBCCDD from DC.L constant
        MOVE.L  D0,$2000        ; store $AA $BB $CC $DD at $2000..$2003
        MOVEQ   #0,D1
        MOVE.W  $2000,D1        ; high word at $2000 = $AABB
        MOVEQ   #0,D2
        MOVE.B  $2003,D2        ; lowest-significance byte at $2003 = $DD
        SIMHALT

INITVAL DC.L    $AABBCCDD

        END     START
