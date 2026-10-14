* Assembler corpus: one line per diagnostic of the original (ERROR.CPP),
* listing text compared with the original assembler. Lines that make the
* original crash (ELSE/ENDI/ENDF/ENDW/UNTIL/... with no open structure:
* "No matching ... statement") or use undefined symbols (the original
* encodes an uninitialized value: unclosed IF/FOR/WHILE/REPEAT/DBLOOP,
* undefined register lists) are not included. CC0-1.0.
        ORG     $1000
START   MOVE.W  $12345,D0               absolute address exceeds 16 bits?
        MOVE.W  ($12345).W,D0           Absolute address exceeds 16 bits
        JMP     D0                      Invalid addressing mode
        MOVE.L  #'ABCDE',D0             ASCII constant exceeds 4 characters
        BRA.S   FAR                     Branch displacement out of range
        MOVE.W  D0 D1                   Comma expected
        MOVE.W  $12345(A0),D0           Displacement out of range
        DC.W    1/0                     Division by zero attempted
        MOVE.W  #1+,D0                  Evaluation could not be completed
        MOVE.W  BEFORE.W,D0             Forcing SHORT addressing
FWD     EQU     LATER                   Forward reference
        DS.B    LATER2                  Forward references not allowed
        MOVE.W  #$123,D0
        ADDQ    #9,D0                   Quick immediate data range must be 1 to 8
        MOVEQ   #$1FF,D0                Quick immediate data range exceeds 1 byte
        ADD.B   #$123,D0                Immediate data exceeds 8 bits
        BFTST   D0{33:1}                Invalid bitfield
        ASL.W   #9,D0                   Invalid constant shift count
        MOVE.W  #1&&2,D0                Invalid operator
        TRAP    #16                     Invalid vector number
        EQU     5                       Label required with this directive
ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ NOP
        DC.L    $123456789              Numeric constant exceeds 32 bits
        ORG     $2001                   Origin value is odd
LIST1   REG     D0-D3
        MOVE.W  #LIST1,D0               Register list symbol used in an expression
DUP     NOP
DUP     NOP                             Symbol defined more than once
NOTLIST EQU     1
        MOVEM.L NOTLIST,-(SP)           Symbol is not a register list symbol
        MOVE.W  #1,9D0                  Illegal symbol?
        MOVE.W  @@,D0                   Illegal symbol
        MOVE.W  D0                      Address expected?
        MOVE.W  (A0,D0,D1),D0           Invalid argument?
MAC1    MACRO
        MAC1
        ENDM
        MAC1                            Nested Macro calls are too many levels deep
MAC2    MACRO
        DC.W    \1,\2,\3,\4,\5,\6,\7,\8,\9,\A,\B,\C,\D,\E,\F,\G,\H,\I,\J,\K,\L,\M,\N,\O,\P,\Q,\R,\S,\T,\U,\V,\W,\X,\Y,\Z,\a
        ENDM
        MAC2    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38
BEFORE  EQU     $4000
LATER   EQU     $10
LATER2  EQU     4
FAR     NOP
        END     START
