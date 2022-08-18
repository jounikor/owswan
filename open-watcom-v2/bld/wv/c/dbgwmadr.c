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
* Description:  Register window layout and sizing.
*
****************************************************************************/


#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgwind.h"
#include "dbgitem.h"
#include "strutil.h"
#include "dbgscan.h"
#include "dbgmad.h"
#include "madinter.h"
#include "madcli.h"
#include "dbgstk.h"
#include "dbgexpr.h"
#include "wndsys.h"
#include "dbgreg.h"
#include "dlgexpr.h"
#include "dbgwglob.h"
#include "dbgwinsp.h"
#include "dbgwmadr.h"
#include "dbgwtogl.h"
#include "menudef.h"


#define WndReg( wnd ) ( (reg_window*)WndExtra( wnd ) )

typedef struct {
    unsigned char           standout;
    const mad_reg_info      *info;
    gui_ord                 max_extent;
    gui_ord                 max_descript;
    unsigned char           max_value;
} a_reg_info;

typedef struct {
    gui_ord                 descript;
    gui_ord                 value;
} reg_indent;

typedef struct {
    const mad_reg_set_data  *data;
    unsigned char           up;
    unsigned char           rows;
    unsigned char           count;
    a_reg_info              *info;
    reg_indent              *indents;
    gui_menu_struct         *popup;
    unsigned                num_toggles;
    mad_type_kind           kind;
} reg_window;

typedef struct {
    const char              *descript;
    size_t                  max_descript;
    const mad_reg_info      *reginfo;
    mad_type_handle         disp_mth;
    size_t                  max_value;
} reg_display_piece;

extern address              AddrRegIP( machine_state *regs );
extern unsigned             GetInsSize( address addr );

static gui_menu_struct RegMenu[] = {
    #include "menureg.h"
};

static bool GetDisplayPiece( reg_display_piece *disp, reg_window *reg, machine_state *mach, int i )
{
    return( MADRegSetDisplayGetPiece( reg->data, &mach->mr, i, &disp->descript,
                                  &disp->max_descript, &disp->reginfo,
                                  &disp->disp_mth, &disp->max_value ) == MS_OK );
}

