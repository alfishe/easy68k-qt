*-----------------------------------------------------------
* Title      : O-05 Block Memory Copy (Byte Loop vs MOVEM.L)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * 1) Naive 32-byte copy with MOVE.B -> cycles in D6
            MOVEQ   #30,D0
            TRAP    #15
            LEA     BLK(PC),A0
            LEA     $2000,A1
            MOVEQ   #32-1,D0
.NLP:       MOVE.B  (A0)+,(A1)+
            DBRA    D0,.NLP
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,$2100            ; save naive cycles at $2100

            * 2) Optimised 32-byte copy with MOVEM.L D0-D7 -> cycles at $2104
            MOVEQ   #30,D0
            TRAP    #15
            LEA     BLK(PC),A0
            LEA     $2000,A1
            MOVEM.L (A0)+,D0-D7
            MOVEM.L D0-D7,(A1)
            MOVEQ   #31,D0
            TRAP    #15
            MOVE.L  D1,$2104            ; save MOVEM.L cycles at $2104

            MOVE.L  $201C,D0            ; last longword = $88888888
            SIMHALT

BLK:        DC.L    $11111111,$22222222,$33333333,$44444444
            DC.L    $55555555,$66666666,$77777777,$88888888
            END     START
