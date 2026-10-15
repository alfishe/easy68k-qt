*-----------------------------------------------------------
* Title      : S-15 Network I/O
* Description: UDP/TCP socket configuration (tasks 100-107)
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
            ORG     $1000
START:
            * Pack UDP (0) on port 8080 ($1F90) into D1.L: $1F900000
            MOVE.L  #$1F900000,D1
            MOVE.L  D1,D2               ; save packed config in D2

            * Create UDP client to 127.0.0.1:8080 (task 100)
            LEA     LOOPBACK_IP(PC),A1
            MOVEQ   #100,D0
            TRAP    #15

            * Query local IP into $2000 (task 105)
            LEA     $2000,A1
            MOVEQ   #105,D0
            TRAP    #15

            * Close network connection (task 104)
            MOVEQ   #104,D0
            TRAP    #15

            SIMHALT

LOOPBACK_IP:
            DC.B    '127.0.0.1',0
            DS.W    0
            END     START
