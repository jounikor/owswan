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
* Description:  Main debugger window.
*
****************************************************************************/


#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgwind.h"
#include "dbgerr.h"
#include "dbgmem.h"
#include "strutil.h"
#include "dbgscan.h"
#include "dbgutil.h"
#include "dbgmain.h"
#include "dbghook.h"
#include "dbgshow.h"
#include "dbgparse.h"
#include "dbgwdlg.h"
#include "dbgwdisp.h"
#include "dbgwtool.h"
#include "dbgwpain.h"


#define DISP_OPTS() \
    pick( "Open",     DISP_OPEN     ) \
    pick( "Close",    DISP_CLOSE    ) \
    pick( "New",      DISP_NEW      ) \
    pick( "MInimize", DISP_MINIMIZE ) \
    pick( "MAximize", DISP_MAXIMIZE ) \
    pick( "Restore",  DISP_RESTORE  ) \
    pick( "FLoating", DISP_FLOATING ) \
    pick( "Fixed",    DISP_FIXED    )

#define MISC_OPTS() \
    pick( "TOolbar",  MISC_TOOL     ) \
    pick( "Status",   MISC_STATUS   )

typedef enum {
    #define pick(t,e)   e,
        DISP_OPTS()
    #undef pick
} disp_optn;

typedef enum {
    #define pick(t,e)   e,
        MISC_OPTS()
    #undef pick
} misc_optn;

wnd_posn            WndPosition[NUM_WNDCLS_ALL];
gui_rect            WndMainRect;

static char         *WndFontInfo[NUM_WNDCLS_ALL];

static const char   DispOptions[] = {
    #define pick(t,e)   t "\0"
        DISP_OPTS()
    #undef pick
};

static const char   MiscTab[] = {
    #define pick(t,e)   t "\0"
        MISC_OPTS()
    #undef pick
};

bool ScanStatus( void )
{
    return( ScanCmd( MiscTab ) == MISC_STATUS );
}

static int      range( int x, int min_x, int max_x, int default_x )
{
    if( x == -1 )
        x = default_x;
    if( x < min_x )
        x = min_x;
    if( x > max_x )
        x = max_x;
    return( x );
}

static disp_optn GetOption( void )
{
    int         cmd;
    disp_optn   optn = DISP_OPEN;

    if( CurrToken == T_DIV ) {
        Scan();
        cmd = ScanCmd( DispOptions );
        if( cmd < 0 ) {
            Error( ERR_LOC, LIT_ENG( ERR_BAD_OPTION ), GetCmdName( CMD_DISPLAY ) );
        } else {
            optn = (disp_optn)cmd;
        }
    }
    return( optn );
}


char    *GetWndFont( a_window wnd )
{
    if( WndHasClass( wnd ) ) {
        if( WndFontInfo[WndClass( wnd )] != NULL ) {
            return( WndFontInfo[WndClass( wnd )] );
        }
    }
    return( WndFontInfo[WND_ALL] );
}


static void SetFont( wnd_class_wv wndclass, char *font )
{
    GUIMemFree( WndFontInfo[wndclass] );
    WndFontInfo[wndclass] = font;
}


void    WndFontHook( a_window wnd )
{
    char        *font;

    if( !WndHasClass( wnd ) )
        return;
    font = WndGetFontInfo( wnd );
    if( font != NULL ) {
        SetFont( WndClass( wnd ), font );
    }
}


gui_coord *WndMainClientSize( void )
{
    return( _IsOn( SW_DETACHABLE_WINDOWS ) ? &WndScale : &WndMax );
}

void WndMainResized( void )
{
    a_window    wnd;
    gui_rect    rect;

    if( _IsOff( SW_DETACHABLE_WINDOWS ) ) {
        WndGetRect( WndMain, &WndMainRect );
        for( wnd = WndNext( NULL ); wnd != NULL; wnd = WndNext( wnd ) ) {
            if( !WndHasClass( wnd ) )
                continue;
            WndPosToRect( &WndPosition[WndClass( wnd )], &rect, WndMainClientSize() );
            WndResizeWindow( wnd, &rect );
        }
    }
}


