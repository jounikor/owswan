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
* Description:  User program step and trace support.
*
****************************************************************************/


#include "_srcmgt.h"
#include "dbgdata.h"
#include "dbgerr.h"
#include "dbglit.h"
#include "dbgrep.h"
#include "dbgmem.h"
#include "srcmgt.h"
#include "strutil.h"
#include "dbgscan.h"
#include "madinter.h"
#include "dbgutil.h"
#include "dbgsrc.h"
#include "dbgexec.h"
#include "dbgmain.h"
#include "dbgshow.h"
#include "dbgovl.h"
#include "dbgparse.h"
#include "dbgdot.h"
#include "dbgtrace.h"
#include "remcore.h"
#include "dbgmisc.h"
#include "removl.h"
#include "dipimp.h"
#include "dipinter.h"
#include "dbgreg.h"
#include "addarith.h"
#include "dbgevent.h"
#include "dbgset.h"
#include "dbgsetfg.h"


enum {
    TS_NONE,
    TS_ACTIVE,
    TS_POSTPONED,
    TS_REACTIVATED
};

typedef struct {
     address            brkpt;          /* address of DbgTmpBrk */
     address            watch_stack;    /* stack pointer at call */
     brk_status         userstat;       /* initial state of UserTmpBrk */
     debug_level        req_level;      /* trace level requested */
     debug_level        cur_level;      /* trace level we are in */
     address            oldaddr;        /* address where this trace started */
     int                type;           /* trace type */
     address            savecode;       /* old Code{Loc/Dot} */
     address            saveaddr;       /* original CS:IP */
     unsigned           state        : 3;   /* is there a trace active */
     boolbit            stop_on_call : 1;   /* we've returned -- stop on call */
     boolbit            stop_now     : 1;   /* did we execute a call since ^ set ?*/
     boolbit            trace_out    : 1;   /* want to trace out of a call */
     boolbit            doing_call   : 1;   /* are we doing a call instruction? */
     boolbit            in_thunk     : 1;   /* inside a THUNK routine */
     boolbit            in_dll_thunk : 1;   /* inside a DLL THUNK routine */
     boolbit            give_it_up   : 1;   /* inside a DLL THUNK routine */
     boolbit            unwinding    : 1;   /* unwinding from a recursive breakpoint ? */
     dtid_t             etid;           /* thread id that we're tracing */
     address            curraddr;       /* current address being traced */
     mad_disasm_control prev_control;   /* control information for prev ins */
     mad_disasm_control curr_control;   /* control information for curr ins */
     mad_trace_how      how;            /* how to trace this instruction */
     unsigned           ddsize;         /* size of a mad_disasm_data */
     unsigned           tdsize;         /* size of a mad_trace_data */
     mad_disasm_data    *dd;            /* disasm info for curr instr */
     mad_trace_data     *td;            /* trace info for curr instr */
} trace_state;

#define         KEEPGOING       0
#define         STOP            1
#define         PROCCMD         2

static trace_state      TraceState;

extern char             Opcode;
extern char             SecondByte;


extern int_16           GetDataWord( void );
extern long             GetDataLong( void );
extern void             GetCurrOpcode( void );
extern bool             SimIntr( char, unsigned int );
extern void             WndPmtNormal( void );

static const char LevelTab[] = {
    #define pick(t,e)   t "\0"
    #include "dbglevel.h"
    #undef pick
};

static const char TraceTab[] = {
    #define pick(t,e)   t "\0"
    #include "_dbgtrac.h"
    #undef pick
};


void            ResizeTraceData( void )
{
    void        *new;
    unsigned    size;

    size = MADDisasmDataSize();
    if( size > TraceState.ddsize ) {
        new = TraceState.dd;
        _Realloc( new, size );
        if( new == NULL ) {
            ReportMADFailure( MS_NO_MEM );
        } else {
            TraceState.dd = new;
            TraceState.ddsize = size;
        }
    }
    size = MADTraceSize();
    if( size > TraceState.tdsize ) {
        new = TraceState.td;
        _Realloc( new, size );
        if( new == NULL ) {
            ReportMADFailure( MS_NO_MEM );
        } else {
            TraceState.td = new;
            TraceState.tdsize = size;
        }
    }
}

static void TraceGetData( void )
{
    address     addr;

    addr = GetRegIP();
    if( AddrComp( TraceState.curraddr, addr ) != 0 ) {
        TraceState.curraddr = addr;
        MADDisasm( TraceState.dd, &addr, 0 );
        TraceState.prev_control = TraceState.curr_control;
        TraceState.curr_control = MADDisasmControl( TraceState.dd, &DbgRegs->mr );
    }
}

