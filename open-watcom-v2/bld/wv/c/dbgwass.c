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
* Description:  Processing of pop-up menu for assembly view.
*
****************************************************************************/


#include "_srcmgt.h"
#include "dbgdata.h"
#include "dbgwind.h"
#include "dbgadget.h"
#include "dbgerr.h"
#include "dbgmem.h"
#include "dbgitem.h"
#include "srcmgt.h"
#include "strutil.h"
#include "dbgscan.h"
#include "madinter.h"
#include "dbgutil.h"
#include "dbgsrc.h"
#include "dbgstk.h"
#include "dbgexpr.h"
#include "dbgbrk.h"
#include "dbgass.h"
#include "dbgdot.h"
#include "wndsys.h"
#include "dbgtrace.h"
#include "modlist.h"
#include "remcore.h"
#include "dbgmisc.h"
#include "dipimp.h"
#include "dipinter.h"
#include "dbgreg.h"
#include "addarith.h"
#include "dbginsty.h"
#include "dbgupdt.h"
#include "dlgexpr.h"
#include "dbgwass.h"
#include "dbgwfil.h"
#include "dbgwglob.h"
#include "dbgwinsp.h"
#include "dbgwtogl.h"
#include "dbgmad.h"
#include "dbgchopt.h"
#include "menudef.h"


#define AVG_INS_SIZE    7

#define TITLE_SIZE      1

#define WndAsm( wnd ) ( (asm_window *)WndExtra( wnd ) )

enum {
    PIECE_BREAK,
    PIECE_ADDRESS,
    PIECE_BRANCH_INDICATOR,
    PIECE_OPCODE,
    PIECE_OPERANDS,
    PIECE_MEMREF,
    PIECE_CURRENT = PIECE_ADDRESS
};

typedef struct {
    address     addr;
    unsigned    line;
} asm_addr;

typedef struct {
    asm_addr        *ins;
    int             ins_size;
    address         active;
    address         dotaddr;
    address         cache_addr;
    mad_disasm_data *cache_dd;
    unsigned        ddsize;
    gui_ord         ins_end;
    gui_ord         address_end;
    gui_ord         last_width;
    a_window        src;
    mod_handle      mod;
    struct {
        mod_handle  mod;
        cue_fileid  file_id;
    }               src_list;
    void            *viewhndl;
    unsigned        num_toggles;
    gui_menu_struct *popup;
    mad_type_handle def_addr;
    unsigned        addr_len;
    boolbit         track           : 1;
    boolbit         toggled_break   : 1;
    boolbit         source          : 1;
    boolbit         hex             : 1;
} asm_window;

static  void    AsmResize( a_window wnd );

static gui_menu_struct AsmShowMenu[] = {
    #include "masmshow.h"
};

static gui_menu_struct AsmMenu[] = {
    #include "menuasm.h"
};

static bool ExactCueAt( asm_window *asw, address addr, cue_handle *cueh )
{
    if( !asw->source )
        return( false );
    if( DeAliasAddrCue( NO_MOD, addr, cueh ) == SR_NONE )
        return( false );
    if( AddrComp( DIPCueAddr( cueh ), addr ) )
        return( false );
    return( true );
}

static void AsmSetFirst( a_window wnd, address addr, bool use_first_source )
{
    int                 row,rows;
    asm_window          *asw = WndAsm( wnd );
    char                chr;
    mad_disasm_data     *dd;
    unsigned            addr_len;
    DIPHDL( cue, cueh );

    _AllocA( dd, asw->ddsize );


    if( IS_NIL_ADDR( addr ) || ProgPeek( addr, &chr, 1 ) != 1 ) {
        addr = NilAddr;
    }
    asw->ins[0].addr = addr;
    asw->ins[0].line = 0;
    addr_len = AddrToString( &addr, MAF_OFFSET, TxtBuff, TXT_LEN ) - TxtBuff;
    if( addr_len != asw->addr_len ) {
        asw->addr_len = addr_len;
        AsmResize( wnd ); // recusively calls this routine!
        WndZapped( wnd );
        return;
    }
    rows = WndRows( wnd );
    for( row = 0; row < rows; ++row ) {
        asw->ins[row].addr = addr;
        if( IS_NIL_ADDR( addr ) )
            continue;
        if( ExactCueAt( asw, addr, cueh ) ) {
            if( row != 0 || use_first_source ) {
                asw->ins[row].addr = addr;
                asw->ins[row].line = DIPCueLine( cueh );
                ++row;
                if( row >= rows ) {
                    break;
                }
            }
        }
        asw->ins[row].addr = addr;
        asw->ins[row].line = 0;
        if( MADDisasm( dd, &addr, 0 ) != MS_OK ) {
            addr = NilAddr;
        }
    }
}