void    WndResizeHook( a_window wnd )
{
    gui_rect    rect;

    if( WndIsMaximized( wnd ) || WndIsMinimized( wnd ) )
        return;
    if( !WndHasClass( wnd ) )
        return;
    WndGetRect( wnd, &rect );
    WndRectToPos( &rect, &WndPosition[WndClass( wnd )], WndMainClientSize() );
}


void InitFont( void )
{
    WndFontInfo[WND_ALL] = WndGetFontInfo( WndMain );
}


void FiniFont( void )
{
    wnd_class_wv    wndclass;

    for( wndclass = 0; wndclass < NUM_WNDCLS_ALL; ++wndclass ) {
        SetFont( wndclass, NULL );
    }
}

static char *GUIDupStrLen( const char *str, size_t len )
{
    char        *dup;

    dup = GUIMemAlloc( len + 1 );
    if( dup != NULL ) {
        memcpy( dup, str, len );
        dup[len] = NULLCHAR;
    }
    return( dup );
}


void ProcFont( void )
{
    wnd_class_wv    wndclass;
    const char      *start;
    size_t          len;
    wnd_class_wv    wndclass1;

    wndclass = ReqWndName();
    if( !ScanItem( false, &start, &len ) )
        return;
    ReqEOC();
    if( wndclass == WND_ALL ) {
        for( wndclass1 = 0; wndclass1 < NUM_WNDCLS_ALL; ++wndclass1 ) {
            SetFont( wndclass1, NULL );
        }
    }
    SetFont( wndclass, GUIDupStrLen( start, len ) );
    _SwitchOn( SW_PENDING_REPAINT );
}



static void PrintFont( wnd_class_wv wndclass, char *def )
{
    char        wndname[20];
    char        *font;

    font = WndFontInfo[wndclass];
    if( font == NULL )
        return;
    if( def == NULL || strcmp( font, def ) != 0 ) {
        GetCmdEntry( WndNameTab, wndclass, wndname );
        Format( TxtBuff, "%s %s {%s}", GetCmdName( CMD_FONT ), wndname, font );
        WndDlgTxt( TxtBuff );
    }
}


void ConfigFont( void )
{
    wnd_class_wv    wndclass;

    PrintFont( WND_ALL, NULL );
    for( wndclass = 0; wndclass < NUM_WNDCLS_ALL; ++wndclass ) {
        if( wndclass == WND_ALL )
            continue;
        PrintFont( wndclass, WndFontInfo[WND_ALL] );
    }
}


void FontChange( void )
{
    wnd_class_wv    wndclass;
    char            *text;

    text = GUIGetFontFromUser( WndFontInfo[WND_ALL] );
    if( text == NULL )
        return;
    for( wndclass = 0; wndclass < NUM_WNDCLS_ALL; ++wndclass ) {
        SetFont( wndclass, NULL );
    }
    SetFont( WND_ALL, text );
    _SwitchOn( SW_PENDING_REPAINT );
}