bool TraceStart( bool tracing )
{
    MADTraceInit( TraceState.td, &DbgRegs->mr );
    return( tracing );
}

mad_trace_how TraceHow( bool force_into )
{
    static const mad_trace_kind MTRKind[] = { MTRK_INTO, MTRK_OVER, MTRK_NEXT };
    mad_trace_kind      kind;
    mad_trace_how       how;

    TraceGetData();
    switch( TraceState.curr_control & MDC_TYPE_MASK ) {
    case MDC_CALL:
    case MDC_SYSCALL:
        TraceState.doing_call = true;
        break;
    default:
        TraceState.doing_call = false;
        break;
    }
    if( TraceState.give_it_up ) {
        DbgTmpBrk.status.b.active = false;
        TraceState.give_it_up = false;
        how = MTRH_BREAK;
        return( how );
    } else if( force_into ) {
        kind = MTRK_INTO;
    } else if( TraceState.trace_out ) {
        TraceState.trace_out = false;
        TraceState.in_dll_thunk = false;
        kind = MTRK_OUT;
    } else {
        kind = MTRKind[TraceState.type];
    }
    if( !force_into && DbgTmpBrk.status.b.active ) {
        how = MTRH_BREAK;
        _SwitchOn( SW_EXECUTE_LONG );
    } else {
        how = MADTraceOne( TraceState.td, TraceState.dd, kind,
                                    &DbgRegs->mr, &DbgTmpBrk.loc.addr );
    }
    switch( how ) {
    case MTRH_BREAK:
    case MTRH_STEPBREAK:
        DbgTmpBrk.status.b.active = true;
    }
    if( DbgTmpBrk.status.b.active ) {
        /* for recursion detection */
        if( !TraceState.unwinding ) {
            TraceState.watch_stack = GetRegSP();
        }
    }
    TraceState.how = how;
    return( how );
}

bool TraceSimulate( void )
{
    mad_status  ms;

    ReadDbgRegs();      /* only SP & IP are valid on entry */
    ms = MADTraceSimulate( TraceState.td, TraceState.dd,
        &DbgRegs->mr, &DbgRegs->mr );
    if( ms != MS_OK )
        return( false );
    WriteDbgRegs();
    return( true );
}

bool TraceModifications( MAD_MEMREF_WALKER *wk, void *d )
{
    switch( TraceState.how ) {
    case MTRH_SIMULATE:
    case MTRH_STEP:
    case MTRH_STEPBREAK:
        if( MADDisasmInsUndoable( TraceState.dd ) != MS_OK )
            return( false );
        if( MADDisasmMemRefWalk( TraceState.dd, wk, &DbgRegs->mr, d ) == WR_CONTINUE ) {
            return( true );
        }
        break;
    }
    return( false );
}

void TraceStop( bool tracing )
{
    /* unused parameters */ (void)tracing;

    MADTraceFini( TraceState.td );
}

void TraceKill( void )
{
    TraceState.state = TS_NONE;
}

static void TracePostponed( void )
{
    switch( TraceState.state ) {
    case TS_ACTIVE:
    case TS_REACTIVATED:
        TraceState.state = TS_POSTPONED;
        TraceState.brkpt = DbgTmpBrk.loc.addr;
    }
}

bool SourceStep( void )
{
    return( TraceState.cur_level == LEVEL_SOURCE );
}

/*
 * CheckTraceSourceStop - see if we want this trap because it ends a
 *                      a source level trace command
 */


