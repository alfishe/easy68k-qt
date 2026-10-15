*-----------------------------------------------------------
* Exercise S-14-EX1: Pack CID=3 SerialSend command and 9600-8N1 params
*-----------------------------------------------------------
            ORG     $1000
START:
            * TODO: Place CID=3 + task 43 ($0003002B) in D0 and 9600-8N1 ($1805) in D1
            SIMHALT
            END     START
