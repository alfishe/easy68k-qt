*-----------------------------------------------------------
* Title      : L1-01 Operation Sizes (.B, .W, .L) and Sign Extension (EXT)
* Written by : EASy68K-Qt contributors
* Description: Partial register updates on .B/.W and sign extension with EXT
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
        ORG     $1000
START:
        MOVE.L  #$12345678,D0
        MOVE.B  #$AA,D0         ; D0 = $123456AA (bits 31..8 preserved)
        MOVE.W  #$BEEF,D0       ; D0 = $1234BEEF (bits 31..16 preserved)

        MOVEQ   #0,D1
        MOVE.B  #$80,D1         ; D1 = $00000080 (-128 as signed byte)
        EXT.W   D1              ; D1 = $0000FF80 (-128 as signed word)
        EXT.L   D1              ; D1 = $FFFFFF80 (-128 as signed long)
        SIMHALT

        END     START