static bool CheckTraceSourceStop( bool *have_source )
{
    DIPHDL( cue, cueh_line );
    DIPHDL( cue, cueh_oldline );
    DIPHDL( sym, sym );
    address                     line_addr;
    search_result               sr;
    sym_info                    info;
    void                        *viewhndl;

    *have_source = false;
    TraceState.in_thunk = false;
    sr = DeAliasAddrSym( NO_MOD, TraceState.curraddr, sym );
    if( sr != SR_NONE
      && DIPSymInfo( sym, NULL, &info ) == DS_OK
      && info.kind == SK_PROCEDURE
      && info.compiler ) {
        /* in a thunk */
        TraceState.in_thunk = true;
        return( false );
    }
    viewhndl = NULL;
    *have_source = DeAliasAddrCue( NO_MOD, TraceState.curraddr, cueh_line ) != SR_NONE;
    if( *have_source && _IsOn( SW_CHECK_SOURCE_EXISTS ) &&
        ( viewhndl = OpenSrcFile( cueh_line ) ) != NULL ) {
        if( viewhndl != NULL )
            FDoneSource( viewhndl );
        line_addr = DIPCueAddr( cueh_line );
        if( AddrComp( TraceState.oldaddr, line_addr ) != 0 ) {
            if( DeAliasAddrCue( NO_MOD, TraceState.oldaddr, cueh_oldline ) != SR_NONE
                && DIPCueLine( cueh_line ) == DIPCueLine( cueh_oldline )
                && DIPCueColumn( cueh_line ) == DIPCueColumn( cueh_oldline )
                && DIPCueFileId( cueh_line ) == DIPCueFileId( cueh_oldline )
                && DIPCueMod( cueh_line ) == DIPCueMod( cueh_oldline ) ) {
                /*
                    We've moved to a different starting address for the
                    cue, but all the source information is the same, so
                    update the current cue start address and keep on going.
                    (This can happen with C++ switch statements since
                    there's a jump in the middle of the switch code.)
                */
                TraceState.oldaddr = line_addr;
                return( false );
            }
            switch( TraceState.curr_control & MDC_TYPE_MASK ) {
            case MDC_RET:
            case MDC_SYSRET:
                break;
            default:
                return( true );
            }
            if( AddrComp( line_addr, TraceState.curraddr ) == 0 ) {
                return( true );
            }
            TraceState.oldaddr = line_addr;
            TraceState.stop_on_call = true;
        } else if( TraceState.stop_now ) {
            return( true );
        }
        return( false );
    }
    if( TraceState.req_level == LEVEL_MIX ) {
        if( sr == SR_NONE )
            return( true );
        if( !IsSupportRoutine( sym ) ) {
            return( true );
        }
    }
    return( false );
}

bool CheckForDLLThunk( void )
/***************************/
{
    address     next_ins;
    DIPHDL( cue, cueh_line );

    switch( TraceState.prev_control & MDC_TYPE_MASK ) {
    case MDC_CALL:
    case MDC_SYSCALL:
        break;
    default:
        return( false );
    }
    switch( TraceState.curr_control & (MDC_TYPE_MASK+MDC_CONDITIONAL_MASK) ) {
    case MDC_JUMP+MDC_UNCONDITIONAL:
        break;
    default:
        return( false );
    }
    ReadDbgRegs();      /* only SP & IP are valid on entry */
    switch( MADDisasmInsNext( TraceState.dd, &DbgRegs->mr, &next_ins ) ) {
    case MS_OK:
        return( DeAliasAddrCue( NO_MOD, next_ins, cueh_line ) == SR_EXACT );
    //NYI: this next case can be removed once all the MAD's are up to snuff
    case MS_ERR|MS_UNSUPPORTED:
        return( true );
    }
    return( false );
}


/*
 *  CheckTrace - check to see if we really want this trace trap
 */

unsigned TraceCheck( unsigned conditions )
{
    bool have_source;
    bool recursed;

    TraceGetData();
    if( DbgTmpBrk.status.b.hit && (conditions & COND_STOPPERS) ) {
        DbgTmpBrk.status.b.active = false;
        conditions &= ~COND_STOPPERS;
        conditions |= COND_TRACE;
    }
    if( !(conditions & COND_TRACE) )
        return( conditions );
    if( DbgTmpBrk.status.b.hit && TraceState.type == TRACE_OVER ) {
        recursed =  MADTraceHaveRecursed( TraceState.watch_stack, &DbgRegs->mr ) == MS_OK;
        if( _IsOn( SW_RECURSE_CHECK ) && recursed && ( TraceState.doing_call || TraceState.unwinding ) ) {
            /* we're down some levels in a recursive call -- want to unwind. */
            TraceState.unwinding = true;
            NullStatus( &DbgTmpBrk );
            DbgTmpBrk.status.b.active = true;
            return( conditions & ~COND_STOPPERS );
        }
        TraceState.unwinding = false;
        if( (conditions & COND_THREAD)
            && TraceState.etid != RemoteSetThread( 0 ) ) {
            /* stepped over a call and another thread ran into bp - keep going */
            NullStatus( &DbgTmpBrk );
            DbgTmpBrk.status.b.active = true;
            return( conditions & ~COND_STOPPERS );
        }
    }
    if( OvlSize != 0
        && (RemoteOvlTransAddr( &TraceState.curraddr ) || TransOvlRetAddr( &TraceState.curraddr, 0 )) ) {
        /* have traced into an overlay vector or overlay mgr return routine
            -- want to get overlay loaded and enter real routine */
        DbgTmpBrk.loc.addr = TraceState.curraddr;
        NullStatus( &DbgTmpBrk );
        DbgTmpBrk.status.b.active = true;
        return( conditions & ~COND_STOPPERS );
    }
    if( TraceState.cur_level == LEVEL_ASM )
        return( conditions | COND_TRACE );
    if( CheckTraceSourceStop( &have_source ) ) {
        if( CheckForDLLThunk() )
            return( conditions & ~COND_STOPPERS );
        return( conditions | COND_TRACE );
    }
    /*
      at this point, we know we're tracing source only!
    */
    if( TraceState.in_dll_thunk ) {
        if( TraceState.type == TRACE_INTO && !TraceState.in_thunk ) {
            TraceState.trace_out = true;
            if( !TraceState.unwinding ) {
                TraceState.watch_stack = GetRegSP();
            }
        }
        return( conditions & ~COND_STOPPERS );
    }
    NullStatus( &DbgTmpBrk );
    switch( TraceState.prev_control & MDC_TYPE_MASK ) {
    case MDC_RET:
    case MDC_SYSRET:
        if( !TraceState.in_thunk && !have_source ) {
            TraceState.give_it_up = true;
            return( conditions & ~COND_STOPPERS );
        }
        break;
    case MDC_CALL:
    case MDC_SYSCALL:
        if( CheckForDLLThunk() ) {
            TraceState.in_dll_thunk = true;
            return( conditions & ~COND_STOPPERS );
        }
        if( TraceState.stop_on_call )
            TraceState.stop_now = true;
        if( TraceState.type == TRACE_INTO && !TraceState.in_thunk ) {
            TraceState.trace_out = true;
            if( !TraceState.unwinding ) {
                TraceState.watch_stack = GetRegSP();
            }
        }
        return( conditions & ~COND_STOPPERS );
    }
    return( conditions & ~COND_STOPPERS );
}

