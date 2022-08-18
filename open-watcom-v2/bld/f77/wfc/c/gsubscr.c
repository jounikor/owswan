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
// GSUBSCR   : subscripting code generation routines
//

#include "ftnstd.h"
#include "global.h"
#include "fcodes.h"
#include "stmtsw.h"
#include "opn.h"
#include "emitobj.h"
#include "gstring.h"
#include "gsubscr.h"
#include "gtypes.h"


void    GBegSubScr( itnode *array_node ) {
//=========================================

// Start a subscript operation.

    /* unused parameters */ (void)array_node;
}


void    GSubScr( void ) {
//=================

// Generate one subscript.

}


void    GEndSubScr( itnode *arr ) {
//=================================

// Finish off a subscripting operation.

    itnode      *arg;
    int         dim_cnt;

    if( arr->opn.us & USOPN_FLD ) {
        PushOpn( arr );
        EmitOp( FC_FIELD_SUBSCRIPT );
        OutPtr( arr->sym_ptr );
        dim_cnt = _DimCount( arr->sym_ptr->u.fd.dim_ext->dim_flags );
    } else {
        EmitOp( FC_SUBSCRIPT );
        OutPtr( arr->sym_ptr );
        dim_cnt = _DimCount( arr->sym_ptr->u.ns.si.va.u.dim_ext->dim_flags );
    }
    for( arg = arr->list; dim_cnt-- > 0; arg = arg->link ) {
        GenType( arg );
    }
    if( (arr->opn.us & USOPN_FLD) == 0 ) {
        if( (StmtSw & SS_DATA_INIT) == 0 ) {
            if( arr->sym_ptr->u.ns.u1.s.typ == FT_CHAR ) {
                OutPtr( GTempString( 0 ) );
            }
        }
    }
    SetOpn( arr, USOPN_SAFE );
}