static  void    CalcAddrLen( a_window wnd, address addr )
{
    asm_window  *asw;
    mad_radix   old_radix;

    asw = WndAsm( wnd );
    old_radix = NewCurrRadix( asw->hex ? 16 : 10 );
    AddrToString( &addr, MAF_OFFSET, TxtBuff, TXT_LEN );
    asw->address_end = MaxGadgetLength;
    asw->address_end += ( strlen( TxtBuff ) + 1 ) * WndMidCharX( wnd );
    NewCurrRadix( old_radix );
}

static  void    AsmResize( a_window wnd )
{
    asm_window  *asw;
    asm_addr    *new_ins;
    address     first;
    int         size;

    asw = WndAsm( wnd );
    size = WndRows( wnd );
    if( size <= 0 )
        size = 1;
    first = asw->ins[0].addr;
    new_ins = WndAlloc( size * sizeof( *new_ins ) );
    memset( new_ins, 0, size * sizeof( *new_ins ) );
    if( new_ins == NULL ) {
        WndClose( wnd );
        WndNoMemory();
    }
    WndFree( asw->ins );
    asw->ins = new_ins;
    asw->ins_size = size;
    AsmSetFirst( wnd, first, asw->ins[0].line != 0 );
    CalcAddrLen( wnd, first );
    if( asw->last_width != WndWidth( wnd ) ) {
        WndZapped( wnd );
    }
    asw->last_width = WndWidth( wnd );
    WndFixedThumb( wnd );
}

static int AsmAddrRow( a_window wnd, address ip )
{
    int         row;
    asm_window *asw;

    asw = WndAsm( wnd );
    for( row = 0; row < WndRows( wnd ); ++row ) {
        if( asw->ins[row].line != 0 )
            continue;
        if( AddrComp( asw->ins[row].addr, ip ) == 0 ) {
            break;
        }
    }
    return( row );
}

void    AsmJoinSrc( a_window wnd, a_window src )
{
    WndAsm( wnd )->src = src;
}

void    AsmNewSrcNotify( a_window src, mod_handle mod, bool track )
{
    asm_window  *asw;
    a_window    wnd;

    for( wnd = WndNext( NULL ); wnd != NULL; wnd = WndNext( wnd ) ) {
        if( WndClass( wnd ) != WND_ASSEMBLY )
            continue;
        asw = WndAsm( wnd );
        if( track != asw->track )
            continue;
        if( mod != asw->mod )
            continue;
        if( asw->src != NULL )
            continue;
        SrcJoinAsm( src, wnd );
        AsmJoinSrc( wnd, src );
        break;
    }
}

static void AsmSetTitle( a_window wnd )
{
    char        *p;
    const char  *image_name;
    asm_window  *asw;

    asw = WndAsm( wnd );
    p = StrCopy( ": ", StrCopy( LIT_DUI( WindowAssembly ), TxtBuff ) );
    p += DIPModName( asw->mod, p, TXT_LEN );
    image_name = ModImageName( asw->mod );
    if( image_name[0] != NULLCHAR ) {
        p = StrCopy( "(", StrCopy( " ", p ) );
        p = StrCopy( ")", StrCopy( SkipPathInfo( image_name, OP_REMOTE ), p ) );
    }
    WndSetTitle( wnd, TxtBuff );
}

static void AsmSetDotAddr( a_window wnd, address addr )
{
    mod_handle  mod;
    asm_window  *asw;

    asw = WndAsm( wnd );
    if( AddrComp( asw->dotaddr, addr ) != 0 ) {
        WndRowDirty( wnd, -TITLE_SIZE );
        asw->dotaddr = addr;
        DeAliasAddrMod( addr, &mod );
        if( mod != asw->mod ) {
            DbgUpdate( UP_OPEN_CHANGE );
            asw->mod = mod;
            AsmSetTitle( wnd );
        }
        if( IS_NIL_ADDR( addr ) )
            return;
        if( wnd == WndFindActive() ) {
            SrcMoveDot( asw->src, addr );
            SetCodeDot( addr );
        }
    }
}