static char DoTrace( debug_level curr_level )
{
    unsigned    conditions;
    DIPHDL( cue, cueh_line );

    if( curr_level == LEVEL_SOURCE ) {
        if( TraceState.type == TRACE_NEXT ) {
            if( DeAliasAddrCue( NO_MOD, GetCodeDot(), cueh_line ) == SR_NONE
              || DIPCueAdjust( cueh_line, 1, cueh_line ) != DS_OK ) {
                Warn( LIT_ENG( WARN_No_Nxt_Src_Ln ) );
                return( STOP );
            }
            DbgTmpBrk.loc.addr = DIPCueAddr( cueh_line );
            DbgTmpBrk.status.b.active = true;
        }
        if( TraceState.state == TS_ACTIVE ) {
            DeAliasAddrCue( NO_MOD, GetCodeDot(), cueh_line );
            TraceState.oldaddr = DIPCueAddr( cueh_line );
            TraceState.stop_on_call = false;
            TraceState.stop_now = false;
        }
    }
    TraceState.unwinding = false;
    TraceState.watch_stack = NilAddr;
    TraceState.doing_call = false;
    TraceState.trace_out = false;
    TraceState.in_dll_thunk = false;
    TraceState.etid = DbgRegs->tid;
    conditions = Execute( true, _IsOn( SW_FLIP ) );
    if( _IsOn( SW_TRAP_CMDS_PUSHED ) ) {
        _SwitchOff( SW_TRAP_CMDS_PUSHED );
        return( PROCCMD );
    }
    if( conditions & COND_TRACE )
        return( STOP );
    return( KEEPGOING );
}

void PerformTrace( void )
{
    char        ret;
    char        level[20];
    char        over[20];

    TraceState.userstat = UserTmpBrk.status.b;
    TraceState.curr_control = MDC_OPER | MDC_TAKEN;
    NullStatus( &UserTmpBrk );
    NullStatus( &DbgTmpBrk );
    switch( TraceState.state ) {
    case TS_ACTIVE:
        GetCmdEntry( LevelTab, TraceState.cur_level, level );
        GetCmdEntry( TraceTab, TraceState.type, over );
        Format( TxtBuff, "%s/%s/%s", GetCmdName( CMD_TRACE ), level, over );
        RecordEvent( TxtBuff );
        ret = DoTrace( TraceState.cur_level );
        CheckEventRecorded();
        if( ret != PROCCMD ) {
            TraceKill();
        } else {
            TracePostponed();
        }
        UserTmpBrk.status.b = TraceState.userstat;
        SetCodeDot( GetCodeDot() );
        break;
    case TS_POSTPONED:
        TraceState.state = TS_REACTIVATED;
        if( !DbgTmpBrk.status.b.active ) {
            DbgTmpBrk.loc.addr = TraceState.brkpt;
            NullStatus( &DbgTmpBrk );
            DbgTmpBrk.status.b.active = true;
        }
        ret = DoTrace( TraceState.cur_level );
        if( ret != PROCCMD ) {
            TraceKill();
        } else {
            TracePostponed();
        }
        UserTmpBrk.status.b = TraceState.userstat;
        SetCodeDot( GetCodeDot() );
        break;
    }
    TraceState.curraddr = NilAddr;
    TraceState.prev_control = MDC_OPER | MDC_TAKEN;
    TraceState.curr_control = MDC_OPER | MDC_TAKEN;
}


