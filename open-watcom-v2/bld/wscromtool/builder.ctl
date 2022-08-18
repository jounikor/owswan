# WSCROMTOOL Builder Control file
# ==========================

set PROJNAME=wscromtool

set PROJDIR=<CWD>

[ INCLUDE <OWROOT>/build/prolog.ctl ]

[ INCLUDE <OWROOT>/build/deftool.ctl ]

[ BLOCK <BLDRULE> rel ]
#======================
    cdsay <PROJDIR>

[ BLOCK <BLDRULE> rel cprel ]
#============================
    <CCCMD> linux386/wscromtool.exe  <OWRELROOT>/binl/wscromtool
    <CCCMD> linuxx64/wscromtool.exe  <OWRELROOT>/binl64/wscromtool
    <CCCMD> osxx64/wscromtool.exe    <OWRELROOT>/osx64/wscromtool

[ BLOCK . . ]

[ INCLUDE <OWROOT>/build/epilog.ctl ]
