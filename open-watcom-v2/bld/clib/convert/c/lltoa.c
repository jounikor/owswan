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
* Description:  Implementation of lltoa() - convert long long to string.
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include <stdlib.h>
#include "watcom.h"
#include "_xtoa.h"


#define TO_WIDE(c)  ((CHAR_TYPE)(c))

typedef unsigned __based(__segname("_STACK")) *uint_stk_ptr;

#if defined( _M_IX86 )
unsigned long long __ulldiv( unsigned long long, uint_stk_ptr );
#if defined(__386__) && defined(__SMALL_DATA__)
    #pragma aux __ulldiv = \
            "xor    ecx,ecx"            /* set high word of quotient to 0 */ \
            "cmp    edx,dword ptr[ebx]" /* if quotient will be >= 4G */ \
            "jb short less4g"           /* then */ \
            "mov    ecx,eax"            /* - save low word of dividend */ \
            "mov    eax,edx"            /* - get high word of dividend */ \
            "xor    edx,edx"            /* - zero high part */ \
            "div    dword ptr[ebx]"     /* - divide into high part of dividend */ \
            "xchg   eax,ecx"            /* - swap high part of quot,low word of dvdnd */ \
        "less4g:"                       \
            "div    dword ptr[ebx]"     /* calculate low part */ \
            "mov    [ebx],edx"          /* store remainder */ \
        __parm  [__eax __edx] [__ebx] \
        __value [__eax __ecx]
#elif defined( __386__ )  && defined(__BIG_DATA__)
    #pragma aux __ulldiv = \
            "xor    ecx,ecx"            /* set high word of quotient to 0 */ \
            "cmp    edx,dword ptr ss:[ebx]" /* if quotient will be >= 4G */ \
            "jb short less4g"           /* then */ \
            "mov    ecx,eax"            /* - save low word of dividend */ \
            "mov    eax,edx"            /* - get high word of dividend */ \
            "xor    edx,edx"            /* - zero high part */ \
            "div    dword ptr ss:[ebx]" /* - divide into high part of dividend */ \
            "xchg   eax,ecx"            /* - swap high part of quot,low word of dvdnd */ \
        "less4g:"                       \
            "div    dword ptr ss:[ebx]" /* calculate low part */ \
            "mov    ss:[ebx],edx"       /* store remainder */ \
        __parm  [__eax __edx] [__ebx] \
        __value [__eax __ecx]
#elif defined( _M_I86 )  && defined(__BIG_DATA__)
    #pragma aux __ulldiv = \
            "mov    di,dx"              /* initial dividend = ax:bx:cx:dx(di); save dx */ \
            "test   ax,ax"              /* less work to do if ax == 0 */ \
            "jz short skip1"            \
            "mov    dx,ax"              /* dx:ax = ax:bx */ \
            "mov    ax,bx"              \
            "xor    bx,bx"              /* set word 3 of quotient to 0 */ \
            "cmp    dx,word ptr ss:[si]" /* if quotient will be >= 64K */ \
            "jb short div2"             /* then */ \
            "mov    bx,ax"              /* restore word 2 of dividend */ \
            "mov    ax,dx"              /* restore word 3 of dividend */ \
            "xor    dx,dx"              /* - zero high part */ \
            "div    word ptr ss:[si]"   /* - divide into word 3 of dividend */ \
            "xchg   ax,bx"              /* - swap word 3,word 2 of dvdnd */ \
        "div2:"                         \
            "div    word ptr ss:[si]"   /* - divide into word 2 of dividend */ \
            "xchg   ax,cx"              /* - swap word 2,word 1 of dvdnd */ \
        "div3:"                         \
            "div    word ptr ss:[si]"   /* - divide into word 1 of dividend */ \
            "xchg   ax,di"              /* - swap word 1,word 0 of dvdnd */ \
        "div4:"                         \
            "div    word ptr ss:[si]"   /* calculate low part */ \
            "mov    ss:[si],dx"         /* store remainder */ \
            "mov    dx,ax"              /* dx is word 0 */ \
            "mov    ax,bx"              /* ax:bx:cx:dx = bx:cx:di:ax */ \
            "mov    bx,cx"              \
            "mov    cx,di"              \
            "jmp short end_div"         \
        "skip1:"                        /* ax==0 */  \
            "test   bx,bx"              /* even less work to do if bx == 0 too */ \
            "jz short skip2"            \
            "mov    dx,bx"              /* dx:ax = bx:cx */ \
            "mov    ax,cx"              \
            "xor    bx,bx"              /* set word 3 of quotient to 0 */ \
            "xor    cx,cx"              /* set word 2 of quotient to 0 */ \
            "cmp    dx,word ptr ss:[si]" /* if quotient will be < 64K */ \
            "jb short div3"             /* then need to do two divisions */ \
            "mov    cx,ax"              /* restore word 1 of dividend */ \
            "mov    ax,dx"              /* restore word 2 of dividend */ \
            "xor    dx,dx"              /* zero high part */ \
            "jmp short div2"            /* do three divisions*/ \
        "skip2:"                        /* ax==bx==0 */ \
            "mov    dx,cx"              /* dx:ax = cx:di */ \
            "mov    ax,di"              \
            "xor    cx,cx"              /* set word 2 of quotient to 0 */ \
            "xor    di,di"              /* set word 1 of quotient to 0 */ \
            "cmp    dx,word ptr ss:[si]" /* if quotient will be < 64K */ \
            "jb short div4"             /* then only one division to do */ \
            "mov    di,ax"              /* restore word 0 of dividend */ \
            "mov    ax,dx"              /* restore word 1 of dividend */ \
            "xor    dx,dx"              /* zero high part */ \
            "jmp short div3"            /* do two divisions */ \
        "end_div:"                      \
        __parm      [__ax __bx __cx __dx] [__si] \
        __value     [__ax __bx __cx __dx] \
        __modify    [__di]