void    AsmMoveDot( a_window wnd, address addr )
{
    wnd_row     row;
    asm_window  *asw;
    DIPHDL( cue, cueh1 );
    DIPHDL( cue, cueh2 );

    if( wnd == NULL )
        return;

    asw = WndAsm( wnd );
    if( DeAliasAddrCue( asw->mod, addr, cueh1 ) != SR_NONE &&
        DeAliasAddrCue( asw->mod, asw->dotaddr, cueh2 ) != SR_NONE ) {
        if( DIPCueMod( cueh1 )    == DIPCueMod( cueh2 ) &&
            DIPCueFileId( cueh1 ) == DIPCueFileId( cueh2 ) &&
            DIPCueLine( cueh1 )   == DIPCueLine( cueh2 ) ) {
            return;
        }
    }
    WndNoSelect( wnd );
    row = AsmAddrRow( wnd, addr );
    if( row == WndRows( wnd ) ) {
        AsmSetFirst( wnd, addr, true );
        row = AsmAddrRow( wnd, addr );
        WndDirtyCurr( wnd );
        WndNewCurrent( wnd, row, PIECE_CURRENT );
        WndSetRepaint( wnd );
        row = 0;
    } else {
        WndDirtyCurr( wnd );
        WndNewCurrent( wnd, row, PIECE_CURRENT );
    }
    AsmSetDotAddr( wnd, addr );
}

a_window AsmWndFind( a_window wnd, address addr, bool track )
{
    a_window    nwnd;

    if( wnd == NULL ) {
        nwnd = DoWndAsmOpen( addr, track );
    } else {
        WndRestoreToFront( wnd );
        nwnd = wnd;
    }
    AsmMoveDot( nwnd, addr );
    return( nwnd );
}


void    AsmFreeSrc( a_window wnd )
{
    if( wnd != NULL ) {
        WndAsm( wnd )->src = NULL;
    }
}

#ifdef DEADCODE
bool    AsmIsTracking( a_window wnd )
{
    return( WndAsm( wnd )->track );
}
#endif

static  void    AsmModify( a_window wnd, wnd_row row, wnd_piece piece )
{
    asm_window  *asw;
    address     addr;
    mad_radix   old_radix;

    asw = WndAsm( wnd );
    old_radix = NewCurrRadix( asw->hex ? 16 : 10 );
    addr = asw->ins[row].addr;
    if( piece == PIECE_BREAK ) {
        asw->toggled_break = ( (UpdateFlags & UP_BREAK_CHANGE) == 0 );
        ToggleBreak( addr );
    } else {
        WndFirstMenuItem( wnd, row, piece );
    }
    WndRowDirty( wnd, row );
    NewCurrRadix( old_radix );
}


static void AsmNotify( a_window wnd, wnd_row row, wnd_piece piece )
{
    asm_window  *asw;
    address     addr;

    /* unused parameters */ (void)piece;

    asw = WndAsm( wnd );
    if( wnd != WndFindActive() )
        return;
    if( row < 0 )
        return;
    addr = asw->ins[row].addr;
    AsmSetDotAddr( wnd, addr );
}


bool AsmOpenGadget( a_window wnd, wnd_line_piece *line, mod_handle mod )
{
    a_window    cwnd;

    for( cwnd = WndNext( NULL ); cwnd != NULL; cwnd = WndNext( cwnd ) ) {
        if( WndClass( cwnd ) != WND_ASSEMBLY )
            continue;
        if( mod == WndAsm( cwnd )->mod ) {
            if( line != NULL )
                SetGadgetLine( wnd, line, GADGET_OPEN_ASSEMBLY );
            return( true );
        }
    }
    if( line != NULL )
        SetGadgetLine( wnd, line, GADGET_CLOSED_ASSEMBLY );
    return( false );
}


