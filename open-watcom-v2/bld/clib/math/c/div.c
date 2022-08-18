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
* Description:  Implementation of div().
*
****************************************************************************/


#undef __INLINE_FUNCTIONS__
#include "variety.h"
#include <stdlib.h>


#if defined( _M_I86 )
extern div_t   __div( int, int );
#pragma aux __div = \
        "cwd"       \
        "idiv cx"   \
    __parm              [__ax] [__cx] \
    __value             [__dx __ax] \
    __modify __exact    [__ax __dx]
#endif


_WCRTLINK div_t div( int numer, int denom )
/*****************************************/
{
#ifdef _M_I86
    return( __div( numer, denom ) );
#else
    div_t   result;

    result.quot = numer / denom;
    result.rem  = numer % denom;
    return( result );
#endif
}
