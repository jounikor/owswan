# DDESPY Builder Control file
# ============================

set PROJNAME=ddespy

set PROJDIR=<CWD>

[ INCLUDE <OWROOT>/build/prolog.ctl ]

[ INCLUDE <OWROOT>/build/defrule.ctl ]

[ BLOCK <BLDRULE> rel ]
#======================
    cdsay <PROJDIR>

[ BLOCK <BLDRULE> rel cprel ]
#============================
    <CCCMD> wini86/wddespy.exe <OWRELROOT>/binw/
    <CCCMD> nt386/wddespy.exe  <OWRELROOT>/binnt/
    <CCCMD> ntaxp/wddespy.exe  <OWRELROOT>/axpnt/

    <CCCMD> ntx64/wddespy.exe  <OWRELROOT>/binnt64/

[ BLOCK . . ]

[ INCLUDE <OWROOT>/build/epilog.ctl ]
