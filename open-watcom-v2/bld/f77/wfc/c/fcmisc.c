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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


//
// FCMISC    : miscellaneous F-Code processor
//

#include "ftnstd.h"
#include "global.h"
#include "wf77defs.h"
#include "wf77aux.h"
#include "tmpdefs.h"
#include "fcgbls.h"
#include "fcjmptab.h"
#include "fctemp.h"
#include "wf77info.h"
#include "fcstack.h"
#include "cgswitch.h"
#include "cgprotos.h"


extern  void            *GetPtr(void);


void    FCDone( void ) {
//======================

// Process end of an expression.

    CGDone( XPop() );
}


void    FCCmplxDone( void ) {
//===========================

// Process end of a complex expression.

    CGDone( CGBinary( O_COMMA, XPop(), XPop(), TY_DEFAULT ) );
}


void    FCStmtDone( void ) {
//==========================

// Finished compiling a statement.

    FreeTmps();
}


void    FCTrash( void ) {
//=======================

// Trash a cg_name.

    CGTrash( XPop() );
}


static  void    Break( RTCODE routine ) {
//=======================================

// Process PAUSE/STOP statement.

    call_handle call;
    sym_id      lit;
    cg_name     arg;

    call = InitCall( routine );
    lit = GetPtr();
    if( lit == NULL ) {
        arg = CGInteger( 0, TY_LOCAL_POINTER );
    } else {
        arg = CGBackName( ConstBack( lit ), TY_LOCAL_POINTER );
    }
    CGAddParm( call, arg, TY_LOCAL_POINTER );
    CGDone( CGCall( call ) );
}


void    FCStop( void ) {
//======================

// Process STOP statement.

    Break( RT_STOP );
}


void    FCPause( void ) {
//=======================

// Process PAUSE statement.

    Break( RT_PAUSE );
}


void    FCNull( void ) {
//======================

// Do nothing.

}