static void     AsmMenuItem( a_window wnd, gui_ctl_id id, wnd_row row, wnd_piece piece )
{
    address     addr;
    asm_window  *asw;
    mod_handle  mod;
    bool        has_popitem;
    char        buff[TXT_LEN];
    mad_radix   old_radix;
    int         i;
    unsigned    bit;

    /* unused parameters */ (void)piece;

    asw = WndAsm( wnd );
    if( row < 0 ) {
        addr = NilAddr;
    } else {
        addr = asw->ins[row].addr;
    }
    old_radix = NewCurrRadix( asw->hex ? 16 : 10 );
    switch( id ) {
    case MENU_INITIALIZE:
        if( IS_NIL_ADDR( addr ) ) {
            WndMenuGrayAll( wnd );
        } else {
            WndMenuEnableAll( wnd );
        }
        WndMenuEnable( wnd, MENU_ASM_NEW_ADDRESS, true );
        WndMenuEnable( wnd, MENU_ASM_SHOW_MODULE, true );
        WndMenuEnable( wnd, MENU_ASM_SHOW, true );
        WndMenuEnable( wnd, MENU_ASM_SOURCE, HasLineInfo( addr ) );
        WndMenuCheck( wnd, MENU_ASM_NOSOURCE, !asw->source );
        WndMenuEnable( wnd, MENU_ASM_FUNCTIONS, asw->mod != NO_MOD );
        WndMenuCheck( wnd, MENU_ASM_HEX, asw->hex );
        has_popitem = ( *WndPopItem( wnd ) != NULLCHAR );
        WndMenuEnable( wnd, MENU_ASM_INSPECT, has_popitem );
        WndMenuEnable( wnd, MENU_ASM_STEP_INTO, has_popitem );
        WndMenuEnable( wnd, MENU_ASM_BREAK, has_popitem );
        WndMenuEnable( wnd, MENU_ASM_RUN, !IS_NIL_ADDR( addr ) );
        WndMenuEnable( wnd, MENU_ASM_SKIP_TO_CURSOR, !IS_NIL_ADDR( addr ) );
        bit = MADDisasmToggle( 0, 0 );
        for( i = 0; i < asw->num_toggles; ++i ) {
            WndMenuCheck( wnd, MENU_ASM_TOGGLES + i, ( bit & 1 ) != 0 );
            bit >>= 1;
        }
        break;
    case MENU_ASM_RUN:
        GoToAddr( addr );
        break;
    case MENU_ASM_SKIP_TO_CURSOR:
        SkipToAddr( addr );
        break;
    case MENU_ASM_HOME:
        GoHome();
        break;
    case MENU_ASM_HEX:
        asw->hex = !asw->hex;
        NewCurrRadix( old_radix );
        AsmResize( wnd );
        WndZapped( wnd );
        break;
    case MENU_ASM_INSPECT:
        StrCopy( WndPopItem( wnd ), buff );
        if( asw->ins[row].line != 0 ) {
            if( WndEvalInspectExpr( buff, false ) ) { // eval in asm window's radix
                NewCurrRadix( old_radix );
                WndInspectExprSP( buff );       // open in default radix
            }
        } else {
            if( MADDisasmInspectAddr( buff, strlen( buff ), CurrRadix, &DbgRegs->mr, &addr ) == MS_OK ) {
                PushAddr( addr );
                NewCurrRadix( old_radix );
                WndInspectExprSP( buff );   // open in default radix
            }
        }
        break;
    case MENU_ASM_SOURCE:
        SrcWndFind( asw->src, addr, asw->track );
        break;
    case MENU_ASM_NOSOURCE:
        asw->source = !asw->source;
        NewCurrRadix( old_radix );
        AsmResize( wnd );
        WndZapped( wnd );
        break;
    case MENU_ASM_BREAK:
        StrCopy( WndPopItem( wnd ), buff );
        if( asw->ins[row].line != 0 ) {
            BreakOnSelected( buff );
        } else {
            if( MADDisasmInspectAddr( buff, strlen( buff ), CurrRadix, &DbgRegs->mr, &addr ) == MS_OK ) {
                PushAddr( addr );
                BreakOnExprSP( buff );
            }
        }
        break;
    case MENU_ASM_STEP_INTO:
        StrCopy( WndPopItem( wnd ), buff );
        if( asw->ins[row].line != 0 ) {
            StepIntoFunction( buff );
        } else {
            if( MADDisasmInspectAddr( buff, strlen( buff ), CurrRadix, &DbgRegs->mr, &addr ) == MS_OK ) {
                GoToAddr( addr );
            }
        }
        break;
    case MENU_ASM_FUNCTIONS:
        WndFuncInspect( asw->mod );
        break;
    case MENU_ASM_SHOW_MODULE:
        mod = asw->mod;
        if( DlgModName( LIT_DUI( New_Module ), &mod ) ) {
            WndAsmInspect( ModFirstAddr( mod ) );
        }
        break;
    case MENU_ASM_NEW_ADDRESS:
        if( DlgCodeAddr( LIT_DUI( New_Addr ), &addr ) ) {
            WndAsmInspect( addr );
        }
        break;
    default:
        bit = 1 << ( id - MENU_ASM_TOGGLES );
        MADDisasmToggle( bit, bit );
        NewCurrRadix( old_radix );
        AsmResize( wnd );
        WndZapped( wnd );
    }
    NewCurrRadix( old_radix );
}


