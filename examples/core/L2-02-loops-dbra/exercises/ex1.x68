* L2-02 Exercise 1: Compute 2^8 = 256 in D0 using a DBRA loop
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #1,D0
        * TODO: loop 8 times doubling D0 with DBRA
        SIMHALT
        END     START
