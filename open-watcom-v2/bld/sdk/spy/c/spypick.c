/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2017-2017 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Spy pick window dialog functions.
*
****************************************************************************/


#include "spy.h"
#include "wclbproc.h"


/* Local Window callback functions prototypes */
WINEXPORT INT_PTR CALLBACK PickDialogDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

static HWND     LastFramed;
static bool     Cancelled = false;
static bool     Picking = false;
static HWND     PickDialogOK;
static HWND     PickDialogIcon;
static ctl_id   pickProcCmdId;

/*
 * FrameAWindow - draw a frame around a window
 */
void FrameAWindow( HWND hwnd )
{
    HDC         hdc;
    RECT        rect;
    HPEN        hpen;

    if( hwnd == NULL ) {
        return;
    }

    hdc = GetWindowDC( hwnd );

    SetROP2( hdc, R2_NOT ); /* reverse screen color */

    SelectObject( hdc, GetStockObject( NULL_BRUSH ) );

    hpen = CreatePen( PS_INSIDEFRAME, 4 * GetSystemMetrics( SM_CXBORDER ),
                      RGB( 0, 0, 0) );
    SelectObject( hdc, hpen );

    GetWindowRect( hwnd, &rect );

    Rectangle( hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top );
    ReleaseDC( hwnd, hdc );

    DeleteObject( hpen );

} /* FrameAWindow */

/*
 * UpdateFramedInfo
 */
void UpdateFramedInfo( HWND dlg, HWND framedhwnd, bool ispick  )
{
    char        name[64];
    char        id[10];
    char        str[512];
    const char  *fmtstr;
    int         len;
    RECT        rect;

    if( ispick ) {

        GetWindowName( framedhwnd, str );
        SetDlgItemText( dlg, PEEKMSG_TITLE, str );

        GetWindowName( GetParent( framedhwnd ), str );
        SetDlgItemText( dlg, PEEKMSG_PARENT, str );

        len = GetClassName( framedhwnd, name, sizeof( name ) );
        name[len] = '\0';
        SetDlgItemText( dlg, PEEKMSG_CLASS, name );

        if( framedhwnd != NULL ) {
            GetWindowRect( framedhwnd, &rect );
            fmtstr = GetRCString( STR_DIM_COORD_FMT );
            sprintf( str, fmtstr, rect.left, rect.top, rect.right, rect.bottom,
                     rect.right - rect.left, rect.bottom - rect.top);
            SetDlgItemText( dlg, PEEKMSG_SIZE, str );
        } else {
            SetDlgItemText( dlg, PEEKMSG_SIZE, NULL );
        }

        GetWindowStyleString( framedhwnd, name, str );
        SetDlgItemText( dlg, PEEKMSG_STYLE, name );
        DumpToComboBox( str, GetDlgItem( dlg, PEEKMSG_STYLECB ) );

        GetClassStyleString( framedhwnd, name, str );
        SetDlgItemText( dlg, PEEKMSG_STYLECLASS, name );
        DumpToComboBox( str, GetDlgItem( dlg, PEEKMSG_STYLECLASSCB ) );

    } else {

        GetHexStr( id, (UINT_PTR)framedhwnd, SPYOUT_HWND_LEN );
        id[SPYOUT_HWND_LEN] = '\0';
        SetDlgItemText( dlg, WINSEL_HWND, id );
        len = GetWindowText( framedhwnd, name, sizeof( name ) );
        name[len] = '\0';
        SetDlgItemText( dlg, WINSEL_NAME, name );
    }

} /* UpdateFramedInfo */

/*
 * GetWindowID - get window ID from mouse coordinates
 */
static void GetWindowID( HWND hwnd, HWND *who, LPARAM lparam )
{
    POINT       p;
    HWND        child;

    p.x = (int_16)LOWORD( lparam );
    p.y = (int_16)HIWORD( lparam );

    ClientToScreen( hwnd, &p );
    *who = WindowFromPoint( p );
    ScreenToClient( *who, &p );
    child = ChildWindowFromPoint( *who, p );
    if( child != NULL ) {
        *who = child;
    }

} /* GetWindowID */

/*
 * PickDialogDlgProc - select a window
 */
