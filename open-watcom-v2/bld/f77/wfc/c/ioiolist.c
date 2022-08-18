/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2021 The Open Watcom Contributors. All Rights Reserved.
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  Process an input or output list.
*
****************************************************************************/


#include "ftnstd.h"
#include "errcod.h"
#include "opr.h"
#include "opn.h"
#include "iodefs.h"
#include "rtconst.h"
#include "fcodes.h"
#include "global.h"
#include "stmtsw.h"
#include "recog.h"
#include "ferror.h"
#include "insert.h"
#include "utility.h"
#include "csloops.h"
#include "csutls.h"
#include "ioiolist.h"
#include "ioperm.h"
#include "ioprockw.h"
#include "ioutls.h"
#include "upscan.h"
#include "gio.h"


void    IOList( void ) {
//================

// Process the input/output list.

    GStartIO();
    if( CITNode->link != NULL ) {
        if( RecNOpn() && RecNextOpr( OPR_TRM ) ) {
            AdvanceITPtr();                   // WRITE(6,3)
        } else if( Already( IO_NAMELIST ) ) {
            Error( IL_NO_IOLIST );
        }
        for(;;) {
            if( CITNode->link == NULL ) {
                break;
            }
            ProcessList();
        }
    }
    GStopIO();
}


static  bool    HasUnion( sym_id fld ) {
//======================================

    for(;;) {
        if( fld == NULL )
            return( false );
        if( fld->u.fd.typ == FT_STRUCTURE ) {
            if( HasUnion( fld->u.fd.xt.record->fl.sym_fields ) ) {
                break;
            }
        }
        if( fld->u.fd.typ == FT_UNION )
            break;
        fld = fld->u.fd.link;
    }
    return( true );
}


static  void    ChkStructIO( sym_id sym ) {
//========================================

    if( HasUnion( sym->u.sd.fl.sym_fields ) ) {
        if( (StmtSw & SS_DATA_INIT) || !NotFormatted() ) {
            StructErr( SP_STRUCT_HAS_UNION, sym );
        }
    }
}


bool    StartImpDo( void ) {
//====================

// This procedure scans the i/o list to recognize an implied do.
// If it is not found false returns, if it is found true returns and:
// -  the implied DO is initialized
// -  a terminal operator is placed over the comma at the
//    end of the i/o list within the implied DO. This is used
//    as a signal to generate closing code for the implied DO.
// -  the nodes containing the do list are released from
//    from the internal text list.
// -  a null operator is placed over the bracket at the

    itnode      *citnode;
    itnode      *lastcomma;
    int         level;

    if( !RecNOpn() )
        return( false );
    if( !RecNextOpr( OPR_LBR ) )
        return( false );
    citnode = CITNode;
    AdvanceITPtr();
    lastcomma = NULL;
    level = 0;
    AdvanceITPtr();
    for(;;) {
        if( RecOpenParen() ) {
            level++;
        } else if( RecCloseParen() ) {
            level--;
        } else if( RecComma() && ( level == 0 ) ) {
            lastcomma = CITNode;
        }
        if( ( level < 0 ) || RecTrmOpr() ) {
            CITNode = citnode;
            return( false );
        }
        AdvanceITPtr();
        if( RecEquSign() && ( level == 0 ) ) {
            break;
        }
    }
    if( ( lastcomma == NULL ) || ( lastcomma->link != CITNode ) ) {
        CITNode = citnode;
        return( false );
    }
    InitImpDo( lastcomma );
    CITNode = citnode;
    AdvanceITPtr();
    if( ( RecNextOpr( OPR_TRM ) && RecNOpn() ) ) {
        Error( IL_EMPTY_IMP_DO );
    }
    return( true );
}


void    ProcessList( void ) {
//=====================

// This procedure will process one 'thing' from the i/o list. A 'thing' is:
//     1) initializing an implied DO
//     2) finishing an implied DO
//     3) an i/o list item

    if( RecTrmOpr() ) {
        FinishImpDo();
        if( !RecTrmOpr() ) {
            ReqComma();
        }
    } else if( !StartImpDo() ) {
        ProcIOExpr();
        ListItem();
        if( !RecTrmOpr() ) {
            ReqComma();
        }
    }
}


void    ListItem( void ) {
//==================

// Process one list item.

    sym_id      sd;

    if( RecNOpn() ) {
        if( !CpError ) {
            Error( SX_SURP_OPR );
        }
    } else if( RecArrName() ) {
        CITNode->sym_ptr->u.ns.u1.s.xflags |= SY_DEFINED;
        ChkAssumed();
        if( CITNode->typ == FT_STRUCTURE ) {
            ChkStructIO( CITNode->sym_ptr->u.ns.xt.sym_record );
            GIOStructArray();
        } else {
            GIOArray();
        }
    } else if( CITNode->typ == FT_STRUCTURE ) {
        CITNode->sym_ptr->u.ns.u1.s.xflags |= SY_DEFINED;
        if( CITNode->opn.us & USOPN_FLD ) {
            sd = CITNode->value.st.field_id->u.fd.xt.sym_record;
        } else {
            sd = CITNode->sym_ptr->u.ns.xt.sym_record;
        }
        ChkStructIO( sd );
        GIOStruct( sd );
    } else {
        if( StmtProc == PR_READ ) {
            CkAssignOk();
        }
        GIOItem();
    }
    AdvanceITPtr();
}


void    InitImpDo( itnode *lastcomma ) {
//======================================

// Initialize the implied DO-loop.

    int         level;
    itnode      *imp_do_list;

    CITNode = lastcomma;
    CITNode->opr = OPR_TRM;     // marks the end of the i/o list
    ImpDo();
    if( !ReqCloseParen() ) {
        level = 0;
        for(;;) {
            if( RecOpenParen() ) {
                level++;
            } else if( RecCloseParen() ) {
                level--;
            }
            if( level < 0 )
                break;
            if( CITNode->link == NULL ) {
                DelCSNode();
                CITNode->opr = OPR_TRM;
                CITNode->oprpos = 9999;
                break;
            }
            AdvanceITPtr();
        }
    }
    ReqNOpn();
    imp_do_list = lastcomma->link;
    lastcomma->link = CITNode->link;
    CITNode->link = NULL;
    FreeITNodes( imp_do_list );
}


void    FinishImpDo( void ) {
//=====================

// Finish the implied DO.

    TermDo();
    AdvanceITPtr();
}
