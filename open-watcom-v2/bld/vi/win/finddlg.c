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
#include "finddlg.rh"
#include "wclbproc.h"


/* Local Windows CALLBACK function prototypes */
WINEXPORT INT_PTR CALLBACK FindDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

static fancy_find findData =
    { -1, -1, NULL, 0, NULL, 0, NULL, 0, NULL, 0, true, false, true, true, false, false };

/*
 * FindDlgProc - callback routine for find dialog
 */
WINEXPORT INT_PTR CALLBACK FindDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    int                 curr;
    int                 i;
    int                 cmd;
    int                 index;
    char                find[MAX_INPUT_LINE];
    history_data        *h;
    char                *ptr;
    RECT                pos;

#ifdef __NT__
    (void)lparam;
#endif
    switch( msg ) {
    case WM_INITDIALOG:
        if( findData.posx == -1 && findData.posy == -1 ) {
            CenterWindowInRoot( hwnd );
        } else {
            SetWindowPos( hwnd, (HWND)NULLHANDLE, findData.posx, findData.posy,
                0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOZORDER );
        }
        h = &EditVars.Hist[HIST_FIND];
        EditSubClass( hwnd, FIND_EDIT, h );
        CheckDlgButton( hwnd, FIND_IGNORE_CASE, ( findData.case_ignore ) ? BST_CHECKED : BST_UNCHECKED );
        CheckDlgButton( hwnd, FIND_REGULAR_EXPRESSIONS, ( findData.use_regexp ) ? BST_CHECKED : BST_UNCHECKED );
        CheckDlgButton( hwnd, FIND_SEARCH_BACKWARDS, ( !findData.search_forward ) ? BST_CHECKED : BST_UNCHECKED );
        CheckDlgButton( hwnd, FIND_SEARCH_WRAP, ( findData.search_wrap ) ? BST_CHECKED : BST_UNCHECKED );
        SetDlgItemText( hwnd, FIND_EDIT, findData.find );
        curr = h->curr + h->max - 1;
        for( i = 0; i < h->max; i++ ) {
            if( h->data[curr % h->max] != NULL ) {
                SendDlgItemMessage( hwnd, FIND_LISTBOX, LB_ADDSTRING, 0, (LPARAM)h->data[curr % h->max] );
            }
            curr--;
            if( curr < 0 ) {
                break;
            }
        }
        return( TRUE );
    case WM_CLOSE:
        GetWindowRect( hwnd, &pos );
        findData.posx = pos.left;
        findData.posy = pos.top;
        PostMessage( hwnd, WM_COMMAND, GET_WM_COMMAND_MPS( IDCANCEL, 0, 0 ) );
        return( TRUE );
    case WM_COMMAND:
        switch( LOWORD( wparam ) ) {
        case FIND_LISTBOX:
            cmd = GET_WM_COMMAND_CMD( wparam, lparam );
            if( cmd == LBN_SELCHANGE || cmd == LBN_DBLCLK ) {
                index = (int)SendDlgItemMessage( hwnd, FIND_LISTBOX, LB_GETCURSEL, 0, 0L );
                if( index == LB_ERR ) {
                    break;
                }
                SendDlgItemMessage( hwnd, FIND_LISTBOX, LB_GETTEXT, index, (LPARAM)(LPSTR)find );
                SetDlgItemText( hwnd, FIND_EDIT, find );
                if( cmd == LBN_DBLCLK ) {
                    PostMessage( hwnd, WM_COMMAND, GET_WM_COMMAND_MPS( IDOK, 0, 0 ) );
                }
            }
            break;
        case IDCANCEL:
            GetWindowRect( hwnd, &pos );
            findData.posx = pos.left;
            findData.posy = pos.top;
            RemoveEditSubClass( hwnd, FIND_EDIT );
            EndDialog( hwnd, FALSE );
            break;
        case IDOK:
            GetDlgItemText( hwnd, FIND_EDIT, findData.find, findData.findlen );
            findData.case_ignore = IsDlgButtonChecked( hwnd, FIND_IGNORE_CASE );
            findData.use_regexp = IsDlgButtonChecked( hwnd, FIND_REGULAR_EXPRESSIONS );
            findData.search_forward = !IsDlgButtonChecked( hwnd, FIND_SEARCH_BACKWARDS );
            findData.search_wrap = IsDlgButtonChecked( hwnd, FIND_SEARCH_WRAP );
            h = &EditVars.Hist[HIST_FIND];
            curr = h->curr + h->max - 1;
            ptr = NULL;
            if( curr >= 0 ) {
                ptr = h->data[curr % h->max];
            }
            if( ptr == NULL || strcmp( ptr, findData.find ) != 0 ) {
                ReplaceString( &(h->data[h->curr % h->max]), findData.find );
                h->curr += 1;
            }
            GetWindowRect( hwnd, &pos );
            findData.posx = pos.left;
            findData.posy = pos.top;
            RemoveEditSubClass( hwnd, FIND_EDIT );
            EndDialog( hwnd, TRUE );
            break;
        default:
            return( FALSE );
        }
        return( TRUE );
    }
    return( FALSE );

} /* FindDlgProc */

/*
 * GetFindStringDialog - create dialog settings
 */
bool GetFindStringDialog( fancy_find *ff )
{
    DLGPROC     dlgproc;
    bool        rc;

    findData.find = ff->find;
    findData.findlen = ff->findlen;
    dlgproc = MakeProcInstance_DLG( FindDlgProc, InstanceHandle );
    rc = DialogBox( InstanceHandle, "FINDDLG", root_window_id, dlgproc );
    FreeProcInstance_DLG( dlgproc );
    SetWindowCursor();
    if( rc ) {
        ff->case_ignore = findData.case_ignore;
        ff->use_regexp = findData.use_regexp;
        ff->search_forward = findData.search_forward;
        ff->search_wrap = findData.search_wrap;
    }
    return( rc );

} /* GetFindStringDialog */
