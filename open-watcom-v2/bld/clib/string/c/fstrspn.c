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
* Description:  Implementation of _fstrspn() - far strspn().
*
****************************************************************************/


#include "variety.h"
#include <string.h>
#include "setbits.h"

/*  The strspn function computes the length of the initial segment of the
    string pointed to by str which consists entirely of characters from
    the string pointed to by charset.
*/

_WCRTLINK size_t _fstrspn( const char _WCFAR *str, const char _WCFAR *charset )
{
    char            tc;
    unsigned char   vector[ CHARVECTOR_SIZE ];
    size_t          len;

    __fsetbits( vector, charset );
    len = 0;
    for( ; tc = *str; ++str, ++len ) {
        /* quit if we find any char not in charset */
        if( GETCHARBIT( vector, tc ) == 0 )
            break;
    }
    return( len );
}
