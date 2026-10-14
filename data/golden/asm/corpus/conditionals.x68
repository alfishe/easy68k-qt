* Conditional assembly: IFC/IFNC/IFEQ/IFNE/IFLT/IFLE/IFGT/IFGE, nesting
DEBUG   EQU     1
LEVEL   EQU     3
        ORG     $1000
START   IFEQ    DEBUG
        DC.B    1
        ENDC
        IFNE    DEBUG
        DC.B    2
        ENDC
        IFLT    LEVEL-5
        DC.B    3
        IFGT    LEVEL-2
        DC.B    4
        ENDC
        IFLE    LEVEL-2
        DC.B    5
        ENDC
        ENDC
        IFGE    LEVEL
        DC.B    6
        ENDC
        IFC     'ABC','abc'
        DC.B    7
        ENDC
        IFNC    'ABC','XYZ'
        DC.B    8
        IFEQ    0
        IFNE    0
        DC.B    9
        ENDC
        DC.B    10
        ENDC
        ENDC
        IFNC    'A','A'
        DC.B    11
        ENDC
LBL     IFEQ    0
        ENDC
        IFEQ
        ENDC
        END     START
