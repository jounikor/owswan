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
// FARGS        : get command line arguments (argc/argv style)
//

#include "ftnstd.h"
#include <string.h>
#include "ftnapi.h"
#include "widechar.h"   /* C run-time library internal variable */
#include "initarg.h"    /* C run-time library internal variable */


intstar4        __fortran IARGC( void ) {
//=====================================

    return( ___Argc );
}


intstar4        __fortran IGETARG( intstar4 *arg, string *dst ) {
//=============================================================

    uint        dst_len;

    if( *arg >= ___Argc ) {
        dst_len = 0;
    } else {
        dst_len = strlen( ___Argv[*arg] );
        if( dst_len > dst->len ) {
            dst_len = dst->len;
        }
        memcpy( dst->strptr, ___Argv[*arg], dst_len );
    }
    memset( &dst->strptr[dst_len], ' ', dst->len - dst_len );
    return( dst_len );
}
