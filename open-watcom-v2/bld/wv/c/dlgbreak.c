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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgwind.h"
#include "dbgerr.h"
#include "guidlg.h"
#include "dlgbrk.h"
#include "strutil.h"
#include "madinter.h"
#include "dbgutil.h"
#include "trapglbl.h"
#include "dbgbrk.h"
#include "wndsys.h"
#include "dbgevent.h"
#include "dbgupdt.h"
#include "dlgexpr.h"
#include "dlgbreak.h"
#include "dlgscomp.h"


typedef struct {
    brkp            *bp;
    brkp            tmpbp;
    gui_ctl_id      last_edit;
    boolbit         brand_new       : 1;
    boolbit         cancel          : 1;
    boolbit         clear           : 1;
    boolbit         cmd_error       : 1;
    boolbit         cond_error      : 1;
} dlg_brk;

static  bool    GetAddr( dlg_brk *dlg, gui_window *gui )
{
    bool        ok;
    address     addr;
    char        *new;

    if( GUIIsChecked( gui, CTL_BRK_EXECUTE ) ) {
        UniqStrAddr( &dlg->tmpbp.loc.addr, TxtBuff, TXT_LEN ); // if it hasn't changed, don't bother cause we might get a disambiguator
        new = GUIGetText( gui, CTL_BRK_ADDRESS );
        if( new == NULL || strcmp( new, TxtBuff ) != 0 ) {
            ok = DlgGetCodeAddr( gui, CTL_BRK_ADDRESS, &addr );
        } else {
            ok = true;
            addr = dlg->tmpbp.loc.addr;
        }
        GUIMemFree( new );
    } else {
        ok = DlgGetDataAddr( gui, CTL_BRK_ADDRESS, &addr );
    }
    if( ok )
        SetPointAddr( &dlg->tmpbp, addr );
    return( ok );
}

static  bool    GetDlgStatus( dlg_brk *dlg, gui_window *gui )
{
    brkp        *bp;
    brkp        *tmp_bp;

    tmp_bp = &dlg->tmpbp;
    GUIDlgBuffGetText( gui, CTL_BRK_COUNTDOWN, TxtBuff, TXT_LEN );
    if( TxtBuff[0] == NULLCHAR ) {
        tmp_bp->countdown = 0;
        tmp_bp->initial_countdown = 0;
    } else if( !DlgGetLong( gui, CTL_BRK_COUNTDOWN, &tmp_bp->countdown ) ) {
        PrevError( TxtBuff );
        GUISetFocus( gui, CTL_BRK_COUNTDOWN );
        dlg->last_edit = CTL_BRK_COUNTDOWN;
        return( false );
    }
    tmp_bp->initial_countdown = tmp_bp->countdown;
    if( GUIIsChecked( gui, CTL_BRK_EXECUTE ) ) {
        tmp_bp->mth = BP_EXECUTE;
    } else if( GUIIsChecked( gui, CTL_BRK_BYTE ) ) {
        tmp_bp->mth = FindMADTypeHandle( MAS_MEMORY | MTK_INTEGER, 1 );
    } else if( GUIIsChecked( gui, CTL_BRK_WORD ) ) {
        tmp_bp->mth = FindMADTypeHandle( MAS_MEMORY | MTK_INTEGER, 2 );
    } else if( GUIIsChecked( gui, CTL_BRK_DWORD ) ) {
        tmp_bp->mth = FindMADTypeHandle( MAS_MEMORY | MTK_INTEGER, 4 );
    } else if( GUIIsChecked( gui, CTL_BRK_QWORD ) ) {
        tmp_bp->mth = FindMADTypeHandle( MAS_MEMORY | MTK_INTEGER, 8 );
    }
    if( !GetAddr( dlg, gui ) ) {
        PrevError( TxtBuff );
        GUISetFocus( gui, CTL_BRK_ADDRESS );
        dlg->last_edit = CTL_BRK_ADDRESS;
        return( false );
    }
    bp = FindBreak( tmp_bp->loc.addr );
    if( bp != NULL && bp != dlg->bp ) {
        ErrorRet( ERR_NONE, LIT_ENG( ERR_POINT_EXISTS ) );
        return( false );
    }
    GUIDlgBuffGetText( gui, CTL_BRK_CMD_LIST, TxtBuff, TXT_LEN );
    if( tmp_bp->cmds != NULL )
        FreeCmdList( tmp_bp->cmds );
    if( TxtBuff[0] != NULLCHAR ) {
        tmp_bp->cmds = AllocCmdList( TxtBuff, strlen( TxtBuff ) );
    } else {
        tmp_bp->cmds = NULL;
    }
    tmp_bp->condition = GUIGetText( gui, CTL_BRK_CONDITION );
    if( tmp_bp->condition != NULL )
        tmp_bp->status.b.use_condition = true;
    if( tmp_bp->cmds != NULL )
        tmp_bp->status.b.use_cmds = true;
    if( tmp_bp->initial_countdown != 0 )
        tmp_bp->status.b.use_countdown = true;
    tmp_bp->status.b.active = GUIIsChecked( gui, CTL_BRK_ACTIVE );
    tmp_bp->status.b.resume = GUIIsChecked( gui, CTL_BRK_RESUME );
    tmp_bp->status.b.silent = tmp_bp->status.b.resume;

    return( true );
}


