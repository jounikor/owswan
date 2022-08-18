/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2019 The Open Watcom Contributors. All Rights Reserved.
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


#include "vi.h"
#include "usage.rh"
#include "wclbproc.h"


/* Local Windows CALLBACK function prototypes */
WINEXPORT INT_PTR CALLBACK UsageDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

static char     **usageList;
static int      usageCnt;
static char     *usageStr;

/*
 * UsageDlgProc - callback routine for usage dialog
 */
WINEXPORT INT_PTR CALLBACK UsageDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    int         i;
    HFONT       font;

    (void)lparam;

    switch( msg ) {
    case WM_INITDIALOG:
        font = GetStockObject( SYSTEM_FIXED_FONT );
        SendDlgItemMessage( hwnd, USAGE_LISTBOX, WM_SETFONT, (UINT)font, 0L );
        SendDlgItemMessage( hwnd, USAGE_TEXT, WM_SETFONT, (UINT)font, 0L );
        if( usageStr[0] != '\0' ) {
            SetDlgItemText( hwnd, USAGE_TEXT, usageStr );
        }
        for( i = 0; i < usageCnt; i++ ) {
            SendDlgItemMessage( hwnd, USAGE_LISTBOX, LB_ADDSTRING, 0, (LPARAM)usageList[i] );
        }
        return( TRUE );
    case WM_CLOSE:
        EndDialog( hwnd, 0 );
        break;
    case WM_COMMAND:
        switch( LOWORD( wparam ) ) {
        case IDOK:
            EndDialog( hwnd, 0 );
            break;
        }
    }
    return( FALSE );

} /* UsageDlgProc */

/*
 * UsageDialog - show the startup dialog
 */
void UsageDialog( char **list, char *msg, int cnt )
{
    DLGPROC     dlgproc;

    usageList = list;
    usageStr = msg;
    usageCnt = cnt;

    dlgproc = MakeProcInstance_DLG( UsageDlgProc, InstanceHandle );
    DialogBox( InstanceHandle, "Usage", root_window_id, dlgproc );
    FreeProcInstance_DLG( dlgproc );

} /* UsageDialog */
