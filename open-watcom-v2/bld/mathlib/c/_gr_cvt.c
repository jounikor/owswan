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
* Description:  Wrapper routines for graph library.
*
****************************************************************************/


#include "variety.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "grfuncs.h"

#if defined( __386__ ) || defined( __AXP__ ) || defined( __PPC__ )
    #define FARstrcpy strcpy
#elif defined( _M_I86 )
    #define FARstrcpy _fstrcpy
#endif


_WMRTLINK void _WCI86FAR _GR_ecvt( float _WCI86FAR *value,
                       int            ndigits,
                       int _WCI86FAR   *far_dec,
                       int _WCI86FAR   *far_sign,
                       char _WCI86FAR  *far_buf )
{
    int         dec;
    int         sign;

    FARstrcpy( far_buf, (char _WCI86FAR *)ecvt( *value, ndigits, &dec, &sign ) );
    *far_dec  = dec;
    *far_sign = sign;
}


_WMRTLINK void _WCI86FAR _GR_fcvt( float _WCI86FAR *value,
                       int            ndigits,
                       int _WCI86FAR   *far_dec,
                       int _WCI86FAR   *far_sign,
                       char _WCI86FAR  *far_buf )
{
    int         dec;
    int         sign;

    FARstrcpy( far_buf, (char _WCI86FAR *)fcvt( *value, ndigits, &dec, &sign ) );
    *far_dec  = dec;
    *far_sign = sign;
}
