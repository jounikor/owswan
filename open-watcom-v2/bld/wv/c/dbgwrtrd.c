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
* Description:  Non-blocking debug window
*
****************************************************************************/


#include <stdlib.h>
#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgwind.h"
#include "wndsys.h"
#include "dbgmisc.h"
#include "remrtrd.h"
#include "dbgupdt.h"
#include "dbgwglob.h"
#include "dbgwrtrd.h"
#include "menudef.h"


#define RUN_THREAD_INFO_TYPE_NONE       0
#define RUN_THREAD_INFO_TYPE_NAME       1
#define RUN_THREAD_INFO_TYPE_STATE      2
#define RUN_THREAD_INFO_TYPE_CS_EIP     3
#define RUN_THREAD_INFO_TYPE_EXTRA      4

#define TITLE_SIZE      2

#define MAX_PIECE_COUNT     4
#define MAX_HEADER_SIZE     80

static a_window         RunThreadWnd = 0;
static int              PieceCount = 0;
static unsigned char    Indents[MAX_PIECE_COUNT + 1];
static unsigned char    InfoType[MAX_PIECE_COUNT];
static char             HeaderArr[MAX_PIECE_COUNT][MAX_HEADER_SIZE + 1];

static gui_menu_struct  RunTrdMenu[] = {
    #include "menurtrd.h"
};

void InitRunThreadInfo( void )
{
    int     Width;
    int     i;

    PieceCount = 0;
    Indents[0] = 0;

    for(i  = 0; i < MAX_PIECE_COUNT; i++ ) {
        if( !RemoteGetRunThreadInfo( i, &InfoType[PieceCount], &Width, HeaderArr[PieceCount], MAX_HEADER_SIZE ) )
            break;
        Indents[PieceCount + 1] = Indents[PieceCount] + (unsigned char)Width;
        PieceCount++;
    }
}

static thread_state     *GetThreadRow( wnd_row row )
{
    thread_state    *thd;
    wnd_row         num;

    num = 0;
    for( thd = HeadThd; thd != NULL; thd = thd->link ) {
        if( num++ == row ) {
            break;
        }
    }
    return( thd );
}

static wnd_row RunTrdNumRows( a_window wnd )
{
    thread_state    *thd;
    wnd_row         num;

    /* unused parameters */ (void)wnd;

    num = 0;
    for( thd = HeadThd; thd != NULL; thd = thd->link )
        num++;
    return( num );
}

static bool RunTrdWndEventProc( a_window wnd, gui_event gui_ev, void *parm )
{
    /* unused parameters */ (void)parm;

    switch( gui_ev ) {
    case GUI_INIT_WINDOW:
        RunThreadWnd = wnd;
        return( true );
    case GUI_DESTROY :
        RunThreadWnd = 0;
        return( true );
    }
    return( false );
}

static void     RunTrdMenuItem( a_window wnd, gui_ctl_id id, wnd_row row, wnd_piece piece )
{
    thread_state        *thd = GetThreadRow( row );

    /* unused parameters */ (void)piece;

    switch( id ) {
    case MENU_INITIALIZE:
        if( thd == NULL ) {
            WndMenuGrayAll( wnd );
        } else {
            switch( thd->state ) {
            case THD_SIGNAL:
                WndMenuEnable( wnd, MENU_RUN_THREAD_STOP, true );
                WndMenuEnable( wnd, MENU_RUN_THREAD_SIGNAL_STOP, true );
                WndMenuEnable( wnd, MENU_RUN_THREAD_CHANGE_TO, false );
                break;

            case THD_DEBUG:
                WndMenuEnable( wnd, MENU_RUN_THREAD_STOP, false );
                WndMenuEnable( wnd, MENU_RUN_THREAD_SIGNAL_STOP, false );
                WndMenuEnable( wnd, MENU_RUN_THREAD_CHANGE_TO, true );
                break;

            case THD_RUN:
            case THD_WAIT:
            case THD_BLOCKED:
                WndMenuEnable( wnd, MENU_RUN_THREAD_STOP, true );
                WndMenuEnable( wnd, MENU_RUN_THREAD_SIGNAL_STOP, false );
                WndMenuEnable( wnd, MENU_RUN_THREAD_CHANGE_TO, false );
                break;

            default:
                WndMenuGrayAll( wnd );
                break;
            }
        }
        return;
    case MENU_RUN_THREAD_STOP:
        RemoteStopThread( thd );
        break;
    case MENU_RUN_THREAD_SIGNAL_STOP:
        RemoteSignalStopThread( thd );
        break;
    case MENU_RUN_THREAD_CHANGE_TO:
        MakeRunThdCurr( thd );
        break;
    }
    DbgUpdate( UP_THREAD_STATE );
}

