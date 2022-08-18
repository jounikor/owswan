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
#include "guixutil.h"
#include "guixdlg.h"

unsigned GUIAPI GUIIsChecked( gui_window *wnd, gui_ctl_id id )
{
    unsigned    ret;

    ret = (unsigned)GUISendDlgItemMessage( wnd->hwnd, id, BM_GETCHECK, (WPI_PARAM1)0, (WPI_PARAM2)0 );

    return( ret & 0x3 );
}

bool GUIAPI GUISetChecked( gui_window *wnd, gui_ctl_id id, unsigned check )
{
    if( ( check == GUI_CHECKED ) && (wnd->flags & IS_RES_DIALOG) == 0 ) {
        if( GUIIsChecked( wnd, id ) != GUI_CHECKED ) {
            return( GUIProcessControlNotification( id, BN_CLICKED, wnd ) );
        }
    } else {
        GUISendDlgItemMessage( wnd->hwnd, id, BM_SETCHECK, (WPI_PARAM1)check, (WPI_PARAM2)0 );
    }

    return( true );
}

