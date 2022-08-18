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
// FCFIELD  : field selection operator
//

#include "ftnstd.h"
#include "symbol.h"
#include "wf77defs.h"
#include "cg.h"
#include "emitobj.h"
#include "fctypes.h"
#include "fcsubscr.h"
#include "tmpdefs.h"
#include "fctemp.h"
#include "fcjmptab.h"
#include "fcfield.h"
#include "fcstack.h"
#include "cgswitch.h"
#include "cgprotos.h"


void            FCFieldOp( void ) {
//===========================

// Perform field selection.

    sym_id      sym;
    cg_name     base;
    cg_type     ptr_type;
    cg_name     addr;

    sym = GetPtr();
    base = XPop();
    if( (sym->u.ns.flags & SY_CLASS) == SY_SUBPROGRAM ) {
        // function returning a structure
        ptr_type = TY_LOCAL_POINTER;
    } else {
        ptr_type = SymPtrType( sym );
    }
    // add offset of field
    addr = CGBinary( O_PLUS, base, XPopValue( TY_INT_4 ), ptr_type );
    if( sym->u.ns.u1.s.xflags & SY_VOLATILE ) {
        addr = CGVolatile( addr );
    }
    XPush( addr );
}


cg_name FieldArrayEltSize( sym_id fd ) {
//======================================

// Return size of an array element.  Array is a field in a structure.

    return( CGInteger( _FieldSize( fd ), TY_INTEGER ) );
}


void    FCFieldSubscript( void ) {
//==========================

    sym_id      fd;
    cg_name     size;
    cg_name     base;

    fd = GetPtr();
    base = XPop();
    size = FieldArrayEltSize( fd );
    XPush( CGBinary( O_PLUS, base,
                     CGBinary( O_TIMES, ConstArrayOffset( fd->u.fd.dim_ext ),
                               size, TY_INTEGER ),
                     TY_INT_4 ) );
}


void    FCFieldSubstring( void ) {
//==========================

    cg_name     base;
    cg_name     start_1;
    cg_name     start_2;
    cg_name     end;
    unsigned_16 typ_info;
    int         len;
    sym_id      fd;

    base = XPop();
    fd = GetPtr();   // skip the field name
    len = GetInt();
    typ_info = GetU16();
    start_1 =  XPopValue( GetType1( typ_info ) );
    if( len == 0 ) {
        CloneCGName( start_1, &start_1, &start_2 );
        end = XPop();
        if( end == NULL ) {
            end = CGInteger( fd->u.fd.xt.size, TY_INTEGER );
        } else {
            XPush( end );
            end = XPopValue( GetType2( typ_info ) );
        }
        XPush( CGBinary( O_PLUS, CGInteger( 1, TY_INTEGER ),
                         CGBinary( O_MINUS, end, start_2, TY_INTEGER ),
                         TY_INTEGER ) );
    } else {
        XPush( CGInteger( len, TY_INTEGER ) );
    }
    XPush( CGBinary( O_PLUS, base,
                     CGBinary( O_MINUS, start_1, CGInteger( 1, TY_INTEGER ),
                               TY_INTEGER ),
                     TY_INTEGER ) );
}
