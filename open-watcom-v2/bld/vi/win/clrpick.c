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
* Description:  Color picker dialog.
*
****************************************************************************/


#include "commonui.h"
#include "vi.h"
#include "utils.h"
#include "clrbar.rh"
#include "color.h"
#include "statwnd.h"
#include "toolbr.h"
#include "sstyle.h"
#include "subclass.h"


/* Local Windows CALLBACK function prototypes */
WINEXPORT LRESULT CALLBACK ClrPickProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

#define NUM_ACROSS      8
#define NUM_DOWN        8
#define NUM_COLOURS     (NUM_ACROSS * NUM_DOWN)

#define WIDTH           15
#define HEIGHT          15

#define SPC             2
#define INDEX_FROM_XY( x, y )   ((y) * NUM_ACROSS + (x))
#define COLOUR_FROM_XY( x, y )  (RGBValues[INDEX_FROM_XY( x, y )])

typedef enum NewColourOps {
    NC_FORE,
    NC_BACK
} NewColourOps;

extern HWND hColorbar;

int         Width;
int         Height;

static int              cursx = 0;
static int              cursy = 0;
static COLORREF         RGBValues[NUM_COLOURS];
static bool             haveCapture = false;
static window_id        mod_wid = NO_WINDOW;
static POINT            m_pt;

static void sendNewColourToolbar( void )
{
    window_id   toolbar_wid;

    /* toolbar has no text_style data in w_info format - change directly
    */
    EditVars.ToolBarColor = INDEX_FROM_XY( cursx, cursy );
    ToolBarChangeSysColors( RGBValues[EditVars.ToolBarColor],
                            GetSysColor( COLOR_BTNHIGHLIGHT ),
                            GetSysColor( COLOR_BTNSHADOW ) );
    /* also redraw seperately
    */
    toolbar_wid = GetToolbarWindow();
    InvalidateRect( toolbar_wid, NULL, TRUE );
    UpdateWindow( toolbar_wid );
}

static void sendNewColourCurrentWindow( NewColourOps op )
{
    /* the edit window has various components in the form of syntax
       elements - figure which we are on & mod SETypes
    */
    int             row, col;
    syntax_element  style;
    vi_color        color;
    syntax_element  i;
    linenum         line_num;

    ScreenToClient( mod_wid, &m_pt );
    ClientToRowCol( mod_wid, m_pt.x, m_pt.y, &row, &col, DIVIDE_BETWEEN );

    /* someone is base 0, someone else isn't.  bummer.
     * also col may not be valid, check this
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
    if( style == SE_UNPARSED ) {
        // for some reason, area was not defined
        return;
    }

    color = INDEX_FROM_XY( cursx, cursy );
    if( CtrlDown() ) {
        // affect all foregrounds/backgrounds
        for( i = 0; i < SE_NUMTYPES; i++ ) {
            // doesn't make sense to change selection
            if( i == SE_SELECTION ) {
                continue;
            }
            if( op == NC_FORE ) {
                SEType[i].foreground = color;
            } else {
                SEType[i].background = color;
            }
        }
    } else {
        if( op == NC_FORE ) {
            SEType[style].foreground = color;
        } else {
            SEType[style].background = color;
        }
    }
    ReDisplayScreen();
}

static void sendNewColour( NewColourOps op )
{
    type_style  *mod_style;

    if( BAD_ID( mod_wid ) ) {
        return;
    }

    if( mod_wid == GetToolbarWindow() ) {
        sendNewColourToolbar();
    } else if( mod_wid == current_window_id ) {
        sendNewColourCurrentWindow( op );
    } else {
        mod_style = WIN_TEXT_STYLE( WINDOW_FROM_ID( mod_wid ) );
        if( op == NC_FORE ) {
            mod_style->foreground = INDEX_FROM_XY( cursx, cursy );
        } else {
            mod_style->background = INDEX_FROM_XY( cursx, cursy );
        }
        StatusWndChangeSysColors( RGBValues[statusw_info.text_style.background],
                                  RGBValues[statusw_info.text_style.foreground],
                                  GetSysColor( COLOR_BTNHIGHLIGHT ),
                                  GetSysColor( COLOR_BTNSHADOW ) );
        ReDisplayScreen();
    }
}

static void drawFocus( HDC hdc, RECT *rect, bool set )
{
    HPEN        hDkGreyPen;
    HPEN        hWhitePen;
    HPEN        hTopLeft, hBottomRight;

    hDkGreyPen = CreatePen( PS_SOLID, 1, RGB( 128, 128, 128 ) );

    hTopLeft = hWhitePen = GetStockObject( WHITE_PEN );
    hBottomRight = hDkGreyPen;
    if( set ) {
        hTopLeft = hDkGreyPen;
        hBottomRight = GetStockObject( WHITE_PEN );
    }

    MoveToEx( hdc, rect->right - 1, rect->top, NULL );
    SelectObject( hdc, hTopLeft );
    LineTo( hdc, rect->left, rect->top );
    LineTo( hdc, rect->left, rect->bottom );
    SelectObject( hdc, hBottomRight );
    LineTo( hdc, rect->right, rect->bottom );
    LineTo( hdc, rect->right, rect->top - 1 );
    SelectObject( hdc, hWhitePen );

    DeleteObject( hDkGreyPen );
}


static void drawUnselected( HDC hdc, int x, int y )
{
    RECT        rect;
    HBRUSH      hColourBrush;
    COLORREF    nearest;

    rect.left = 0;
    rect.top = 0;
    rect.right = WIDTH * NUM_ACROSS;
    rect.bottom = HEIGHT * NUM_DOWN;
    MapDialogRect( hColorbar, &rect );

    Width = (rect.right - rect.left) / NUM_ACROSS;
    Height = (rect.bottom - rect.top) / NUM_DOWN;


    rect.left = x * Width + SPC;
    rect.top = y * Height + SPC;
    rect.right = (x + 1) * Width - SPC;
    rect.bottom = (y + 1) * Height - SPC;
    // MapDialogRect( hColorbar, &rect );

    nearest = GetNearestColor( hdc, RGBValues[y * NUM_ACROSS + x] );
    hColourBrush = CreateSolidBrush( nearest );
    FillRect( hdc, &rect, hColourBrush );
    drawFocus( hdc, &rect, false );

    DeleteObject( hColourBrush );
}

static void drawSelected( HDC hdc, int x, int y )
{
    RECT    rect;

    rect.left = x * Width + SPC;
    rect.top = y * Height + SPC;
    rect.right = (x + 1) * Width - SPC;
    rect.bottom = (y + 1) * Height - SPC;

    drawFocus( hdc, &rect, true );
}

static void paintBlocks( HWND hwnd )
{
    PAINTSTRUCT ps;
    int         x, y;
    HDC         hdc;

    hdc = BeginPaint( hwnd, &ps );
    for( x = 0; x < NUM_ACROSS; x++ ) {
        for( y = 0; y < NUM_DOWN; y++ ) {
            drawUnselected( hdc, x, y );
        }
    }

    EndPaint( hwnd, &ps );
}

static void initRGBValues( void )
{
    vi_color    i;
    rgb         tcol;


    for( i = 0; i < NUM_COLOURS; i++ ) {
        GetColorSetting( i, &tcol );
        RGBValues[i] = RGB( tcol.red, tcol.green, tcol.blue );
    }
}

static LRESULT gotoNewBlock( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    HDC     hdc;

    /* unused parameters */ (void)wparam; (void)msg;

    hdc = GetDC( hwnd );
    drawUnselected( hdc, cursx, cursy );

    cursx = GET_X( lparam ) / Width;
    if( cursx > NUM_ACROSS - 1 )
        cursx = NUM_ACROSS - 1;
    cursy = GET_Y( lparam ) / Height;
    if( cursy > NUM_DOWN - 1 ) {
        cursy = NUM_DOWN - 1;
    }

    drawSelected( hdc, cursx, cursy );
    ReleaseDC( hwnd, hdc );

    CursorOp( COP_DROPCLR );
    SetCapture( hwnd );
    haveCapture = true;
    mod_wid = NO_WINDOW;
    return( 0 );
}


