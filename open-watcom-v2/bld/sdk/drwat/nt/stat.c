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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "drwatcom.h"
#include <ctype.h>
#include <dos.h>
#include "wclbproc.h"
#include "segmap.rh"
#include "jdlg.h"
#include "srchmsg.h"
#include "madrtn.h"
#include "madcli.h"
#include "reglist.h"
#include "memwnd.h"


/* Local Window callback functions prototypes */
#ifndef __NT__
WINEXPORT INT_PTR CALLBACK SegMapDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
#endif
WINEXPORT INT_PTR CALLBACK StatDialogDlgProc( HWND hwnd, UINT msg,WPARAM  wparam, LPARAM lparam );

typedef struct{
    address curr_addr;
    int     reg_set_index;
} StatData;


void GetChildPos( HWND parent, HWND child, RECT *c_rect );

#define MAXRANGE 8192
static ExceptDlgInfo * StatGetExceptDlgInfo( HWND stat )
{
    HWND fault;

    fault = GetWindow( stat, GW_OWNER );
    return( FaultGetExceptDlgInfo( fault ) );
}


mad_registers * StatGetMadRegisters( HWND stat )
{
    ExceptDlgInfo * info;

    info = StatGetExceptDlgInfo( stat );
    return ( info->regs );
}

/*
 * DisplayAsmLines - display all assembler lines
 *      hwnd     - dialog handle
 *      paddr    - address to start dissassembly
 */
static void DisplayAsmLines( HWND hwnd, address *paddr )
{
    int         i;
    address     addr;
    address     flagaddr;
    char        buff[256];
    HWND        hscrl;
    DWORD       max;
    int         curr;
    mad_registers   *regs;

    addr = *paddr;
    regs = StatGetMadRegisters( hwnd );
    GetCurrAddr( &flagaddr, regs );

    for( i = STAT_DISASM_1;i <= STAT_DISASM_8; i++ ) {
        buff[0] = ' ';
        if( MADAddrComp( &addr, &flagaddr, MAF_FULL ) == 0 ) {
            buff[0] = '*';
        }

        Disassemble( &addr, buff+1, FALSE, 255 );
        SetDlgItemText( hwnd, i, buff );
    }

#ifdef __NT__
    max = max;
    curr = curr;
    hscrl = GetDlgItem( hwnd, STAT_SCROLL );
    SetScrollRange( hscrl, SB_CTL, 0, 2, FALSE );
    SetScrollPos( hscrl, SB_CTL, 1, TRUE );
#else
    max = GetASelectorLimit( paddr->mach.segment );
    if( max > MAXRANGE ) {
        curr = ( MAXRANGE * paddr->mach.offset ) / max;
        max = MAXRANGE;
    } else {
        curr = paddr->mach.offset;
    }
    hscrl = GetDlgItem( hwnd, STAT_SCROLL );
    SetScrollRange( hscrl, SB_CTL, 0, max, FALSE);
    SetScrollPos( hscrl, SB_CTL, curr, TRUE );
#endif
} /* DisplayAsmLines */

/*
 * ScrollAsmDisplay - move asm display in response to a scroll request
 */
static void ScrollAsmDisplay( HWND hwnd, WORD wparam, address *paddr )
{
    switch( wparam ) {
    case SB_PAGEDOWN:
        if( !InstructionFoward( 8, paddr ) ) {
            return;
        }
        break;
    case SB_PAGEUP:
        if( !InstructionBackward( 8, paddr ) ) {
            return;
        }
        break;
    case SB_LINEDOWN:
        if( !InstructionFoward(1, paddr)){
            return;
        }
        break;
    case SB_LINEUP:
        if( !InstructionBackward( 1, paddr ) ){
            return;
        }
        break;
    default:
        return;
    }
    DisplayAsmLines( hwnd, paddr );

} /* ScrollAsmDisplay */


#define TXT_LEN 256

