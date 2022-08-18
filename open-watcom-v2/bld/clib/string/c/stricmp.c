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
* Description:  Implementation of _stricmp() and _wcsicmp().
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include <ctype.h>
#include <string.h>
#include "riscstr.h"

/* return <0 if s<t, 0 if s==t, >0 if s>t */

#if defined(__RISCSTR__) && defined(__WIDECHAR__)
 int __simple__wcsicmp( const CHAR_TYPE *s, const CHAR_TYPE *t ) {
#else
 _WCRTLINK int __F_NAME(_stricmp,_wcsicmp)( const CHAR_TYPE *s, const CHAR_TYPE *t ) {
#endif
    UCHAR_TYPE c1;
    UCHAR_TYPE c2;

    for( ;; ) {
        c1 = *s;
        c2 = *t;
        if( IS_ASCII_CHAR( c1 ) && IS_ASCII_CHAR( c2 ) ) {
            if( c1 >= STRING( 'A' ) && c1 <= STRING( 'Z' ) )
                c1 += STRING( 'a' ) - STRING( 'A' );
            if( c2 >= STRING( 'A' ) && c2 <= STRING( 'Z' ) ) {
                c2 += STRING( 'a' ) - STRING( 'A' );
            }
        }
        if( c1 != c2 )
            break;
        if( c1 == NULLCHAR )
            break;
        ++s;
        ++t;
    }
    return( c1 - c2 );
}
