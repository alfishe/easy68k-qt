*-----------------------------------------------------------
* Solution O-05-EX1: Copy 32 bytes from BLK to $2000 in <= 190 cycles
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            LEA     BLK(PC),A0
            LEA     $2000,A1
            MOVEM.L (A0)+,D0-D7
            MOVEM.L D0-D7,(A1)
            SIMHALT
BLK:        DC.L    $11111111,$22222222,$33333333,$44444444
            DC.L    $55555555,$66666666,$77777777,$88888888
            END     START
