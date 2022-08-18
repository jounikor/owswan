/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2021 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Editor's main window.
*
****************************************************************************/


#include "vi.h"
#include "window.h"
#include "win.h"
#include "wclbproc.h"


/* Local Windows CALLBACK function prototypes */
WINEXPORT LRESULT CALLBACK MainWindowProc( HWND, UINT, WPARAM, LPARAM );

void    DefaultWindows( RECT *, RECT * );

RECT    RootRect;
WORD    RootState;

/*
 * RegisterMainWindow - register the main (root) window class
 */
bool RegisterMainWindow( HANDLE inst )
{
    WNDCLASS    wc;

    wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = GetWndProc( MainWindowProc );
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = inst;
    wc.hIcon = LoadIcon( inst, "APPLICON" );
    wc.hCursor = LoadCursor( (HINSTANCE)NULLHANDLE, IDC_ARROW );
    wc.hbrBackground = 0;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = EditorName;
    return( RegisterClass( &wc ) != 0 );

} /* RegisterMainWindow */

static int      initX;
static int      initY;
static int      initWidth;
static int      initHeight = CW_USEDEFAULT;

/*
 * setDefault - set default window size
 */
static void setDefault( void )
{
    initX = CW_USEDEFAULT;
    initY = CW_USEDEFAULT;
    initWidth = CW_USEDEFAULT;
    initHeight = CW_USEDEFAULT;

} /* setDefault */

/*
 * CreateMainWindow - create the root window
 */
window_id CreateMainWindow( HANDLE inst )
{
    window_id   wid;
    int         maxx, maxy;

    if( initHeight <= 0 || initWidth <= 0 ) {
        setDefault();
    } else {
        maxx = GetSystemMetrics( SM_CXSCREEN );
        maxy = GetSystemMetrics( SM_CYSCREEN );
        if( initX < 0 ) {
            initX = 0;
        } else if( initX >= maxx ) {
            initX = maxx - 50;
        }
        if( initY < 0 ) {
            initY = 0;
        } else if( initY > maxy ) {
            initY = maxy - 50;
        }
        if( initWidth > maxx ) {
            initWidth = maxx;
        }
        if( initHeight > maxy ) {
            initHeight = maxy;
        }
    }

    wid = CreateWindow( EditorName, EditorName,
                         WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                         initX, initY, initWidth, initHeight,
                         NO_WINDOW, (HMENU)NULLHANDLE, inst, NULL );
    return( wid );

} /* CreateMainWindow */

/*
 * SetInitialWindowSize - process -P command line parm
 */
void SetInitialWindowSize( const char *arg )
{
    DWORD       a;

    if( !GetDWORD( &arg, &a ) ) {
        return;
    }
    initX = a;

    if( !GetDWORD( &arg, &a ) ) {
        return;
    }
    initY = a;

    if( !GetDWORD( &arg, &a ) ) {
        return;
    }
    initWidth = a;

    if( !GetDWORD( &arg, &a ) ) {
        return;
    }
    initHeight = a;

} /* SetInitialWindowSize */

/*
 * SetInitialWindowRect - set the initial size of the main window based
 *                        on a specified rectangle
 */
void SetInitialWindowRect( RECT *r )
{
    initX = r->left;
    initY = r->top;
    initHeight = r->bottom - r->top;
    initWidth = r->right - r->left;

} /* SetInitialWindowRect */

/*
 * ResizeRoot - change root to new size
 */
void ResizeRoot( void )
{
    RECT        rect;
    RECT        root_rect;
    int         height;
    HWND        bufHwnd;

    if( !EditFlags.WindowsStarted ) {
        return;
    }
    GetClientRect( root_window_id, &root_rect );
    DefaultWindows( &root_rect, &rect );
    if( !BAD_ID( message_window_id ) ) {
        NewMessageWindow();
        if( !BAD_ID( message_window_id ) ) {
            InvalidateRect( message_window_id, NULL, FALSE );
            SendMessage( message_window_id, WM_PAINT, 0, 0L );
        }
    }
    if( !BAD_ID( status_window_id ) ) {
        NewStatusWindow();
        if( !BAD_ID( status_window_id ) ) {
            InvalidateRect( status_window_id, NULL, FALSE );
            SendMessage( status_window_id, WM_PAINT, 0, 0L );
        }
    }
    height = rect.bottom - rect.top;
    MoveWindow( edit_container_window_id, rect.left, rect.top, rect.right - rect.left, height, TRUE );
    if( CurrentInfo ) {
        bufHwnd = CurrentInfo->current_window_id;
        if( IsWindow( bufHwnd ) && IsZoomed( bufHwnd ) ) {
            ShowWindow( bufHwnd, SW_SHOWMAXIMIZED );
        }
    }
    InvalidateRect( root_window_id, NULL, TRUE );

} /* ResizeRoot */

#define TIMER_ID    0x02
static UINT_PTR     timerID;

/*
 * MainWindowProc - procedure for main (root) window
 */