static int AsmScroll( a_window wnd, int lines )
{
    address             addr;
    int                 moved;
    asm_window          *asw;
    bool                use_first_source;
    mad_disasm_data     *dd;
    DIPHDL( cue, cueh );

    asw = WndAsm( wnd );
    _AllocA( dd, asw->ddsize );
    addr = asw->ins[0].addr;
    if( lines == -1 && asw->ins[0].line == 0 &&
        ExactCueAt( asw, addr, cueh ) ) {
        moved = -1;
        AsmSetFirst( wnd, addr, true );
    } else if( lines < 0 ) {
        if( -lines > WndRows( wnd ) )
            return( 0 );
        moved = 0;
        use_first_source = true;
        do {
            if( MADDisasm( dd, &addr, -1 ) != MS_OK )
                break;
            addr.mach.offset -= MADDisasmInsSize( dd );
            if( ExactCueAt( asw, addr, cueh ) ) {
                ++lines;
                --moved;
                use_first_source = false;
                if( lines >= 0 ) {
                    break;
                }
            }
            --moved;
            ++lines;
            use_first_source = true;
        } while( lines < 0 );
        AsmSetFirst( wnd, addr, use_first_source );
    } else {
        if( lines >= WndRows( wnd ) )
            return( 0 );
        AsmSetFirst( wnd, asw->ins[lines].addr, asw->ins[lines].line != 0 );
        moved = lines;
    }
    WndFixedThumb( wnd );
    return( moved );
}


static  void    AsmBegPaint( a_window wnd, wnd_row row, int num )
{
    asm_window  *asw;

    /* unused parameters */ (void)row; (void)num;

    asw = WndAsm( wnd );
    InitCache( asw->ins[0].addr, WndRows( wnd ) * AVG_INS_SIZE );
}


static  void    AsmEndPaint( a_window wnd, wnd_row row, int num )
{
    /* unused parameters */ (void)wnd; (void)row; (void)num;

    FiniCache();
}


static  void    DoDisAsm( asm_window *asw, address addr )
{
    if( IS_NIL_ADDR( addr ) || AddrComp( asw->cache_addr, addr ) != 0 ) {
        asw->ins_end = 0;
        asw->cache_addr = addr;
        MADDisasm( asw->cache_dd, &addr, 0 );
    }
}


static void AsmNewSource( asm_window *asw, cue_handle *cueh )
{
    if( asw->viewhndl != NULL )
        FDoneSource( asw->viewhndl );
    asw->viewhndl = NULL;
    if( cueh != NULL ) {
        asw->viewhndl = OpenSrcFile( cueh );
        if( asw->viewhndl != NULL ) {
            asw->src_list.mod = DIPCueMod( cueh );
            asw->src_list.file_id = DIPCueFileId( cueh );
        }
    } else {
        asw->src_list.mod = NO_MOD;
    }
}

