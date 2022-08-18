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
* Description:  Repeat count window.
*
****************************************************************************/


#include "vi.h"
#include "color.h"
#include "vifont.h"
#include "utils.h"
#include "wclbproc.h"
#include "win.h"
#include "winifini.h"


/* Local Windows CALLBACK function prototypes */
WINEXPORT LRESULT CALLBACK RepeatWindowProc( HWND, UINT, WPARAM, LPARAM );

window RepeatCountWindow = {
    &repcntw_info,
    { 0, 0, 0, 0 }
};

static char         *className = "RepeatWindow";
static char         repString[MAX_STR];

bool RepeatCountWindowInit( void )
{
    WNDCLASS        wc;

    repString[0] = '\0';

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = GetWndProc( RepeatWindowProc );
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof( LONG_PTR );
    wc.hInstance = InstanceHandle;
    wc.hIcon = LoadIcon( (HINSTANCE)NULLHANDLE, IDI_APPLICATION );
    wc.hCursor = LoadCursor( (HINSTANCE)NULLHANDLE, IDC_ARROW );
    wc.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = className;
    return( RegisterClass( &wc ) != 0 );

} /* init */

bool RepeatCountWindowFini( void )
{
    return( true );

} /* fini */

/*
 * drawRepeatString - draw the current repeat string
 */
static void drawRepeatString( void )
{
    int     height;
    RECT    rect;
    HDC     hdc;

    if( !AllowDisplay || BAD_ID( repeat_window_id ) ) {
        return;
    }
    GetClientRect( repeat_window_id, &rect );
    height = FontHeight( WIN_TEXT_FONT( &RepeatCountWindow ) );
    rect.bottom = rect.top + height;
    hdc = TextGetDC( repeat_window_id, WIN_TEXT_STYLE( &RepeatCountWindow ) );
    FillRect( hdc, &rect, ColorBrush( WIN_TEXT_BACKCOLOR( &RepeatCountWindow ) ) );
    TextReleaseDC( repeat_window_id, hdc );
    WriteString( repeat_window_id, 0, rect.top, WIN_TEXT_STYLE( &RepeatCountWindow ), repString );

} /* drawRepeatString */

/*
 * RepeatWindowProc - message procedure for the repeat count window
 */
WINEXPORT LRESULT CALLBACK RepeatWindowProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    PAINTSTRUCT ps;

    switch( msg ) {
    case WM_CREATE:
        SET_WNDINFO( hwnd, (LONG_PTR)&RepeatCountWindow );
        break;
    case WM_PAINT:
        BeginPaint( hwnd, &ps );
        if( !BAD_ID( repeat_window_id ) ) {
            drawRepeatString();
        }
        EndPaint( hwnd, &ps );
        return( 0 );
    case WM_SETFOCUS:
        SetFocus( root_window_id );
        return( 0 );
    }
    return( DefWindowProc( hwnd, msg, wparam, lparam ) );

} /* RepeatWindowProc */

/*
 * NewRepeatCountWindow - create a new repeat count window
 */
window_id NewRepeatCountWindow( void )
{
    RECT        *size;
    POINT       p;
    window_id   wid;

    size = &RepeatCountWindow.def_area;

    p.x = size->left;
    p.y = size->top;
    ClientToScreen( root_window_id, &p );

    repString[0] = '\0';
    wid = CreateWindow( className, "Repeat Count",
        WS_POPUPWINDOW | WS_BORDER | WS_CLIPSIBLINGS,
        p.x, p.y, size->right - size->left, size->bottom - size->top,
        root_window_id, (HMENU)NULLHANDLE, InstanceHandle, NULL );
    ShowWindow( wid, SW_SHOWNORMAL );
    UpdateWindow( wid );
    return( wid );

} /* NewRepeatCountWindow */

/*
 * UpdateRepeatString - a new repeat count has come in
 */
void UpdateRepeatString( char *str )
{
    ClearWindow( repeat_window_id );
    strcpy( repString, str );
    drawRepeatString();

} /* UpdateRepeatString */
