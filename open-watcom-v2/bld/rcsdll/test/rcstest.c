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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


/****************************************************************************

    PROGRAM: EditCntl.c

    PURPOSE: Creates an edit window

    FUNCTIONS:

        WinMain() - calls initialization function, processes message loop
        InitApplication() - initializes window data and registers window
        InitInstance() - saves instance handle and creates main window
        MainWndProc() - processes messages
        About() - processes messages for "About" dialog box

    COMMENTS:

        After setting up the application's window, the size of the client
        area is determined and a child window is created to use for editing.

****************************************************************************/

#include <windows.h>
#include <commdlg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "rcstest.h"
#include "rcsapi.h"
#include "wclbproc.h"


/* common functions */
extern RCSGetVersionFn  RCSGetVersion;
extern RCSSetSystemFn   RCSSetSystem;
extern RCSQuerySystemFn RCSQuerySystem;
extern RCSRegBatchCbFn  RCSRegisterBatchCallback;
extern RCSRegMsgBoxCbFn RCSRegisterMessageBoxCallback;
/* system specific functions -- mapped to function for appropriate system */
extern RCSInitFn        RCSInit;
extern RCSCheckoutFn    RCSCheckout;
extern RCSCheckinFn     RCSCheckin;
extern RCSHasShellFn    RCSHasShell;
extern RCSRunShellFn    RCSRunShell;
extern RCSFiniFn        RCSFini;
extern RCSSetPauseFn    RCSSetPause;

HINSTANCE hInst;

BatchCallback           *BatchProc;
BatchCallback           Batcher;
MessageBoxCallback      *MsgProc;
MessageBoxCallback      Messager;
rcsdata                 Cookie;

HANDLE hAccTable;                                /* handle to accelerator table */
HWND hwnd;                    /* handle to main windows  */

/****************************************************************************

    FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)

    PURPOSE: calls initialization function, processes message loop

****************************************************************************/

int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    lpCmdLine = lpCmdLine;

    if (!hPrevInstance) {
        if (!InitApplication(hInstance)) {
            return( FALSE );
        }
    }
    if( !InitInstance( hInstance, nCmdShow ) )
        return( FALSE );

    while( GetMessage(&msg, NULL, 0, 0) ) {

        //if( !TranslateAccelerator(hwnd, hAccTable, &msg)
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return( (int)msg.wParam );
}

int RCSAPI Batcher( rcsstring str, void *cookie )
{
    cookie = cookie;
    return( MessageBox( hwnd, (LPCSTR)str, (LPCSTR)"run batch", MB_OK ) );
}

int RCSAPI Messager( rcsstring text, rcsstring title, char *buffer, int len, void *cookie )
{
    /* unused parameters */ (void)len;

    cookie = cookie;
    strcpy( (char*)buffer, "this is a message" );
    return( MessageBox( hwnd, (LPCSTR)text, (LPCSTR)title, MB_OK ) );
}


/****************************************************************************

    FUNCTION: InitApplication(HANDLE)

    PURPOSE: Initializes window data and registers window class

****************************************************************************/

BOOL InitApplication(HANDLE hInstance)
{
    WNDCLASS  wc;

    wc.style = 0;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  "WatcomEditCntlMenu";
    wc.lpszClassName = "WEditCntlWClass";

    Cookie = RCSInit( hwnd, getenv( "WATCOM" ) );
    BatchProc = (BatchCallback *)MakeProcInstance( (FARPROC)&Batcher, hInst );
    MsgProc = (MessageBoxCallback *)MakeProcInstance( (FARPROC)&Messager, hInst );
    RCSRegisterBatchCallback( Cookie, BatchProc, NULL );
    RCSRegisterMessageBoxCallback( Cookie, MsgProc, NULL );

    return( RegisterClass( &wc ) );
}


/****************************************************************************

    FUNCTION:  InitInstance(HANDLE, int)

    PURPOSE:  Saves instance handle and creates main window

****************************************************************************/

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;
    hAccTable = LoadAccelerators(hInst, "WatcomEditCntlAcc");

    hwnd = CreateWindow(
        "WEditCntlWClass",
        "Watcom EditCntl Sample Application",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if( !hwnd )
        return( FALSE );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    return( TRUE );

}