#elif defined( _M_I86 ) && defined(__SMALL_DATA__)
    #pragma aux __ulldiv = \
            "mov    di,dx"              /* initial dividend = ax:bx:cx:dx(di); save dx */ \
            "test   ax,ax"              /* less work to do if ax == 0 */ \
            "jz short skip1"            \
            "mov    dx,ax"              /* dx:ax = ax:bx */ \
            "mov    ax,bx"              \
            "xor    bx,bx"              /* set word 3 of quotient to 0 */ \
            "cmp    dx,word ptr[si]"    /* if quotient will be >= 64K */ \
            "jb short div2"             /* then */ \
            "mov    bx,ax"              /* restore word 2 of dividend */ \
            "mov    ax,dx"              /* restore word 3 of dividend */ \
            "xor    dx,dx"              /* - zero high part */ \
            "div    word ptr[si]"       /* - divide into word 3 of dividend */ \
            "xchg   ax,bx"              /* - swap word 3,word 2 of dvdnd */ \
        "div2:"                         \
            "div    word ptr[si]"       /* - divide into word 2 of dividend */ \
            "xchg   ax,cx"              /* - swap word 2,word 1 of dvdnd */ \
        "div3:"                         \
            "div    word ptr[si]"       /* - divide into word 1 of dividend */ \
            "xchg   ax,di"              /* - swap word 1,word 0 of dvdnd */ \
        "div4:"                         \
            "div    word ptr[si]"       /* calculate low part */ \
            "mov    [si],dx"            /* store remainder */ \
            "mov    dx,ax"              /* dx is word 0 */ \
            "mov    ax,bx"              /* ax:bx:cx:dx = bx:cx:di:ax */ \
            "mov    bx,cx"              \
            "mov    cx,di"              \
            "jmp short end_div"         \
        "skip1:"                        /* dx==0 */  \
            "test   bx,bx"              /* even less work to do if bx == 0 too */ \
            "jz short skip2"            \
            "mov    dx,bx"              /* dx:ax = bx:cx */ \
            "mov    ax,cx"              \
            "xor    bx,bx"              /* set word 3 of quotient to 0 */ \
            "xor    cx,cx"              /* set word 2 of quotient to 0 */ \
            "cmp    dx,word ptr[si]"    /* if quotient will be < 64K */ \
            "jb short div3"             /* then need to do two divisions */ \
            "mov    cx,ax"              /* restore word 1 of dividend */ \
            "mov    ax,dx"              /* restore word 2 of dividend */ \
            "xor    dx,dx"              /* zero high part */ \
            "jmp short div2"            /* do three divisions*/ \
        "skip2:"                        /* ax==bx==0 */ \
            "mov    dx,cx"              /* dx:ax = cx:di */ \
            "mov    ax,di"              \
            "xor    cx,cx"              /* set word 2 of quotient to 0 */ \
            "xor    di,di"              /* set word 1 of quotient to 0 */ \
            "cmp    dx,word ptr[si]"    /* if quotient will be < 64K */ \
            "jb short div4"             /* then only one division to do */ \
            "mov    di,ax"              /* restore word 0 of dividend */ \
            "mov    ax,dx"              /* restore word 1 of dividend */ \
            "xor    dx,dx"              /* zero high part */ \
            "jmp short div3"            /* do two divisions */ \
        "end_div:"                      \
        __parm      [__ax __bx __cx __dx] [__si] \
        __value     [__ax __bx __cx __dx] \
        __modify    [__di]
#endif
#endif

_WCRTLINK CHAR_TYPE *__F_NAME(ulltoa,_ulltow)(
        unsigned long long value,
        CHAR_TYPE *buffer,
        int radix )
{
    CHAR_TYPE       *p = buffer;
    char            *q;
    unsigned        rem;
    char            buf[66];    // only holds ASCII so 'char' is OK

    buf[0] = '\0';
    q = &buf[1];
    do {
#if defined( _M_IX86 )
        rem = radix;
        value = __ulldiv( value, &rem );
#else
        rem = value % radix;
        value = value / radix;
#endif
        *q = __Alphabet[ rem ];
        ++q;
    } while( value );
    while( (*p++ = TO_WIDE( *--q )) != NULLCHAR )
        ;
    return( buffer );
}


_WCRTLINK CHAR_TYPE *__F_NAME(lltoa,_lltow)(
        long long value,
        CHAR_TYPE *buffer,
        int radix )
{
    CHAR_TYPE       *p = buffer;

    if( radix == 10 ) {
        if( value < 0 ) {
            *p++ = STRING( '-' );
            value = -value;
        }
    }
    __F_NAME(ulltoa,_ulltow)( value, p, radix );
    return( buffer );
}
