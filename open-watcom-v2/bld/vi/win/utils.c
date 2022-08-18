/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2020 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Miscellaneous utility routines.
*
****************************************************************************/


#include "vi.h"
#include <assert.h>
#ifdef __NT__
    #include <commctrl.h>
#endif
#include <dos.h>
#include "color.h"
#include "vifont.h"
#include "utils.h"
#include "banner.h"
#include "aboutdlg.h"
#include "win.h"
#include "viabout.h"

#include "clibext.h"


char *windowName[] = {
    "Buffer Window",
    "MessageWindow",
    "RepeatWindow",
    "FileCompleteWindow",
    "CommandWindow",
    "StatusWnd",
    "WTool",
#ifdef __NT__
    STATUSCLASSNAME,
#endif
#if 0
    EditorName,         // nothing to change
    "Edit Container"    // should use standard Windows colour for this
#endif
};

static char windowBordersG[] =  {
#if defined( __UNIX__ )
    #define vi_pick( enum, UnixNG, UnixG, DosNG, DosG ) UnixG,
#else
    #define vi_pick( enum, UnixNG, UnixG, DosNG, DosG ) DosG,
#endif
    #include "borders.h"
    #undef vi_pick
    '\0'
};
#define GADGET_SIZE (sizeof( windowBordersG ) - 1)

int FileSysNeedsCR( int handle )
{
    handle = handle;
    return( true );
}

void SetGadgetString( char *str )
{
    size_t  len;

    if( str != NULL && *str != '\0' ) {
        len = strlen( str );
        if( len > GADGET_SIZE ) {
            len = GADGET_SIZE;
        }
        if( EditVars.GadgetString == NULL ) {
            EditVars.GadgetString = MemAlloc( GADGET_SIZE + 1 );
            EditVars.GadgetString[GADGET_SIZE] = '\0';
        }
        memset( EditVars.GadgetString, ' ', GADGET_SIZE );
        memcpy( EditVars.GadgetString, str, len );
    } else {
        ReplaceString( &EditVars.GadgetString, windowBordersG );
    }
}

bool IsGadgetStringChanged( const char *str )
{
    return( strcmp( str, windowBordersG ) != 0 );
}

/*
 * WriteText - write a length specified string to a window
 */
void WriteText( window_id wid, int x, int y, type_style *style, const char * text, int len )
{
    HDC     hdc;
#ifdef __WINDOWS_386__
    short   tab;
#else
    int     tab;
#endif

    if( len > 0 ) {
        hdc = TextGetDC( wid, style );
        tab = FontTabWidth( style->font );
        TabbedTextOut( hdc, x, y, text, len, 1, &tab, 0 );
        TextReleaseDC( wid, hdc );
    }

} /* WriteText */

/*
 * WriteString - write a null delimited string to a window
 */
void WriteString( window_id wid, int x, int y, type_style *style, const char *text )
{
    WriteText( wid, x, y, style, text, strlen( text ) );

} /* WriteString */

/*
 * TextGetDC - get the DC for a window, and set its properties
 */
HDC TextGetDC( window_id wid, type_style *style )
{
    HDC     hdc;

    hdc = GetDC( wid );
    SaveDC( hdc );
    SelectObject( hdc, FontHandle( style->font ) );
    // SelectObject( hdc, ColorPen( style->foreground ) );
    // SelectObject( hdc, ColorBrush( style->background ) );
    SetTextColor( hdc, ColorRGB( style->foreground ) );
    SetBkColor( hdc, ColorRGB( style->background ) );
    return( hdc );

} /* GetTextDC */

/*
 * TextReleaseDC - release the dc for a window
 */
void TextReleaseDC( window_id wid, HDC hdc )
{
    RestoreDC( hdc, -1 );
    ReleaseDC( wid, hdc );

} /* TextReleaseDC */

/*
 * BlankRectIndirect - blank out a rectangle given a pointer to the rectangle
 */
void BlankRectIndirect( window_id wid, vi_color color, RECT *rect )
{
    HDC     hdc;

    hdc = GetDC( wid );
    FillRect( hdc, rect, ColorBrush( color ) );
    ReleaseDC( wid, hdc );

} /* BlankRectIndirect */

/*
 * BlankRect - blank out a rectangle given its coordinates
 */
void BlankRect( window_id wid, vi_color color, int x1, int x2, int y1, int y2 )
{
    RECT    rect;

    rect.left = x1;
    rect.right = x2;
    rect.top = y1;
    rect.bottom = y2;
    BlankRectIndirect( wid, color, &rect );

} /* BlankRect */

