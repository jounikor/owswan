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
* Description:  Mark dialog.
*
****************************************************************************/


#include "commonui.h"
#include <string.h>
#include "bool.h"
#include "mark.h"
#ifndef NOUSE3D
    #include "ctl3dcvr.h"
#endif
#include "wclbproc.h"


/* Window callback functions prototypes */
WINEXPORT INT_PTR CALLBACK MarkDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

static void     (*WriteFn)( const char * );

/*
 * MarkDlgProc - handle messages from the mark dialog
 */
INT_PTR CALLBACK MarkDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    char        buf[MARK_LEN];
    char        boxbuf[MARK_LEN];
    int         ret;
    HWND        hdl;

    lparam = lparam;
    switch( msg ) {
    case WM_INITDIALOG:
        SendDlgItemMessage( hwnd, MARK_BOXED, BM_SETCHECK, 1, 0L );
        hdl = GetDlgItem( hwnd, MARK_EDIT );
        SetFocus( hdl );
        return( FALSE );
//      strcpy( buf, "---------" );
//      SendDlgItemMessage( hwnd, MARK_EDIT, WM_SETTEXT, 0, (LPARAM)(LPCSTR)buf );
        break;
#ifndef NOUSE3D
    case WM_SYSCOLORCHANGE:
        CvrCtl3dColorChange();
        break;
#endif
    case WM_COMMAND:
        switch( wparam ) {
            case MARK_OK:
                ret = GetDlgItemText( hwnd, MARK_EDIT, buf, MARK_LEN );
                if( ret == 0 ) {
                    buf[0] = '\0';
                }
                if( IsDlgButtonChecked( hwnd, MARK_BOXED ) ) {
                    if( *buf == '\0' ) {
                        WriteFn( "*" );
                    } else {
                        strcpy( boxbuf, buf );
                        if( ret < MARK_LEN - 5 ) {
                            strcpy( boxbuf, "* " );
                            strcpy( boxbuf + 2, buf );
                            strcpy( boxbuf + ret + 2, " *" );
                        }
                        strcpy( buf, boxbuf );
                        strset( boxbuf, '*' );
                        WriteFn( boxbuf );
                        WriteFn( buf );
                        WriteFn( boxbuf );
                    }
                } else {
                    WriteFn( buf );
                }
                EndDialog( hwnd, -1 );
                break;
            case MARK_CANCEL:
                EndDialog( hwnd, -1 );
                break;
            default:
                return( FALSE );
        }
        break;
    default:
        return( FALSE );
    }
    return( TRUE );

} /* MarkDlgProc */

/*
 * ProcessMark - start a mark dialog
 */
void ProcessMark( HWND owner, HANDLE instance, void (*func)( const char * ) )
{
    DLGPROC     dlgproc;

    if( WriteFn != NULL ) {
        return;
    }
    WriteFn = func;
    dlgproc = MakeProcInstance_DLG( MarkDlgProc, instance );
    DialogBox( instance, "MARK_DLG", owner, dlgproc );
    FreeProcInstance_DLG( dlgproc );
    WriteFn = NULL;

} /* ProcessMark */
