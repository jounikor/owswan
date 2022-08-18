/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2018 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Implementation of helper function for assert macro.
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include "enterdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libwchar.h"
#include "rterrmsg.h"
#undef NDEBUG
#include <assert.h>

#define STR_SIZE        128

#define TITLE_STRING    STRING("Assertion Failed")
#define FMT_STRING      STRING("%hs, file %hs, line %d.\n")

#ifndef __WIDECHAR__
    static int __extra_return = 0;
#endif


_WCRTLINK void __F_NAME(_assert,_wassert)( char *expr, char *fn, int line_num )
{
    CHAR_TYPE   str[STR_SIZE];

#ifndef __WIDECHAR__
    int     after_num_returns = 1;

    if( __extra_return ) {
        after_num_returns++;
        __extra_return = 0;
    }
#endif

    /* Have to use snprintf() here. The error message can be arbitrarily long */
    __F_NAME(_snprintf,swprintf)( str, STR_SIZE, FMT_STRING, expr, fn, line_num );
#ifndef __WIDECHAR__
    if( __WD_Present ) {
        char    *buf;

        buf = alloca( strlen( str ) + sizeof( TITLE_STRING ) + 1 );
        strcpy( buf, TITLE_STRING );
        strcat( buf, ": " );
        strcat( buf, str );
        DebuggerBreakAfterReturnWithMessage( after_num_returns, buf );
    } else {
#endif
        __F_NAME(__rterr_msg,__wrterr_msg)( TITLE_STRING, str );
        abort();
#ifndef __WIDECHAR__
    }
#endif
}


_WCRTLINK void __F_NAME(__assert,__wassert)( int value, char *expr, char *fn, int line_num )
{
    if( !value ) {
#ifndef __WIDECHAR__
        __extra_return = 1;
#endif
        __F_NAME(_assert,_wassert)( expr, fn, line_num );
    }
}
