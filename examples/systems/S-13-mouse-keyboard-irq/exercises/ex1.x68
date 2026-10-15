*-----------------------------------------------------------
* Exercise S-13-EX1: Configure Level 3 Keyboard IRQ (task 62)
*-----------------------------------------------------------
            ORG     $1000
START:
            * TODO: Install ISR at $006C, enable Key IRQ Level 3 ($0301) with
            * task 62, disable it ($0000), and set D0 = 62
            SIMHALT
            END     START
