*-----------------------------------------------------------
* Title      : P-04 Direct-Threaded Forth Interpreter
* SPDX-License-Identifier: CC0-1.0
*-----------------------------------------------------------
* Register convention:
*   A4 = Forth Instruction Pointer (IP)
*   A5 = Parameter Stack Pointer (PSP, grows down from $4000)
*   A7 = Return Stack Pointer (RSP)

NEXT        MACRO
            MOVEA.L (A4)+,A0
            JMP     (A0)
            ENDM

            ORG     $1000
START:
            LEA     $4000,A5            ; Initialize Parameter Stack (PSP)
            LEA     PROG_STREAM(PC),A4  ; Initialize Forth IP
            NEXT                        ; Start threaded execution!

*--- Primitive: LIT ( -- n ) pushes inline 32-bit cell at (IP)+ ---
W_LIT:
            MOVE.L  (A4)+,-(A5)
            NEXT

*--- Primitive: DUP ( a -- a a ) ---
W_DUP:
            MOVE.L  (A5),-(A5)
            NEXT

*--- Primitive: SWAP ( a b -- b a ) ---
W_SWAP:
            MOVE.L  (A5)+,D0
            MOVE.L  (A5),D1
            MOVE.L  D0,(A5)
            MOVE.L  D1,-(A5)
            NEXT

*--- Primitive: ADD ( a b -- a+b ) ---
W_ADD:
            MOVE.L  (A5)+,D0
            ADD.L   D0,(A5)
            NEXT

*--- Primitive: SUB ( a b -- a-b ) ---
W_SUB:
            MOVE.L  (A5)+,D0
            SUB.L   D0,(A5)
            NEXT

*--- Primitive: MUL ( a b -- a*b ) ---
W_MUL:
            MOVE.L  (A5)+,D0
            MOVE.L  (A5),D1
            MULS.W  D0,D1
            MOVE.L  D1,(A5)
            NEXT

*--- Colon word entry: DOCOL pushes IP on RSP, enters body at (A0)+4 ---
W_SQUARE:
            MOVE.L  A4,-(A7)            ; Push caller IP onto Return Stack
            LEA     SQUARE_BODY(PC),A4
            NEXT

*--- Primitive: SEMIS ( ; ) pops IP from RSP ---
W_SEMIS:
            MOVEA.L (A7)+,A4
            NEXT

*--- Primitive: HALT_WORD pops top of data stack into D0 and prints it ---
W_HALT:
            MOVE.L  (A5)+,D1
            MOVE.L  D1,D6
            MOVEQ   #3,D0
            TRAP    #15
            LEA     CRLF(PC),A1
            MOVEQ   #14,D0
            TRAP    #15
            MOVE.L  D6,D0
            SIMHALT

            DS.W    0
* : SQUARE DUP MUL ;
SQUARE_BODY:
            DC.L    W_DUP, W_MUL, W_SEMIS

* Main Forth program: 7 SQUARE 5 + HALT  (7*7 + 5 = 54)
PROG_STREAM:
            DC.L    W_LIT, 7
            DC.L    W_SQUARE
            DC.L    W_LIT, 5
            DC.L    W_ADD
            DC.L    W_HALT

CRLF:       DC.B    13,10,0
            END     START
