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
* Description:  Sample window #2.
*
****************************************************************************/



/*
 * IMPORTANT NOTE: This is a very weird window. The NextRow routine
 *                 is only needed if you want to have tabstops in
 *                 your title line. If not, there's a much easier way
 *                 to have a title. See window 4b.
 */


#include "app.h"

typedef struct {
    int         top;
    int         saved_top;
    a_window    wnd;
    char        *words[1];
} w2_struct;

#define TITLE_SIZE 2
static gui_menu_struct W2PopUp[] = {
    { "&Say",       MENU_W2_SAY,    GUI_STYLE_MENU_ENABLED },
    { "&Top",       MENU_W2_TOP,    GUI_STYLE_MENU_ENABLED },
    { "&Bottom",    MENU_W2_BOTTOM, GUI_STYLE_MENU_ENABLED },
    { "&New Title", MENU_W2_TITLE,  GUI_STYLE_MENU_ENABLED },
    { "&Open 1",    MENU_W2_OPEN1,  GUI_STYLE_MENU_ENABLED },
};

static gui_menu_struct W2AltPopUp[] = {
    { "&New Title", MENU_W2_TITLE,  GUI_STYLE_MENU_ENABLED },
    { "&Open 1",    MENU_W2_OPEN1,  GUI_STYLE_MENU_ENABLED },
    { "&Say",       MENU_W2_SAY,    GUI_STYLE_MENU_ENABLED },
    { "&Top",       MENU_W2_TOP,    GUI_STYLE_MENU_ENABLED },
};

static void Pos( a_window wnd, int pos )
{
    w2_struct   *w2 = (w2_struct *)WndExtra( wnd );
    int         last;

    last = WORD_SIZE - WndRows( wnd ) + TITLE_SIZE;
    if( pos < 0 )
        pos = 0;
    if( pos > last )
        pos = last;
    w2->top = pos;
    WndSetThumbPercent( w2->wnd, pos * 100L / last );
}


static int W2Scroll( a_window wnd, int lines )
{
    w2_struct   *w2 = (w2_struct *)WndExtra( wnd );
    int         old_top;

    old_top = w2->top;
    Pos( wnd, old_top + lines );
    WndSetRepaint( wnd );
    return( w2->top - old_top );
}


static void W2Modify( a_window wnd, wnd_row row, wnd_piece piece )
{
    w2_struct   *w2 = (w2_struct *)WndExtra( wnd );

    piece=piece;
    if( row == 0 ) {
        Say( "Modify title" );
    } else if( row == 1 ) {
        Say( "Shouldn't get this event" );
    } else {
        row += w2->top - TITLE_SIZE;
        Say2( "Modify", w2->words[row] );
    }
}


static wnd_row W2NextRow( a_window wnd, wnd_row row, int inc )
{
    wnd_row     new;
    w2_struct   *w2 = (w2_struct *)WndExtra( wnd );

    if( row == WND_NO_ROW ) {
        if( inc == WND_SAVE_ROW ) {
            w2->saved_top = w2->top;
        } else if( inc == WND_RESTORE_ROW ) {
            Pos( wnd, w2->saved_top );
        }
        return( 0 );
    }
    new = row + inc;
    if( row >= TITLE_SIZE && new < TITLE_SIZE ) { // backing over title
        if( w2->top >= -inc ) {
            new = row;
            W2Scroll( wnd, inc );
        } else {
            new = row + inc + w2->top;
            W2Scroll( wnd, -w2->top );
        }
    } else if( new >= WndRows( wnd ) ) {
        new = row;
        W2Scroll( wnd, inc );
    }
    return( new );
}


