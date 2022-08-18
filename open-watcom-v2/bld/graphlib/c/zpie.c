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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "gdefn.h"


_WCRTLINK short _WCI86FAR _CGRAPH _pie_w( short fill, double x1, double y1,
/*==================================*/ double x2, double y2,
                                       double x3, double y3,
                                       double x4, double y4 )

/* This really yecchy function draws or fills a slice of a pie defined as
   per the documentation, using window coordinates. */

{
    short               success;

    if( _GrProlog() ) {
        success = _L2pie( fill, _WtoPhysX( x1 ), _WtoPhysY( y1 ),
                                _WtoPhysX( x2 ), _WtoPhysY( y2 ),
                                _WtoPhysX( x3 ), _WtoPhysY( y3 ),
                                _WtoPhysX( x4 ), _WtoPhysY( y4 ) );
        _GrEpilog();
    } else {
        success = 0;
    }
    return( success );
}

Entry1( _PIE_W, _pie_w ) // alternate entry-point


_WCRTLINK short _WCI86FAR _CGRAPH _pie_wxy( short fill, struct _wxycoord _WCI86FAR * p1,
/*====================================*/ struct _wxycoord _WCI86FAR * p2,
                                         struct _wxycoord _WCI86FAR * p3,
                                         struct _wxycoord _WCI86FAR * p4 )

{
    return( _pie_w( fill, p1->wx, p1->wy, p2->wx, p2->wy,
                          p3->wx, p3->wy, p4->wx, p4->wy ) );
}

Entry1( _PIE_WXY, _pie_wxy ) // alternate entry-point


_WCRTLINK short _WCI86FAR _CGRAPH _pie_w_87( short fill, double x1, double y1,
/*=====================================*/ double x2, double y2,
                                          double x3, double y3,
                                          double x4, double y4 )

{
    return( _pie_w( fill, x1, y1, x2, y2, x3, y3, x4, y4 ) );
}

Entry1( _PIE_W_87, _pie_w_87 ) // alternate entry-point
