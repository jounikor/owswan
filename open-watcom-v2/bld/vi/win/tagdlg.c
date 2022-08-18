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
#include "tagdlg.rh"
#include "wclbproc.h"


/* Local Windows CALLBACK function prototypes */
WINEXPORT INT_PTR CALLBACK TagListDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

static int      tagCount = 0;
static char     **tagList = NULL;

/*
 * TagListDlgProc - handle the tag selection dialog
 */
WINEXPORT INT_PTR CALLBACK TagListDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    int         i;

    switch( msg ) {
    case WM_INITDIALOG:
        CenterWindowInRoot( hwnd );
        SetDlgItemText( hwnd, TAGS_TAGNAME, (LPSTR) lparam );
        for( i = 0; i < tagCount; i++ ) {
            SendDlgItemMessage( hwnd, TAGS_LISTBOX, LB_ADDSTRING, 0, (LPARAM)tagList[i] );
        }
        SendDlgItemMessage( hwnd, TAGS_LISTBOX, LB_SETCURSEL, 0, 0L );
        return( TRUE );
    case WM_COMMAND:
        switch( LOWORD( wparam ) ) {
        case IDCANCEL:
            EndDialog( hwnd, -1 );
            break;
        case IDOK:
            i = (int)SendDlgItemMessage( hwnd, TAGS_LISTBOX, LB_GETCURSEL, 0, 0L );
            if( i == LB_ERR ) {
                MessageBox( hwnd, "No selection", EditorName, MB_ICONEXCLAMATION | MB_OK );
            } else {
                EndDialog( hwnd, i );
            }
            break;
        case TAGS_LISTBOX:
            if( GET_WM_COMMAND_CMD( wparam, lparam ) == LBN_DBLCLK ) {
                i = (int)SendDlgItemMessage( hwnd, TAGS_LISTBOX, LB_GETCURSEL, 0, 0L );
                EndDialog( hwnd, i );
            }
            break;
        default:
            return( FALSE );
        }
        return( TRUE );
    }
    return( FALSE );

} /* TagListDlgProc */

/*
 * PickATag - create dialog to select a specific tag
 */
list_linenum PickATag( list_linenum tag_count, char **tag_list, const char *tagname )
{
    DLGPROC         dlgproc;
    list_linenum    rc;

    tagCount = (int)tag_count;
    tagList = tag_list;

    dlgproc = MakeProcInstance_DLG( TagListDlgProc, InstanceHandle );
    rc = (list_linenum)DialogBoxParam( InstanceHandle, "TAGS", root_window_id, dlgproc, (LPARAM)tagname );
    FreeProcInstance_DLG( dlgproc );
    return( rc );

} /* PickATag */
