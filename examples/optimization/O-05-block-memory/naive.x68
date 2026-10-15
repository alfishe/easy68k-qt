*-----------------------------------------------------------
* Title      : O-05 Naive Byte-by-Byte 32-Byte Copy
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     BLK,A0
            LEA     $2000,A1
            MOVEQ   #32-1,D0
.LP:        MOVE.B  (A0)+,(A1)+
            DBRA    D0,.LP
            SIMHALT
BLK:        DC.L    $11111111,$22222222,$33333333,$44444444
            DC.L    $55555555,$66666666,$77777777,$88888888
            END     START
