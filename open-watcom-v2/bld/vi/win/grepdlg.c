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
* Description:  Grep dialog.
*
****************************************************************************/


#include "vi.h"
#include "grep.rh"
#include "wclbproc.h"


/* Local Windows CALLBACK function prototypes */
WINEXPORT INT_PTR CALLBACK GrepDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

static DLGPROC  grepProc;
static HWND     grepHwnd;
static bool     cancelPressed;

/*
 * GrepDlgProc - callback routine for grep dialog
 */
WINEXPORT INT_PTR CALLBACK GrepDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    (void)lparam;

    switch( msg ) {
    case WM_INITDIALOG:
        return( TRUE );
    case WM_CLOSE:
        PostMessage( hwnd, WM_COMMAND, GET_WM_COMMAND_MPS( IDCANCEL, 0, 0 ) );
        return( TRUE );
    case WM_COMMAND:
        switch( LOWORD( wparam ) ) {
        case IDCANCEL:
            cancelPressed = true;
            break;
        default:
            return( FALSE );
        }
        return( TRUE );
    }
    return( FALSE );

} /* GrepDlgProc */

/*
 * InitGrepDialog - start the grep dialog
 */
void InitGrepDialog( void )
{
    grepProc = MakeProcInstance_DLG( GrepDlgProc, InstanceHandle );
    cancelPressed = false;
    grepHwnd = CreateDialog( InstanceHandle, "GREPDLG", root_window_id, grepProc );

} /* InitGrepDialog */

/*
 * FiniGrepDialog - done with the grep dialog
 */
void FiniGrepDialog( void )
{
    BringWindowToTop( root_window_id );
    SetFocus( root_window_id );
    if( !BAD_ID( grepHwnd ) ) {
        DestroyWindow( grepHwnd );
    }
    FreeProcInstance_DLG( grepProc );
    grepProc = NULL;
    grepHwnd = NO_WINDOW;

} /* FiniGrepDialog */

/*
 * SetGrepDialogFile - set the current file being used by the dialog
 */
bool SetGrepDialogFile( const char *str )
{
    if( !BAD_ID( grepHwnd ) ) {
        SetDlgItemText( grepHwnd, GREP_CURRENT_FILE, str );
        MessageLoop( false );
    }
    return( cancelPressed );

} /* SetGrepDialogFile */