static  bool    AsmGetLine( a_window wnd, wnd_row row, wnd_piece piece, wnd_line_piece *line )
{
    address     addr;
    asm_window  *asw;
    bool        ret;
    int         indent;
    bool        curr;
    unsigned    src_line;
    size_t      len;
    bool        rc;
    mad_radix   old_radix;
    char        buff[TXT_LEN];
    mad_disasm_control  ctrl;
    DIPHDL( cue, cueh );

    asw = WndAsm( wnd );
    if( row < 0 ) {
        row += TITLE_SIZE;
        if( row == 0 ) {
            old_radix = NewCurrRadix( asw->hex ? 16 : 10 );
            line->text = TxtBuff;
            if( IS_NIL_ADDR( asw->dotaddr ) ) {
                addr = asw->ins[0].addr;
            } else {
                addr = asw->dotaddr;
            }
            AddrToString( &addr, MAF_FULL, buff, sizeof( buff ) );
            line->tabstop = false;
            rc = false;
            line->indent = MaxGadgetLength;
            switch( piece ) {
            case 0:
                StrCopy( buff, TxtBuff );
                rc = true;
                break;
            case 1:
                line->indent += WndExtentX( wnd, buff ) + 2 * WndMidCharX( wnd );
                StrAddr( &addr, TxtBuff, TXT_LEN );
                if( strcmp( buff, TxtBuff ) != 0 )
                    rc = true;
                break;
            }
            NewCurrRadix( old_radix );
            return( rc );
#if 0
        } else if( row == 1 ) {
            if( piece != 0 )
                return( false );
            SetUnderLine( wnd, line );
            return( true );
#endif
        } else {
            return( false );
        }
    }
    if( row >= WndRows( wnd ) )
        return( false );
    old_radix = NewCurrRadix( asw->hex ? 16 : 10 );
    ret = true;
    addr = asw->ins[row].addr;
    src_line = asw->ins[row].line;
    line->text = TxtBuff;
    StrCopy( " ", TxtBuff );
    curr = !IS_NIL_ADDR(addr) && AddrComp(addr, asw->active) == 0 && src_line == 0;
    line->tabstop = true;
    if( curr )
        line->attr = WND_STANDOUT;
    switch( piece ) {
    case PIECE_BREAK:
        line->tabstop = false;
        line->extent = WND_NO_EXTEND;
        if( src_line == 0 )
            FileBreakGadget( wnd, line, curr, FindBreak( addr ) );
        break;
    case PIECE_ADDRESS:
        if( src_line != 0 ) {
            line->text = TxtBuff;
            if( DeAliasAddrCue( NO_MOD, addr, cueh ) != SR_NONE ) {
                if( DIPCueMod( cueh ) != asw->src_list.mod
                 || DIPCueFileId( cueh ) != asw->src_list.file_id ) {
                    AsmNewSource( asw, cueh );
                }
                Format( TxtBuff, LIT_DUI( No_Source_Line ), src_line );
                if( asw->viewhndl != NULL ) {
                    len = FReadLine( asw->viewhndl, src_line, 0, TxtBuff, TXT_LEN );
                    if( len == FREADLINE_ERROR )
                        len = 0;
                    TxtBuff[len] = NULLCHAR;
                }
            }
        } else {
            AddrToString( &addr, MAF_OFFSET, TxtBuff, TXT_LEN );
        }
        line->indent = MaxGadgetLength;
        break;
    case PIECE_BRANCH_INDICATOR:
        if( src_line != 0 ) {
            ret = false;
            break;
        }
        line->text = " ";
        DoDisAsm( asw, addr );
        if( AddrComp( Context.execution, addr ) == 0 ) {
            ctrl = MADDisasmControl( asw->cache_dd, &DbgRegs->mr );
            if( ctrl & MDC_CONDITIONAL ) {
                switch( ctrl & MDC_TAKEN_MASK ) {
                case MDC_TAKEN_BACK:
#ifdef EXPERIMENTAL
                    //MAD: can be more explicit about where things are going
                    line->text = "\x18";
                    break;
#endif
                case MDC_TAKEN_FORWARD:
#ifdef EXPERIMENTAL
                    //MAD: can be more explicit about where things are going
                    line->text = "\x19";
                    break;
#endif
                case MDC_TAKEN:
                    line->text = "*";
                    break;
                }
            }
        }
        line->indent = asw->address_end;
        break;
    case PIECE_OPCODE:
        if( src_line != 0 ) {
            ret = false;
            break;
        }
        DoDisAsm( asw, addr );
        MADDisasmFormat( asw->cache_dd, MDP_INSTRUCTION, CurrRadix, TxtBuff, TXT_LEN );
        line->indent = asw->address_end + WndMaxCharX( wnd );
        break;
    case PIECE_OPERANDS:
        if( src_line != 0 ) {
            ret = false;
            break;
        }
        DoDisAsm( asw, addr );
        MADDisasmFormat( asw->cache_dd, MDP_OPERANDS, CurrRadix, TxtBuff, TXT_LEN );
        line->indent = asw->address_end + (MADDisasmNameMax()+1) * WndAvgCharX( wnd );
        asw->ins_end = line->indent + WndExtentX( wnd, line->text );
        break;
    case PIECE_MEMREF:
        if( src_line != 0 ) {
            ret = false;
            break;
        }
        DoDisAsm( asw, addr );
        if( asw->ins_end == 0 ) {
            MADDisasmFormat( asw->cache_dd, MDP_ALL, CurrRadix, TxtBuff, TXT_LEN );
            asw->ins_end = asw->address_end + WndExtentX( wnd, TxtBuff );
        }
        if( InsMemRef( asw->cache_dd ) ) {
            indent = WndWidth( wnd ) - WndExtentX( wnd, TxtBuff );
            if( indent < asw->ins_end ) {
                line->indent = asw->ins_end + WndMidCharX( wnd );
            } else {
                line->indent = indent;
            }
        } else {
            ret = false;
        }
        break;
    default:
        ret = false;
    }
    NewCurrRadix( old_radix );
    return( ret );
}