static void RunTrdRefresh( a_window wnd )
{
    thread_state    *thd;
    wnd_row         row;

    row = 0;
    for( thd = HeadThd; thd != NULL; thd = thd->link ) {
        if( IsThdCurr( thd ) ) {
            WndMoveCurrent( wnd, row, 0 );
            break;
        }
        ++row;
    }
    WndNoSelect( wnd );
    WndSetRepaint( wnd );
}

static bool    RunTrdGetLine( a_window wnd, wnd_row row, wnd_piece piece, wnd_line_piece *line )
{
    thread_state        *thd = GetThreadRow( row );

    line->indent = Indents[piece] * WndAvgCharX( wnd );
    if( row < 0 ) {
        row += TITLE_SIZE;
        if( row == 0 ) {
            if( piece < PieceCount ) {
                line->text = HeaderArr[piece];
                return( true );
            }
            return( false );
        } else if( row == 1 ) {
            if( piece != 0 )
                return( false );
            SetUnderLine( wnd, line );
            return( true );
        } else {
            return( false );
        }
    } else {
        if( thd == NULL )
            return( false );
        line->tabstop = false;
        line->use_prev_attr = true;
        line->extent = WND_MAX_EXTEND;
        switch( InfoType[piece] ) {
        case RUN_THREAD_INFO_TYPE_NAME:
            line->tabstop = true;
            line->use_prev_attr = false;
            line->text = thd->name;
            return( true );
        case RUN_THREAD_INFO_TYPE_STATE:
            if( IsThdCurr( thd ) && ( thd->state == THD_DEBUG ) ) {
                line->text = LIT_ENG( Current );
            } else {
                switch( thd->state ) {
                case THD_THAW:
                    line->text = LIT_ENG( Runnable );
                    break;
                case THD_FREEZE:
                    line->text = LIT_ENG( Frozen );
                    break;
                case THD_WAIT:
                    line->text = LIT_ENG( Wait );
                    break;
                case THD_SIGNAL:
                    line->text = LIT_ENG( Signal );
                    break;
                case THD_KEYBOARD:
                    line->text = LIT_ENG( Keyboard );
                    break;
                case THD_BLOCKED:
                    line->text = LIT_ENG( Blocked );
                    break;
                case THD_RUN:
                    line->text = LIT_ENG( Executing );
                    break;
                case THD_DEBUG:
                    line->text = LIT_ENG( Debug );
                    break;
                case THD_DEAD:
                    line->text = LIT_ENG( Dead );
                    break;
                }
            }
            return( true );
        case RUN_THREAD_INFO_TYPE_EXTRA:
            line->tabstop = false;
            line->use_prev_attr = true;
            line->text = thd->extra;
            return( true );
        case RUN_THREAD_INFO_TYPE_CS_EIP:
            line->tabstop = false;
            line->use_prev_attr = true;
            if( thd->cs ) {
                sprintf(TxtBuff, "%04hX:%08lX", thd->cs, (unsigned long)thd->eip );
                line->text = TxtBuff;
            } else {
                line->text = "";
            }
            return( true );
        }
    }
    return( false );
}

static bool ChkUpdate( void )
{
    return( UpdateFlags & UP_THREAD_STATE );
}

wnd_info RunTrdInfo = {
    RunTrdWndEventProc,
    RunTrdRefresh,
    RunTrdGetLine,
    RunTrdMenuItem,
    NoVScroll,
    NoBegPaint,
    NoEndPaint,
    WndFirstMenuItem,
    RunTrdNumRows,
    NoNextRow,
    NoNotify,
    ChkUpdate,
    PopUp( RunTrdMenu ),
};


a_window WndRunTrdOpen( void )
{
    return( DbgTitleWndCreate( LIT_DUI( WindowThreads ), &RunTrdInfo, WND_RUN_THREAD, NULL, &TrdIcon, TITLE_SIZE, true ) );
}

void RunThreadNotify( void )
{
    thread_state    *thd;

    if( HeadThd != NULL && HaveRemoteRunThread() ) {
        RemotePollRunThread();

        if( RunThreadWnd ) {
            for( thd = HeadThd; thd != NULL; thd = thd->link ) {
                RemoteUpdateRunThread( thd );
            }
            WndSetRepaint( RunThreadWnd );
        }
    }
}
