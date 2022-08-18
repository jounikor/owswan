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
* Description:  Scale a number up or down into desired interval.
*
****************************************************************************/


#include "variety.h"

typedef struct pow10table {
    short   exponent;
    double  powerof10;
} pow10table;


extern  const pow10table    _BigPow10Table[8];
extern  void                _Rnd2Int( double _WCNEAR *,int _WCNEAR * );
extern  int                 _CmpBigInt(int, int _WCNEAR *);
extern  double              _Scale10V( double, int );

#if defined(__386__)
#pragma aux     _Rnd2Int        "_*" __parm __routine [__eax] [__edx]
#pragma aux     _CmpBigInt      "_*" __parm __caller [__eax] [__edx]
#elif defined( _M_I86 )
#pragma aux     _Rnd2Int        "_*" __parm __routine [__ax] [__dx]
#pragma aux     _CmpBigInt      "_*" __parm __caller [__ax] [__dx]
#endif


/**************************************************************************/
/***                                                                    ***/
/*** Scale - scale a number up or down so that it lies between 10**k    ***/
/***    and 10**(k-1).                                                  ***/
/***                                                                    ***/
/**************************************************************************/

int _Scale( int exponent, double realnum, int sigdigits,
            int _WCNEAR *bigint )
/******************************************************/
{
    if( exponent != sigdigits ) {
        realnum = _Scale10V( realnum, sigdigits - exponent );
    }
    _Rnd2Int( (double _WCNEAR *)&realnum, bigint );
    return( _CmpBigInt( sigdigits, bigint ) );
}


double _Scale10V( double x, int scale )
/*************************************/
{
    const pow10table    *table;
    int                 n;
    double              z;
    union u {
        unsigned short  ui[4];
        double          x;
    };
    static const union u    PlusInf  = { 0x0000, 0x0000, 0x0000, 0x7ff0 };
    static const union u    NegInf   = { 0x0000, 0x0000, 0x0000, 0xfff0 };
    static const union u    PlusInf1 = { 0xffff, 0xffff, 0xffff, 0x7fef };
    static const union u    NegInf1  = { 0xffff, 0xffff, 0xffff, 0xffef };

    if( x == PlusInf.x ) {                          /* 31-jan-91 */
        x = PlusInf1.x;
    } else if( x == NegInf.x ) {
        x = NegInf1.x;
    }
    table = _BigPow10Table;
    if( scale < 0 ) {
        n = -scale;
        if( n > 308 ) {
            x /= table->powerof10;
            n -= 216;
        }
    } else {
        n = scale;
        if( n > 308 ) {
            x *= table->powerof10;
            n -= 216;
        }
    }
    z = 1.0;
    for( ;; ) {
        if( n >= table->exponent ) {
            n -= table->exponent;
            z *= table->powerof10;
        }
        if( n == 0 ) break;
        if( table->exponent != 1 ) {
            ++table;
        }
    }
    if( scale < 0 ) {
        x /= z;
    } else {
        x *= z;
    }
    return( x );
}
