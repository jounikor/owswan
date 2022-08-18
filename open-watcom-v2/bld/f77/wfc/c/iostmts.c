/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2020 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


//
// IOSTMTS   : processing for all io statements
//

#include "ftnstd.h"
#include "errcod.h"
#include "opr.h"
#include "opn.h"
#include "iodefs.h"
#include "units.h"
#include "global.h"
#include "recog.h"
#include "ferror.h"
#include "utility.h"
#include "ioiolist.h"
#include "ioperm.h"
#include "ioprockw.h"
#include "proctbl.h"
#include "ioutls.h"
#include "symtab.h"
#include "rtconst.h"
#include "fcodes.h"
#include "gio.h"


/* Forward declararations */
static  void    UnitOrList( void );
static  void    JustList( void );
static  void    Form( void );
static  void    DoKWList( void );

void    CpBackSp( void ) {
//==================

// Compile BACKSPACE statement.

    UnitOrList();
}


void    CpClose( void ) {
//=================

// Compile CLOSE statement.

    JustList();
}


void    CpEndfile( void ) {
//===================

// Compile ENDFILE statement.

    UnitOrList();
}


void    CpInquire( void ) {
//===================

// Compile INQUIRE statement.

    JustList();
}


void    CpOpen( void ) {
//================

// Compile OPEN statement.

    JustList();
}


void    CpPrint( void ) {
//=================

// Compile PRINT statement.

    itnode      *cit;

    InitIO();
    Form();
    if( !RecEOS() ) {
        ReqComma();
        if( RecNOpn() ) {
            cit = CITNode;
            AdvanceITPtr();
            if( RecEOS() ) {
                CITNode = cit;
                Error( SX_SURP_OPR );
            }
            CITNode = cit;
        }
    }
    IOList();
    ReqEOS();
    FiniIO();
}


static  bool    Scan4ListOprs( void ) {
//===============================

    itnode      *cit;
    int         level;

    level = 0;
    cit = CITNode;
    for(;;) {
        if( cit == SPtr1 )
            return( false );
        if( cit->opr == OPR_LBR ) {
            ++level;
        } else if( cit->opr == OPR_RBR ) {
            --level;
        }
        if( level == 1 ) {
            if( cit->opr == OPR_COM )
                break;
            if( cit->opr == OPR_EQU )
                break;
            if( cit->opr == OPR_MUL ) {
                break;
            }
        }
        cit = cit->link;
    }
    return( true );
}


static  bool            ReadKWList( void ) {
//====================================

    OPR         opr;

    if( Scan4ListOprs() )
        return( true );
    if( SPtr1->opn.ds != DSOPN_PHI )
        return( true ); // have ( ciolist ) name
    opr = SPtr1->link->opr;
    if( opr == OPR_COM )
        return( false ); // have ( fmt ),
    if( opr == OPR_LBR )
        return( true ); // we have ( ciolist ) (a(i) i==1,10)
    if( opr != OPR_TRM )
        return( false );
    if( RecNOpn() )
        return( false );
    if( CITNode->opn.ds == DSOPN_LIT )
        return( false );
    if( CITNode->opn.ds > DSOPN_LIT )
        return( true );
    if( LkSym()->u.ns.u1.s.typ == FT_CHAR )
        return( false );
    return( true );
}


void    CpRead( void ) {
//================

// Compile READ statement.

    itnode      *cit;

    Remember.read = true;
    InitIO();
    cit = CITNode;
    AdvanceITPtr();
    if( RecOpenParen() && ReadKWList() ) {
        KeywordList();
        ReqCloseParen();
    } else {
        CITNode = cit;
        Form();
        if( !RecEOS() ) {
            ReqComma();
        }
    }
    if( !Remember.end_equals ) {
        GNullEofStmt();
    }
    IOList();
    ReqEOS();
    FiniIO();
}


void    CpRewind( void ) {
//==================

// Compile REWIND statement.

    UnitOrList();
}


void    CpWrite( void ) {
//=================

// Compile WRITE statement.

    InitIO();
    if( RecTrmOpr() && RecNOpn() ) {
        AdvanceITPtr();
    }
    if( ReqOpenParen() ) {
        KeywordList();
        ReqCloseParen();
        IOList();
    }
    ReqEOS();
    FiniIO();
}


static  void    UnitOrList( void ) {
//============================

// The io statement can have a unit id by itself or have a keyword list
// in brackets.

    InitIO();
    if( RecNextOpr( OPR_LBR ) && RecNOpn() ) {
        AdvanceITPtr();
        DoKWList();
    } else {
        Permission( IO_UNIT );           // remembers unit= specified
        Unit();
        AdvanceITPtr();
    }
    ReqEOS();
    GStartIO();
    FiniIO();
}


static  void    JustList( void ) {
//==========================

// The io statement must have a keyword list in brackets.

    InitIO();
    if( RecTrmOpr() && RecNOpn() ) {
        AdvanceITPtr();
    }
    if( ReqOpenParen() ) {
        DoKWList();
    }
    ReqEOS();
    GStartIO();
    FiniIO();
}


static  void    DoKWList( void ) {
//==========================

// Call KeywordList() and check for closing parenthesis.

    KeywordList();
    if( ReqCloseParen() ) {
        if( !RecNOpn() ) {
            Error( SX_EOS_EXPECTED );
        }
        AdvanceITPtr();
    }
}


static  void    Form( void ) {
//======================

    Permission( IO_FMT );                  // remember FMT=
    Permission( IO_UNIT );                 // default  UNIT=*
    FormatIdd();
    AdvanceITPtr();
}
