* L3-11 Exercise 1: Load 2nd word (250) of TBL using TBL(PC,D1.W)
* SPDX-License-Identifier: CC0-1.0
        ORG     $1000
START:
        MOVEQ   #0,D0
        MOVEQ   #2,D1
        * TODO: load TBL(PC,D1.W) into D0.W
        SIMHALT

TBL     DC.W    150,250,350
        END     START