static void AsmTrack( a_window wnd, address ip )
{
    int         row;
    int         slack;
    asm_window  *asw;
    address     old_active;
    wnd_row     curr_row;
    wnd_piece   curr_piece;

    asw = WndAsm( wnd );
    if( AddrComp( ip, asw->active ) != 0 ) {
        WndGetCurrent( wnd, &curr_row, &curr_piece );
        WndNoCurrent( wnd );
        if( curr_row != WND_NO_ROW )
            WndRowDirty( wnd, curr_row );
        row = AsmAddrRow( wnd, ip );
        if( row == WndRows( wnd ) ) {
            AsmSetFirst( wnd, ip, true );
            WndZapped( wnd );
            row = AsmAddrRow( wnd, ip );
        } else {
            old_active = asw->active;
            asw->active = NilAddr;
            slack = WndRows( wnd ) / 4;
            if( slack > 2 )
                slack = 2;
            if( row >= WndRows( wnd ) - slack ) {
                WndRowDirtyImmed( wnd, AsmAddrRow( wnd, old_active ) );
                WndVScroll( wnd, WndRows( wnd ) - 2 * slack );
            } else {
                WndRowDirty( wnd, AsmAddrRow( wnd, old_active ) );
            }
            row = AsmAddrRow( wnd, ip );
            WndRowDirty( wnd, row );
        }
        WndNewCurrent( wnd, row, PIECE_CURRENT );
        WndRowDirty( wnd, row );
        AsmSetDotAddr( wnd, ip );
        asw->active = ip;
    } else {
        AsmMoveDot( wnd, ip );
        WndRowDirty( wnd, AsmAddrRow( wnd, ip ) );
    }
}


static  void    AsmNewIP( a_window wnd )
{
    asm_window          *asw;
    address             ip;
    mad_type_handle     mth;

    asw = WndAsm( wnd );
    ip = Context.execution;
    if( asw->track ) {
        AsmTrack( wnd, ip );
    } else {
        asw->active = ip;
    }
    WndRowDirty( wnd, -TITLE_SIZE );
    mth = GetMADTypeHandleDefaultAt( ip, MTK_ADDRESS );
    if( mth != asw->def_addr ) {
        asw->def_addr = mth;
        AsmResize( wnd );
    }
}

static void     AsmRefresh( a_window wnd )
{
    asm_window          *asw;
    unsigned            new_size;
    mad_disasm_data     *new;

    asw = WndAsm( wnd );
    if( UpdateFlags & UP_MAD_CHANGE ) {
        /* _have_ to check this one first */
        WndZapped( wnd );
        new_size = MADDisasmDataSize();
        if( new_size > asw->ddsize ) {
            new = asw->cache_dd;
            _Realloc( new, new_size );
            if( new == NULL ) {
                ReportMADFailure( MS_NO_MEM );
            } else {
                asw->cache_dd = new;
                asw->ddsize = new_size;
            }
        }
    }
    if( UpdateFlags & UP_ASM_RESIZE ) {
        AsmResize( wnd );
        WndZapped( wnd );
    }
    if( UpdateFlags & UP_NEW_PROGRAM ) {
        AsmSetFirst( wnd, NilAddr, true );
        asw->active = NilAddr;
        AsmSetDotAddr( wnd, NilAddr );
        asw->cache_addr = NilAddr;
        CalcAddrLen( wnd, NilAddr );
        AsmNewSource( asw, NULL );
    }
    if( UpdateFlags & (UP_SYM_CHANGE | UP_NEW_SRC) ) {
        asw->mod = NO_MOD;
        AsmNewSource( asw, NULL );
        AsmNewIP( wnd );
        WndZapped( wnd );
    } else if( UpdateFlags & (UP_STACKPOS_CHANGE | UP_CSIP_CHANGE) ) {
        AsmNewIP( wnd );
    } else if( UpdateFlags & (UP_RADIX_CHANGE) ) {
        WndZapped( wnd );
    } else if( UpdateFlags & UP_BREAK_CHANGE ) {
        if( asw->toggled_break ) {
            asw->toggled_break = false;
        } else {
            WndSetRepaint( wnd );
        }
    }
}


static  void    AsmFini( asm_window *asw )
{
    AsmNewSource( asw, NULL );
    _Free( asw->cache_dd );
    WndFree( asw->ins );
    WndFree( asw );
}

static  void    AsmInit( a_window wnd )
{
    asm_window  *asw;
    int         size;

    asw = WndAsm( wnd );
    size = WndRows( wnd );
    if( size <= 0 )
        size = 1;
    asw->ins = WndAlloc( size * sizeof( *asw->ins ) );
    memset( asw->ins, 0, size * sizeof( *asw->ins ) );
    asw->ins_size = size;
    if( asw->ins == NULL ) {
        WndClose( wnd );
        WndNoMemory();
    }
    asw->num_toggles = 0;
    asw->source = _IsOn( SW_ASM_SOURCE );
    asw->hex = _IsOn( SW_ASM_HEX );
    AsmSetFirst( wnd, asw->active, true ); // hidden here by Open
    asw->viewhndl = NULL;
    AsmRefresh( wnd );
    asw->src = NULL;
    asw->mod = NO_MOD;
    AsmSetDotAddr( wnd, asw->active );
    asw->active = NilAddr;
    asw->src_list.mod = NO_MOD;
    asw->src_list.file_id = 0;
    WndFixedThumb( wnd );
    WndSetIDChars( wnd, "@_$:[]+-*" );
    CalcAddrLen( wnd, Context.execution );
    WndZapped( wnd );
}

