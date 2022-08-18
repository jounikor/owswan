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
* Description:  Sample window #4.
*
****************************************************************************/



#include "app.h"


static gui_menu_struct W4PopUp[] = {
    { "&Say",       MENU_W2_SAY,    GUI_STYLE_MENU_ENABLED },
    { "&Top",       MENU_W2_TOP,    GUI_STYLE_MENU_ENABLED },
    { "&Open 1",    MENU_W2_OPEN1,  GUI_STYLE_MENU_ENABLED },
};

static char * Stuff[] = {
        "Aardvark",
        "Aaron",
        "Abacus",
        "Balloon",
        "Bazaar",
        "Bizare",
        "Crazy",
        "Dashing",
        "Dastardly",
        "Deadly",
        "Driven",
        "Enough",
        "Hardly",
        "Justify",
        "Loony",
        "Mulroney",
        "Mystique",
        "Nuts",
        "Poor",
        "Queen",
        "Queer",
        "Quiet",
        "Schmooze",
        "Smart",
        "Stupid",
        "Vegtable",
        "Veritable",
        "Virtual",
        "Wobble",
        "Weeble",
        "Woozy",
        "Xray",
        "Xylophone",
        "Zap",
        "Znaimer",
        "Zoot Suit",
        "Zulu",
};

#define SIZE ArraySize( Stuff )

static wnd_row W4NumRows( a_window wnd )
{
    wnd=wnd;
    return( SIZE );
}

static void    W4Modify( a_window wnd, wnd_row row, wnd_piece piece )
{
    wnd=wnd;piece=piece;
    if( row < 0 ) {
        Say( "Shouldn't get this event" );
    } else {
        Say2( "Modify", Stuff[row] );
    }
}


#if 0
static char UiMapChar[] = { 0xC6, 0xEA, 0xC7, 0xD0,
                                        0xD1, 0xEB, 0xD2, 0xD3,
                                        0xD4, 0xCB, 0xCA, 0xC5,
                                        0xCC, 0xBA, 0xCE, 0xCD,
                                        0xDF, 0xDC, 0xFD, 0xF5, 0 };
#endif

static bool W4GetLine( a_window wnd, wnd_row row, wnd_piece piece, wnd_line_piece *line )
{
    static char buff[20];

    wnd=wnd;
    if( row == -2 ) {
        if( piece != 0 )
            return( false );
        line->text = "Title line 1";
        line->tabstop = false;
        line->static_text = true;
    } else if( row == -1 ) {
        if( piece != 0 )
            return( false );
        line->tabstop = false;
        line->static_text = true;
        #if 0
            line->text = UiMapChar;
        #else
            line->text = "";
            line->underline = true;
            line->extent = WndWidth( wnd );
            line->indent = 0;
        #endif
    } else if( row >= SIZE ) {
        return( false );
    } else {
        switch( piece ) {
        case 0:
            line->tabstop = true;
            itoa( row, buff, 10 );
            line->text = buff;
            line->extent = WND_MAX_EXTEND;
            return( true );
        case 1:
            line->tabstop = false;
            line->use_prev_attr = true;
            line->text = "";
            line->extent = WND_MAX_EXTEND;
            line->indent = 1000;
            return( true );
        case 2:
            line->tabstop = false;
            line->use_prev_attr = true;
            line->text = Stuff[row];
            line->extent = WND_MAX_EXTEND;
            line->indent = 2000;
            return( true );
        case 3:
            line->tabstop = false;
            line->use_prev_attr = true;
            line->text = "";
            line->extent = WND_MAX_EXTEND;
            line->indent = 3000;
            return( true );
        default:
            return( false );
        }
    }
    return( true );
}


static void W4Refresh( a_window wnd )
{
    WndSetRepaint( wnd );
}

static void W4MenuItem( a_window wnd, gui_ctl_id id, wnd_row row, wnd_piece piece )
{

    row=row;piece=piece;
    switch( id ) {
    case MENU_INITIALIZE:
        WndMenuGrayAll( wnd );
        if( row < 0 )
            break;
        WndMenuEnableAll( wnd );
        break;
    case MENU_W2_SAY:
        Say2( "Say", WndPopItem( wnd ) );
        break;
    case MENU_W2_OPEN1:
        W1Open();
        break;
    case MENU_W2_TOP:
        WndVScrollAbs( wnd, 0 );
        break;
    }
}

static wnd_info W4Info = {
    NoWndEventProc,
    W4Refresh,
    W4GetLine,
    W4MenuItem,
    NoVScroll,
    NoBegPaint,
    NoEndPaint,
    W4Modify,
    W4NumRows,
    NoNextRow,
    NoNotify,
    NoChkUpdate,
    PopUp( W4PopUp )
};

a_window W4Open( void )
{
    a_window    wnd;

    wnd = WndCreate( "", &W4Info, 0, NULL );
    if( wnd != NULL )
        WndSetKeyPiece( wnd, 1 );
    return( wnd );
}
