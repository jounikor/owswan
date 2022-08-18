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
* Description:  Implementation of scanf_s() - safe formatted stream input.
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include "saferlib.h"
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include "libwchar.h"
#include "scanf.h"


static INTCHAR_TYPE cget_stdin( PTR_SCNF_SPECS specs )
{
    INTCHAR_TYPE    c;

    if( (c = __F_NAME(getc,getwc)( stdin )) == INTCHAR_EOF ) {
        specs->eoinp = 1;
    }
    return( c );
}


static void uncget_stdin( INTCHAR_TYPE c, PTR_SCNF_SPECS specs )
{
    /* unused parameters */ (void)specs;

    __F_NAME(ungetc,ungetwc)( c, stdin );
}


_WCRTLINK int __F_NAME(scanf_s,wscanf_s)( const CHAR_TYPE * __restrict format, ... )
{
    va_list         args;
    SCNF_SPECS      specs;
    const char      *msg;
    int             rc;

    /* Basic check for null pointers to see if we can continue */
    if( __check_constraint_nullptr_msg( msg, format ) ) {

        specs.cget_rtn   = cget_stdin;
        specs.uncget_rtn = uncget_stdin;
        msg = NULL;
        va_start( args, format );
        rc = __F_NAME(__scnf_s,__wscnf_s)( (PTR_SCNF_SPECS)&specs, format, &msg, args );
        va_end( args );
        if( msg == NULL ) {
            /* no rt-constraint violation inside worker routine */
            return( rc );
        }
    }
    __rtct_fail( __func__, msg, NULL );
    return( EOF );
}