static void ProcSize( wnd_class_wv wndclass )
{
    gui_rect    size;
    disp_optn   optn;
    a_window    wnd;
    a_window    next;
    gui_coord   min;
    bool        coord_specified;
    gui_rect    def_rect;

    optn = GetOption();
    if( optn == DISP_FLOATING || optn == DISP_FIXED ) {
        Error( ERR_LOC, LIT_ENG( ERR_BAD_OPTION ), GetCmdName( CMD_DISPLAY ) );
    }
    size.x = OptExpr( -1 );
    size.y = -1;
    size.width = -1;
    size.height = -1;
    if( CurrToken == T_COMMA ) {
        Scan();
        size.y = OptExpr( -1 );
        if( CurrToken == T_COMMA ) {
            Scan();
            size.width = OptExpr( -1 );
            if( CurrToken == T_COMMA ) {
                Scan();
                size.height = OptExpr( -1 );
            }
        }
    }
    ReqEOC();

    GUIGetMinSize( &min );
    if( size.x != -1 || size.y != -1 || size.width != -1 || size.height != -1 ) {
        coord_specified = true;
    } else {
        coord_specified = false;
    }
    WndPosToRect( &WndPosition[wndclass], &def_rect, &WndScreen );
    size.x = range( size.x, 0, WndScreen.x, def_rect.x );
    size.y = range( size.y, 0, WndScreen.y, def_rect.y );
    if( size.x + size.width > WndScreen.x )
        size.width = WndScreen.x - size.x;
    if( size.y + size.height > WndScreen.y )
        size.height = WndScreen.y - size.y;
    size.width = range( size.width, min.x, WndScreen.x, def_rect.width );
    size.height = range( size.height, min.y, WndScreen.y, def_rect.height );
    if( coord_specified ) {
        WndRectToPos( &size, &WndPosition[wndclass], &WndScreen );
    }
    if( wndclass == WND_CURRENT ) {
        wnd = WndFindActive();
    } else {
        wnd = WndFindClass( NULL, wndclass );
    }
    switch( optn ) {
    case DISP_OPEN:
    case DISP_NEW:
        if( optn == DISP_NEW || wnd == NULL ) {
            WndOpenTab[wndclass]();
        } else {
            WndRestoreToFront( wnd );
            WndPosToRect( &WndPosition[wndclass], &size, WndMainClientSize() );
            WndResizeWindow( wnd, &size );
        }
        break;
    case DISP_CLOSE:
        if( wndclass == WND_ALL ) {
            for( wnd = WndNext( NULL ); wnd != NULL; wnd = next ) {
                next = WndNext( wnd );
                if( WndHasClass( wnd ) ) {
                    WndClose( wnd );
                }
            }
        } else {
            if( wnd != NULL ) {
                WndClose( wnd );
            }
        }
        break;
    case DISP_MINIMIZE:
        if( wnd == NULL )
            wnd = WndOpenTab[wndclass]();
        WndMinimizeWindow( wnd );
        break;
    case DISP_MAXIMIZE:
        if( wnd == NULL )
            wnd = WndOpenTab[wndclass]();
        WndMaximizeWindow( wnd );
        break;
    case DISP_RESTORE:
        if( wnd == NULL )
            wnd = WndOpenTab[wndclass]();
        WndRestoreWindow( wnd );
        break;
    }
}


static void ProcTool( void )
{
    int         height;
    disp_optn   optn, type, tmp;

    type = DISP_FIXED;
    optn = DISP_OPEN;
    while( CurrToken == T_DIV ) {
        tmp = GetOption();
        if( tmp == DISP_FLOATING || tmp == DISP_FIXED ) {
            type = tmp;
        } else {
            optn = tmp;
        }
    }
    height = OptExpr( 0 );
    if( optn == DISP_CLOSE ) {
        WndToolClose();
    } else {
        WndToolOpen( height, type == DISP_FIXED );
    }
}


static void ProcStatus( void )
{
    disp_optn   optn;

    optn = GetOption();
    if( optn == DISP_OPEN ) {
        WndCreateStatusWindow( &WndStatusColour );
    } else if( optn == DISP_CLOSE ) {
        WndCloseStatusWindow();
    } else {
        Error( ERR_LOC, LIT_ENG( ERR_BAD_OPTION ), GetCmdName( CMD_DISPLAY ) );
    }
}


static bool DoneRefresh( inp_data_handle dummy, inp_rtn_action action )
{
    /* unused parameters */ (void)dummy;

    switch( action ) {
    case INP_RTN_INIT:
        return( true );
    case INP_RTN_EOL:
        WndFreshAll();
        WndDirty( NULL );
        return( false );
    case INP_RTN_FINI:
        return( true );
    default:
        return( false );
    }
}


static void PushRefresh( void )
{
    PushInpStack( NULL, DoneRefresh, false );
    TypeInpStack( INP_NO_CMD );
}

/*
 * ProcDisplay -- process a display command
 */

void ProcDisplay( void )
{
    wnd_class_wv    wndclass;
    mad_radix       old_radix;

    if( ScanEOC() ) {
        while( HookPendingPush() );
        PushRefresh();
        return;
    }
    old_radix = NewCurrRadix( 10 );
    switch( ScanCmd( MiscTab ) ) {
    case MISC_TOOL:
        ProcTool();
        break;
    case MISC_STATUS:
        ProcStatus();
        break;
    default:
        wndclass = ReqWndName();
        ProcSize( wndclass );
    }
    NewCurrRadix( old_radix );
}


