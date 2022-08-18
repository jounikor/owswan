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


#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgwind.h"
#include "dbgadget.h"
#include "dbgbrk.h"
#include "wndsys.h"
#include "addarith.h"
#include "dbgwfil.h"
#include "dbgwglob.h"
#include "dbgwinsp.h"
#include "dlgbreak.h"
#include "menudef.h"


#define WndBreak( wnd ) ( (break_window *)WndExtra( wnd ) )

enum {
#ifdef OPENER_GADGET
    PIECE_OPENER,
#endif
    PIECE_ACTIVE,
    PIECE_ADDR,
    PIECE_SOURCE,
};

typedef struct break_window {
    gui_ord     addr_indent;
    gui_ord     source_indent;
    boolbit     toggled_break   : 1;
} break_window;

extern char             *AddrLineNum( address *addr, char *buff );

static gui_menu_struct BrkMenu[] = {
    #include "menubrk.h"
};

static brkp     *BrkGetBP( int row )
{
    brkp        *bp;
    int         count;

    if( row < 0 )
        return( NULL );
    count = 0;
    for( bp = BrkList; bp != NULL; bp = bp->next ) {
        if( count == row )
            break;
        ++count;
    }
    return( bp );
}

static void     BrkMenuItem( a_window wnd, gui_ctl_id id, wnd_row row, wnd_piece piece )
{
    brkp        *bp;

    /* unused parameters */ (void)piece;

    if( id == MENU_BREAK_CREATE_NEW ) {
        if( DlgBreak( NilAddr ) ) {
            WndScrollBottom( wnd );
        }
    } else {
        if( row < 0 ) {
            bp = NULL;
        } else {
            bp = BrkGetBP( row );
        }
        if( bp == NULL ) {
            if( id == MENU_INITIALIZE ) {
                WndMenuGrayAll( wnd );
                WndMenuEnable( wnd, MENU_BREAK_CREATE_NEW, true );
            }
        } else {
            switch( id ) {
            case MENU_INITIALIZE:
                WndMenuEnableAll( wnd );
                WndMenuEnable( wnd, MENU_BREAK_ENABLE, !bp->status.b.active );
                WndMenuEnable( wnd, MENU_BREAK_DISABLE, bp->status.b.active );
                WndMenuEnable( wnd, MENU_BREAK_SOURCE, bp != NULL && IS_BP_EXECUTE( bp->mth ) );
                WndMenuEnable( wnd, MENU_BREAK_ASSEMBLY, bp != NULL && IS_BP_EXECUTE( bp->mth ) );
                break;
            case MENU_BREAK_ENABLE:
                ActPoint( bp, true );
                break;
            case MENU_BREAK_DISABLE:
                ActPoint( bp, false );
                break;
            case MENU_BREAK_CLEAR:
                RemoveBreak( bp->loc.addr );
                break;
            case MENU_BREAK_MODIFY:
                DlgBreak( bp->loc.addr );
                break;
            case MENU_BREAK_SOURCE:
                WndSrcInspect( bp->loc.addr );
                break;
            case MENU_BREAK_ASSEMBLY:
                WndAsmInspect( bp->loc.addr );
                break;
            }
        }
    }
}

static void     BrkModify( a_window wnd, wnd_row row, wnd_piece piece )
{
    brkp        *bp;

    if( row < 0 ) {
        BrkMenuItem( wnd, MENU_BREAK_CREATE_NEW, row, piece );
        return;
    }
    bp = BrkGetBP( row );
    if( bp == NULL )
        return;
    if( piece == PIECE_ACTIVE ) {
        WndRowDirty( wnd, row );
        WndBreak( wnd )->toggled_break = true;
        ActPoint( bp, !bp->status.b.active );
#ifdef OPENER_GADGET
    } else if( piece == PIECE_OPENER ) {
        if( bp->size == 0 ) {
            if( bp->source_line != NULL ) {
                WndSrcInspect( bp->loc.addr );
            } else {
                WndAsmInspect( bp->loc.addr );
            }
        }
#endif
    } else {
        WndFirstMenuItem( wnd, row, piece );
    }
}

static wnd_row BrkNumRows( a_window wnd )
{
    brkp        *bp;
    wnd_row     count;

    /* unused parameters */ (void)wnd;

    count = 0;
    for( bp = BrkList; bp != NULL; bp = bp->next ) {
        ++count;
    }
    return( count );
}

