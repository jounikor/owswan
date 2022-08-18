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
* Description:  Implementation of fgets() - read string from stream.
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include <stddef.h>
#include <stdio.h>
#include "libwchar.h"
#include "fileacc.h"


_WCRTLINK CHAR_TYPE *__F_NAME(fgets,fgetws)( CHAR_TYPE *s, int n, FILE *fp )
{
    INTCHAR_TYPE    c;
    CHAR_TYPE       *cs;
    unsigned        oflag;

    _ValidFile( fp, 0 );
    _AccessFile( fp );

    oflag = fp->_flag & (_SFERR | _EOF);
    fp->_flag &= ~(_SFERR | _EOF);
    cs = s;

    /* don't use macro version of getc: multi-threading issues */
    while( (--n > 0) && (c = __F_NAME(fgetc,fgetwc)( fp )) != INTCHAR_EOF ) {
        if( (*cs++ = (CHAR_TYPE)c) == STRING( '\n' ) ) {
            break;
        }
    }

    if( c == INTCHAR_EOF && (cs == s || ferror( fp )) ) {
        s = NULL;
    } else {
        *cs = NULLCHAR;
    }
    fp->_flag |= oflag;
    _ReleaseFile( fp );
    return( s );
}
