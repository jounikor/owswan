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
#include <wchar.h>
#include "farfunc.h"


_WCRTLINK size_t _NEARFAR(mbsrtowcs,_fmbsrtowcs)( wchar_t _FFAR *dst, const char _FFAR * _FFAR *src, size_t len, mbstate_t _FFAR *ps )
{
    wchar_t             wc;
    size_t              charsConverted = 0;
    const char _FFAR *  mbcPtr = *src;
    size_t              rc;

    /*** Process the characters, one by one ***/
    if( dst != NULL ) {
        while( len-- > 0 ) {
            rc = _NEARFAR(mbrtowc,_fmbrtowc)( &wc, mbcPtr, MB_LEN_MAX, ps );
            if( rc == (size_t)-1 || rc == (size_t)-2 ) {
                return( (size_t)-1 );
            } else if( rc == 0 ) {
                break;
            } else {
                *dst++ = wc;
                mbcPtr += rc;
                charsConverted++;
            }
        }
        if( rc == 0 ) {
            *src = NULL;
        } else {
            *src = mbcPtr;
        }
    } else {
        for( ;; ) {
            rc = _NEARFAR(mbrtowc,_fmbrtowc)( &wc, mbcPtr, MB_LEN_MAX, ps );
            if( rc == (size_t)-1 || rc == (size_t)-2 ) {
                return( (size_t)-1 );
            } else if( rc == 0 ) {
                break;
            } else {
                mbcPtr += rc;
                charsConverted++;
            }
        }
    }
    return( charsConverted );
}
