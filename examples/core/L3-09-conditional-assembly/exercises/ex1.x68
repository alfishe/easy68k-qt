* L3-09 Exercise 1: Toggle USE_ADD so D0 = 50
* SPDX-License-Identifier: CC0-1.0
USE_ADD EQU     0
        ORG     $1000
START:
        * TODO: use IFNE USE_ADD / ENDC and IFEQ USE_ADD / ENDC so D0 = 50
        SIMHALT
        END     START
