* L2-04 Exercise 1: Compute 4^3 = 64 using a CUBE subroutine
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #4,D0
        BSR.S   CUBE
        SIMHALT

CUBE:
        * TODO: multiply D0 by itself twice and return
        RTS
        END     START