static bool AsmWndEventProc( a_window wnd, gui_event gui_ev, void *parm )
{
    asm_window  *asw;

    /* unused parameters */ (void)parm;

    asw = WndAsm( wnd );
    switch( gui_ev ) {
    case GUI_NOW_ACTIVE:
        ActiveWindowLevel = LEVEL_ASM;
        if( IS_NIL_ADDR( asw->dotaddr ) )
            return( true );
        SetCodeDot( asw->dotaddr );
        SrcMoveDot( asw->src, asw->dotaddr );
        return( true );
    case GUI_RESIZE:
        AsmResize( wnd );
        return( true );
    case GUI_INIT_WINDOW:
        AsmInit( wnd );
        AsmNewIP( wnd );
        DbgUpdate( UP_OPEN_CHANGE );
        asw->popup = WndAppendToggles( MADDisasmToggleList(), &asw->num_toggles, AsmMenu, ArraySize( AsmMenu ), MENU_ASM_TOGGLES );
        WndSetPopUpMenu( wnd, ArraySize( AsmMenu ) + asw->num_toggles, asw->popup );
        return( true );
    case GUI_DESTROY :
        SrcFreeAsm( asw->src );
        WndDeleteToggles( asw->popup, ArraySize( AsmMenu ), asw->num_toggles );
        AsmFini( asw );
        DbgUpdate( UP_OPEN_CHANGE );
        return( true );
    }
    return( false );
}

static void DoAsmChangeOptions( a_window wnd )
{
    asm_window  *asw;

    asw = WndAsm( wnd );
    asw->hex = _IsOn( SW_ASM_HEX );
    asw->source = _IsOn( SW_ASM_SOURCE );
    AsmSetFirst( wnd, asw->ins[0].addr, asw->ins[0].line != 0 );
    WndZapped( wnd );
}

void AsmChangeOptions( void )
{
    WndForAllClass( WND_ASSEMBLY, DoAsmChangeOptions );
}

static bool ChkUpdate( void )
{
    return( UpdateFlags & (UP_MAD_CHANGE | UP_SYM_CHANGE | UP_NEW_PROGRAM | UP_NEW_SRC | UP_STACKPOS_CHANGE
            | UP_CSIP_CHANGE | UP_BREAK_CHANGE | UP_RADIX_CHANGE | UP_ASM_RESIZE) );
}

wnd_info AsmInfo = {
    AsmWndEventProc,
    AsmRefresh,
    AsmGetLine,
    AsmMenuItem,
    AsmScroll,
    AsmBegPaint,
    AsmEndPaint,
    AsmModify,
    NoNumRows,
    NoNextRow,
    AsmNotify,
    ChkUpdate,
    PopUp( AsmMenu )
};


a_window DoWndAsmOpen( address addr, bool track )
{
    asm_window  *asw;
    a_window    wnd;

    asw = WndMustAlloc( sizeof( asm_window ) );
    asw->ddsize = MADDisasmDataSize();
    _Alloc( asw->cache_dd, asw->ddsize );
    if( asw->cache_dd == NULL ) {
        WndFree( asw );
        return( NULL );
    }
    asw->active = addr;
    asw->track = false;
    asw->cache_addr = NilAddr;
    asw->dotaddr = NilAddr;
    asw->last_width = 0;
    wnd = DbgTitleWndCreate( LIT_DUI( WindowAssembly ), &AsmInfo, WND_ASSEMBLY, asw, &AsmIcon, TITLE_SIZE, false );
    if( wnd == NULL )
        return( wnd );
    asw->track = track;
    asw->def_addr = MAD_NIL_TYPE_HANDLE;
    AsmSetDotAddr( wnd, addr );
    AsmSetTitle( wnd );
    WndSetSwitches( wnd, WSW_LBUTTON_SELECTS | WSW_RBUTTON_SELECTS | WSW_CHAR_CURSOR | WSW_SUBWORD_SELECT );
    WndClrSwitches( wnd, WSW_HIGHLIGHT_CURRENT );
    SrcNewAsmNotify( wnd, asw->mod, asw->track );
    return( wnd );
}


a_window WndAsmOpen( void )
{
    address     addr;

    addr = GetCodeDot();
    if( IS_NIL_ADDR( addr ) ) {
        addr = Context.execution;
    }
    return( DoWndAsmOpen( addr, true ) );
}
