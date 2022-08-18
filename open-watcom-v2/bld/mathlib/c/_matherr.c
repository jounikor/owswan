/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2017 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  _matherr() implementation.
*
****************************************************************************/


#include "variety.h"
#include <fenv.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include "clibsupp.h"
#include "_matherr.h"
#include "extfunc.h"


#define _RWD_matherr    __matherr_handler

typedef int     matherr_fn( struct _exception * );
typedef matherr_fn __matherr_fn;
#if defined(_M_IX86)
    #pragma aux (__outside_CLIB) __matherr_fn;
#endif

static __matherr_fn *__matherr_handler = (__matherr_fn *)matherr;

static const char * const Msgs[] = {
    /* see cmath/math.h header file for definitions */
    0,
    "Domain error",
    "Argument singularity",
    "Overflow range error",
    "Underflow range error",
    "Total loss of significance",
    "Partial loss of significance"
};

static char *MathFuncNames[] = {
    #define pick(enum,name) name,
    #include "_mathfun.h"
    #undef pick
};

_WMRTLINK void _set_matherr( matherr_fn *rtn )
{
    _RWD_matherr = (__matherr_fn *)rtn;
}

void __rterrmsg( int errcode, const char *funcname )
{
    FILE    *fp;

    fp = __get_std_stream( STDERR_FILENO );
    fputs( Msgs[errcode], fp );
    fputs( " in ", fp );
    fputs( funcname, fp );
    fputc( '\n', fp );
}

char *__rtmathfuncname( int funccode )
{
    return( MathFuncNames[funccode] );
}

_WMRTLINK double _matherr( struct _exception *excp )
/**************************************************/
{
    if( _RWD_matherr == NULL || (*_RWD_matherr)( excp ) == 0 ) {
        __rterrmsg( excp->type, excp->name );
        excp->type == DOMAIN ? __set_EDOM() : __set_ERANGE();
    }
    return( excp->retval );
}

_WMRTLINK void __reporterrorsimple( int type )
{
   if( math_errhandling & MATH_ERRNO ) {
        switch( type ) {
        case DOMAIN:
            errno = EDOM;
            break;
        case OVERFLOW:
        case UNDERFLOW:
        case SING:
            errno = ERANGE;
            break;
        }
    }

    if( math_errhandling & MATH_ERREXCEPT ) {
        switch( type ) {
        case DOMAIN:
            feraiseexcept( FE_INVALID );
            break;
        case OVERFLOW:
            feraiseexcept( FE_OVERFLOW );
            break;
        case UNDERFLOW:
            feraiseexcept( FE_UNDERFLOW );
            break;
        case TLOSS:
        case PLOSS:
            feraiseexcept( FE_INEXACT );
            break;
        case SING:
            feraiseexcept( FE_DIVBYZERO );
            break;
        }
    }
}

_WMRTLINK int __reporterror( int type, const char *name, double arg1, double arg2, double retval )
{
    int                 ret;
    struct _exception   report;

    ret = 0;
    __reporterrorsimple( type );
    if( _RWD_matherr != NULL ) {
        if( math_errhandling & MATH_ERRWATCOM ) {
            report.type = type;
            report.name = name;
            report.arg1 = arg1;
            report.arg2 = arg2;
            report.retval = retval;
            ret = (*_RWD_matherr)( &report );
        }
    }
    return( ret );
}
