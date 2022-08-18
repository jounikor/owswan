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


#include "variety.h"
#include <mbstring.h>
#include <stdlib.h>
#include "farfunc.h"



/****
***** Convert a multibyte character string to a wide character string.
***** Returns the number of wide characters written (excluding NULL if
***** one was encountered) on success, or -1 on error.
****/

_WCRTLINK size_t _NEARFAR(mbstowcs,_fmbstowcs)( wchar_t _FFAR *pwcs, const char _FFAR *s, size_t n )
{
    size_t          numChars = 0;
    int             rc;

    if( pwcs != NULL ) {                /* convert the string */
        while( n > 0 ) {
            if( *s != '\0' ) {
                rc = _NEARFAR(mbtowc,_fmbtowc)( pwcs, s, MB_LEN_MAX );
                if( rc == -1 )
                    return( (size_t)-1 );
                n--;
                s = (char _FFAR *)_NEARFAR(_mbsinc,_fmbsinc)( (unsigned char _FFAR *)s );
                pwcs++;
                numChars++;
            } else {
                *pwcs = L'\0';
                break;
            }
        }
    } else {                            /* get required size */
        for( ;; ) {
            if( *s != '\0' ) {
                rc = _NEARFAR(mbtowc,_fmbtowc)( NULL, s, MB_LEN_MAX );
                if( rc == -1 )
                    return( (size_t)-1 );
                s = (char _FFAR *)_NEARFAR(_mbsinc,_fmbsinc)( (unsigned char _FFAR *)s );
                numChars++;
            } else {
                break;
            }
        }
    }
    return( numChars );
}
