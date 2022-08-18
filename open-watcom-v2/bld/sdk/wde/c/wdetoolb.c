/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2021 The Open Watcom Contributors. All Rights Reserved.
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


#include "wdeglbl.h"
#include <limits.h>
#include "wdestat.h"
#include "wdemain.h"
#include "wdemsgbx.h"
#include "rcstr.grh"
#include "wdehints.h"
#include "wdelist.h"
#include "wdetoolb.h"
#include "wrdll.h"

/****************************************************************************/
/* macro definitions                                                        */
/****************************************************************************/
#define OUTLINE_AMOUNT    4
#define WDE_TOOL_BORDER_X 1
#define WDE_TOOL_BORDER_Y 1

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/
//static WdeToolBar *WdeFindToolBar( HWND );
static WdeToolBar *WdeAllocToolBar( void );
static void        WdeAddToolBar( WdeToolBar * );
static void        WdeRemoveToolBar( WdeToolBar * );

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/
static LIST       *WdeToolBarList      = NULL;

WdeToolBar *WdeCreateToolBar( WdeToolBarInfo *info, HWND parent )
{
    WdeToolBar  *tbar;
    int         i;
    int         width;
    int         height;
    HMENU       hsysmenu;
    char        *text;

    if( info == NULL ) {
        return( NULL );
    }

    tbar = WdeAllocToolBar();
    if( tbar == NULL ) {
        return( NULL );
    }

    tbar->last_pos = info->dinfo.area;
    tbar->info = info;
    tbar->parent = parent;
    tbar->tbar = ToolBarInit( parent );

    ToolBarDisplay( tbar->tbar, &info->dinfo );

    for( i = 0; i < info->num_items; i++ ) {
        if( info->items[i].u.hbitmap != (HBITMAP)NULL ) {
            ToolBarAddItem( tbar->tbar, &info->items[i] );
        }
    }

    tbar->win = ToolBarWindow( tbar->tbar );

    if( (info->dinfo.style & TOOLBAR_FLOAT_STYLE) == TOOLBAR_FLOAT_STYLE ) {
        hsysmenu = GetSystemMenu( tbar->win, FALSE );
        i = GetMenuItemCount( hsysmenu );
        for( ; i > 0; i-- ) {
            DeleteMenu( hsysmenu, 0, MF_BYPOSITION );
        }
        text = WdeAllocRCString( WDE_SYSMENUMOVE );
        AppendMenu( hsysmenu, MF_STRING, SC_MOVE, text != NULL ? text : "Move" );
        if( text != NULL ) {
            WdeFreeRCString( text );
        }
        text = WdeAllocRCString( WDE_SYSMENUSIZE );
        AppendMenu( hsysmenu, MF_STRING, SC_SIZE, text != NULL ? text : "Size" );
        if( text != NULL ) {
            WdeFreeRCString( text );
        }
        text = WdeAllocRCString( WDE_SYSMENUHIDE );
        AppendMenu( hsysmenu, MF_STRING, SC_CLOSE, text != NULL ? text : "Hide" );
        if( text != NULL ) {
            WdeFreeRCString( text );
        }
    }

    width = info->dinfo.area.right - info->dinfo.area.left;
    height = info->dinfo.area.bottom - info->dinfo.area.top;
    SetWindowPos( tbar->win, (HWND)NULL, 0 ,0 , width, height, SWP_NOMOVE | SWP_NOZORDER );

    ShowWindow( tbar->win, SW_SHOWNORMAL );

    UpdateWindow( tbar->win );

    WdeAddToolBar( tbar );

    return( tbar );
}

#if 0
bool WdeToolBarHook( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    MINMAXINFO  *minmax;
    WdeToolBar  *tbar;
    bool        ret;

    if( (tbar = WdeFindToolBar( hwnd )) == NULL || tbar->win == NULL ) {
        if( msg == WM_GETMINMAXINFO ) {
            minmax = (MINMAXINFO *)lParam;
            minmax->ptMinTrackSize.x = 8;
        }
        return( false );
    }

    ret = false;

    switch( msg ) {
    case WM_USER:
        WdeHandleToolHint( (ctl_id)LOWORD( wParam ), lParam != 0 );
        WdeHandleStickyToolPress( tbar, wParam, lParam );
        break;

    case WM_SIZE:
        if( wParam != SIZE_MAXIMIZED && wParam != SIZE_MINIMIZED ) {
            GetWindowRect( hwnd, &tbar->last_pos );
        }
        break;

    case WM_MOVE:
        if( !IsZoomed( hwnd ) ) {
            GetWindowRect( hwnd, &tbar->last_pos );
        }
        break;

    case WM_GETMINMAXINFO:
        minmax = (MINMAXINFO *)lParam;
        minmax->ptMinTrackSize.x = 2 * GetSystemMetrics( SM_CXFRAME ) +
            tbar->info->dinfo.border_size.x + tbar->info->dinfo.button_size.x;
        minmax->ptMinTrackSize.y = 2 * GetSystemMetrics( SM_CYFRAME ) +
            tbar->info->dinfo.border_size.y + GetSystemMetrics( SM_CYCAPTION ) +
            tbar->info->dinfo.button_size.y;
        ret = true;
        break;

    case WM_DESTROY:
        WdeCloseToolBar( tbar );
        break;
    }

    return( ret );
}
#endif

