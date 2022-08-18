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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "guiwind.h"
#include "guiscale.h"
#include "guixutil.h"
#include "guimenus.h"
#include "guifloat.h"
#include "guistr.h"
#include "guixwind.h"


extern  HMENU           GUIHFloatingPopup;

static  gui_ctl_id      CurrId = 0;
static  bool            InitComplete = false;

bool GUIAPI GUITrackFloatingPopup( gui_window *wnd, const gui_point *location,
                            gui_mouse_track track, gui_ctl_id *curr_id )
{
    WPI_POINT   wpi_point;
    ULONG       flags;
    WPI_RECTDIM left, top, right, bottom;
    HMENU       hpopup;
    guix_ord    scr_x;
    guix_ord    scr_y;

    if( ( hpopup = GUIHFloatingPopup ) == NULLHANDLE ) {
        return( false );
    }

    scr_x = GUIScaleToScreenH( location->x );
    scr_y = GUIScaleToScreenV( location->y );
    _wpi_getrectvalues( wnd->hwnd_client_rect, &left, &top, &right, &bottom );
    scr_x += left;
    scr_y += top;
    if( GUI_DO_HSCROLL( wnd ) ) {
        scr_x -= GUIGetScrollPos( wnd, SB_HORZ );
    }
    if( GUI_DO_VSCROLL( wnd ) ) {
        scr_y -= GUIGetScrollPos( wnd, SB_VERT );
    }

    CurrId = 0;
    if( ( curr_id != NULL ) && ( *curr_id != 0 ) ) {
        CurrId = *curr_id;
    }

    scr_y = _wpi_cvth_y( scr_y, (bottom - top) );

    wpi_point.x = scr_x;
    wpi_point.y = scr_y;

    _wpi_mapwindowpoints( wnd->hwnd, HWND_DESKTOP, &wpi_point, 1 );

    flags = TPM_LEFTALIGN;
    if( track & GUI_TRACK_LEFT ) {
        flags |= TPM_LEFTBUTTON;
    }
    if( track & GUI_TRACK_RIGHT ) {
        flags |= TPM_RIGHTBUTTON;
    }
    InitComplete = false;

    GUIFlushKeys();

    _wpi_trackpopupmenu( hpopup, flags, wpi_point.x, wpi_point.y, wnd->hwnd_frame );

    _wpi_destroymenu( hpopup );

    GUIHFloatingPopup = NULLHANDLE;

    if( ( CurrId != 0 ) && ( curr_id != NULL ) ) {
        *curr_id = CurrId;
    }
    CurrId = 0;

    GUIDeleteFloatingPopups( wnd );
    return( true );
}

/*
 * GUIXCreateFloatingPopup -- create a floating popup menu
 */

bool GUIXCreateFloatingPopup( gui_window *wnd, const gui_point *location,
                             const gui_menu_items *menus,
                             gui_mouse_track track, gui_ctl_id *curr_id )
{
    if( GUIHFloatingPopup != NULLHANDLE ) {
        _wpi_destroymenu( GUIHFloatingPopup );
        GUIHFloatingPopup = NULLHANDLE;
    }

    GUIHFloatingPopup = GUICreateSubMenu( wnd, menus, FLOAT_HINT );
    if( GUIHFloatingPopup == NULLHANDLE ) {
        GUIError( LIT( Open_Failed ) );
        return( false );
    }

    return( GUITrackFloatingPopup( wnd, location, track, curr_id ) );
}

void GUIPopupMenuSelect( WPI_PARAM1 wparam, WPI_PARAM2 lparam )
{
    bool                menu_closed;
    gui_ctl_id          id;
    bool                is_hilite;
#ifdef __OS2_PM__
    WPI_MENUSTATE       mstate;
    HMENU               hmenu;
#endif

    lparam=lparam;
    id = GET_WM_MENUSELECT_ITEM( wparam, lparam );
    menu_closed = ( _wpi_is_close_menuselect( wparam, lparam ) != 0 );

#ifndef __OS2_PM__
    is_hilite = ( (GET_WM_MENUSELECT_FLAGS( wparam, lparam ) & MF_HILITE) != 0 );
#else
    hmenu = (HMENU)lparam;
    if( !menu_closed && !WinSendMsg( hmenu, MM_QUERYITEM, MPFROM2SHORT( id, true ), MPFROMP( &mstate ) ) ) {
        return;
    }
    is_hilite = ( (mstate.afAttribute & MF_HILITE) != 0 );
#endif

    if( menu_closed ) {
        CurrId = 0;
    } else {
        if( !InitComplete ) {
            InitComplete = true;
        } else {
            if( is_hilite ) {
                CurrId = id;
            }
        }
    }
}

