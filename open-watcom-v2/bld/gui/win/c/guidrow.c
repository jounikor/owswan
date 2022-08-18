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
#include "guiscrol.h"
#include "guixutil.h"
#include "guipaint.h"

/*
 * GUIWndDirtyRow -- tell the user interface that one row of the contents of
 *                    window wnd are bad.
 */

void GUIAPI GUIWndDirtyRow( gui_window *wnd, gui_text_ord row )
{
    WPI_RECT    wpi_rect;
    int         height;
    int         h;
    WPI_RECTDIM left, top, right, bottom;

    height = GUIFromTextY( 1, wnd );
    wpi_rect = wnd->hwnd_client_rect;
    h = _wpi_getheightrect( wpi_rect );
    _wpi_getrectvalues( wpi_rect, &left, &top, &right, &bottom );
    top = row * height;
    if( GUI_DO_VSCROLL( wnd ) )  {
        top -= GUIGetScrollPos( wnd, SB_VERT );
    }
    bottom = top + height;
    top--; // experimental kludge type stuff
    _wpi_setwrectvalues( &wpi_rect, left, top, right, bottom );
    _wpi_cvth_rect_plus1( &wpi_rect, h );
    //GUIInvalidatePaintHandles( wnd );
    _wpi_invalidaterect( wnd->hwnd, &wpi_rect, TRUE );
    _wpi_updatewindow( wnd->hwnd );
}
