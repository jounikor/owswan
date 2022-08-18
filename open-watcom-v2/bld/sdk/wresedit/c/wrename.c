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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "wglbl.h"
#include "wrdll.h"
#include "wmsg.h"
#include "winst.h"
#include "wsetedit.h"
#include "wrename.h"
#include "wctl3d.h"
#include "sysindep.rh"
#include "wresall.h"
#include "jdlg.h"
#include "wclbproc.h"


/****************************************************************************/
/* macro definitions                                                        */
/****************************************************************************/

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/
typedef struct WResRenameInfo {
    HELPFUNC            help_callback;
    WResID              *old_name;
    WResID              *new_name;
} WResRenameInfo;

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/
WINEXPORT INT_PTR CALLBACK WResRenameDlgProc( HWND, UINT, WPARAM, LPARAM );

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/
static void WSetWinInfo( HWND, WResRenameInfo * );
static void WGetWinInfo( HWND, WResRenameInfo * );
static bool WGetNewName( HWND, WResRenameInfo * );

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/

bool WRenameResource( HWND parent, WResID **name, HELPFUNC help_callback )
{
    WResRenameInfo  info;
    bool            ok;

    info.old_name = NULL;
    info.new_name = NULL;

    ok = (name != NULL);

    if( ok )  {
        ok = false;
        info.help_callback = help_callback;
        info.old_name = *name;
        if( WGetNewName( parent, &info ) && info.new_name != NULL ) {
            if( *name != NULL ) {
                WRMemFree( *name );
            }
            *name = info.new_name;
            ok = true;
        }
    }

    return( ok );
}

bool WGetNewName( HWND parent, WResRenameInfo *info )
{
    DLGPROC     dlgproc;
    HINSTANCE   app_inst;
    INT_PTR     modified;

    app_inst = WGetEditInstance();

    dlgproc = MakeProcInstance_DLG( WResRenameDlgProc, app_inst );

    modified = JDialogBoxParam( app_inst, "WRenameResource", parent, dlgproc, (LPARAM)info );

    FreeProcInstance_DLG( dlgproc );

    return( modified != -1 && modified == IDOK );
}

void WSetWinInfo( HWND hDlg, WResRenameInfo *info )
{
    if( info != NULL && info->old_name != NULL ) {
        WSetEditWithWResID( GetDlgItem( hDlg, IDM_RENOLD ), info->old_name );
        WSetEditWithWResID( GetDlgItem( hDlg, IDM_RENNEW ), info->old_name );
    }
}

void WGetWinInfo( HWND hDlg, WResRenameInfo *info )
{
    bool mod;

    if( info != NULL ) {
        info->new_name = WGetWResIDFromEdit( GetDlgItem( hDlg, IDM_RENNEW ), &mod );
    }
}

WINEXPORT INT_PTR CALLBACK WResRenameDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    WResRenameInfo  *info;
    BOOL            ret;

    ret = FALSE;

    switch( message ) {
    case WM_SYSCOLORCHANGE:
        WCtl3dColorChange();
        break;

    case WM_INITDIALOG:
        info = (WResRenameInfo *)lParam;
        SET_DLGDATA( hDlg, info );
        WSetWinInfo( hDlg, info );
        ret = TRUE;
        break;

    case WM_COMMAND:
        info = (WResRenameInfo *)GET_DLGDATA( hDlg );
        switch( LOWORD( wParam ) ) {
        case IDM_HELP:
            if( info != NULL && info->help_callback != NULL ) {
                info->help_callback();
            }
            break;

        case IDOK:
            WGetWinInfo( hDlg, info );
            EndDialog( hDlg, TRUE );
            ret = TRUE;
            break;

        case IDCANCEL:
            EndDialog( hDlg, FALSE );
            ret = TRUE;
            break;
        }
        break;
    }

    return( ret );
}
