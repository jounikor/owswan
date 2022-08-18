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
* Description:  logical operator code generation routines
*
****************************************************************************/


#include "ftnstd.h"
#include "global.h"
#include "opn.h"
#include "fcodes.h"
#include "optr.h"
#include "emitobj.h"
#include "logops.h"
#include "gtypes.h"


void    LogOp( TYPE typ1, TYPE typ2, OPTR op ) {
//==============================================

// Generate code for a relational operator.

    bool        flip;

    op -= OPTR_FIRST_LOGOP;
    flip = false;
    if( ( (CITNode->opn.us & USOPN_WHERE) == USOPN_SAFE ) &&
        ( (CITNode->link->opn.us & USOPN_WHERE) != USOPN_SAFE ) ) {
        flip = true;
    }
    PushOpn( CITNode->link );
    if( typ1 == FT_NO_TYPE ) {  // unary
        if( _IsTypeInteger( typ2 ) ) {
            EmitOp( FC_BIT_NOT );
        } else {
            EmitOp( FC_NOT );
        }
        GenType( CITNode->link );
        SetOpn( CITNode, USOPN_SAFE );
    } else {
        PushOpn( CITNode );
        if( _IsTypeInteger( typ2 ) ) {
            EmitOp( FC_BITOPS + op );
        } else {
            EmitOp( FC_LOGOPS + op );
        }
        if( flip ) {
            GenTypes( CITNode->link, CITNode );
        } else {
            GenTypes( CITNode, CITNode->link );
        }
    }
}