void WdeHandleToolHint( ctl_id id, bool pressed )
{
    if( pressed ) {
        WdeDisplayHint( id );
    } else {
        WdeSetStatusText( NULL, "", true );
    }
}

void WdeHandleStickyToolPress( WdeToolBar *tbar, WPARAM wParam, LPARAM lParam )
{
    int bstate;

    if( lParam ) {
        bstate = BUTTON_UP;
    } else {
        bstate = BUTTON_DOWN;
    }

    WdeSetToolBarItemState( tbar, LOWORD( wParam ), bstate );
}

#if 0
WdeToolBar *WdeFindToolBar( HWND win )
{
    WdeToolBar *tbar;
    LIST       *tlist;

    for( tlist = WdeToolBarList; tlist != NULL; tlist = ListNext( tlist ) ) {
        tbar = ListElement( tlist );
        if( tbar->win == win ) {
            return( tbar );
        }
    }

    return( NULL );
}
#endif

bool WdeCloseToolBar( WdeToolBar *tbar )
{
    if( tbar != NULL ) {
        tbar->win = (HWND)NULL;
        WdeRemoveToolBar( tbar );
        WdeFreeToolBar( tbar );
    }

    return( TRUE );
}

void WdeFreeToolBarInfo( WdeToolBarInfo *info )
{
    if( info != NULL ) {
        if( info->items != NULL ) {
            WRMemFree( info->items );
        }
        if( info->dinfo.background != NULL ) {
            DeleteObject( info->dinfo.background );
        }
        WRMemFree( info );
    }
}

WdeToolBarInfo *WdeAllocToolBarInfo( int num )
{
    WdeToolBarInfo *info;

    info = (WdeToolBarInfo *)WRMemAlloc( sizeof( WdeToolBarInfo ) );

    if( info != NULL ) {
        memset( info, 0, sizeof( WdeToolBarInfo ) );
        info->items = (TOOLITEMINFO *)WRMemAlloc( sizeof( TOOLITEMINFO ) * num );
        if( info->items != NULL ) {
            memset( info->items, 0, sizeof( TOOLITEMINFO ) * num );
            info->num_items = num;
        } else {
            WRMemFree( info );
            info = NULL;
        }
    }

    return( info );
}

WdeToolBar *WdeAllocToolBar( void )
{
    WdeToolBar *tbar;

    tbar = (WdeToolBar *)WRMemAlloc( sizeof( WdeToolBar ) );
    if( tbar != NULL ) {
        memset( tbar, 0, sizeof( WdeToolBar ) );
    }

    return( tbar );
}

void WdeFreeToolBar( WdeToolBar *tbar )
{
    if( tbar != NULL ) {
        WRMemFree( tbar );
    }
}

void WdeAddToolBar( WdeToolBar *tbar )
{
    WdeInsertObject( &WdeToolBarList, (OBJPTR)tbar );
}

void WdeDestroyToolBar( WdeToolBar *tbar )
{
    ToolBarDestroy( tbar->tbar );
}

void WdeRemoveToolBar( WdeToolBar *tbar )
{
    ListRemoveElt( &WdeToolBarList, (void *)tbar );
}

void WdeShutdownToolBars( void )
{
    WdeToolBar  *tbar;
    LIST        *tlist;

    tlist = WdeListCopy( WdeToolBarList );
    for( ; tlist != NULL; tlist = ListConsume( tlist ) ) {
        tbar = (WdeToolBar *)ListElement( tlist );
        ToolBarDestroy( tbar->tbar );
    }
    ToolBarFini( NULL );

    ListFree( WdeToolBarList );
}

void WdeSetToolBarItemState( WdeToolBar *tbar, ctl_id id, UINT state )
{
    if( tbar != NULL /* && ToolBarGetState( tbar->tbar, id ) != state */ ) {
        ToolBarSetState( tbar->tbar, id, state );
    }
}
