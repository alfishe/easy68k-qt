*-----------------------------------------------------------
* Title      : S-14 Serial I/O
* Description: COM port CID addressing and parameter packing (tasks 40-43)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * Initialise CID 1 (D0 high word = 1, task = 40)
            MOVE.L  #$00010028,D0       ; CID=1, task 40 ($28)
            LEA     PORT_NAME(PC),A1
            TRAP    #15

            * Configure 9600 baud (5), no parity (0), 8 data bits (3<<11 = $1800), 1 stop bit
            MOVE.L  #$00010029,D0       ; CID=1, task 41 ($29)
            MOVE.L  #$00001805,D1       ; 9600-8N1
            MOVE.L  D1,D2               ; save config word in D2
            TRAP    #15

            * Send 4 bytes from TX_BUF on CID 1 (task 43 = $2B)
            MOVE.L  #$0001002B,D0
            LEA     TX_BUF(PC),A1
            MOVEQ   #4,D1
            TRAP    #15

            SIMHALT

PORT_NAME:  DC.B    'COM1',0
TX_BUF:     DC.B    'PING',0
            DS.W    0
            END     START
