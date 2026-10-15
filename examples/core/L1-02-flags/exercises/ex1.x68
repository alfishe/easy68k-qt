* L1-02 Exercise 1: Capture XNZVC after 0 - 1 (byte) into D1
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #0,D0
        MOVEQ   #0,D1
        * TODO: subtract 1 from D0.B and store SR & $001F in D1
        SIMHALT
        END     START
