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
* Description:  WHEN YOU FIGURE OUT WHAT THIS MODULE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "app.h"


static char * Stuff[] = {
        "Line 1",
        "Line 2",
        "Line 3",
        "Line 4",
        "Line 5",
};

static wnd_row W6NumRows( a_window wnd )
{
    wnd=wnd;
    return( ArraySize( Stuff ) );
}

static bool    W6GetLine( a_window wnd, wnd_row row, wnd_piece piece, wnd_line_piece *line )
{
    wnd=wnd;
    if( row >= ArraySize( Stuff ) )
        return( false );
    if( piece != 0 )
        return( false );
    line->text = Stuff[row];
    return( true );
}


static void    W6Refresh( a_window wnd )
{
    WndSetRepaint( wnd );
}

static wnd_metrics W6Metrics = { 3, 12, 0, 0 };

static wnd_info W6Info = {
    NoWndEventProc,
    W6Refresh,
    W6GetLine,
    NoMenuItem,
    NoVScroll,
    NoBegPaint,
    NoEndPaint,
    NoModify,
    W6NumRows,
    NoNextRow,
    NoNotify,
    NoChkUpdate,
    NoPopUp
};

a_window W6Open( void )
{
    wnd_create_struct   info;
    a_window            wnd;

    WndInitCreateStruct( &info );
    info.info = &W6Info;
    info.style |= GUI_INIT_INVISIBLE | GUI_POPUP;
    wnd = WndCreateWithStruct( &info );
    WndSetFontInfo( wnd, "-13 0 0 0 700 0 0 0 0 1 2 1 18 \"MS Serif\"" );
    WndForcePaint( wnd );
    WndShrinkToMouse( wnd, &W6Metrics );
    GUIShowWindow( WndGui( wnd ) );
    return( wnd );
}
