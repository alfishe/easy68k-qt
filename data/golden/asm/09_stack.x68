* Stack manipulation instructions
    ORG $1000
    PEA $2000
    LINK A6,#-16
    UNLK A6
    END $1000
