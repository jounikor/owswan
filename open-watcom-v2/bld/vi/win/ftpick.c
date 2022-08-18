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
* Description:  Font picker.
*
****************************************************************************/


#include "vi.h"
#include "utils.h"
#include "ftbar.rh"
#include "vifont.h"
#include "sstyle.h"
#include "wclbproc.h"


/* Local Windows CALLBACK function prototypes */
WINEXPORT LRESULT CALLBACK FtPickProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

extern  LOGFONT     CurLogfont;

static  bool        haveCapture = false;
static  window_id   mod_wid = NO_WINDOW;
static  POINT       m_pt;

static void sendNewFontCurrentWindow( void )
{
    int             row, col;
    syntax_element  style;
    bool            totally;
    linenum         line_num;

    ScreenToClient( mod_wid, &m_pt );
    ClientToRowCol( mod_wid, m_pt.x, m_pt.y, &row, &col, DIVIDE_BETWEEN );

    /* someone is base 0, someone else isn't.  bummer.
     * Also row may not be valid if attemping to drop beyond bottom
     * of visible text, so check!
     */

    if( col < 1 ) {
        return;
    }
    col--;

    // SStyle expect real not virtual columns!
    // Hmmm.
    line_num = (linenum)(LeftTopPos.line + row - 1);
    col = RealCursorPositionOnLine( line_num, col );

    style = SSGetStyle( row, col );
    if( style != SE_UNPARSED ) {
        /*
         * ASSUMPTION: font #s in win.cfg match SE_XXX enum values!
         */
        totally = false;
        if( CtrlDown() ) {
            totally = true;
        }
        EnsureUniformFonts( 0, SE_NUMTYPES - 1, &CurLogfont, totally );
        SetUpFont( &CurLogfont, SEType[style].font );
    }
}

static void sendNewFont( void )
{
    type_style  *mod_style;

    if( BAD_ID( mod_wid ) ) {
        return;
    }

    mod_style = WIN_TEXT_STYLE( WINDOW_FROM_ID( mod_wid ) );

    if( mod_wid == current_window_id ) {
        sendNewFontCurrentWindow();
    } else if( mod_wid != GetToolbarWindow() ) {
        /* (toolbar has no font)
        */
        SetUpFont( &CurLogfont, mod_style->font );
    }
}

static LRESULT doDrop( HWND hwnd, WPARAM wparam )
{
    DrawRectangleUpDown( GetDlgItem( GetParent( hwnd ), FT_RECTANGLE ), DRAW_UP );
    CursorOp( COP_ARROW );
    ReleaseCapture();
    haveCapture = false;
    if( (wparam & MK_RBUTTON) == 0 ) {
        RECT r;
        sendNewFont();
        GetWindowRect( hwnd, &r );
        InvalidateRect( hwnd, &r, 0 );
        UpdateWindow( hwnd );
    }
    return( 0 );
}

static LRESULT processMouseMove( window_id wid, UINT msg, WPARAM wparam, LPARAM lparam )
{
    RECT    rect;

    if( !haveCapture ) {
        return( DefWindowProc( wid, msg, wparam, lparam ) );
    }

    // check we aren't on ourselves first
    m_pt.x = GET_X( lparam );
    m_pt.y = GET_Y( lparam );
    ClientToScreen( wid, &m_pt );
    GetWindowRect( GetParent( wid ), &rect );
    if( PtInRect( &rect, m_pt ) ) {
        CursorOp( COP_DROPFT );
        mod_wid = NO_WINDOW;
        return( 0 );
    }

    /* otherwise, figure out what we're over & change element display
    */
    mod_wid = GetOwnedWindow( m_pt );
    if( !BAD_ID( mod_wid ) && mod_wid != GetToolbarWindow() ) {
        CursorOp( COP_DROPFT );
    } else {
        mod_wid = NO_WINDOW;
        CursorOp( COP_NODROP );
    }
    return( 0 );
}

static LRESULT drawCurLogfont( window_id wid, UINT msg, WPARAM wparam, LPARAM lparam )
{
    PAINTSTRUCT ps;
    HDC         hdc;
    HFONT       hFont;
    RECT        rect;
    TEXTMETRIC  tm;
    int         x, y;
    int         trim;
    SIZE        size;

    (void)msg; (void)wparam; (void)lparam;

    hFont = CreateFontIndirect( &CurLogfont );
    GetClientRect( wid, &rect );

    hdc = BeginPaint( wid, &ps );
    SelectObject( hdc, hFont );
    SetTextColor( hdc, GetSysColor( COLOR_BTNTEXT ) );
    SetBkColor( hdc, GetSysColor( COLOR_BTNFACE ) );
    GetTextExtentPoint( hdc, "Waterloo", 8, &size );
    GetTextMetrics( hdc, &tm );
    trim = tm.tmDescent + tm.tmInternalLeading;
    x = (rect.right - size.cx) / 2;
    y = (rect.bottom - size.cy + trim) / 2;
    if( x < 0 ) {
        x = 0;
    }
    if( (size.cy - trim) > rect.bottom  ) {
        /* align baseline with bottom of window
        */
        y = rect.bottom - size.cy + trim;
    }
    y -= tm.tmInternalLeading;
    TextOut( hdc, x, y, "Waterloo", 8 );
    EndPaint( wid, &ps );

    DeleteObject( hFont );

    return( 0 );
}

static LRESULT setupForDrop( window_id wid )
{
    DrawRectangleUpDown( GetDlgItem( GetParent( wid ), FT_RECTANGLE ), DRAW_DOWN );
    CursorOp( COP_DROPFT );
    SetCapture( wid );
    haveCapture = true;
    mod_wid = NO_WINDOW;
    return( 0 );
}

WINEXPORT LRESULT CALLBACK FtPickProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    switch( msg ) {
    case WM_CREATE:
        return( 0 );
    case WM_PAINT:
        return( drawCurLogfont( hwnd, msg, wparam, lparam ) );
    case WM_LBUTTONDOWN:
        return( setupForDrop( hwnd ) );
    case WM_LBUTTONUP:
        return( doDrop( hwnd, wparam ) );
    case WM_MOUSEMOVE:
        return( processMouseMove( hwnd, msg, wparam, lparam ) );
    case WM_DESTROY:
        return( 0 );
    }
    return( DefWindowProc( hwnd, msg, wparam, lparam ) );
}

void InitFtPick( void )
{
    WNDCLASS    wndclass;

    if( GetClassInfo( InstanceHandle, "FtPick", &wndclass ) ) {
        return;
    }

    wndclass.style          = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = GetWndProc( FtPickProc );
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = InstanceHandle;
    wndclass.hIcon          = (HICON)NULLHANDLE;
    wndclass.hCursor        = LoadCursor( (HINSTANCE)NULLHANDLE, IDC_ARROW );
    wndclass.hbrBackground  = (HBRUSH)COLOR_APPWORKSPACE;
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = "FtPick";

    RegisterClass( &wndclass );
}

void FiniFtPick( void )
{
    UnregisterClass( "FtPick", InstanceHandle );
}