static walk_result CreateAllRegLists( const mad_reg_set_data *data, void *_crld )
{
    CreateRegListData *crld = _crld;
    HWND        list;
    HWND        combo;
    HDC         dc;
    SIZE        size;
    char        *p;
    char        TxtBuff[TXT_LEN];
    unsigned    len;

    p = TxtBuff + MADCli( String )( MADRegSetName( data ), TxtBuff, TXT_LEN );
    *p++ = ' ';
    *p++ = '(';
    len = MADRegSetLevel( data, p, TXT_LEN - ( p - TxtBuff ) );
    if( len == 0 ) {
        p -= 2;
    } else {
        p += len;
        *p++ = ')';
    }
    *p++ = '\0';
    combo = GetDlgItem( crld->parent, STAT_REGISTER_COMBO );
    SendMessage( combo, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)TxtBuff );
    dc = GetDC( combo );
    GetTextExtentPoint( dc, TxtBuff, strlen( TxtBuff ), &size );
    ReleaseDC( combo, dc );
    if( crld->max_len < size.cx )
        crld->max_len = size.cx;
    crld->reg_set = (mad_reg_set_data *)data;
    list=CreateRegList( crld );

    if( crld->index != 0 ) {
        ShowWindow( list, SW_HIDE );
    }
    crld->index++;
    return( WR_CONTINUE );
}

static walk_result UpdateRegList( const mad_reg_set_data *data, void *_cmp)
{
    CreateRegListData *cmp = _cmp;

    if( data == cmp->reg_set ) {
        SendDlgItemMessage( cmp->parent, cmp->index, UPDATE_REG_LIST, 0, 0 );
        return ( WR_STOP );
    }
    cmp->index++;
    return( WR_CONTINUE );
}

/*
 * InitStatDialog
 */