static void     W2MenuItem( a_window wnd, gui_ctl_id id, wnd_row row, wnd_piece piece )
{

    char        buff[80];

    row=row;piece=piece;
    switch( id ) {
    case MENU_INITIALIZE:
        if( row < 0 )
            break;
        if( row & 1 ) {
            WndSetPopUpMenu( wnd, ArraySize( W2PopUp ), W2PopUp );
        } else {
            WndSetPopUpMenu( wnd, ArraySize( W2AltPopUp ), W2AltPopUp );
        }
        break;
    case MENU_W2_SAY:
        Say2( "Say", WndPopItem( wnd ) );
        break;
    case MENU_W2_OPEN1:
        W1Open();
        break;
    case MENU_W2_TOP:
        Pos( wnd, 0 );
        break;
    case MENU_W2_BOTTOM:
        Pos( wnd, WORD_SIZE - 1 );
        break;
    case MENU_W2_TITLE:
        buff[0] = '\0';
        DlgNew( "Enter New Title", buff, sizeof( buff ) );
        WndSetTitle( wnd, buff );
        break;
    }
}

static bool W2GetLine( a_window wnd, wnd_row row, wnd_piece piece, wnd_line_piece *line )
{
    w2_struct   *w2 = (w2_struct *)WndExtra( wnd );

    if( row < TITLE_SIZE ) {
        if( row == 0 ) {
            if( piece == 0 ) {
                line->text = "<TAB1>";
            } else if( piece == 1 ) {
                line->text = "<TAB2>";
                line->indent = WndWidth( wnd ) / 2;
            } else {
                return( false );
            }
        } else if( row == 1 ) {
            if( piece != 0 )
                return( false );
            line->text = "--------------------------------------------";
            line->static_text = true;
            line->tabstop = false;
            return( true );
        }
    } else {
        if( piece != 0 )
            return( false );
        row += w2->top - TITLE_SIZE;
        if( row >= WORD_SIZE )
            return( false );
        line->text = w2->words[row];
    }
    return( true );
}


static void W2Refresh( a_window wnd )
{
    WndSetRepaint( wnd );
}

static int WordCompare( char **a, char **b )
{
    return( stricmp( *a, *b ) );
}

static bool W2WndEventProc( a_window wnd, gui_event gui_ev, void *parm )
{
    w2_struct   *w2 = (w2_struct *)WndExtra( wnd );

    parm=parm;
    switch( gui_ev ) {
    case GUI_INIT_WINDOW:
        memcpy( w2->words, Word, WORD_SIZE * sizeof( char ** ) );
        qsort( w2->words,
               WORD_SIZE,
               sizeof( char** ),
               (int (*)(const void *, const void *))WordCompare );
        w2->wnd = wnd;
        w2->top = 0;
        WndSetRepaint( wnd );
        return( true );
    case GUI_RESIZE :
        Pos( wnd, 0 );
        WndSetRepaint( wnd );
        return( true );
    case GUI_DESTROY :
        WndFree( w2 );
        return( true );
    default :
        break;
    }
    return( false );
}

static bool ChkUpdate( void )
{
    return( WndUpdateFlags & EV_UPDATE_2 );
}

static wnd_info W2Info = {
    W2WndEventProc,
    W2Refresh,
    W2GetLine,
    W2MenuItem,
    W2Scroll,
    NoBegPaint,
    NoEndPaint,
    W2Modify,
    NoNumRows,
    W2NextRow,
    NoNotify,
    ChkUpdate,
    PopUp( W2PopUp )
};

a_window W2Open( void )
{
    w2_struct   *w2;
    a_window    wnd;
    wnd_create_struct   info;

    w2 = WndMustAlloc( WORD_SIZE * sizeof( char * ) + sizeof( *w2 ) );
    WndInitCreateStruct( &info );
    info.scroll_style &= ~GUI_VDRAG;
    info.title = "window with a title";
    info.info = &W2Info;
    info.wndclass = CLASS_W2;
    info.extra = w2;
    wnd = WndCreateWithStruct( &info );
    if( wnd != NULL ) {
        WndSetSwitches( wnd, WSW_RBUTTON_SELECTS | WSW_MULTILINE_SELECT | WSW_SUBWORD_SELECT | WSW_MAP_CURSOR_TO_SCROLL );
        Pos( wnd, 0 );
    }
    return( wnd );
}
