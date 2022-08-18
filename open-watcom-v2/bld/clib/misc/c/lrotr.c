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
* Description:  Implementation of _lrotr().
*
****************************************************************************/


#undef __INLINE_FUNCTIONS__
#include "variety.h"
#include <stdlib.h>
#include <limits.h>

extern  unsigned long __lrotr( unsigned long, unsigned );

#if defined(__386__)
#pragma aux __lrotr = \
        "ror eax,cl" \
    __parm      [__eax] [__ecx] \
    __value     [__eax] \
    __modify    [__ecx]
#elif defined( _M_I86 )
#pragma aux __lrotr = \
        "and cx,31"     \
        "jcxz short L2" \
    "L1: ror dx,1"      \
        "rol dx,1"      \
        "rcr ax,1"      \
        "rcr dx,1"      \
        "dec cx"        \
        "jne short L1"  \
    "L2:"               \
    __parm [__ax __dx] [__cx] \
    __value [__ax __dx]
#endif


_WCRTLINK unsigned long _lrotr( unsigned long value, unsigned int shift )
{
#if defined( _M_IX86 )
    return( __lrotr( value, shift ) );
#else
    unsigned long   tmp;

    tmp = value;
    value = value >> shift;
    tmp = tmp << ((sizeof( tmp ) * CHAR_BIT) - shift);
    value = value | tmp;
    return( value );
#endif
}
