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
#include "cmd.rh"
#include "wclbproc.h"


/* Local Windows CALLBACK function prototypes */
WINEXPORT INT_PTR CALLBACK CmdDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

static char     *cmdStr;
static int      cmdLen;

/*
 * CmdDlgProc - callback routine for command dialog
 */
WINEXPORT INT_PTR CALLBACK CmdDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    int                 curr;
    int                 i;
    int                 cmd;
    int                 index;
    char                str[MAX_INPUT_LINE];
    history_data        *h;
    char                *ptr;

#ifdef __NT__
    /* unused parameters */ (void)lparam;
#endif

    switch( msg ) {
    case WM_INITDIALOG:
        CenterWindowInRoot( hwnd );
        h = &EditVars.Hist[HIST_CMD];
        EditSubClass( hwnd, CMD_EDIT, h );
        SetDlgItemText( hwnd, CMD_EDIT, cmdStr );
        curr = h->curr + h->max - 1;
        for( i = 0; i < h->max; i++ ) {
            if( h->data[curr % h->max] != NULL ) {
                SendDlgItemMessage( hwnd, CMD_LISTBOX, LB_ADDSTRING, 0, (LPARAM)h->data[curr % h->max] );
            }
            curr--;
            if( curr < 0 ) {
                break;
            }
        }
        return( TRUE );
    case WM_CLOSE:
        PostMessage( hwnd, WM_COMMAND, IDCANCEL, 0L );
        return( TRUE );
    case WM_COMMAND:
        switch( LOWORD( wparam ) ) {
        case CMD_LISTBOX:
            cmd = GET_WM_COMMAND_CMD( wparam, lparam );
            if( cmd == LBN_SELCHANGE || cmd == LBN_DBLCLK ) {
                index = (int)SendDlgItemMessage( hwnd, CMD_LISTBOX, LB_GETCURSEL, 0, 0L );
                if( index == LB_ERR ) {
                    break;
                }
                SendDlgItemMessage( hwnd, CMD_LISTBOX, LB_GETTEXT, index, (LPARAM)(LPSTR)str );
                SetDlgItemText( hwnd, CMD_EDIT, str );
                if( cmd == LBN_DBLCLK ) {
                    PostMessage( hwnd, WM_COMMAND, IDOK, 0L );
                }
            }
            break;
        case IDCANCEL:
            RemoveEditSubClass( hwnd, CMD_EDIT );
            EndDialog( hwnd, FALSE );
            break;
        case IDOK:
            GetDlgItemText( hwnd, CMD_EDIT, cmdStr, cmdLen );
            h = &EditVars.Hist[HIST_CMD];
            curr = h->curr + h->max - 1;
            ptr = NULL;
            if( curr >= 0 ) {
                ptr = h->data[curr % h->max];
            }
            if( ptr == NULL || strcmp( ptr, cmdStr ) ) {
                ReplaceString( &(h->data[h->curr % h->max]), cmdStr );
                h->curr += 1;
            }
            RemoveEditSubClass( hwnd, CMD_EDIT );
            EndDialog( hwnd, TRUE );
            break;
        default:
            return( FALSE );
        }
        return( TRUE );
    }
    return( FALSE );

} /* CmdDlgProc */

/*
 * GetCmdDialog - create dialog settings
 */
bool GetCmdDialog( char *str, int len )
{
    DLGPROC     dlgproc;
    bool        rc;

    cmdStr = str;
    cmdLen = len;
    dlgproc = MakeProcInstance_DLG( CmdDlgProc, InstanceHandle );
    rc = DialogBox( InstanceHandle, "CMDDLG", root_window_id, dlgproc );
    FreeProcInstance_DLG( dlgproc );

    /* this is technically a bug of some kind - if the above command
     * was a DDE message to another window to take focus, we will
     * temporarily be grabbing back the focus.  luckily this probably
     * won't happen too often.
     */
    SetWindowCursor();

    return( rc );

} /* GetCmdDialog */
