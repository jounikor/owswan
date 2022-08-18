/****************************************************************************
*
*                            Open Watcom Project
*
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
* Description:  Safer C Library tmpfile_s(), Wrapper around tmpfile().
*
****************************************************************************/


#include "variety.h"
#include "saferlib.h"
#include <stdio.h>
#include <stdlib.h>


_WCRTLINK errno_t tmpfile_s( FILE * __restrict * __restrict streamptr )
/***********************************************************************/
{
    errno_t     rc = -1;

    /* runtime-constraints */
    /* streamptr  not NULL */
    if( __check_constraint_nullptr( streamptr ) ) {

        /* now call the old tmpfile() */
        *streamptr = tmpfile();

        if( *streamptr != NULL ) {
            rc = 0;  /* successful */
        }
    }
    return( rc );
}