/*
 * MyTextExtent - get the text extend of a string in a specified style
 */
int MyTextExtent( window_id wid, type_style *style, char *text, unsigned length )
{
    HDC         hdc;
    int         extent;
    unsigned    text_len, extra;
    int         font_width;
#ifdef __WINDOWS_386__
    short       tab;
#else
    int         tab;
#endif

    hdc = TextGetDC( wid, style );
    font_width = FontAverageWidth( style->font );
    tab = FontTabWidth( style->font );
    text_len = strlen( text );
    extra = 0;
    if( length > text_len ) {
        extra = length - text_len - 1;
        length = text_len;
    }
    extent = LOWORD( GetTabbedTextExtent( hdc, text, length, 1, &tab ) );
    extent += extra * font_width;
    TextReleaseDC( wid, hdc );
    return( extent );

} /* MyTextExtent */

int MyStringExtent( window_id wid, type_style *style, char *text )
{
    return( MyTextExtent( wid, style, text, strlen( text ) ) );
}

/*
 * ClientToRowCol - Given an (x,y) in client coords inside an *EDIT* window,
 *                  fill in the row and col with the correct values (base 1).
 */
void ClientToRowCol( window_id wid, int x, int y, int *row, int *col, int divide )
{
    window      *w;
    dc_line     *dcline;
    ss_block    *ss, *ss_start, *ss_prev;
    int         startCols, intoCols;
    int         startPixel, lenBlock;
    int         intoExtent, difExtent;
    int         toleftExtent;
    int         avg_width;
    char        *str;

    w = WINDOW_FROM_ID( wid );
    *row = y / FontHeight( WIN_TEXT_FONT( w ) ) + 1;

    if( x < 0 ) {
        *col = 1;
        return;
    }

    if( *row < 1 || *row > CurrentInfo->dc_size ) {
        *col = 0;
        return;
    }

    // get line data
    dcline = DCFindLine( *row - 1, wid );
    if( dcline->display != 0 ) {
        // line needs to be displayed
        // therefore ss information has not been set!
        // therefore we cant use it to calculate anything of value!
        // best we can do is a good solid guess.
        avg_width = FontAverageWidth( WIN_TEXT_FONT( w ) );
        *col = x / avg_width + 1;
        return;
    }
    assert( dcline->valid );
    if( dcline->start_col < LeftTopPos.column ) {
        // entire line has been scrolled off to left - go to end of that line
        *col = 10000;
        return;
    }
    assert( dcline->start_col == LeftTopPos.column );

    // find which block x lies on
    ss = ss_start = dcline->ss;
    while( ss->offset < x ) {
        // this could be the problem ?
        // will ss->offset always be valid
        // if not, this goes right off the end of the valid blocks
        ss++;
    }

    // Not needed anymore now we have slayed the dragon

#if 0
    if( (ss->type < 0) ||
        (ss->type >= SE_NUMTYPES) ||
        (ss->end > BEYOND_TEXT) ||
        (ss->len > BEYOND_TEXT) ) {
        assert( 0 );
    }
#endif

    // grab info about this block
    if( ss != ss_start ) {
        ss_prev = ss - 1;
        startPixel = ss_prev->offset;
        startCols = ss_prev->end + 1;
        lenBlock = ss->end - ss_prev->end;
        str = dcline->text + startCols;
    } else {
        startPixel = 0;
        startCols = 0;
        str = dcline->text;
        lenBlock = ss->end + 1;
    }
    // lenBlock must be less than the length of the text
    // and greater than zero
    if( ss->end >= BEYOND_TEXT ) {
        lenBlock = strlen( str );
    }
    if( lenBlock < 0 )
        lenBlock = 0;

    // avg_width must be greater than 0 (this probablly isn't needed but ...)
    avg_width = FontAverageWidth( SEType[ss->type].font );
    if( avg_width < 1 )
        avg_width = 1;

    if( EditFlags.RealTabs ) {
        char    *start_str, *end_str;
        int     cur_pixel;
        linenum line_num = (linenum)(LeftTopPos.line + *row - 1);
        line    *line;
        fcb     *fcb;
        int     v_pos;
        vi_rc   rc;

        rc = CGimmeLinePtr( line_num, &fcb, &line );
        if( rc == ERR_NO_ERR ) {
            char *text = line->data;
            // get the tab boundries in this block
#if 0
            v_pos = WinVirtualCursorPosition( text, startCols );
            if( v_pos < LeftTopPos.column ) {
                // block begins off left edge, advance forward to LeftTopPos.column
                int rp = WinRealCursorPosition( text, LeftTopPos.column );
                start_str = text + rp;
                v_pos = LeftTopPos.column;
                startPixel = 0;
                lenBlock -= (rp - startCols);
            }
#else
            if( LeftTopPos.column > 1 ) {
                 // this only works for fixed fonts but its better than
                 // being wrong in every case; It's also correct
                 // immediately after every tab stop. And it should be
                 // at least close everywhere else :(
                 *col = x / avg_width + 1;
                 return;
            }
#endif
            start_str = text + startCols;
            for( end_str = start_str; end_str != start_str + lenBlock; end_str++ ) {
                if( *end_str == '\t' ) {
                    cur_pixel = (WinVirtualCursorPosition( text, end_str + 1 - text ) - LeftTopPos.column ) * avg_width;
                    if( cur_pixel > x ) {
                        // we've found the new boundries for the block.
                        // that do not contain any tabs!
                        break;
                    }
                    start_str = end_str + 1;
                    startPixel = cur_pixel;
                }
            }
            // startCols are virtual# columns before the block
            v_pos = WinVirtualCursorPosition( text, start_str - text + 1 ) - 1;
            if( start_str == text + startCols ) {
                if( startCols != v_pos ) {
                    startCols = v_pos - LeftTopPos.column;
                }
            } else {
                startCols = v_pos - LeftTopPos.column;
            }
            lenBlock = end_str - start_str;
            str = start_str;
        } /* else use the previous values */
    }

    // guess how far we are into block
    x -= startPixel;
    intoCols = x / avg_width;
    if( intoCols > lenBlock ) {
        intoCols = lenBlock;
    }

    // refine guess
    toleftExtent = 0;
    difExtent = 0;
    intoExtent = MyTextExtent( wid, &SEType[ss->type], str, intoCols );
    if( intoExtent > x ) {
        while( intoExtent > x ) {
            intoCols--;
            difExtent = intoExtent - MyTextExtent( wid, &SEType[ss->type], str, intoCols );
            intoExtent -= difExtent;
        }
        intoCols++;
        toleftExtent = intoExtent;
    } else {
        while( intoExtent <= x ) {
            intoCols++;
            toleftExtent = intoExtent;
            difExtent = MyTextExtent( wid, &SEType[ss->type], str, intoCols ) - intoExtent;
            intoExtent += difExtent;
        }
    }

    // fine-tune if have | cursor
    if( divide == DIVIDE_MIDDLE ) {
        if( (x - toleftExtent) > (difExtent / 2) ) {
            intoCols++;
        }
    }
    *col = startCols + intoCols;

} /* ClientToRowCol */