static  bool    BrkGetLine( a_window wnd, wnd_row row, wnd_piece piece, wnd_line_piece *line )
{
    brkp                *bp;
    break_window        *wndbreak;
    bool                curr;

    wndbreak = WndBreak( wnd );
    bp = BrkGetBP( row );
    if( bp == NULL )
        return( false );
    line->text = TxtBuff;
    line->tabstop = false;
    switch( piece ) {
#ifdef OPENER_GADGET
    case PIECE_OPENER:
    {
        mod_handle              mh;
        line->text = LIT_DUI( Empty );
        if( bp->size == 0 ) {
            DeAliasAddrMod( bp->loc.addr, &mh );
            bp->status.b.source_open =
                OpenGadget( wnd, line, mh, bp->source_line != NULL );
        }
        return( true );
    }
#endif
    case PIECE_ACTIVE:
#ifdef OPENER_GADGET
        line->indent = MaxGadgetLength + WndAvgCharX( wnd );
#endif
        curr = AddrComp( bp->loc.addr, Context.execution ) == 0;
        FileBreakGadget( wnd, line, curr, bp );
        return( true );
    case PIECE_ADDR:
        line->indent = wndbreak->addr_indent;
        line->extent = WND_MAX_EXTEND;
        line->tabstop = true;
        GetBPAddr( bp, TxtBuff );
        return( true );
    case PIECE_SOURCE:
        line->indent = wndbreak->source_indent;
        line->tabstop = false;
        line->use_prev_attr = true;
        GetBPText( bp, TxtBuff );
        return( true );
    default:
        return( false );
    }
}


static void     BrkInit( a_window wnd )
{
    gui_ord             length, max;
    break_window        *wndbreak;
    brkp                *bp;
    int                 count;

    wndbreak = WndBreak( wnd );
    max = 0;
    count = 0;
    for( bp = BrkList; bp != NULL; bp = bp->next ) {
        GetBPAddr( bp, TxtBuff );
        length = WndExtentX( wnd, TxtBuff );
        if( max < length )
            max = length;
        ++count;
    }
    length = MaxGadgetLength + WndAvgCharX( wnd );
#ifdef OPENER_GADGET
    length += length;
#endif
    wndbreak->addr_indent = length;

    length += max + 2 * WndMaxCharX( wnd );
    wndbreak->source_indent = length;

    if( wndbreak->toggled_break ) {
        wndbreak->toggled_break = false;
        return;
    }
    WndNoSelect( wnd );
    WndSetRepaint( wnd );
}


static void     BrkRefresh( a_window wnd )
{
    brkp        *bp;
    int         row;

    if( UpdateFlags & (UP_RADIX_CHANGE | UP_SYM_CHANGE | UP_BREAK_CHANGE) ) {
        BrkInit( wnd );
    } else if( UpdateFlags & UP_MEM_CHANGE ) {
        row = 0;
        for( bp = BrkList; bp != NULL; bp = bp->next ) {
            if( !IS_BP_EXECUTE( bp->mth ) ) {
                WndPieceDirty( wnd, row, PIECE_SOURCE );
            }
            ++row;
        }
    }
#ifdef OPENER_GADGET
    {
        int             i;
        brkp            *bp;
        mod_handle      mh;

        if( UpdateFlags & UP_OPEN_CHANGE ) {
            for( i = 0, bp = BrkList; bp != NULL; bp = bp->next, ++i ) {
                DeAliasAddrMod( bp->loc.addr, &mh );
                bp->status.b.source_open =
                    CheckOpenGadget( wnd, i, bp->status.b.source_open, mh,
                                     bp->source_line != NULL, PIECE_OPENER );
            }
        }
    }
#endif
}


static bool BrkWndEventProc( a_window wnd, gui_event gui_ev, void *parm )
{
    break_window        *wndbreak = WndBreak( wnd );

    /* unused parameters */ (void)parm;

    switch( gui_ev ) {
    case GUI_INIT_WINDOW:
        wndbreak->toggled_break = false;
        BrkInit( wnd );
        return( true );
    case GUI_RESIZE:
        BrkInit( wnd );
        return( true );
    case GUI_DESTROY:
        WndFree( wndbreak );
        return( true );
    }
    return( false );
}

static bool ChkUpdate( void )
{
    return( UpdateFlags & (UP_MEM_CHANGE | UP_RADIX_CHANGE | UP_SYM_CHANGE | UP_BREAK_CHANGE | UP_OPEN_CHANGE) );
}

wnd_info BrkInfo = {
    BrkWndEventProc,
    BrkRefresh,
    BrkGetLine,
    BrkMenuItem,
    NoVScroll,
    NoBegPaint,
    NoEndPaint,
    BrkModify,
    BrkNumRows,
    NoNextRow,
    NoNotify,
    ChkUpdate,
    PopUp( BrkMenu )
};

a_window WndBrkOpen( void )
{
    a_window            wnd;
    break_window        *brkw;

    brkw = WndMustAlloc( sizeof( *brkw ) );
    wnd = DbgWndCreate( LIT_DUI( WindowBrk ), &BrkInfo, WND_BREAK, brkw, &BrkIcon );
    if( wnd != NULL )
        WndClrSwitches( wnd, WSW_ONLY_MODIFY_TABSTOP );
    return( wnd );
}
