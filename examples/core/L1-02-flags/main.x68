*-----------------------------------------------------------
* Title      : L1-02 Condition Codes (CCR): X, N, Z, V, C
* Written by : EASy68K-Qt contributors
* Description: Observe X, N, Z, V, C flags on ADD.B and CMP.B
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        * Case 1: $FF + 1 -> $00 (X=1, N=0, Z=1, V=0, C=1 -> CCR = $15)
        MOVEQ   #0,D3
        MOVE.B  #$FF,D0
        ADDQ.B  #1,D0
        MOVE.W  SR,D3
        ANDI.W  #$001F,D3       ; isolate XNZVC ($0015)

        * Case 2: $7F + 1 -> $80 (X=0, N=1, Z=0, V=1, C=0 -> CCR = $0A)
        MOVEQ   #0,D4
        MOVE.B  #$7F,D1
        ADDQ.B  #1,D1
        MOVE.W  SR,D4
        ANDI.W  #$001F,D4       ; isolate XNZVC ($000A)

        * Case 3: CMP.B #5,D2 when D2=5 (N=0, Z=1, V=0, C=0 -> NZVC = $04)
        MOVEQ   #0,D5
        MOVEQ   #5,D2
        CMP.B   #5,D2
        MOVE.W  SR,D5
        ANDI.W  #$000F,D5       ; isolate NZVC ($0004)
        SIMHALT

        END     START