static int eitherButtonDown( UINT w )
{
    return( (w & MK_RBUTTON) || (w & MK_LBUTTON) );
}


static LRESULT selectedNewColour( HWND hwnd, NewColourOps op, WPARAM wparam )
{
    HDC     hdc;

    hdc = GetDC( hwnd );
    drawUnselected( hdc, cursx, cursy );
    ReleaseDC( hwnd, hdc );

    CursorOp( COP_ARROW );
    ReleaseCapture();
    haveCapture = false;
    if( !eitherButtonDown( wparam ) ) {
        sendNewColour( op );
    }
    return( 0 );
}

extern char *windowName[1];

static LRESULT processMouseMove( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    RECT    rect;

    if( !haveCapture ) {
        return( DefWindowProc( hwnd, msg, wparam, lparam ) );
    }

    // check we aren't on ourselves first
    m_pt.x = GET_X( lparam );
    m_pt.y = GET_Y( lparam );
    ClientToScreen( hwnd, &m_pt );
    GetWindowRect( GetParent( hwnd ), &rect );
    if( PtInRect( &rect, m_pt ) ) {
        CursorOp( COP_DROPCLR );
        mod_wid = NO_WINDOW;
        return( 0 );
    }

    // otherwise, figure out what we're over & change element display
    mod_wid = GetOwnedWindow( m_pt );
    if( !BAD_ID( mod_wid ) ) {
        CursorOp( COP_DROPCLR );
    } else {
        CursorOp( COP_NODROP );
    }

    return( 0 );
}

WINEXPORT LRESULT CALLBACK ClrPickProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    switch( msg ) {
    case WM_CREATE:
        initRGBValues();
        // our subclass slows things down - force a paint before we subclass
        UpdateWindow( hwnd );
        // SubclassGenericAdd( hwnd, HotkeyProc, InstanceHandle );
        return( 0 );
    case WM_PAINT:
        paintBlocks( hwnd );
        return( 0 );
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        return( gotoNewBlock( hwnd, msg, wparam, lparam ) );
    case WM_LBUTTONUP:
        return( selectedNewColour( hwnd, NC_FORE, wparam ) );
    case WM_RBUTTONUP:
        return( selectedNewColour( hwnd, NC_BACK, wparam ) );
    case WM_MOUSEMOVE:
        return( processMouseMove( hwnd, msg, wparam, lparam ) );
    case WM_DESTROY:
        // SubclassGenericRemove( hwnd );
        return( 0 );
    }
    return( DefWindowProc( hwnd, msg, wparam, lparam ) );
}

void InitClrPick( void )
{
    WNDCLASS    wndclass;

    if( GetClassInfo( InstanceHandle, "ClrPick", &wndclass ) ) {
        return;
    }

    wndclass.style          = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = GetWndProc( ClrPickProc );
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = InstanceHandle;
    wndclass.hIcon          = (HICON)NULLHANDLE;
    wndclass.hCursor        = LoadCursor( (HINSTANCE)NULLHANDLE, IDC_ARROW );
    wndclass.hbrBackground  = (HBRUSH)COLOR_APPWORKSPACE;
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = "ClrPick";

    RegisterClass( &wndclass );
}

void FiniClrPick( void )
{
    UnregisterClass( "ClrPick", InstanceHandle );
}