static  void    PrintPosition( disp_optn optn, wnd_class_wv wndclass,
                               gui_rect *rect, char *buff, char *buff2 )
{
    GetCmdEntry( WndNameTab, wndclass, buff );
    GetCmdEntry( DispOptions, optn, buff2 );
    Format( TxtBuff, "%s %s /%s %d,%d,%d,%d", GetCmdName( CMD_DISPLAY ), buff, buff2,
                     rect->x, rect->y, rect->width, rect->height );
    WndDlgTxt( TxtBuff );
}


void ConfigDisp( void )
{

    a_window        wnd;
    a_window        scan;
    char            buff[20];
    char            buff2[20];
    a_window        head;
    a_window        next;
    int             h;
    wnd_class_wv    wndclass;
    gui_rect        rect;

    ReqEOC();
    GetCmdEntry( WndNameTab, WND_ALL, buff );
    GetCmdEntry( DispOptions, DISP_CLOSE, buff2 );
    Format( TxtBuff, "%s %s /%s", GetCmdName( CMD_DISPLAY ), buff, buff2 );
    WndDlgTxt( TxtBuff );
    if( WndHaveStatusWindow() ) {
        GetCmdEntry( MiscTab, MISC_STATUS, buff );
        Format( TxtBuff, "%s %s", GetCmdName( CMD_DISPLAY ), buff );
        WndDlgTxt( TxtBuff );
    }
    if( WndHaveToolBar() ) {
        h = WndToolHeight();
        GetCmdEntry( MiscTab, MISC_TOOL, buff );
        GetCmdEntry( DispOptions, WndToolFixed() ? DISP_FIXED : DISP_FLOATING, buff2 );
        Format( TxtBuff, "%s %s /%s %d", GetCmdName( CMD_DISPLAY ),
                buff, buff2, h );
        WndDlgTxt( TxtBuff );
    }
    for( wndclass = 0; wndclass < NUM_WNDCLS_ALL; ++wndclass ) {
        if( wndclass == WND_ALL )
            continue;
        if( WndFindClass( NULL, wndclass ) != NULL )
            continue;
        WndPosToRect( &WndPosition[wndclass], &rect, &WndScreen );
        if( rect.width == 0 )
            continue;
        if( rect.height == 0 )
            continue;
        PrintPosition( DISP_CLOSE, wndclass, &rect, buff, buff2 );
    }
    head = WndNext( NULL );
    if( head == NULL )
        return;
    // traverse in reverse order so that windows get created in correct order
    for( wnd = head; (next = WndNext( wnd )) != NULL; wnd = next )
        ;
    for( ;; ) {
        if( WndHasClass( wnd ) ) {
            wndclass = WndClass( wnd );
            switch( wndclass ) {
            case WND_ALL:
                break;
            case WND_BINARY:
            case WND_FILE:
            case WND_MEMORY:
                WndResizeHook( wnd );
                WndPosToRect( &WndPosition[wndclass], &rect, &WndScreen );
                PrintPosition( DISP_CLOSE, wndclass, &rect, buff, buff2 );
                break;
            case WND_VARIABLE:
            case WND_TMPFILE:
                break;
            default:
                WndResizeHook( wnd );
                WndPosToRect( &WndPosition[wndclass], &rect, &WndScreen );
                PrintPosition( DISP_OPEN, wndclass, &rect, buff, buff2 );
                break;
            }
        }
        if( wnd == head )
            break;
        for( scan = head; (next = WndNext( scan )) != wnd; scan = next )
            ;
        wnd = scan;
    }
}

wnd_class_wv ReqWndName( void )
{
    int     cmd;

    cmd = ScanCmd( WndNameTab );
    if( cmd < 0 )
        Error( ERR_LOC, LIT_DUI( ERR_BAD_WIND_NAME ) );
    return( (wnd_class_wv)cmd );
}
