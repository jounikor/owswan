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

static bool W1Init( a_window wnd );

static gui_menu_struct W1Sub[] = {
    { "&Align",    MENU_W1_ALIGN,   GUI_STYLE_MENU_ENABLED },
    { "&UnAlign",  MENU_W1_UNALIGN, GUI_STYLE_MENU_ENABLED },
};

static gui_menu_struct W1PopUp[] = {
    { "&Say",      MENU_W1_SAY,     GUI_STYLE_MENU_ENABLED },
    { "&More",     MENU_W1_MORE,    GUI_STYLE_MENU_ENABLED, NULL, ArraySize( W1Sub ), W1Sub },
    { "&NewWord",  MENU_W1_NEWWORD, GUI_STYLE_MENU_ENABLED },
};

typedef struct {
    int         num_pieces;
    char        **pieces;
} w1_row;

typedef struct {
    int         num_rows;
    w1_row      *rows;
    unsigned    align : 1;
} w1_window;

static void W1MenuItem( a_window wnd, gui_ctl_id id, wnd_row row, wnd_piece piece )
{
    w1_window   *w1 = (w1_window *)WndExtra( wnd );
    char        buff[80];

    if( piece == 0 )
        return;
    --piece;
    switch( id ) {
    case MENU_INITIALIZE:
        WndMenuGrayAll( wnd );
        if( row < 0 )
            break;
        WndMenuEnable( wnd, MENU_W1_ALIGN, ( row & 1 ) == 0 );
        WndMenuEnable( wnd, MENU_W1_UNALIGN, ( row & 1 ) != 0 );
        WndMenuEnable( wnd, MENU_W1_SAY, ( row & 2 ) == 0 );
        WndMenuEnable( wnd, MENU_W1_NEWWORD, ( row & 2 ) != 0 );
        break;
    case MENU_W1_ALIGN:
        w1->align = true;
        WndUpdateFlags |= EV_UPDATE_1;
        break;
    case MENU_W1_UNALIGN:
        w1->align = false;
        WndUpdateFlags |= EV_UPDATE_1;
        break;
    case MENU_W1_SAY:
        Say( w1->rows[row].pieces[piece] );
        break;
    case MENU_W1_NEWWORD:
        buff[0]='\0';
        DlgNew( "Enter New Word", buff, 80 );
        Word[RandNum( WORD_SIZE ) - 1] = (char *)strdup( buff ); // nyi - never freed
        WndUpdateFlags |= EV_UPDATE_1;
        W1Init( wnd );
        break;
    }
}

static wnd_row W1NumRows( a_window wnd )
{
    w1_window   *w1 = (w1_window *)WndExtra( wnd );

    return( w1->num_rows );
}

static void W1Modify( a_window wnd, wnd_row row, wnd_piece piece )
{
    w1_window   *w1 = (w1_window *)WndExtra( wnd );

    if( piece == 0 )
        return;
    --piece;
    Say2( "Modify", w1->rows[row].pieces[piece] );
}


static bool    W1GetLine( a_window wnd, wnd_row row, wnd_piece piece, wnd_line_piece *line )
{
    w1_window   *w1 = (w1_window *)WndExtra( wnd );
    int         i;

    if( row >= w1->num_rows )
        return( false );
    if( piece == 0 ) {
        WndSetGadgetLine( wnd, line, row & 1, 500 );
        return( true );
    }
    --piece;
    if( piece >= w1->rows[row].num_pieces )
        return( false );
    line->text = w1->rows[row].pieces[piece];
    line->hint = (( row + 1 ) & 1) ? "This is help for an even row" : "This is help for an odd row";
    if( strcmp( line->text, "Censorship" ) == 0 ) {
        line->attr = APP_COLOR_CENSORED;
        line->static_text = true;
    }
    if( WndSwitchOn( wnd, WSW_LBUTTON_SELECTS ) ) {
        line->tabstop = false;
    }
    if( strcmp( line->text, "Baseball" ) == 0 ) {
        line->attr = APP_COLOR_BASEBALL;
        line->tabstop = true;
    }
    if( strcmp( line->text, "*HotSpot*" ) == 0 ) {
        line->hot = true;
        line->attr = APP_COLOR_HOTSPOT;
    }
    if( w1->align ) {
        line->indent = ( MAX_WORD + 1 ) * piece * WndAvgCharX( wnd );
    } else {
        line->indent = 0;
        for( i = 0; i < piece; ++i ) {
            line->indent += WndAvgCharX( wnd ) + WndExtentX( wnd,w1->rows[row].pieces[i] );
        }
    }
    line->indent += 500;
    return( true );
}


static void    W1Refresh( a_window wnd )
{
    WndSetRepaint( wnd );
}

static void W1Fini( a_window wnd )
{
    w1_window   *w1 = (w1_window *)WndExtra( wnd );
    int         num_rows;
    int         i;

    num_rows = w1->num_rows;
    for( i = 0; i < num_rows; ++i ) {
        WndFree( w1->rows[i].pieces );
    }
    WndFree( w1->rows );
    WndFree( w1 );
}


static bool W1Init( a_window wnd )
{
    w1_window   *w1 = (w1_window *)WndExtra( wnd );
    int         num_rows;
    int         pieces;
    int         i,j;

    num_rows = RandNum( 200 );
    w1->num_rows = num_rows;
    w1->rows = WndAlloc( num_rows * sizeof( w1_row ) );
    w1->align = true;
    if( w1->rows == NULL ) {
        WndClose( wnd );
        WndNoMemory();
        return( false );
    }
    for( i = 0; i < num_rows; ++i ) {
        pieces = RandNum( 10 );
        w1->rows[i].pieces = WndAlloc( pieces * sizeof( char * ) );
        w1->rows[i].num_pieces = pieces;
        for( j = 0; j < pieces; ++j ) {
            w1->rows[i].pieces[j] = Word[RandNum( WORD_SIZE ) - 1];
        }
    }
    return( true );
}

static bool W1WndEventProc( a_window wnd, gui_event gui_ev, void *parm )
{
    parm=parm;
    switch( gui_ev ) {
    case GUI_INIT_WINDOW:
        W1Init( wnd );
        WndFirstCurrent( wnd );
        W1Refresh( wnd );
        return( true );
    case GUI_DESTROY :
        W1Fini( wnd );
        return( true );
    default :
        break;
    }
    return( false );
}

static bool ChkUpdate( void )
{
    return( WndUpdateFlags & EV_UPDATE_1 );
}

static wnd_info W1Info = {
    W1WndEventProc,
    W1Refresh,
    W1GetLine,
    W1MenuItem,
    NoVScroll,
    NoBegPaint,
    NoEndPaint,
    W1Modify,
    W1NumRows,
    NoNextRow,
    NoNotify,
    ChkUpdate,
    PopUp( W1PopUp )
};

static gui_resource W1Icon = { ICON_WORDS, "W1" };

a_window W1Open( void )
{
    wnd_create_struct   info;
    a_window            wnd;

    WndInitCreateStruct( &info );
    info.info = &W1Info;
    info.extra = WndMustAlloc( sizeof( w1_window ) );
    info.wndclass = CLASS_W1;
    info.title = "Words";
    info.style |= GUI_INIT_INVISIBLE;
    wnd = WndCreateWithStruct( &info );
    WndSetIcon( wnd, &W1Icon );
    WndForcePaint( wnd );
    GUIShowWindow( WndGui( wnd ) );
    return( wnd );
}
