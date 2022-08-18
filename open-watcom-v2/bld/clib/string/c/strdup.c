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
* Description:  Implementation of strdup() and wcsdup().
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "libwchar.h"
#include "strdup.h"
#include "liballoc.h"


CHAR_TYPE *__F_NAME(__clib_strdup,__clib_wcsdup)( const CHAR_TYPE *str )
{
    CHAR_TYPE   *mem;
    int         len;

    len = __F_NAME(strlen,wcslen)( str ) + 1;
    mem = lib_malloc( len * CHARSIZE );
    if( mem ) {
        (memcpy)( mem, str, len * CHARSIZE );
    }
    return( mem );
}

_WCRTLINK CHAR_TYPE *__F_NAME(strdup,wcsdup)( const CHAR_TYPE *str )
{
    return( __F_NAME(__clib_strdup,__clib_wcsdup)( str ) );
}