static bool RegResize( a_window wnd )
{
    reg_window          *reg = WndReg( wnd );
    gui_ord             space;
    int                 old_up;
    int                 i,j;
    reg_display_piece   disp;
    gui_ord             max_extent;
    gui_ord             max_descript;
    a_reg_info          *info;
    gui_ord             indent;
    gui_ord             value,descript;
    char                *p;
    unsigned            len;

    old_up = reg->up;
    reg->up = 1;

    RegFindData( reg->kind, &reg->data );
    reg->count = 0;
    while( GetDisplayPiece( &disp, reg, DbgRegs, reg->count ) ) {
        reg->count++;
    }

    WndFree( reg->info );
    reg->info = WndMustAlloc( reg->count * sizeof( *reg->info ) );
    space = WndAvgCharX( wnd );

    max_extent = 0;
    max_descript = 0;
    for( i = 0; i < reg->count; ++i ) {
        GetDisplayPiece( &disp, reg, DbgRegs, i );
        if( disp.max_value == 0 && disp.reginfo != NULL ) {
            disp.max_value = GetMADMaxFormatWidth( disp.disp_mth );
        }
        info = &reg->info[i];
        info->max_value = disp.max_value;
        info->info = disp.reginfo;
        if( disp.max_descript > strlen( disp.descript ) ) {
            info->max_descript = space + disp.max_descript * WndAvgCharX( wnd );
        } else {
            info->max_descript = space + WndExtentX( wnd, disp.descript );
        }
        info->max_extent = space + disp.max_value * WndAvgCharX( wnd );
        info->standout = false;
        if( max_extent < info->max_extent ) {
            max_extent = info->max_extent;
        }
        if( max_descript < info->max_descript ) {
            max_descript = info->max_descript;
        }
    }

    reg->up = MADRegSetDisplayGrouping( reg->data );
    if( reg->up == 0 ) {
        reg->up = WndWidth( wnd ) / ( max_extent + max_descript );
        if( reg->up < 1 )
            reg->up = 1;
        if( reg->up > reg->count ) {
            reg->up = reg->count;
        }
    }
    reg->rows = ( reg->count + reg->up - 1 ) / reg->up;

    // calculate the indents

    WndFree( reg->indents );
    reg->indents = WndMustAlloc( reg->count * sizeof( *reg->indents ) );

    // For each column
    for( i = 0; i < reg->up; ++i ) {
        reg->indents[i].descript = 0;
        reg->indents[i].value = 0;
        // Calc max widths for column
        for( j = i; j < reg->count; j += reg->up ) {
            if( reg->indents[i].value < reg->info[j].max_extent ) {
                reg->indents[i].value = reg->info[j].max_extent;
            }
            if( reg->indents[i].descript < reg->info[j].max_descript ) {
                reg->indents[i].descript = reg->info[j].max_descript;
            }
        }
    }
    // Calc indents for each column
    indent = 0;
    value = 0;
    descript = 0;
    // For each column
    for( i = 0; i < reg->up; ++i ) {
        value = reg->indents[i].value;
        descript = reg->indents[i].descript;
        reg->indents[i].descript = indent;
        reg->indents[i].value = indent + descript;
        indent += value + descript;
#if ( defined( GUI_IS_GUI ) && defined( __OS2__ ) )
        // OS/2 PM GUI needs more space between columns
        indent += space;
#endif
    }
    // Copy indents to all registers by column
    for( i = reg->up; i < reg->count; ++i ) {
        reg->indents[i] = reg->indents[i % reg->up];
    }

    if( reg->up != old_up ) {
        WndVScrollAbs( wnd, 0 );
        WndNoCurrent( wnd );
    }

    p = TxtBuff + MADCli( String )( MADRegSetName( reg->data ), TxtBuff, TXT_LEN );
    *p++ = ' ';
    *p++ = '(';
    len = MADRegSetLevel( reg->data, p, TXT_LEN - ( p - TxtBuff ) );
    if( len == 0 ) {
        p -= 2;
    } else {
        p += len;
        *p++ = ')';
    }
    *p++ = NULLCHAR;
    WndSetTitle( wnd, TxtBuff );

    return( true );
}


static wnd_row RegNumRows( a_window wnd )
{
    return( WndReg( wnd )->rows );
}


static int GetRegIdx( reg_window *reg, wnd_row row, wnd_piece piece )
{
    int         i;

    if( row == WND_NO_ROW )
        return( -1 );
    i = row * reg->up + piece;
    if( i >= reg->count )
        i = -1;
    return( i );
}


static const char *RegValueName( const void *data_handle, int item )
{
    mad_modify_list const *possible = (mad_modify_list const *)data_handle + item;
    size_t          buff_len;

    buff_len = TXT_LEN;
    if( possible->name == MAD_MSTR_NIL ) {
        MADTypeHandleToString( MADTypePreferredRadix( possible->mth ),
                possible->mth, possible->data, TxtBuff, &buff_len );
    } else {
        MADCli( String )( possible->name, TxtBuff, buff_len );
    }
    return( TxtBuff );
}

