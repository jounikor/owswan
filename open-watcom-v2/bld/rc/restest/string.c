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
* Description:  Display a string resource.
*
****************************************************************************/


#include <windows.h>
#include "restest.h"
#include "verinfo.h"
#include "wclbproc.h"


void DisplayStrings( HWND hwnd )
{
    UINT        i;
    char        buf[256];
    HWND        lb;
    int         ret;

    lb = GetDlgItem( hwnd, LBOX );
    for( i = 0; i < 1000; i++ ) {
#ifdef __NT__
        ret = LoadStringW( Instance, i, (LPWSTR)buf, sizeof( buf ) );
#else
        ret = LoadString( Instance, i, buf, sizeof( buf ) );
#endif
        if( ret > 0 ) {
            ret++;
#ifdef __NT__
            ret *= 2;
#endif
            LBPrintf( lb, "" );
            LBPrintf( lb, "STRING: %u", i );
            LBDump( lb, buf, ret );
        }
    }
}

INT_PTR CALLBACK StrTableDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    wparam = wparam;
    lparam = lparam;
    switch( msg ) {
    case WM_INITDIALOG:
        DisplayStrings( hwnd );
        break;
    case WM_CLOSE:
        EndDialog( hwnd, 0 );
        break;
    default:
        return( FALSE );
    }
    return( TRUE );
}

void DisplayStringTable( void )
{
    DLGPROC     dlgproc;

    dlgproc = MakeProcInstance_DLG( StrTableDlgProc, Instance );
    DialogBox( Instance, "VERINFODLG" , NULL, dlgproc );
    FreeProcInstance_DLG( dlgproc );
}
