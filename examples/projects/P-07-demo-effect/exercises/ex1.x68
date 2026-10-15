*-----------------------------------------------------------
* Exercise P-07-EX1: Copy 16-byte SIN16 scanline to $2000 and sum to D0=1024 in <= 400 cycles
*-----------------------------------------------------------
            ORG     $1000
START:
            * TODO: Write 16 SIN16 bytes to $2000 and accumulate sum (1024) in D0 in <= 400 cycles
            SIMHALT
SIN16:      DC.B    64,88,110,123,128,123,110,88
            DC.B    64,40,18,5,0,5,18,40
            END     START
