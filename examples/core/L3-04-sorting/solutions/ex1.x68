* L3-04 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVE.W  #30,$2000
        MOVE.W  #10,$2002
        MOVE.W  $2000,D0
        MOVE.W  $2002,D1
        CMP.W   D1,D0
        BLE.S   .DONE
        MOVE.W  D1,$2000
        MOVE.W  D0,$2002
.DONE:
        SIMHALT
        END     START
