*-----------------------------------------------------------
* Solution S-14-EX1: Pack CID=3 SerialSend command and 9600-8N1 params
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            MOVE.L  #$0003002B,D0
            MOVE.L  #$00001805,D1
            SIMHALT
            END     START