/*
 * ToggleHourglass - turn the hourglass cursor on/off
 */
void ToggleHourglass( bool on )
{
    static int          isOn;
    static HCURSOR      lastCursor;
    static HCURSOR      waitCursor;

    if( !on ) {
        isOn--;
        if( isOn == 0 ) {
            SetCursor( lastCursor );
        }
    } else {
        if( waitCursor == NULL ) {
            waitCursor = LoadCursor( (HANDLE)NULLHANDLE, IDC_WAIT );
        }
        lastCursor = SetCursor( waitCursor );
        isOn++;
    }

} /* ToggleHourglass */

long MemSize( void )
{
#ifdef __WINDOWS__
    return( GlobalCompact( 0 ) );
#else
    return( 0 );
#endif
}

void MyDelay( int ms )
{
#if !defined( __WATCOMC__ ) && defined( _WIN64 )
    Sleep( ms );
#else
    delay( ms );
#endif
}

void MyBeep( void )
{
    if( EditFlags.BeepFlag ) {
        MessageBeep( (UINT)-1 );
    }
}

/*
 * DoAboutBox - do an about box
 */
vi_rc DoAboutBox( void )
{
    about_info  ai;

    ai.owner = root_window_id;
    ai.inst = InstanceHandle;
    ai.name = WATCOM_ABOUT_EDITOR STR_BITNESS;
    ai.version = banner1v( _VI_VERSION_ );
    ai.title = "About Open Watcom Text Editor";
    DoAbout( &ai );
    return( ERR_NO_ERR );

} /* DoAboutBox */

