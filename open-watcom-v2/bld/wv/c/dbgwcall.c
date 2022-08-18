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
* Description:  The 'Calls' window (call stack).
*
****************************************************************************/


#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgwind.h"
#include "dbgtback.h"
#include "dbgadget.h"
#include "dbgmem.h"
#include "dbgchain.h"
#include "mad.h"
#include "strutil.h"
#include "dbgutil.h"
#include "dbgbrk.h"
#include "wndsys.h"
#include "dbgmisc.h"
#include "dbgreg.h"
#include "addarith.h"
#include "dbgwglob.h"
#include "dbgwinsp.h"
#include "dlgbreak.h"
#include "menudef.h"


enum {
    PIECE_SYMBOL,
    PIECE_SOURCE,
    PIECE_TABSTOP = PIECE_SYMBOL,
};

#define WndCall( wnd ) ( (call_window *)WndExtra( wnd ) )

typedef struct call_window {
    cached_traceback    tb;
    gui_ord             max_sym_len;
} call_window;

extern address          FindLclBlock( address addr );
extern unsigned         LineNumLkup( address );

static gui_menu_struct CallMenu[] = {
    #include "menucall.h"
};

static wnd_row CallNumRows( a_window wnd )
{
    return( WndCall( wnd )->tb.curr->total_depth );
}

static void     CallMenuItem( a_window wnd, gui_ctl_id id, wnd_row row, wnd_piece piece )
{
    call_chain  *chain;
    call_window *call = WndCall( wnd );

    /* unused parameters */ (void)piece;

    chain = GetCallChain( &call->tb, row );
    switch( id ) {
    case MENU_INITIALIZE:
        if( chain == NULL ) {
            WndMenuGrayAll( wnd );
        } else {
            WndMenuEnableAll( wnd );
            WndMenuEnable( wnd, MENU_CALL_EXECUTE_TO_RETURN, ( row != 0 ) );
            WndMenuEnable( wnd, MENU_CALL_SOURCE, ( chain->source_line != NULL ) );
        }
        break;
    case MENU_CALL_BREAK:
        DlgBreak( row == 0 ? chain->lc.execution : FindNextIns( chain->lc.execution ) );
        break;
    case MENU_CALL_SOURCE:
        WndSrcInspect( chain->lc.execution );
        break;
    case MENU_CALL_UNWIND:
        UnWindToFrame( chain, row, CallNumRows( wnd ) );
        break;
    case MENU_CALL_ASSEMBLY:
        WndAsmInspect( chain->lc.execution );
        break;
    case MENU_CALL_EXECUTE_TO_RETURN:
        GoToAddr( FindNextIns( chain->lc.execution ) );
        break;
    }
}

static  bool    CallGetLine( a_window wnd, wnd_row row, wnd_piece piece, wnd_line_piece *line )
{
    call_chain  *chain;
    call_window *call = WndCall( wnd );

    chain = GetCallChain( &call->tb, row );
    if( chain == NULL )
        return( false );
    line->extent = WND_MAX_EXTEND;
    switch( piece ) {
    case PIECE_SYMBOL:
        StrCopy( ":", StrCopy( chain->symbol, TxtBuff ) );
        line->text = TxtBuff;
        return( true );
    case PIECE_SOURCE:
        line->indent = call->max_sym_len + 3 * WndAvgCharX( wnd );
        line->tabstop = false;
        line->use_prev_attr = true;
        if( chain->source_line == NULL ) {
            line->text = TxtBuff;
            UnAsm( chain->lc.execution, TxtBuff, TXT_LEN );
        } else {
            line->text = chain->source_line;
        }
        return( true );
    default:
        return( false );
    }
}


static void     CallInit( a_window wnd )
{
    int                 row;
    call_window         *call = WndCall( wnd );
    traceback           *curr,*prev;
    call_chain          *chain;
    int                 i;

    UpdateTraceBack( &call->tb );
    curr = call->tb.curr;
    prev = call->tb.prev;
    WndNoSelect( wnd );
    if( curr->clean_size == 0 || curr->total_depth < prev->total_depth ) {
        WndSetRepaint( wnd );
    } else {
        row = curr->total_depth;
        if( row < prev->total_depth ) {
            row = prev->total_depth;
        }
        while( --row >= curr->clean_size ) {
            WndRowDirty( wnd, row );
        }
    }
    call->max_sym_len = 0;
    chain = curr->chain;
    for( i = 0; i < curr->current_depth; ++i ) {
        if( chain[i].sym_len == 0 ) {
            chain[i].sym_len = WndExtentX( wnd, chain[i].symbol );
        }
        if( call->max_sym_len < chain[i].sym_len ) {
            call->max_sym_len = chain[i].sym_len;
        }
    }
}


static void CallScrollPos( a_window wnd )
{
    WndMoveCurrent( wnd, CallNumRows(wnd) - 1 + GetStackPos(), PIECE_TABSTOP );
}


static void CallRefresh( a_window wnd )
{

    if( UpdateFlags & (UP_RADIX_CHANGE | UP_SYM_CHANGE | UP_CSIP_CHANGE) ) {
        CallInit( wnd );
    }
    CallScrollPos( wnd );
}


static void CallClose( a_window wnd )
{
    call_window *call = WndCall( wnd );

    FiniTraceBack( &call->tb );
    WndFree( call );
}


static bool CallWndEventProc( a_window wnd, gui_event gui_ev, void *parm )
{
    call_window *call = WndCall( wnd );

    /* unused parameters */ (void)parm;

    switch( gui_ev ) {
    case GUI_INIT_WINDOW:
        InitTraceBack( &call->tb );
        /* fall through */
    case GUI_RESIZE:
        CallInit( wnd );
        CallScrollPos( wnd );
        return( true );
    case GUI_DESTROY :
        CallClose( wnd );
        return( true );
    }
    return( false );
}


static bool ChkUpdate( void )
{
    return( UpdateFlags & (UP_RADIX_CHANGE | UP_SYM_CHANGE | UP_CSIP_CHANGE | UP_STACKPOS_CHANGE) );
}

wnd_info CallInfo = {
    CallWndEventProc,
    CallRefresh,
    CallGetLine,
    CallMenuItem,
    NoVScroll,
    NoBegPaint,
    NoEndPaint,
    WndFirstMenuItem,
    CallNumRows,
    NoNextRow,
    NoNotify,
    ChkUpdate,
    PopUp( CallMenu )
};

a_window WndCallOpen( void )
{
    call_window *call;

    call = WndMustAlloc( sizeof( call_window ) );
    return( DbgWndCreate( LIT_DUI( WindowCalls ), &CallInfo, WND_CALL, call, &CallIcon ) );
}
