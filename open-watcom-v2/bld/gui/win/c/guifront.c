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
#include "guihook.h"
#include "guixutil.h"
#include "guizlist.h"
#include "guifront.h"
#include "guixwind.h"


void ActivateNC( gui_window *wnd, bool activate )
{
    gui_window  *current;

    for( current = wnd; current != NULL; current = GUIGetParentWindow( current ) ) {
        if( current->flags & HAS_CAPTION ) {
            GUISendMessage( current->hwnd_frame, WM_NCACTIVATE, (WPI_PARAM1)activate, (WPI_PARAM2)0 );
            break;
        }
    }
}

void GUIAPI GUIBringToFront( gui_window *wnd )
{
    bool        parent_is_dlg;
    bool        same_window;

    if( wnd == NULL ) {
        return;
    }

    same_window = ( GUICurrWnd == wnd );
    parent_is_dlg = false;
    if( wnd->parent ) {
        if( GUI_IS_DIALOG( wnd->parent ) ) {
            parent_is_dlg = true;
        }
    }

    if( !same_window || parent_is_dlg ) {
        if( !same_window && ( GUICurrWnd != NULL ) ) {
            if( _wpi_getparent( GUICurrWnd->hwnd_frame ) != HWND_DESKTOP ) {
                ActivateNC( GUICurrWnd, false );
            }
            GUIEVENT( GUICurrWnd, GUI_NOT_ACTIVE, NULL );
        }
        // if the application indicates that it has processed the message
        // then do not proceed to bring the window to top
        if( !GUIEVENT( wnd, GUI_NOW_ACTIVE, NULL ) ) {
            GUICurrWnd = wnd;
            GUIFrontOfList( wnd );
            ActivateNC( wnd, true );
            GUIMDIBroughtToFront( wnd );
            _wpi_bringwindowtotop( wnd->hwnd_frame );
            _wpi_setfocus( wnd->hwnd_frame );
        }
    }

    if( same_window && !parent_is_dlg ) {
        ActivateNC( wnd, true );
    }

    if( parent_is_dlg ) {
        ActivateNC( wnd->parent, true );
    }
}

void GUIAPI GUIActivateNC( gui_window *wnd, bool activate )
{
    ActivateNC( wnd, activate );
}