static  void    SetDlgStatus( dlg_brk *dlg, gui_window *gui )
{
    brkp                *tmp_bp = &dlg->tmpbp;
    gui_ctl_id          id;
    mad_type_info       mti;

    if( !IS_NIL_ADDR( tmp_bp->loc.addr ) ) {
        UniqStrAddr( &tmp_bp->loc.addr, TxtBuff, TXT_LEN );
        GUISetText( gui, CTL_BRK_ADDRESS, TxtBuff );
    }
    GetBPText( &dlg->tmpbp, TxtBuff );
    GUISetText( gui, CTL_BRK_WHATIS, TxtBuff );
    if( tmp_bp->cmds != NULL ) {
        GUISetText( gui, CTL_BRK_CMD_LIST, tmp_bp->cmds->buff );
    } else {
        GUISetText( gui, CTL_BRK_CMD_LIST, NULL );
    }
    GUISetText( gui, CTL_BRK_CONDITION, tmp_bp->condition );
    if( tmp_bp->countdown == 0 ) {
        GUISetText( gui, CTL_BRK_COUNTDOWN, NULL );
    } else {
        DlgSetLong( gui, CTL_BRK_COUNTDOWN, tmp_bp->countdown );
    }
    DlgSetLong( gui, CTL_BRK_TOTAL, tmp_bp->total_hits );

    GUISetChecked( gui, CTL_BRK_ACTIVE, tmp_bp->status.b.active );
    GUISetChecked( gui, CTL_BRK_RESUME, tmp_bp->status.b.resume );

    if( IS_BP_EXECUTE( tmp_bp->mth ) ) {
        mti.b.bits = 0;
    } else {
        MADTypeInfo( tmp_bp->mth, &mti );
    }
    GUISetChecked( gui, CTL_BRK_EXECUTE, mti.b.bits == BYTES2BITS( 0 ) );
    GUISetChecked( gui, CTL_BRK_BYTE,    mti.b.bits == BYTES2BITS( 1 ) );
    GUISetChecked( gui, CTL_BRK_WORD,    mti.b.bits == BYTES2BITS( 2 ) );
    GUISetChecked( gui, CTL_BRK_DWORD,   mti.b.bits == BYTES2BITS( 4 ) );

    GUIEnableControl( gui, CTL_BRK_QWORD, Is8ByteBreakpointsSupported() );
    GUISetChecked( gui, CTL_BRK_QWORD,   mti.b.bits == BYTES2BITS( 8 ) );

    if( dlg->cmd_error ) {
        id = CTL_BRK_CMD_LIST;
    } else if( dlg->cond_error ) {
        id = CTL_BRK_CONDITION;
    } else {
        id = CTL_BRK_ADDRESS;
    }
    GUISetFocus( gui, id );
    dlg->last_edit = id;
}


static bool DlgEditField( gui_ctl_id id )
{
    switch( id ) {
    case CTL_BRK_ADDRESS:
    case CTL_BRK_CONDITION:
    case CTL_BRK_COUNTDOWN:
    case CTL_BRK_CMD_LIST:
        return( true );
    default:
        return( false );
    }
}