static  void    RegModify( a_window wnd, wnd_row row, wnd_piece piece )
{
    int                     i;
    item_mach               value;
    reg_window              *reg = WndReg( wnd );
    bool                    ok;
    mad_radix               old_radix;
    reg_display_piece       disp;
    mad_type_info           mti;
    mad_modify_list const   *possible;
    int                     num_possible;

    if( row < 0 )
        return;
    piece >>= 1;
    i = GetRegIdx( reg, row, piece );
    if( i == -1 )
        return;
    if( !GetDisplayPiece( &disp, reg, DbgRegs, i ) )
        return;
    if( disp.reginfo == NULL )
        return;
    if( MADRegSetDisplayModify( reg->data, disp.reginfo, &possible, &num_possible ) != MS_OK )
        return;
    old_radix = NewCurrRadix( MADTypePreferredRadix( disp.disp_mth ) );
    MADRegFullName( disp.reginfo, ".", TxtBuff, TXT_LEN );
    RegValue( &value, disp.reginfo, DbgRegs );
    if( num_possible == 1 ) {
        ok = DlgMadTypeExpr( TxtBuff, &value, disp.disp_mth );
        if( ok ) {
            RegNewValue( disp.reginfo, &value, possible->mth );
        }
    } else {
        for( i = 0; i < num_possible; ++i ) {
            MADTypeInfo( possible[i].mth, &mti );
            if( memcmp( &value, possible[i].data, BITS2BYTES( mti.b.bits ) ) == 0 ) {
                break;
            }
        }
        if( num_possible == 2 ) {
            if( i == 0 ) {
                i = 1;
            } else {
                i = 0;
            }
            RegNewValue( disp.reginfo, possible[i].data, possible[i].mth );
        } else {  //MJC const cast
            if( DlgPickWithRtn( TxtBuff, possible, i, RegValueName, num_possible, &i ) ) {
                RegNewValue( disp.reginfo, possible[i].data, possible[i].mth );
            }
        }
    }
    NewCurrRadix( old_radix );
}

static void     RegMenuItem( a_window wnd, gui_ctl_id id, wnd_row row, wnd_piece piece )
{
    reg_window              *reg = WndReg( wnd );
    int                     i;
    unsigned                bit;
    mad_modify_list const   *possible;
    int                     num_possible;
    address                 addr;
    bool                    valid_reg;

    i = GetRegIdx( reg, row, piece >>1 );
    switch( id ) {
    case MENU_INITIALIZE:
        valid_reg = i != -1 && reg->info[i].info != NULL &&
                    MADRegSetDisplayModify( reg->data, reg->info[i].info,
                                            &possible, &num_possible ) == MS_OK;
        WndMenuEnable( wnd, MENU_REGISTER_MODIFY, valid_reg );
        WndMenuEnable( wnd, MENU_REGISTER_INSPECT, valid_reg );
        bit = MADRegSetDisplayToggle( reg->data, 0, 0 );
        for( i = 0; i < reg->num_toggles; ++i ) {
            WndMenuCheck( wnd, MENU_REGISTER_TOGGLES + i, ( bit & 1 ) != 0 );
            bit >>= 1;
        }
        break;
    case MENU_REGISTER_INSPECT:
        if( MADRegInspectAddr( reg->info[i].info, &DbgRegs->mr, &addr ) == MS_OK ) {
            PushAddr( addr );
            WndInspectExprSP( "" );
        }
        break;
    case MENU_REGISTER_MODIFY:
        RegModify( wnd, row, piece );
        break;
    default:
        bit = 1 << ( id - MENU_REGISTER_TOGGLES );
        MADRegSetDisplayToggle( reg->data, bit, bit );
        RegResize( wnd );
        WndZapped( wnd );
    }
}


static  bool    RegGetLine( a_window wnd, wnd_row row, wnd_piece piece, wnd_line_piece *line )
{
    int                 column;
    int                 i;
    reg_window          *reg = WndReg( wnd );
    size_t              max = TXT_LEN;
    mad_radix           old_radix, new_radix;
    item_mach           value;
    reg_display_piece   disp;

    column = piece >> 1;
    if( column >= reg->up )
        return( false );
    i = GetRegIdx( reg, row, column );
    if( i >= reg->count )
        return( false );
    if( i == -1 )
        return( false );
    if( !GetDisplayPiece( &disp, reg, DbgRegs, i ) )
        return( false );
    line->text = TxtBuff;
    if( piece & 1 ) {
        line->indent = reg->indents[column].value;
        if( reg->info[i].info == NULL ) {
            strcpy( TxtBuff, "   " );
        } else {
            new_radix = MADTypePreferredRadix( disp.disp_mth );
            old_radix = NewCurrRadix( new_radix );
            RegValue( &value, reg->info[i].info, DbgRegs );
            max = reg->info[i].max_value + 1;
            MADTypeHandleToString( new_radix, disp.disp_mth, &value, TxtBuff, &max );
            NewCurrRadix( old_radix );
            reg->info[i].standout = false;
            if( MADRegModified( reg->data, reg->info[i].info, &PrevRegs->mr, &DbgRegs->mr ) == MS_MODIFIED_SIGNIFICANTLY ) {
                reg->info[i].standout = true;
                line->attr = WND_STANDOUT;
            }
        }
    } else {
        line->indent = reg->indents[column].descript;
        strcpy( TxtBuff, disp.descript );
        if( TxtBuff[0] != NULLCHAR ) {
            strcat( TxtBuff, ":" );
        }
        line->tabstop = false;
    }
    return( true );
}


