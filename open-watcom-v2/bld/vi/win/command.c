/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2016 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Command window.
*
****************************************************************************/


#include "vi.h"
#include "color.h"
#include "vifont.h"
#include "utils.h"
#include "win.h"
#include "wclbproc.h"
#include "winifini.h"


/* Local Windows CALLBACK function prototypes */
WINEXPORT LRESULT CALLBACK CommandWindowProc( HWND, UINT, WPARAM, LPARAM );

#define LIST_BOX        1000

extern bool ReadingAString;

window CommandWindow = {
    &cmdlinew_info,
    { 0, 0, 0, 0 }
};

window_id command_window_id = NO_WINDOW;

static char *className = "CommandWindow";

bool CommandWindowInit( void )
{
    WNDCLASS        wc;

    wc.style = 0;
    wc.lpfnWndProc = GetWndProc( CommandWindowProc );
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof( LONG_PTR );
    wc.hInstance = InstanceHandle;
    wc.hIcon = LoadIcon( (HINSTANCE)NULLHANDLE, IDI_APPLICATION );
    wc.hCursor = LoadCursor( (HINSTANCE)NULLHANDLE, IDC_ARROW );
    wc.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = className;
    return( RegisterClass( &wc ) != 0 );
}

bool CommandWindowFini( void )
{
    return( true );
}

WINEXPORT LRESULT CALLBACK CommandWindowProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    PAINTSTRUCT ps;
    HDC         hdc;
    window_id   wid;

    switch( msg ) {
    case WM_CREATE:
        command_window_id = hwnd;
        SET_WNDINFO( hwnd, (LONG_PTR)&CommandWindow );
        break;
    case WM_SETFOCUS:
        /* turn on caret */
        NewCursor( hwnd, EditVars.NormalCursorType );
        break;
    case WM_KILLFOCUS:
        /* turn off the caret */
        MyHideCaret( hwnd );
        DestroyCaret();
        wid = (window_id)wparam;
        if( !BAD_ID( wid ) && ( wid == root_window_id || GetWindow( wid, GW_OWNER ) == edit_container_window_id ) ) {
            /* hmmm... losing focus to one of our own windows - suicide */
            if( ReadingAString ) {
                KeyAdd( VI_KEY( ESC ) );
            }
        }
        break;
    case WM_KEYDOWN:
        if( WindowsKeyPush( LOWORD( wparam ), HIWORD( lparam ) ) ) {
            return( 0 );
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint( hwnd, &ps );
        FillRect( hdc, &ps.rcPaint, ColorBrush( WIN_TEXT_BACKCOLOR( &CommandWindow ) ) );
        /* this will cause the command window to redraw itself in readstr.c */
        KeyAdd( VI_KEY( ALT_END ) );
        EndPaint( hwnd, &ps );
        break;
    case WM_DESTROY:
        command_window_id = NO_WINDOW;
        break;
    }
    return( DefWindowProc( hwnd, msg, wparam, lparam ) );
}


window_id NewCommandWindow( void )
{
    RECT        *size;
    window_id   wid;
    POINT       p;
    int         bottom;

    size = &CommandWindow.def_area;
    bottom = size->bottom;

    p.x = size->left;
    p.y = size->top;
    ClientToScreen( root_window_id, &p );
    wid = CreateWindow( className, "Prompt",
        WS_POPUPWINDOW | WS_CLIPSIBLINGS,
        p.x, p.y,
        size->right - size->left, bottom - size->top, root_window_id,
        (HMENU)NULLHANDLE, InstanceHandle, NULL );
    ShowWindow( wid, SW_SHOWNORMAL );
    UpdateWindow( wid );
    return( wid );
}