WINEXPORT LRESULT CALLBACK MainWindowProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    RECT        rect;
    vi_rc       rc;
    HANDLE      hfileinfo;
    int         cnt, i;
    char        buff[FILENAME_MAX + 2];   /* we add a " at the beginning and at the end so we can handle path- and filenames with spaces */

    switch( msg ) {
    case WM_CREATE:
        root_window_id = hwnd;
        GetClientRect( hwnd, &rect );
        edit_container_window_id = CreateContainerWindow( &rect );
        InitWindows();
        DragAcceptFiles( hwnd, TRUE );
        timerID = SetTimer( hwnd, TIMER_ID, 60L * 1000L, NULL );
        break;
    case WM_DROPFILES:
        hfileinfo = (HANDLE)wparam;
        cnt = DragQueryFile( hfileinfo, (UINT)-1, NULL, 0 );
        buff[0] = '"';      /* one " at the beginning of the filename */
        for( i = 0; i < cnt; i++ ) {
            if( DragQueryFile( hfileinfo, i, buff + 1, FILENAME_MAX ) == (UINT)-1 ) {
                break;
            }
            strcat( buff, SingleDQuote );
            rc = EditFile( buff, false );
            if( rc > ERR_NO_ERR ) {
                Error( GetErrorMsg( rc ) );
            }
        }
        DragFinish( hfileinfo );
        break;
    case WM_TIMER:
        UpdateStatusWindow();
        break;
    case WM_KEYDOWN:
        if( WindowsKeyPush( LOWORD( wparam ), HIWORD( lparam ) ) ) {
            return( 0 );
        }
        break;
    case WM_SIZE:
        DefFrameProc( hwnd, edit_container_window_id, msg, wparam, lparam );
        RootState = LOWORD( wparam );
        if( wparam != SIZE_MINIMIZED ) {
            ResizeRoot();
            GetWindowRect( hwnd, &RootRect );
            if( wparam != SIZE_MAXIMIZED ) {
                RootState = SIZE_RESTORED;
            }
        }
        return( 0 );
    case WM_MOVE:
        DefFrameProc( hwnd, edit_container_window_id, msg, wparam, lparam );
        if( RootState != SIZE_MINIMIZED ) {
            GetWindowRect( hwnd, &RootRect );
        }
        return( 0 );
    case WM_ACTIVATEAPP:
        if( BAD_ID( current_window_id ) ) {
            break;
        }
        SetFocus( root_window_id );
#if 0
        if( !wparam ) {
            InactiveWindow( current_window_id );
        } else {
            SendMessage( edit_container_window_id, WM_MDIACTIVATE, (WPARAM)current_window_id, 0L );
        }
#endif
        if( wparam ) {
            ResetEditWindowCursor( current_window_id );
        } else {
            GoodbyeCursor( current_window_id );
        }
        break;
    case WM_MOUSEACTIVATE:
        SetFocus( hwnd );
        return( MA_ACTIVATE );
    case WM_SETFOCUS:
        if( BAD_ID( current_window_id ) ) {
            break;
        }
        if( !IsIconic( current_window_id ) ) {
            SendMessage( edit_container_window_id, WM_MDIACTIVATE, (WPARAM)current_window_id, 0L );
            DCUpdate();
            SetWindowCursor();
            SetWindowCursorForReal();
            return( 0 );
        }
        break;
    case WM_NCLBUTTONDBLCLK:
        break;
    case WM_COMMAND:
        if( LOWORD( wparam ) > 0xF000 ) {
            break;
        } else {
            rc = MenuCommand( LOWORD( wparam ) );
            if( rc != MENU_COMMAND_NOT_HANDLED ) {
                DCUpdateAll();
                if( rc > ERR_NO_ERR ) {
                    Error( GetErrorMsg( rc ) );
                }
            }
            SetWindowCursor();
        }
        return( 0 );
    case WM_INITMENU:
        if( (HMENU)wparam == GetMenu( hwnd ) ) {
            HandleInitMenu( (HMENU)wparam );
        } else {
            ResetMenuBits();
        }
        break;
    case WM_MENUSELECT:
        HandleMenuSelect( wparam, lparam );
        break;
    case WM_ENDSESSION:
        if( wparam ) {
            ExitEditor( 0 );
            // will not return
        }
        return( 0 );
    case WM_QUERYENDSESSION:
        return( ExitWithPrompt( false, true ) );
    case WM_CLOSE:
        ExitWithPrompt( true, true );
        return( 0 );
#ifdef __NT__
    case WM_MOUSEWHEEL:
        {
            int     increment;
            ULONG   linesPerNotch;
            HWND    activeWnd;

            activeWnd = (HWND)SendMessage( edit_container_window_id, WM_MDIGETACTIVE, 0, 0 );
            SystemParametersInfo( SPI_GETWHEELSCROLLLINES, 0, &linesPerNotch, 0 );

            increment = GET_WHEEL_DELTA_WPARAM( wparam ) / 120;
                // see WM_MOUSEWHEEL-documentation for information about the "120"

            if( increment > 0 ) {
                for( i = 0; i < increment * (int)linesPerNotch; i++ ) {
                    SendMessage( activeWnd, WM_VSCROLL, SB_LINEUP, 0 );
                }
            } else {
                for( i = 0; i < (-increment) * (int)linesPerNotch; i++ ) {
                    SendMessage( activeWnd, WM_VSCROLL, SB_LINEDOWN, 0 );
                }
            }
        }
        return( 0 );
#endif
    case WM_DESTROY:
        DestroyToolBar();
        DragAcceptFiles( hwnd, FALSE );
        edit_container_window_id = NO_WINDOW;
        if( timerID ) {
            KillTimer( hwnd, TIMER_ID );
        }
        return( 0 );
    }
    return( DefFrameProc( hwnd, edit_container_window_id, msg, wparam, lparam ) );

} /* MainWindowProc */
