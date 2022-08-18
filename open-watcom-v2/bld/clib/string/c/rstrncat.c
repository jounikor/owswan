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
* Description:  Implementation of strncat() for RISC architectures.
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include <string.h>
#include "riscstr.h"


CHAR_TYPE *__F_NAME(strncat,wcsncat)( CHAR_TYPE *dest, const CHAR_TYPE *src,
                                      size_t n )
/**************************************************************************/
{
    size_t              srcLen;
    size_t              destLen;

    srcLen = __F_NAME(strlen,wcslen)( src );
    destLen = __F_NAME(strlen,wcslen)( dest );

    if( srcLen < n ) {
        __F_NAME(strncpy,wcsncpy)( dest + destLen, src, srcLen + 1 );
    } else {
        __F_NAME(strncpy,wcsncpy)( dest + destLen, src, n );
        dest[destLen + n] = NULLCHAR;
    }

    return( dest );
}
