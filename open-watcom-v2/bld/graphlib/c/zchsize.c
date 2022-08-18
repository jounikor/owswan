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


_WCRTLINK void _WCI86FAR _CGRAPH _setcharsize_w( double height, double width )
/*=============================================================

   This routine sets the height and width for graphics text output. */

{
    _setcharsize( _WtoScaleY( height ), _WtoScaleX( width ) );
}

Entry1( _SETCHARSIZE_W, _setcharsize_w ) // alternate entry-point


_WCRTLINK void _WCI86FAR _CGRAPH _setcharsize_w_87( double height, double width )
/*==============================================================*/

{
    _setcharsize_w( height, width );
}

Entry1( _SETCHARSIZE_W_87, _setcharsize_w_87 ) // alternate entry-point


_WCRTLINK void _WCI86FAR _CGRAPH _setcharspacing_w( double space )
/*=================================================

   This routine sets the character spacing for graphics text output. */

{
    _setcharspacing( _WtoScaleX( space ) );
}

Entry1( _SETCHARSPACING_W, _setcharspacing_w ) // alternate entry-point


_WCRTLINK void _WCI86FAR _CGRAPH _setcharspacing_w_87( double space )
/*==================================================*/

{
    _setcharspacing_w( space );
}

Entry1( _SETCHARSPACING_W_87, _setcharspacing_w_87 ) // alternate entry-point