/****************************************************************************

    FUNCTION: MainWndProc(HWND, UINT, WPARAM, LPARAM)

    PURPOSE:  Processes messages

    MESSAGES:

        WM_COMMAND    - application menu (About dialog box)
        WM_DESTROY    - destroy window

****************************************************************************/

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DLGPROC lpProcAbout;

    switch (message) {
    case WM_COMMAND:
        switch (wParam) {
        case IDM_ABOUT:
            lpProcAbout = MakeProcInstance_DLG(About, hInst);
            DialogBox(hInst, "AboutBox", hWnd, lpProcAbout);
            FreeProcInstance_DLG(lpProcAbout);
            break;
        /* RCS menu commands */
        case IDM_CHECKIN:
            RCSCheckin( Cookie, "foo.c", "", "" );
            break;
        case IDM_CHECKOUT:
            RCSCheckout( Cookie, "foo.c", "", "" );
            break;
        case IDM_RUNSHELL:
            RCSRunShell( Cookie );
            break;
        case IDM_SET_MKS_RCS:
            RCSSetSystem( Cookie, MKS_RCS );
            if( RCSQuerySystem( Cookie ) == MKS_RCS ) {
                SetWindowText( hwnd, "MKS RCS" );
            } else {
                SetWindowText( hwnd, "error setting RCS system" );
            }
            break;
        case IDM_SET_MKS_SI:
            RCSSetSystem( Cookie, MKS_SI );
            if( RCSQuerySystem( Cookie ) == MKS_SI ) {
                SetWindowText( hwnd, "MKS SI" );
            } else {
                SetWindowText( hwnd, "error setting RCS system" );
            }
            break;
        case IDM_SET_PVCS:
            RCSSetSystem( Cookie, PVCS );
            if( RCSQuerySystem( Cookie ) == PVCS ) {
                SetWindowText( hwnd, "PVCS" );
            } else {
                SetWindowText( hwnd, "error setting RCS system" );
            }
            break;
        case IDM_SET_WPROJ:
            RCSSetSystem( Cookie, WPROJ );
            if( RCSQuerySystem( Cookie ) == WPROJ ) {
                SetWindowText( hwnd, "WPROJ" );
            } else {
                SetWindowText( hwnd, "error setting RCS system" );
            }
            break;
        case IDM_SET_GENERIC:
            RCSSetSystem( Cookie, GENERIC );
            if( RCSQuerySystem( Cookie ) == GENERIC ) {
                SetWindowText( hwnd, "GENERIC" );
            } else {
                SetWindowText( hwnd, "error setting RCS system" );
            }
            break;
        case IDM_SET_PERFORCE:
            RCSSetSystem( Cookie, PERFORCE );
            if( RCSQuerySystem( Cookie ) == PERFORCE ) {
                SetWindowText( hwnd, "Perforce" );
            } else {
                SetWindowText( hwnd, "error setting RCS system" );
            }
            break;
        case IDM_SET_GIT:
            RCSSetSystem( Cookie, GIT );
            if( RCSQuerySystem( Cookie ) == GIT ) {
                SetWindowText( hwnd, "Git" );
            } else {
                SetWindowText( hwnd, "error setting RCS system" );
            }
            break;
        case IDM_QUERY_SYS:
            switch( RCSQuerySystem( Cookie ) ) {
                case NO_RCS: SetWindowText( hwnd, "none" ); break;
                case MKS_RCS: SetWindowText( hwnd, "mksrcs" ); break;
                case MKS_SI: SetWindowText( hwnd, "mkssi" ); break;
                case PVCS: SetWindowText( hwnd, "pvcs" ); break;
                case GENERIC: SetWindowText( hwnd, "GENERIC" ); break;
                case WPROJ: SetWindowText( hwnd, "wproj" ); break;
                case PERFORCE: SetWindowText( hwnd, "Perforce" ); break;
                case GIT: SetWindowText( hwnd, "Git" ); break;
            }
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return( DefWindowProc(hWnd, message, wParam, lParam) );
    }
    return( 0 );
}


/****************************************************************************

    FUNCTION: About(HWND, unsigned, WORD, LONG)

    PURPOSE:  Processes messages for "About" dialog box

    MESSAGES:

        WM_INITDIALOG - initialize dialog box
        WM_COMMAND    - Input received

****************************************************************************/

INT_PTR WINAPI About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    lParam = lParam;
    switch (message) {
        case WM_INITDIALOG:
            return( TRUE );

        case WM_COMMAND:
            if (wParam == IDOK
                || wParam == IDCANCEL) {
                EndDialog(hDlg, TRUE);
                return( TRUE );
            }
            break;
    }
    return( FALSE );
}
