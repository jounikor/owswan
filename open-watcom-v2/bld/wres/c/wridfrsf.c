/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2016-2016 The Open Watcom Contributors. All Rights Reserved.
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


#include <string.h>
#include "layer0.h"
#include "util.h"
#include "reserr.h"
#include "wresrtns.h"
#include "wresset2.h"
#include "seekres.h"


WResID *WResIDFromStrF( lpcstr newstr )
/*************************************/
/* allocate an ID and fill it in */
{
    WResID  *newid;
    size_t  strsize;

#if defined( _M_I86 )
    strsize = _fstrlen( newstr );
#else
    strsize = strlen( newstr );
    /* check the size of the string:  can it fit in two bytes? */
    if( strsize > 0xffff ) {
        WRES_ERROR( WRS_BAD_PARAMETER );
        return( NULL );
    }
#endif

    /* allocate the new ID */
    // if strsize is non-zero then the memory allocated is larger
    // than required by 1 byte
    newid = WRESALLOC( sizeof( WResID ) + strsize );
    if( newid == NULL ) {
        WRES_ERROR( WRS_MALLOC_FAILED );
    } else {
        newid->IsName = true;
        newid->ID.Name.NumChars = strsize;
#if defined( _M_I86 )
        _fmemcpy( newid->ID.Name.Name, newstr, strsize );
#else
        memcpy( newid->ID.Name.Name, newstr, strsize );
#endif
    }
    return( newid );
} /* WResIDFromStrF */
