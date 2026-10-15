*-----------------------------------------------------------
* Title      : L3-11 Position-Independent Code (PIC)
* Written by : EASy68K-Qt contributors
* Description: Relocate a PC-relative routine to $3000 and call the copy
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        * Copy PIC_START..PIC_END to $3000
        LEA     PIC_START(PC),A0
        LEA     $3000,A1
        MOVE.W  #(PIC_END-PIC_START)/2-1,D1
.COPY:
        MOVE.W  (A0)+,(A1)+
        DBRA    D1,.COPY

        * Call the relocated routine at $3000!
        JSR     $3000
        SIMHALT

* Position-independent routine (contains no absolute addresses)
PIC_START:
        LEA     PIC_DATA(PC),A0
        MOVEQ   #0,D0
        ADD.L   (A0)+,D0
        ADD.L   (A0)+,D0
        ADD.L   (A0)+,D0
        RTS
PIC_DATA:
        DC.L    100,200,300
PIC_END:

        END     START
