# libwsc Builder Control file
# =========================

set PROJNAME=libwsc

set PROJDIR=<CWD>

[ INCLUDE <OWROOT>/build/prolog.ctl ]

[ INCLUDE <OWROOT>/build/defrule.ctl ]

[ BLOCK <BLDRULE> rel ]
#======================
    cdsay <PROJDIR>

[ BLOCK <BLDRULE> build rel cprel ]
#============================
    <CCCMD> startup/library/msdos.086/ml/wsccrt0.obj                    <OWRELROOT>/libwsc/wsccrt0.obj
    <CCCMD> lib/library/msdos.086/ml/libwscl.lib                        <OWRELROOT>/libwsc/libwscl.lib
    <CCCMD> h/libwsc.h                                                  <OWRELROOT>/libwsc/h/libwsc.h

[ BLOCK . . ]

[ INCLUDE <OWROOT>/build/epilog.ctl ]
