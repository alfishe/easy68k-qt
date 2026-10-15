*-----------------------------------------------------------
* Title      : P-03 TRAP-Based Mini Monitor & Debugger
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
VEC_TRAP0   EQU     $0080

            ORG     $1000
START:
            * Install TRAP #0 monitor service handler
            MOVE.L  #TRAP0_ISR,VEC_TRAP0

.CMD_LOOP:
            LEA     CMD_BUF,A1
            MOVEQ   #2,D0               ; Read command line into (A1)
            TRAP    #15

            LEA     CMD_BUF,A0
            MOVE.B  (A0)+,D2
            CMP.B   #'W',D2
            BEQ.S   .CMD_WRITE
            CMP.B   #'M',D2
            BEQ.S   .CMD_DUMP
            CMP.B   #'Q',D2
            BEQ.S   .CMD_QUIT
            BRA.S   .CMD_LOOP

.CMD_WRITE:
            BSR.S   SKIP_SP
            BSR.S   PARSE_HEX           ; D3 = address
            MOVEA.L D3,A1
            BSR.S   SKIP_SP
            BSR.S   PARSE_HEX           ; D3 = byte value
            MOVE.B  D3,D1
            MOVEQ   #1,D0               ; TRAP #0 fn 1: write byte D1 to (A1)
            TRAP    #0
            BRA.S   .CMD_LOOP

.CMD_DUMP:
            BSR.S   SKIP_SP
            BSR.S   PARSE_HEX           ; D3 = address
            MOVEA.L D3,A1
            MOVE.L  D3,D1
            MOVEQ   #16,D2
            MOVEQ   #15,D0              ; Print address in base 16
            TRAP    #15
            LEA     COLON_SP(PC),A1
            MOVEQ   #14,D0
            TRAP    #15

            MOVEA.L D3,A1
            MOVEQ   #0,D0               ; TRAP #0 fn 0: read word at (A1) -> D1.W
            TRAP    #0
            ANDI.L  #$FFFF,D1
            MOVE.L  D1,D6               ; Save last inspected word in D6
            MOVEQ   #16,D2
            MOVEQ   #15,D0              ; Print word in base 16
            TRAP    #15
            LEA     CRLF(PC),A1
            MOVEQ   #14,D0
            TRAP    #15
            BRA.S   .CMD_LOOP

.CMD_QUIT:
            MOVE.L  D6,D0               ; Return last inspected word in D0
            SIMHALT

*--- Helper: advance A0 past spaces ---
SKIP_SP:
            CMP.B   #' ',(A0)
            BNE.S   .RTS
            ADDQ.L  #1,A0
            BRA.S   SKIP_SP
.RTS:       RTS

*--- Helper: parse hex token at (A0) into D3.L ---
PARSE_HEX:
            MOVEQ   #0,D3
.HLP:       MOVE.B  (A0),D4
            BEQ.S   .HDONE
            CMP.B   #' ',D4
            BEQ.S   .HDONE
            ADDQ.L  #1,A0
            CMP.B   #'9',D4
            BLE.S   .HDIG
            SUBI.B  #'A'-10,D4
            BRA.S   .HACC
.HDIG:      SUBI.B  #'0',D4
.HACC:      EXT.W   D4
            EXT.L   D4
            ASL.L   #4,D3
            OR.L    D4,D3
            BRA.S   .HLP
.HDONE:     RTS

*--- TRAP #0 Handler: D0=0 read word (A1)->D1, D0=1 write byte D1->(A1) ---
TRAP0_ISR:
            TST.W   D0
            BNE.S   .T_WRITE
            MOVE.W  (A1),D1
            RTE
.T_WRITE:
            MOVE.B  D1,(A1)
            RTE

COLON_SP:   DC.B    ': ',0
CRLF:       DC.B    13,10,0
            DS.W    0
CMD_BUF:    DS.B    84
            END     START