static void     RegRefresh( a_window wnd )
{
    int                 row,rows;
    int                 reg_num;
    int                 i;
    reg_window          *reg = WndReg( wnd );
    reg_display_piece   disp;

    // if register type changes, from 16 to 32-bit,
    // this will force the mad to call back NOW, setting UP_REG_RESIZE
    GetDisplayPiece( &disp, reg, DbgRegs, 0 );
    if( UpdateFlags & UP_REG_RESIZE ) {
        RegResize( wnd );
        WndZapped( wnd );
        return;
    }
    if( UpdateFlags & UP_MAD_CHANGE ) {
        WndZapped( wnd );
        return;
    }
    rows = RegNumRows( wnd );
    for( row = 0; row < rows; ++row ) {
        for( reg_num = 0; reg_num < reg->up; ++reg_num ) {
            i = GetRegIdx( reg, row, reg_num );
            if( i == -1 )
                break;
            if( reg->info[i].standout || ( reg->info[i].info != NULL &&
                MADRegModified( reg->data, reg->info[i].info, &PrevRegs->mr, &DbgRegs->mr ) != MS_OK ) ) {
                WndPieceDirty( wnd, row, reg_num*2+1 );
            }
        }
    }
}

static bool RegWndEventProc( a_window wnd, gui_event gui_ev, void *parm )
{
    reg_window          *reg = WndReg( wnd );

    /* unused parameters */ (void)parm;

    switch( gui_ev ) {
    case GUI_RESIZE:
        if( RegResize( wnd ) ) {
            WndZapped( wnd );
        }
        return( true );
    case GUI_INIT_WINDOW:
        reg->info = NULL;
        reg->indents = NULL;
        RegResize( wnd );
        reg->popup = WndAppendToggles( MADRegSetDisplayToggleList( reg->data ), &reg->num_toggles, RegMenu, ArraySize( RegMenu ), MENU_REGISTER_TOGGLES );
        WndSetPopUpMenu( wnd, ArraySize( RegMenu ) + reg->num_toggles, reg->popup );
        return( true );
    case GUI_DESTROY :
        WndDeleteToggles( reg->popup, ArraySize( RegMenu ), reg->num_toggles );
        WndFree( reg->info );
        WndFree( reg->indents );
        WndFree( reg );
        return( true );
    }
    return( false );
}

static bool ChkUpdate( void )
{
    return( UpdateFlags & (UP_MAD_CHANGE | UP_REG_CHANGE | UP_REG_RESIZE) );
}

wnd_info MadRegInfo = {
    RegWndEventProc,
    RegRefresh,
    RegGetLine,
    RegMenuItem,
    NoVScroll,
    NoBegPaint,
    NoEndPaint,
    WndFirstMenuItem,
    RegNumRows,
    NoNextRow,
    NoNotify,
    ChkUpdate,
    PopUp( RegMenu )
};

void MadRegChangeOptions( a_window wnd )
{
    RegResize( wnd );
    WndZapped( wnd );
}

a_window WndMadRegOpen( mad_type_kind kind, wnd_class_wv wndclass, gui_resource *icon )
{
    reg_window  *reg;
    a_window    wnd;

    reg = WndMustAlloc( sizeof( reg_window ) );
    reg->kind = kind;
    wnd = DbgWndCreate( LIT_ENG( Empty ), &MadRegInfo, wndclass, reg, icon );
    return( wnd );
}