INT_PTR CALLBACK PickDialogDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    RECT    rect;
    RECT    client_rect;
    POINT   point;
    HWND    who;

    switch( msg ) {
    case WM_INITDIALOG:
        PickDialogOK = GetDlgItem( hwnd, IDOK );
        if( pickProcCmdId == SPY_PEEK_WINDOW ) {
            PickDialogIcon = GetDlgItem( hwnd, PEEKMSG_ICON );
        } else {
            PickDialogIcon = GetDlgItem( hwnd, WINSEL_ICON );
        }
        GetWindowRect( PickDialogIcon, &rect );
        ScreenToClient( hwnd, (POINT *)&rect );
        ScreenToClient( hwnd, (POINT *)&rect + 1 );
        GetClientRect( hwnd, &client_rect );
        rect.left = (client_rect.left + client_rect.right) / 2 -
                    (rect.right - rect.left) / 2;
        SetWindowPos( PickDialogIcon, NULL, rect.left, rect.top, 0, 0,
                      SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER );
        SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
        UpdateFramedInfo( hwnd, NULL, (pickProcCmdId == SPY_PEEK_WINDOW) );
        if( pickProcCmdId != SPY_PEEK_WINDOW ) {
            EnableWindow( PickDialogOK, FALSE );
        }
        Picking = false;
        LastFramed = NULL;
        break;
#ifndef NOUSE3D
    case WM_SYSCOLORCHANGE:
        CvrCtl3dColorChange();
        break;
#endif
    case WM_COMMAND:
        switch( LOWORD( wparam ) ) {
        case IDCANCEL:
            Cancelled = true;
            EndDialog( hwnd, 0 );
            break;
        case IDOK:
            EndDialog( hwnd, 0 );
            break;
        }
        break;
    case WM_LBUTTONDOWN:
        point.x = LOWORD( lparam );
        point.y = HIWORD( lparam );
        ClientToScreen( hwnd, &point );
        GetWindowRect( PickDialogIcon, &rect);
        if( PtInRect( &rect, point ) ) {
            Picking = true;
            LastFramed = NULL;
            UpdateFramedInfo( hwnd, NULL, (pickProcCmdId == SPY_PEEK_WINDOW) );
            SetCapture( hwnd );
        }
        break;
    case WM_MOUSEMOVE:
        if( Picking ) {
            GetWindowID( hwnd, &who, lparam );
            if( LastFramed != who ) {
                if( who != NULL && who != hwnd && GetParent( who ) != hwnd ) {
                    if( LastFramed != NULL ) {
                        FrameAWindow( LastFramed );
                    }
                    FrameAWindow( who );
                    UpdateFramedInfo( hwnd, who, (pickProcCmdId == SPY_PEEK_WINDOW) );
                    LastFramed = who;
                    if( pickProcCmdId != SPY_PEEK_WINDOW ) {
                        EnableWindow( PickDialogOK, TRUE );
                    }
                } else {
                    if( LastFramed != NULL ) {
                        FrameAWindow( LastFramed );
                    }
                    UpdateFramedInfo( hwnd, NULL, (pickProcCmdId == SPY_PEEK_WINDOW ) );
                    LastFramed = NULL;
                    if( pickProcCmdId != SPY_PEEK_WINDOW ) {
                        EnableWindow( PickDialogOK, FALSE );
                    }
                }
            }
        }
        break;
    case WM_LBUTTONUP:
        if( Picking ) {
            Picking = false;
            ReleaseCapture();
            if( LastFramed != NULL ) {
                FrameAWindow( LastFramed );
            }
        }
        break;
    default:
        return( FALSE );
    }
    return( TRUE );

} /* PickDialogDlgProc */

/*
 * DoPickDialog - start dialog for window selection
 */
HWND DoPickDialog( ctl_id cmdid )
{
    DLGPROC dlgproc;

    pickProcCmdId = cmdid;
    ShowWindow( SpyMainWindow, SW_MINIMIZE );

    LastFramed = NULL;
    Cancelled = false;

    dlgproc = MakeProcInstance_DLG( PickDialogDlgProc, Instance );
    if( cmdid == SPY_PEEK_WINDOW ) {
        JDialogBox( ResInstance, "PEEKMSGS", SpyMainWindow, dlgproc );
    } else {
        JDialogBox( ResInstance, "WINDOWPICK", SpyMainWindow, dlgproc );
    }
    FreeProcInstance_DLG( dlgproc );
    ShowWindow( SpyMainWindow, SW_NORMAL );
    if( !Cancelled ) {
        return( LastFramed );
    }
    return( NULL );

} /* DoPickDialog */

