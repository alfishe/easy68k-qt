*-----------------------------------------------------------
* Solution P-02-EX1: Step PADDLE_X from 20 to TARGET_X=40 (+4/frame, 5 frames)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVEQ   #20,D0
            MOVEQ   #0,D1
            MOVEQ   #5-1,D2
.LP:        ADDQ.W  #4,D0
            DBRA    D2,.LP
            CMPI.W  #40,D0
            BNE.S   .DONE
            MOVEQ   #1,D1
.DONE:      SIMHALT
            END     START