static bool BrkGUIEventProc( gui_window *gui, gui_event gui_ev, void *param )
{
    gui_ctl_id  id;
    gui_ctl_id  from;
    gui_ctl_id  to;
    dlg_brk     *dlg;
    brkp        *bp;
    brkp        saved;
    bool        ok;

    dlg = GUIGetExtra( gui );
    bp = dlg->bp;
    switch( gui_ev ) {
    case GUI_CONTROL_NOT_ACTIVE:
        GUI_GETID( param, from );
        GUIGetFocus( gui, &to );
        if( DlgEditField( to ) ) {
            dlg->last_edit = to;
        } else if( DlgEditField( from ) ) {
            dlg->last_edit = from;
        }
        break;
    case GUI_INIT_DIALOG:
        SetDlgStatus( dlg, gui );
        return( true );
    case GUI_CONTROL_CLICKED :
        GUI_GETID( param, id );
        switch( id ) {
        case CTL_BRK_EXECUTE:
        case CTL_BRK_BYTE:
        case CTL_BRK_WORD:
        case CTL_BRK_DWORD:
        case CTL_BRK_QWORD:
            return( true );
#if 0
        case CTL_BRK_GET_TOTAL:
            DlgSetLong( gui, CTL_BRK_COUNTDOWN, dlg->tmpbp.total_hits );
            return( true );
#endif
        case CTL_BRK_RESET:
            dlg->tmpbp.total_hits = 0;
            DlgSetLong( gui, CTL_BRK_TOTAL, dlg->tmpbp.total_hits );
            return( true );
        case CTL_BRK_OK:
            if( GetDlgStatus( dlg, gui ) ) {
                ok = true;
                if( dlg->brand_new ) {
                    if( !BrkCheckWatchLimit( dlg->tmpbp.loc.addr, dlg->tmpbp.mth ) ) {
                        ok = false;
                    }
                } else {
                    saved = *bp;
                    *bp = dlg->tmpbp;
                    if( !BrkCheckWatchLimit( NilAddr, BP_EXECUTE ) ) {
                        ok = false;
                    }
                    *bp = saved;
                }
                if( ok ) {
                    dlg->cancel = false;
                    GUICloseDialog( gui );
                }
            }
            return( true );
        case CTL_BRK_CLEAR:
            dlg->clear = true;
            GUICloseDialog( gui );
            return( true );
        case CTL_BRK_CANCEL:
            GUICloseDialog( gui );
            return( true );
        case CTL_BRK_SYMBOL:
            GUIGetFocus( gui, &from );
            if( from != CTL_BRK_SYMBOL ) {
                dlg->last_edit = from;
            }
            if( DlgEditField( dlg->last_edit ) ) {
                SymComplete( gui, dlg->last_edit );
            } else {
                WndMsgBox( LIT_DUI( No_text_to_complete ) );
            }
            return( true );
        }
        break;
    case GUI_DESTROY:
        if( dlg->brand_new ) {
            if( dlg->cancel || dlg->clear ) {
                RemovePoint( bp );
            } else {
                *bp = dlg->tmpbp;
                RecordNewPoint( bp );
            }
        } else {
            if( dlg->clear ) {
                SetRecord( true );
                RemovePoint( bp );
            } else if( !dlg->cancel ) {
                saved = *bp;
                *bp = dlg->tmpbp;
                RecordClearPoint( &saved );
                RecordNewPoint( bp );
            }
        }
        DbgUpdate( UP_BREAK_CHANGE );
        return( true );
    }
    return( false );
}


bool DlgBreak( address addr )
{
    brkp        *bp;
    dlg_brk     dlg;

    SetRecord( false );
    bp = FindBreak( addr );
    dlg.brand_new = false;
    if( bp == NULL ) {
        bp = AddBreak( addr );
        dlg.brand_new = true;
    }
    dlg.bp = bp;
    dlg.cancel = true;
    dlg.clear = false;
    dlg.cmd_error = false;
    dlg.cond_error = false;
    if( bp->status.b.expr_error ) {
        PrevError( bp->error );
        WndFree( bp->error );
        bp->error = NULL;
        bp->status.b.expr_error = false;
        dlg.cond_error = true;
    } else if( bp->status.b.cmd_error ) {
        bp->status.b.cmd_error = false;
        dlg.cmd_error = true;
    }
    dlg.tmpbp = *bp;
    CnvULongDec( bp->index, StrCopy( " ", StrCopy( LIT_DUI( DlgBreak ), TxtBuff ) ), TXT_LEN );
    ResDlgOpen( BrkGUIEventProc, &dlg, DIALOG_BREAK );
    SetRecord( true );
    return( !dlg.cancel );
}