void CursorOp( CursorOps op )
{
    static int          lastop = COP_FINI;
    static HCURSOR      noDrop, dropFt, dropClr, dropSS, statMove;

//    if( op == lastop ) {
//        return;
//    }

//    ShowCursor( FALSE );
    switch( op ) {
    case COP_INIT:
        noDrop = LoadCursor( InstanceHandle, "NODROP" );
        dropFt = LoadCursor( InstanceHandle, "DROPFT" );
        dropClr = LoadCursor( InstanceHandle, "DROPCLR" );
        dropSS = LoadCursor( InstanceHandle, "DROPSS" );
        statMove = LoadCursor( InstanceHandle, "STATMOVE" );
        break;
    case COP_FINI:
        DestroyCursor( noDrop );
        DestroyCursor( dropClr );
        DestroyCursor( dropFt );
        DestroyCursor( dropSS );
        DestroyCursor( statMove );
        break;
    case COP_ARROW:
        SetCursor( LoadCursor( (HINSTANCE)NULLHANDLE, IDC_ARROW ) );
        break;
    case COP_DROPFT:
        SetCursor( dropFt );
        break;
    case COP_DROPSS:
        SetCursor( dropSS );
        break;
    case COP_DROPCLR:
        SetCursor( dropClr );
        break;
    case COP_NODROP:
        SetCursor( noDrop );
        break;
    case COP_STATMOVE:
        SetCursor( statMove );
        break;
    }
//    ShowCursor( TRUE );

    lastop = op;
}

HWND GetOwnedWindow( POINT pt )
{
    char        textBuffer[80];
    int         i, nTypes;
    HWND        hwndElement, hwndChild;
    POINT       ptSave;

    /* point expected to be in screen coordinates
    */
    ptSave = pt;
    hwndElement = WindowFromPoint( pt );
    ScreenToClient( hwndElement, &pt );
    hwndChild = ChildWindowFromPoint( hwndElement, pt );
    if( !BAD_ID( hwndChild ) ) {
        /* must go 2 generations down
           (BufferWindows children of ContainerWindow, child of EditorName)
        */
        ClientToScreen( hwndElement, &pt );
        hwndElement = hwndChild;
        ScreenToClient( hwndElement, &pt );
        hwndChild = ChildWindowFromPoint( hwndElement, pt );
        if( !BAD_ID( hwndChild ) ) {
            hwndElement = hwndChild;
        }
    }

    i = GetClassName( hwndElement, textBuffer, sizeof( textBuffer ) );
    textBuffer[i] = '\0';
    nTypes = GetNumWindowTypes();
    for( i = 0; i < nTypes; i++ ) {
        if( strcmp( textBuffer, windowName[i] ) == 0 ) {
            /* a recognized window - return handle to it
            */
            if( GET_HINSTANCE( hwndElement ) == GET_HINSTANCE( root_window_id ) ) {
                return( hwndElement );
            }
            return( NO_WINDOW );
        }
    }
    return( NO_WINDOW );
}

int GetNumWindowTypes( void )
{
    return( sizeof( windowName ) / sizeof( windowName[0] ) );
}

void MoveWindowTopRight( HWND hwnd )
{
    /* move window to top-right corner of container
       (a tool-bar-like position)
    */
    RECT        rcClient;
    RECT        rcUs;
    RECT        rcTB;
    POINT       pt;
    int         clientWidth;
    int         usWidth;
    int         usHeight;
    int         xshift;
    int         xshiftmax;
    window_id   toolbar_wid;

    if( !BAD_ID( current_window_id ) ) {
        GetClientRect( current_window_id, &rcClient );
        GetWindowRect( hwnd, &rcUs );

        clientWidth = rcClient.right - rcClient.left;
        usWidth = rcUs.right - rcUs.left;
        usHeight = rcUs.bottom - rcUs.top;

        pt.x = rcClient.left;
        pt.y = rcClient.top;
        ClientToScreen( current_window_id, &pt );
        xshift = FontAverageWidth( 1 ) * 80;
        xshiftmax = clientWidth - usWidth - 35;
        if( xshift > xshiftmax ) {
            xshift = xshiftmax;
        }
        pt.x += xshift;
        pt.y += 30;
        toolbar_wid = GetToolbarWindow();
        if( toolbar_wid != NULL ) {
            GetWindowRect( toolbar_wid, &rcTB );
            pt.y += rcTB.bottom - rcTB.top;
        }
        ScreenToClient( GetParent( hwnd ), &pt );
        MoveWindow( hwnd, pt.x, pt.y, usWidth, usHeight, TRUE );
    }
}

