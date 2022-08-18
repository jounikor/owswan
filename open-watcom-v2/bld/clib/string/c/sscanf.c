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
* Description:  Implementation of sscanf() - formatted input from string.
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include <stdio.h>
#include <stdarg.h>
#include "libwchar.h"
#include "scanf.h"


static INTCHAR_TYPE cget_string( PTR_SCNF_SPECS specs )
{
    INTCHAR_TYPE    c;

    if( (c = *(UCHAR_TYPE *)(specs->ptr)) != NULLCHAR ) {
        ++(specs->ptr);
    } else {
        c = INTCHAR_EOF;
        specs->eoinp = 1;
    }
    return( c );
}


static void uncget_string( INTCHAR_TYPE c, PTR_SCNF_SPECS specs )
{
    /* unused parameters */ (void)c;

    --specs->ptr;
}


_WCRTLINK int __F_NAME(vsscanf,vswscanf)( const CHAR_TYPE *dest, const CHAR_TYPE *format, va_list args )
{
    SCNF_SPECS      specs;

    specs.ptr = (CHAR_TYPE *)dest;
    specs.cget_rtn = cget_string;
    specs.uncget_rtn = uncget_string;
    return( __F_NAME(__scnf,__wscnf)( (PTR_SCNF_SPECS)&specs, format, args ) );
}


_WCRTLINK int __F_NAME(sscanf,swscanf)( const CHAR_TYPE *dest, const CHAR_TYPE *format, ... )
{
    va_list     args;
    int         ret;

    va_start( args, format );
    ret = __F_NAME(vsscanf,vswscanf)( dest, format, args );
    va_end( args );
    return( ret );
}
