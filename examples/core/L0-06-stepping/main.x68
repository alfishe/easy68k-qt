*-----------------------------------------------------------
* Title      : L0-06 Stepping, Tracing, Breakpoints, and Rewind
* Written by : EASy68K-Qt contributors
* Description: Practice Trace Into (F7) vs Step Over (F8) on BSR calls
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        MOVEQ   #5,D0           ; start with D0 = 5
        BSR.S   ADD_TEN         ; try F7 (Trace Into) here: D0 becomes 15
        BSR.S   ADD_TEN         ; try F8 (Step Over) here: D0 becomes 25
        MOVE.L  D0,D1           ; breakpoint target: D1 = 25
        SIMHALT

ADD_TEN:
        ADDQ.L  #8,D0
        ADDQ.L  #2,D0
        RTS

        END     START
