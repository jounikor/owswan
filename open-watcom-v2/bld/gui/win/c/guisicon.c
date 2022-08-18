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
#include "guistr.h"
#include "guirdlg.h"


/*
 * GUISetIcon --
 */

bool GUIAPI GUISetIcon( gui_window *wnd, gui_resource *res )
{
    WPI_HICON icon;

    icon = (WPI_HICON)0;

    if( res != NULL ) {
        icon = _wpi_loadicon( GUIResHInst, MAKEINTRESOURCE( res->res_id ) );
    } else {
        icon = _wpi_loadicon( GUIResHInst, LIT( ApplIcon ) );
        if( icon == NULL ) {
            icon = _wpi_getsysicon( NULLHANDLE, IDI_APPLICATION );
        }
    }

    if( icon != NULL ) {
        if( wnd->icon != NULL ) {
            _wpi_destroyicon( wnd->icon );
        }
        wnd->icon = icon;
    }

#ifndef __OS2_PM__
    // this code forces Windows to use the correct icon when you press
    // alt-TAB
    if( wnd->flags & IS_ROOT ) {
        SET_HICON( wnd->hwnd, wnd->icon );
    }
#endif

#ifdef __OS2_PM__
    _wpi_sendmessage( wnd->hwnd_frame, WM_SETICON, icon, 0 );
#endif

    return( icon != NULL );
}

