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


#include <stdlib.h>
#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgwind.h"
#include "mempiece.h"
#include "dbgadget.h"
#include "mad.h"
#include "memtypes.h"
#include "dbgitem.h"
#include "strutil.h"
#include "dbgscan.h"
#include "dbgutil.h"
#include "dbgmemor.h"
#include "wndsys.h"
#include "dlgexpr.h"
#include "dbgwglob.h"
#include "dbgwio.h"
#include "menudef.h"


#define PIECE_TYPE( x )     ((x) - MENU_IO_FIRST_TYPE)

#define WndIO( wnd ) ( (io_window *)WndExtra( wnd ) )

enum {
    PIECE_READ,
    PIECE_WRITE,
    PIECE_ADDRESS,
    PIECE_VALUE
};

typedef struct {
    item_mach   value;
    address     addr;
    int         type;
    boolbit     value_known     : 1;
} io_location;

typedef struct {
    int         num_rows;
    io_location *list;
} io_window;

static mem_type_walk_data   IOData;

static gui_menu_struct      *IOTypeMenu = NULL;

static gui_menu_struct IOMenu[] = {
    #include "menuio.h"
};

static wnd_row IONumRows( a_window wnd )
{
    return( WndIO( wnd )->num_rows );
}

static void IOAddNewAddr( a_window wnd, address *addr, int type )
{
    io_window   *io = WndIO( wnd );
    int         row;
    io_location *curr;

    row = io->num_rows;
    io->num_rows++;
    io->list = WndMustRealloc( io->list, io->num_rows * sizeof( io_location ) );
    curr = &io->list[row];
    curr->type = PIECE_TYPE( type );
    curr->addr = *addr;
    curr->value_known = false;
}

static void     IOMenuItem( a_window wnd, gui_ctl_id id, wnd_row row, wnd_piece piece )
{
    io_window   *io = WndIO( wnd );
    address     addr;
    bool        ok;
    item_mach   item;
    io_location *curr;
    mad_radix   old_radix;

    /* unused parameters */ (void)piece;

    if( row < io->num_rows && row >= 0 ) {
        curr = &io->list[row];
    } else {
        curr = NULL;
    }
    switch( id ) {
    case MENU_INITIALIZE:
        if( curr == NULL ) {
            WndMenuGrayAll( wnd );
        } else {
            WndMenuEnableAll( wnd );
        }
        WndMenuEnable( wnd, MENU_IO_NEW_ADDRESS, true );
        break;
    case MENU_IO_DELETE:
        io->num_rows--;
        memcpy( &io->list[row], &io->list[row + 1],
                ( io->num_rows - row ) * sizeof( io_location ) );
        WndNoSelect( wnd );
        WndSetRepaint( wnd );
        break;
    case MENU_IO_NEW_ADDRESS:
        addr = NilAddr;
        if( !DlgGivenAddr( LIT_DUI( New_Port_Addr ), &addr ) )
            return;
        WndRowDirty( wnd, io->num_rows );
        IOAddNewAddr( wnd, &addr, MENU_IO_FIRST_TYPE );
        WndScrollBottom( wnd );
        break;
    case MENU_IO_MODIFY:
        if( row >= io->num_rows || row < 0 )
            break;
        if( piece == PIECE_VALUE ) {
            old_radix = NewCurrRadix( IOData.info[curr->type].piece_radix );
            item.ud = curr->value_known ? curr->value.ud : 0;
            ok = DlgMadTypeExpr( TxtBuff, &item, IOData.info[curr->type].mth );
            if( ok ) {
                curr->value = item;
                curr->value_known = true;
            }
            NewCurrRadix( old_radix );
        } else {
            addr = curr->addr;
            if( !DlgGivenAddr( LIT_DUI( New_Port_Addr ), &addr ) )
                return;
            curr->addr = addr;
            curr->value_known = false;
        }
        WndRowDirty( wnd, row );
        break;
    case MENU_IO_READ:
        curr->value_known = true;
        if( ItemGetMAD( &curr->addr, &curr->value, IT_IO, IOData.info[curr->type].mth ) == IT_NIL ) {
            curr->value_known = false;
        }
        WndPieceDirty( wnd, row, PIECE_VALUE );
        break;
    case MENU_IO_WRITE:
        if( curr->value_known ) {
            ItemPutMAD( &curr->addr, &curr->value, IT_IO, IOData.info[curr->type].mth );
        }
        break;
    default:
        curr->type = PIECE_TYPE( id );
        WndZapped( wnd );
        break;
    }
}


static void     IOModify( a_window wnd, wnd_row row, wnd_piece piece )
{
    if( row < 0 ) {
        IOMenuItem( wnd, MENU_IO_NEW_ADDRESS, row, piece );
        return;
    }
    if( row >= IONumRows( wnd ) )
        return;
    switch( piece ) {
    case PIECE_READ:
        IOMenuItem( wnd, MENU_IO_READ, row, piece );
        break;
    case PIECE_WRITE:
        IOMenuItem( wnd, MENU_IO_WRITE, row, piece );
        break;
    default:
        WndFirstMenuItem( wnd, row, piece );
        break;
    }
}