/*
 * DoneTraceCmd
 */

static bool DoneTraceCmd( inp_data_handle cmds, inp_rtn_action action )
{
    switch( action ) {
    case INP_RTN_INIT:
        ReScan( cmds );
        return( true );
    case INP_RTN_EOL:
        if( TraceState.state == TS_NONE )
            return( false );
        PerformTrace();
        return( true );
    case INP_RTN_FINI:
        return( true );
    }
    return( false ); // silence compiler
}

/*
 * PushTraceCmd
 */

static void PushTraceCmd( void )
{
    PushInpStack( "\0", DoneTraceCmd, true );
    TypeInpStack( INP_HOLD );
}

/*
 * ProcTrace -- process trace command
 */

bool HasLineInfo( address addr )
{
    mod_handle  mod;

    if( DeAliasAddrMod( addr, &mod ) == SR_NONE )
        return( false );
    if( DIPModHasInfo( mod, HK_CUE ) != DS_OK )
        return( false );
    return( true );
}

void ExecTrace( trace_cmd_type type, debug_level level )
{
    TraceState.type = type;
    TraceState.state = TS_ACTIVE;
    TraceState.savecode = GetCodeDot();
    TraceState.saveaddr = GetRegIP();
    SetCodeLoc( TraceState.saveaddr );
    TraceState.req_level = level;
    switch( level ) {
    case LEVEL_MIX:
        if( ActiveWindowLevel == LEVEL_ASM || !HasLineInfo( TraceState.saveaddr ) ) {
            TraceState.cur_level = LEVEL_ASM;
        } else {
            TraceState.cur_level = LEVEL_SOURCE;
        }
        break;
    case LEVEL_SOURCE:
        if( !HasLineInfo( TraceState.saveaddr ) ) {
            Error( ERR_NONE, LIT_ENG( ERR_NO_SOURCE_INFO ) );
        }
        /* fall through */
    default:
        TraceState.cur_level = level;
    }
    TraceState.userstat = UserTmpBrk.status.b;
    SetRegIP( GetCodeDot() );
    NullStatus( &DbgTmpBrk );
    PushTraceCmd();
}


void ProcTrace( void )
{
    int                 level_index;
    int                 type_index;
    debug_level         trace_level;
    trace_cmd_type      trace_type;
    address             addr;

    type_index = -1;
    level_index = -1;
    if( CurrToken == T_DIV ) {
        Scan();                         /* try first set of commands */
        level_index = ScanCmd( LevelTab );
        if( level_index < 0 ) {
            type_index = ScanCmd( TraceTab );
            if( type_index < 0 ) {
                Error( ERR_LOC, LIT_ENG( ERR_BAD_OPTION ), GetCmdName( CMD_TRACE ) );
            }
        } else {
            if( CurrToken == T_DIV ) {  /* do second set of commands */
                Scan();
                type_index = ScanCmd( TraceTab );
                if( type_index < 0 ) {
                    Error( ERR_LOC, LIT_ENG( ERR_BAD_OPTION ), GetCmdName( CMD_TRACE ) );
                }
            }
        }
    }
    addr = GetCodeDot();
    OptMemAddr( EXPR_CODE, &addr );
    ReqEOC();
    trace_level = ( level_index < 0 ) ? DbgLevel : level_index;
    trace_type = ( type_index < 0 ) ? TRACE_OVER : type_index;
    ExecTrace( trace_type, trace_level );
}


void FiniTrace( void )
{
    _Free( TraceState.td );
    TraceState.td = NULL;
    TraceState.tdsize = 0;
    _Free( TraceState.dd );
    TraceState.dd = NULL;
    TraceState.ddsize = 0;
}


/*
 * LevelSet -- process set/level command
 */

void LevelSet( void )
{
    int trace_level;

    trace_level = ScanCmd( LevelTab );
    if( trace_level < 0 )
        Error( ERR_LOC, LIT_ENG( ERR_BAD_SUBCOMMAND ), GetCmdName( CMD_SET ) );
    ReqEOC();
    DbgLevel = trace_level;
}

void LevelConf( void )
{
    GetCmdEntry( LevelTab, DbgLevel, TxtBuff );
    ConfigLine( TxtBuff );
}
