* L3-09 Exercise 1 Solution
* SPDX-License-Identifier: CC0-1.0
USE_ADD EQU     0
        ORG     $1000
START:
        IFNE    USE_ADD
        MOVEQ   #10,D0
        ENDC
        IFEQ    USE_ADD
        MOVEQ   #50,D0
        ENDC
        SIMHALT
        END     START