static void InitStatDialog( HWND hwnd )
{
    int                 i;
    char                buff[256];
    ExceptDlgInfo       *info;
    syminfo             si;
    CreateRegListData   data;
    StatData            *statdata;
    RECT                c_rect;
    HWND                combo;

    info = StatGetExceptDlgInfo( hwnd );
    StatHdl = hwnd;
    statdata = MemAlloc( sizeof( StatData ) );

    /*
     * fill in source information
     */
    GetCurrAddr( &( statdata->curr_addr ), info->regs );
    if( FindWatSymbol( &( statdata->curr_addr ), &si, true ) ) {
        RCsprintf( buff, STR_SRC_INFO_FMT, si.linenum, si.filename );
        StatShowSymbols = true;
        CheckDlgButton( hwnd, STAT_SYMBOLS, ( StatShowSymbols ) ? BST_CHECKED : BST_UNCHECKED );
    } else {
        RCsprintf( buff, STR_N_A );
        StatShowSymbols = false;
        EnableWindow( GetDlgItem( hwnd, STAT_SYMBOLS ), FALSE );
    }
    SetDlgMonoFont( hwnd, STAT_SRC_INFO );
    SetDlgItemText( hwnd, STAT_SRC_INFO, buff );

#ifdef __NT__
    {
        ProcStats       procinfo;
        HWND            button;
        if( GetProcessInfo( info->procinfo->procid, &procinfo ) ) {
            RCsprintf( buff, STR_STATUS_4_PROC_X, info->procinfo->procid, procinfo.name );
            SetWindowText( hwnd, buff );
        }
        CopyRCString( STR_VIEW_MEM_HT_KEY, buff, sizeof( buff ) );
        SetDlgItemText( hwnd, STAT_SEG_MAP, buff );
        button = GetDlgItem( hwnd, STAT_STACK_TRACE );
        ShowWindow( button, SW_HIDE );
    }
#endif
    InstructionBackward( 2, &( statdata->curr_addr ) );
    for( i = STAT_DISASM_1;i <= STAT_DISASM_8; i++ ) {
        SetDlgCourierFont( hwnd, i );
    }
    DisplayAsmLines( hwnd, &( statdata->curr_addr ) );
    data.index = 0;
    data.parent = hwnd;
    data.max_len = 0;
    combo = GetDlgItem( hwnd, STAT_REGISTER_COMBO );
    MADRegSetWalk( MTK_ALL, CreateAllRegLists, &data );
    if( data.index == 1 ) {
        SendMessage( combo, CB_GETLBTEXT, (WPARAM)0, (LPARAM)(LPSTR)buff );
        SetDlgItemText( hwnd, STAT_REGISTER_SET, buff );
        DestroyWindow( combo );
    } else {
        GetChildPos( hwnd, combo, &c_rect );
        SendMessage( combo, CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
        c_rect.right += data.max_len;
        c_rect.bottom += SendMessage( combo, CB_GETITEMHEIGHT, 0, 0 ) * ( data.index + 1);
        MoveWindow( combo, c_rect.left, c_rect.top, c_rect.right, c_rect.bottom, FALSE );
    }
    SetFocus( GetDlgItem( hwnd, IDOK ) );
    statdata->reg_set_index = 0;
    SET_DLGDATA( hwnd, statdata );
} /* InitStatDialog */

#ifndef __NT__
WINEXPORT INT_PTR CALLBACK SegMapDlgProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    char        buff[128];
    WORD        i;
    WORD        seg;
    bool        ret;

    lparam = lparam;

    ret = false;

    switch( msg ) {
    case WM_INITDIALOG:
        /*
         * fill out segment mappings
         */
        SetDlgCourierFont( hwnd, SEGMAP_LIST );
        SendDlgItemMessage( hwnd, SEGMAP_LIST, LB_RESETCONTENT, 0, 0L );
        for( i = 0; i <= 1024; i++ ) {
            seg = NumToAddr( DTTaskEntry.hModule, i );
            if( seg != 0 ) {
                sprintf( buff,"%04d->%04x", i, seg );
                SendDlgItemMessage( hwnd, SEGMAP_LIST, LB_ADDSTRING, 0, (LPARAM)(LPCSTR)buff );
            }
        }
        ret = true;
        break;
    case WM_COMMAND:
        switch( wparam ) {
        case IDOK:
            EndDialog( hwnd, 0 );
            ret = true;
            break;
        case SEGMAP_LIST:
            if( HIWORD( lparam ) == LBN_DBLCLK ) {

                GLOBALENTRY     ge;
                char            str[100];
                int             sel;

                sel = (int)SendDlgItemMessage( hwnd, SEGMAP_LIST, LB_GETCURSEL, 0, 0L );
                SendDlgItemMessage( hwnd, SEGMAP_LIST, LB_GETTEXT, sel, (LPARAM)(LPSTR)str );
                str[4] = '\0';
                seg = atoi( str );
                if( DoGlobalEntryModule( &ge, DTTaskEntry.hModule, seg ) ) {
                    DispMem( Instance, hwnd, ge.hBlock, (ge.dwSize == 1) );
                }
            }
            ret = true;
            break;
        break;
    }
    return( ret );
}
#endif
static void SwitchRegSets( HWND hwnd, StatData *statdata )
{
    int         old_index;
    int         new_index;

    new_index = (int)SendDlgItemMessage( hwnd, STAT_REGISTER_COMBO, CB_GETCURSEL, 1, 0L );
    old_index = statdata->reg_set_index;
    if( old_index == new_index ) {
        return;
    }
    SendDlgItemMessage( hwnd, old_index + REG_LIST_FIRST, HIDE_REG_LIST, (WPARAM)0, (LPARAM)0 );
    SendDlgItemMessage( hwnd, new_index + REG_LIST_FIRST, UNHIDE_REG_LIST, (WPARAM)0, (LPARAM)0 );
    statdata->reg_set_index = new_index;
}

static void DoUpdateWalk( HWND hwnd, mad_reg_set_data * reg_set )
{
    CreateRegListData data;

    data.index = REG_LIST_FIRST;
    data.parent = hwnd;
    data.reg_set = reg_set;
    MADRegSetWalk( MTK_ALL, UpdateRegList, &data );
}

/*
 * StatDialogDlgProc - show task status
 */
WINEXPORT INT_PTR CALLBACK StatDialogDlgProc( HWND hwnd, UINT msg,WPARAM  wparam, LPARAM lparam )
{
    WORD        cmd;
    StatData    *statdata;
#ifndef __NT__
    DLGPROC     dlgproc;
#endif
    bool        ret;

    ret = false;
    statdata = (StatData *)GET_DLGDATA( hwnd );
    switch( msg ) {
    case STAT_FOREGROUND:
#ifdef __NT__
        SetForegroundWindow( hwnd );
#else
        SetActiveWindow( hwnd );
#endif
        break;
    case WM_INITDIALOG:
        InitStatDialog( hwnd );
        break;
    case STAT_MAD_NOTIFY:
        switch( wparam ) {
        case MNT_REDRAW_REG:
        case MNT_MODIFY_REG:
            DoUpdateWalk( hwnd, (mad_reg_set_data *)lparam );
            break;
        case MNT_MODIFY_IP:
        case MNT_REDRAW_DISASM:
            DisplayAsmLines( hwnd, &( statdata->curr_addr ) );
            break;
        }
        break;
    case WM_VSCROLL:
        ScrollAsmDisplay( hwnd, wparam, &( statdata->curr_addr ) );
        break;
    case WM_CLOSE:
        PostMessage( hwnd, WM_COMMAND, IDCANCEL, 0L );
        ret = true;
        break;
    case WM_DESTROY:
        StatHdl = NULL;
        MemFree( statdata );
        break;
    case WM_COMMAND:
        if( HIWORD( wparam ) == CBN_SELCHANGE ) {
            SwitchRegSets( hwnd, statdata );
            break;
        }
        cmd = LOWORD( wparam );
        switch( cmd ) {
#ifndef __NT__
        case STAT_SEG_MAP:
            dlgproc = MakeProcInstance_DLG( SegMapDlgProc, Instance );
            JDialogBox( Instance, "SEG_MAP_DLG", hwnd, dlgproc );
            FreeProcInstance_DLG( dlgproc );
            break;
        case STAT_STACK_TRACE:
            StartStackTraceDialog( hwnd );
            break;
#else
        case STAT_SEG_MAP:
            {
                HANDLE                  hdl;
                ExceptDlgInfo * info;

                info = StatGetExceptDlgInfo( hwnd );

                DuplicateHandle(
                            GetCurrentProcess(),
                            info->procinfo->prochdl,
                            GetCurrentProcess(),
                            &hdl,
                            0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS );
                WalkMemory( hwnd, hdl, info->procinfo->procid );
            }
            break;
#endif
        case STAT_SYMBOLS:
            StatShowSymbols = !StatShowSymbols;
            DisplayAsmLines( hwnd, &( statdata->curr_addr ) );
            break;
        case IDCANCEL:
            EndDialog( hwnd, 0 );
            ret = true;
            break;
        case IDOK:
            EndDialog( hwnd, 1 );
            ret = true;
            break;
        }
    }
    return( ret );

} /* StatDialogDlgProc */


/*
 * DoStatDialog - run the stat dialog
 */
int DoStatDialog( HWND hwnd )
{
    DLGPROC     dlgproc;
    INT_PTR     ret;

    dlgproc = MakeProcInstance_DLG( StatDialogDlgProc, Instance );
    ret = JDialogBox( Instance, "TASKSTATUS", hwnd, dlgproc );
    FreeProcInstance_DLG( dlgproc );
    return( ret );
} /* DoStatDialog */