/*
 * SetEditInt - set an integer in an edit window
 */
void SetEditInt( HWND hwnd, UINT id, int value )
{
    char        buff[16];

    itoa( value, buff, 10 );
    SetDlgItemText( hwnd, id, buff );

} /* SetEditInt */

/*
 * UpdateBoolSetting - update an boolean setting
 */
void UpdateBoolSetting( HWND hwnd, int token, int id, bool oldval )
{
    char        *ptr;
    char        result[MAX_STR];
    bool        val;

    val = IsDlgButtonChecked( hwnd, id );
    if( val != oldval ) {
        ptr = result;
        if( !val ) {
            *ptr++ = 'n';
            *ptr++ = 'o';
        }
        strcpy( ptr, GetTokenString( SetFlagTokens, token ) );
        Set( result );
    }

} /* UpdateBoolSetting */

/*
 * DoStrSet - do a set for a specified string
 */
void DoStrSet( char *value, int token )
{
    char        result[MAX_STR];
    const char  *str;

    str = GetTokenString( SetVarTokens, token );
    strcpy( result, str );
    strcat( result, " " );
    strcat( result, value );
    Set( result );

} /* DoStrSet */

/*
 * UpdateStrSetting - update a string setting
 */
void UpdateStrSetting( HWND hwnd, int token, int id, char *oldval )
{
    char        value[MAX_STR];

    GetDlgItemText( hwnd, id, value, sizeof( value ) );
    if( strcmp( oldval, value ) != 0 ) {
        DoStrSet( value, token );
    }

} /* UpdateStrSetting */

/*
 * UpdateIntSetting - update a string setting
 */
void UpdateIntSetting( HWND hwnd, int token, int id, long oldval )
{
    char        value[MAX_STR];
    long        lval;

    GetDlgItemText( hwnd, id, value, sizeof( value ) );
    lval = atol( value );
    if( lval != oldval ) {
        DoStrSet( value, token );
    }

} /* UpdateIntSetting */

/*
 * CenterWindowInRoot - nicely center a child of Root
 */
void CenterWindowInRoot( HWND hwnd )
{
    RECT    rR, rH;
    int     x, y, w, h, d;

    GetWindowRect( root_window_id, &rR );
    GetWindowRect( hwnd, &rH );

    // center in root
    w = rH.right - rH.left;
    h = rH.bottom - rH.top;
    x = ((rR.right - rR.left + 1) - (w + 1)) / 2 + rR.left;
    y = ((rR.bottom - rR.top + 1) - (h + 1)) / 2 + rR.top;

    // try to keep on-screen
    if( x < 0 ) {
        x = 0;
    }
    if( y < 0 ) {
        y = 0;
    }
    d = GetSystemMetrics( SM_CXSCREEN ) - (x + w);
    if( d < 0 ) {
        x += d;
    }
    d = GetSystemMetrics( SM_CYSCREEN ) - (y + h);
    if( d < 0 ) {
        y += d;
    }
    SetWindowPos( hwnd, (HWND)NULLHANDLE, x, y, 0, 0,
                  SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOZORDER );
}

void DrawRectangleUpDown( HWND hwnd, int which )
{
    LONG    l;
    l = GET_WNDSTYLE( hwnd );
    l &= ~SS_WHITEFRAME;
    l |= SS_BLACKFRAME;
    if( which == DRAW_UP ) {
        l &= ~SS_BLACKFRAME;
        l |= SS_WHITEFRAME;
    }
    SET_WNDSTYLE( hwnd, l );
    InvalidateRect( hwnd, NULL, TRUE );
    UpdateWindow( hwnd );
}

#if 0
// sanity check on list of ss blocks

static void dumpSSBlocks( ss_block *ss_start, dc_line *dcline )
{
    ss_block    *ss = ss_start;
    FILE *f = fopen( "C:\\vi.out", "a+t" );

    fprintf( f, "Bad SSBlock:: dumping current DC line\n" );
    fprintf( f, "%s %d %d %d %d\n", dcline->text, ss->type, ss->end, ss->len, ss->offset );

    for( ss++; ss->end <= BEYOND_TEXT; ss++ ) {
        fprintf( f, "%d %d %d %d\n", ss->type, ss->end, ss->len, ss->offset );
        if( (ss->end == BEYOND_TEXT) || (ss->offset == 10000) ) {
            break;
        }
    }
    fprintf( f, "\n" );
    fclose( f );
}
#endif