static  bool    IOGetLine( a_window wnd, wnd_row row, wnd_piece piece, wnd_line_piece *line )
{
    io_window   *io = WndIO( wnd );
//    bool        ret;
    io_location *curr;
    int         i;
    mad_radix   old_radix, new_radix;
    size_t      max;

    if( row >= io->num_rows )
        return( false );
    curr = &io->list[row];
//    ret = true;
    line->text = TxtBuff;
    switch( piece ) {
    case PIECE_READ:
        SetGadgetLine( wnd, line, GADGET_READ );
        return( true );
    case PIECE_WRITE:
        SetGadgetLine( wnd, line, GADGET_WRITE );
        line->indent = MaxGadgetLength;
        return( true );
    case PIECE_ADDRESS:
        AddrToIOString( &curr->addr, TxtBuff, TXT_LEN );
        line->indent = 2 * MaxGadgetLength;
        return( true );
    case PIECE_VALUE:
        new_radix = IOData.info[curr->type].piece_radix;
        old_radix = NewCurrRadix( new_radix );
        line->indent = 2 * MaxGadgetLength + 10 * WndMaxCharX( wnd );
        if( curr->value_known ) {
            max = TXT_LEN;
            MADTypeHandleToString( new_radix, IOData.info[curr->type].mth, &curr->value, TxtBuff, &max );
        } else {
            for( i = 0; i < IOData.info[curr->type].item_width; ++i ) {
                TxtBuff[i] = '?';
            }
            TxtBuff[i] = NULLCHAR;
        }
        NewCurrRadix( old_radix );
        return( true );
    default:
        return( false );
    }
}


static void     IORefresh( a_window wnd )
{
    WndNoSelect( wnd );
    WndSetRepaint( wnd );
}


void SetIOMenuItems( void )
{
    WndEnableMainMenu( MENU_MAIN_OPEN_IO, IOData.num_types != 0 );
}

void InitIOWindow( void )
{
    int                 i;

    MemInitTypes( MAS_IO | MTK_INTEGER, &IOData );
    if( IOData.num_types == 0 ) {
        return;
    }
    IOTypeMenu = WndMustAlloc( IOData.num_types * sizeof( *IOTypeMenu ) );
    for( i = 0; i < IOData.num_types; ++i ) {
        IOTypeMenu[i].id = MENU_IO_FIRST_TYPE + i;
        IOTypeMenu[i].style = GUI_STYLE_MENU_ENABLED | WND_MENU_ALLOCATED;
        IOTypeMenu[i].label = DupStr( IOData.labels[i] );
        IOTypeMenu[i].hinttext = DupStr( LIT_ENG( Empty ) );
        IOTypeMenu[i].child = NoMenu;
    }
    for( i = 0; i < ArraySize( IOMenu ); ++i ) {
        if( IOMenu[i].id == MENU_IO_TYPE ) {
            IOMenu[i].child.menu = IOTypeMenu;
            IOMenu[i].child.num_items = IOData.num_types;
            break;
        }
    }
}

void FiniIOWindow( void )
{
    WndFree( IOTypeMenu );
    MemFiniTypes( &IOData );
}

static bool IOWndEventProc( a_window wnd, gui_event gui_ev, void *parm )
{
    io_window   *io = WndIO( wnd );

    /* unused parameters */ (void)parm;

    switch( gui_ev ) {
    case GUI_INIT_WINDOW:
        if( io->num_rows != 0 ) {
            IOMenuItem( wnd, MENU_IO_READ, 0, PIECE_VALUE );
        }
        return( true );
    case GUI_DESTROY :
        WndFree( io->list );
        WndFree( io );
        return( true );
    }
    return( false );
}

static bool ChkUpdate( void )
{
    return( UpdateFlags & UP_RADIX_CHANGE );
}

wnd_info IOInfo = {
    IOWndEventProc,
    IORefresh,
    IOGetLine,
    IOMenuItem,
    NoVScroll,
    NoBegPaint,
    NoEndPaint,
    IOModify,
    IONumRows,
    NoNextRow,
    NoNotify,
    ChkUpdate,
    PopUp( IOMenu )
};

void IONewAddr( a_window wnd, address *addr, int type )
{
    IOAddNewAddr( wnd, addr, type );
    IOMenuItem( wnd, MENU_IO_READ, WndIO( wnd )->num_rows - 1, PIECE_VALUE );
    WndSetRepaint( wnd );
}


a_window DoWndIOOpen( address *addr, mad_type_handle mth )
{
    io_window   *io;
    int         i;

    if( IOData.num_types == 0 )
        return( NULL );
    io = WndMustAlloc( sizeof( io_window ) );
    io->list = WndMustAlloc( sizeof( io_location ) );
    io->num_rows = 1;
    io->list->addr = *addr;
    io->list->type = PIECE_TYPE( MENU_IO_FIRST_TYPE );
    if( mth != MAD_NIL_TYPE_HANDLE ) {
        for( i = 0; i < IOData.num_types; i++ ) {
            if( IOData.info[i].mth == mth ) {
                break;
            }
        }
        if( i != IOData.num_types ) {
            io->list->type = i;
        }
    }
    io->list->value.ud = 0;
    io->list->value_known = false;
    return( DbgWndCreate( LIT_DUI( WindowIO_Ports ), &IOInfo, WND_IO, io, &IOIcon ) );
}

a_window WndIOOpen( void )
{
    io_window   *io;
    a_window    wnd;

    io = WndMustAlloc( sizeof( io_window ) );
    io->list = NULL;
    io->num_rows = 0;
    wnd = DbgWndCreate( LIT_DUI( WindowIO_Ports ), &IOInfo, WND_IO, io, &IOIcon );
    if( wnd != NULL )
        WndClrSwitches( wnd, WSW_ONLY_MODIFY_TABSTOP );
    return( wnd );
}
