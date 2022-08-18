/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2020 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Display an icon resource.
*
****************************************************************************/


#include <stdio.h>
#include <wwindows.h>
#include "restest.h"
#include "resname.h"
#include "wclbproc.h"


static char iconName[256];

INT_PTR CALLBACK GetIconNameDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    lparam = lparam;
    switch( msg ) {
    case WM_COMMAND:
        if( LOWORD( wparam ) == IDOK ) {
            GetDlgItemText( hwnd, INPUT_FIELD, iconName, sizeof( iconName ) );
            EndDialog( hwnd, 0 );
        }
        break;
    default:
        return( FALSE );
        break;
    }
    return( TRUE );
}

void DisplayIcon( HWND hwnd )
{
    DLGPROC     dlgproc;
    HICON       oldicon;
    HICON       newicon;
    char        buf[256];

    dlgproc = MakeProcInstance_DLG( GetIconNameDlgProc, Instance );
    DialogBox( Instance, "GET_RES_NAME_DLG" , NULL, dlgproc );
    FreeProcInstance_DLG( dlgproc );
    newicon = LoadIcon( Instance, iconName );
    if( newicon == NULL ) {
        sprintf( buf, "Can't Load Icon %s", iconName );
        Error( "icon", buf );
        return;
    }
    oldicon = SET_HICON( hwnd, newicon );
    ShowWindow( hwnd, SW_MINIMIZE );
    MessageBox( NULL, "The Icon is displayed", "ICON", MB_OK );
    ShowWindow( hwnd, SW_SHOWNORMAL );
    SET_HICON( hwnd, oldicon );
}
