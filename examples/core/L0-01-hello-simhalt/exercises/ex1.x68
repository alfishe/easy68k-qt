* L0-01 Exercise 1: Execute two NOP instructions before SIMHALT
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        NOP
        * TODO: add a second NOP here
        SIMHALT
        END     START
