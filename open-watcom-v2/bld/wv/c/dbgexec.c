/****************************************************************************
*
*                            Open Watcom Project
*
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
* Description:  Run program until the next debugger event.
*
****************************************************************************/


#include <stdlib.h>
#include <ctype.h>
#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbglit.h"
#include "dbghook.h"
#include "dbgrep.h"
#include "mad.h"
#include "dui.h"
#include "enterdb.h"
#include "strutil.h"
#include "dbgscan.h"
#include "dbgutil.h"
#include "dbgexec.h"
#include "dbgmain.h"
#include "dbgovl.h"
#include "dbgbrk.h"
#include "dbgdot.h"
#include "dbgprog.h"
#include "dbgtrace.h"
#include "dbgdll.h"
#include "remcore.h"
#include "dbgmisc.h"
#include "remthrd.h"
#include "remasync.h"
#include "dbgreg.h"
#include "dbgevent.h"
#include "dlgscan.h"
#include "dbgupdt.h"
#include "dbginit.h"


extern void             DoSetWatchPnt( unsigned int, brkp * );
extern bool             SetUpTrace( bool );
extern char             DlgFatal( void );

static char             *MsgText;

static int              InCall = 0;

void SetProgState( unsigned run_conditions )
{
    if( run_conditions & COND_THREAD ) {
        DbgRegs->tid = RemoteSetThread( 0 );
        CheckForNewThreads( true );
    }
    ReadDbgRegs();
    if( run_conditions & COND_THREAD_EXTRA ) {
        _SwitchOn( SW_THREAD_EXTRA_CHANGED );
    }
    if( run_conditions & COND_ALIASING ) {
        CheckSegAlias();
    }
}

static void SetThreadStates( void )
{
    thread_state        *thd;

    for( thd = HeadThd; thd != NULL; thd = thd->link ) {
        if( thd->tid == DbgRegs->tid ) {
            RemoteThawThread( thd->tid );
        } else if( _IsOn( SW_EXECUTE_LONG ) && !SourceStep() ) {
            RemoteFreezeThread( thd->tid );
        } else {
            switch( thd->state ) {
            case THD_THAW:
                RemoteThawThread( thd->tid );
                break;
            case THD_FREEZE:
                RemoteFreezeThread( thd->tid );
                break;
            }
        }
    }
}

static void AddMessageText( char *str )
/*************************************/
{
    size_t      len;

    if( MsgText == NULL ) {
        len = 0;
    } else {
        len = strlen( MsgText );
    }
    MsgText = DbgRealloc( MsgText, len + strlen( str ) + 1 );
    StrCopy( str, MsgText + len );
}

bool SetMsgText( char *message, unsigned *conditions )
/****************************************************/
{
    char        *equal,*comma1,*comma2;
    address     addr,buff_addr;
    long        buff_len;
    size_t      sym_len;
    long        num_returns;
    cmd_list    *cmds;

#define IS_DEBUGGER_COMMAND(m,x)   (memcmp( m, DEBUGGER_COMMAND( x ), sizeof( DEBUGGER_COMMAND( x ) ) - 1 ) == 0)
#define DEBUGGER_COMMAND_SKIP(m,x)   m += sizeof( DEBUGGER_COMMAND( x ) ) - 1

    if( IS_DEBUGGER_COMMAND( message, THREADID ) ) {
        DEBUGGER_COMMAND_SKIP( message, THREADID );
        equal = strchr( message, '=' );
        if( equal == NULL )
            return( true );
        *equal = NULLCHAR;
        CheckForNewThreads( false );
        NameThread( strtoul( message, NULL, 16 ), equal + 1 );
        return( false );
    } else if( IS_DEBUGGER_COMMAND( message, SETTRUE ) ) {
        DEBUGGER_COMMAND_SKIP( message, SETTRUE );
        if( DlgScanDataAddr( message, &addr ) ) {
            ProgPoke( addr, "\x1", 1 );
        }
        return( false );
    } else if( IS_DEBUGGER_COMMAND( message, EXECUTE ) ) {
        DEBUGGER_COMMAND_SKIP( message, EXECUTE );
        if( InCall == 0 ) {
            cmds = AllocCmdList( "go/keep", sizeof( "go/keep" ) - 1 );
            PushCmdList( cmds );
            TypeInpStack( INP_HOOK );
            FreeCmdList( cmds );
        }
        cmds = AllocCmdList( message, strlen( message ) );
        PushCmdList( cmds );
        TypeInpStack( INP_HOOK );
        FreeCmdList( cmds );
        *conditions |= COND_STOP;
        return( false );
    } else if( IS_DEBUGGER_COMMAND( message, MESSAGE ) ) {
        DEBUGGER_COMMAND_SKIP( message, MESSAGE );
        AddMessageText( message );
        return( false );
    } else if( IS_DEBUGGER_COMMAND( message, LOOKUP ) ) {
        DEBUGGER_COMMAND_SKIP( message, LOOKUP );
        comma1 = strchr( message, ',' );
        if( comma1 == NULL )
            return( true );
        *comma1++ = NULLCHAR;
        comma2 = strchr( comma1, ',' );
        if( comma2 == NULL )
            return( true );
        *comma2++ = NULLCHAR;
        if( !DlgScanDataAddr( message, &addr ) )
            return( true );
        if( !DlgScanDataAddr( comma1, &buff_addr ) )
            return( true );
        if( !DlgScanLong( comma2, &buff_len ) )
            return( true );
        CnvNearestAddr( addr, TxtBuff, TXT_LEN );
        sym_len = strlen( TxtBuff ) + 1;
        if( sym_len > buff_len ) {
            sym_len = (size_t)buff_len;
            TxtBuff[sym_len - 1] = NULLCHAR;
        }
        ProgPoke( buff_addr, TxtBuff, sym_len );
        return( false );
    } else if( IS_DEBUGGER_COMMAND( message, LOADMODULE ) ) {
        DEBUGGER_COMMAND_SKIP( message, LOADMODULE );
        comma1 = strchr( message, ',' );
        if( comma1 == NULL )
            return( true );
        *comma1++ = NULLCHAR;
        if( !DlgScanDataAddr( message, &addr ) )
            return( true );
        SymUserModLoad( comma1, &addr );
        return( false );
    } else if( IS_DEBUGGER_COMMAND( message, UNLOADMODULE ) ) {
        DEBUGGER_COMMAND_SKIP( message, UNLOADMODULE );
        SymUserModUnload( message );
        return( false );
    } else if( IS_DEBUGGER_COMMAND( message, BREAKRETURN ) ) {
        DEBUGGER_COMMAND_SKIP( message, BREAKRETURN );
        if( !DlgScanLong( message, &num_returns ) )
            return( true );
        // TODO: do something with num_returns value
        return( false );
    } else {
        AddMessageText( message );
        return( true );
    }

#undef IS_DEBUGGER_COMMAND
#undef DEBUGGER_COMMAND_SKIP

}

static bool RecordMsgText( unsigned *conditions )
{
    char        *p,*p2;
    unsigned    flags;
    bool        rc = false;

    do {
        flags = RemoteGetMsgText( TxtBuff, TXT_LEN );
        p2 = TxtBuff;
        for( p = TxtBuff; *p != NULLCHAR ;p++ ) {
            if( *p != '\r' && *p != '\n' ) {
                *p2++ = *p;
            }
        }
        if( MsgText != NULL ) {
            DUIDlgTxt( MsgText );
            DbgFree( MsgText );
            MsgText = NULL;
        }
        *p2++ = '\n';
        *p2 = NULLCHAR;
        rc = SetMsgText( TxtBuff, conditions );
    } while( flags & MSG_MORE );
    return( rc );
}


typedef enum {
    ES_NORMAL,
    ES_STEP_ONE,
    ES_FORCE_BREAK
} execute_state;

static unsigned DoRun( bool step )
{
    unsigned     conditions;

    if( HaveRemoteAsync() ) {
        conditions = MakeAsyncRun( step );
        if( conditions & COND_RUNNING ) {
            conditions = DUIDlgAsyncRun();
            ReadDbgRegs();
        }
    } else {
        conditions = MakeProgRun( step );
    }
    return( conditions );
}

unsigned ExecProg( bool tracing, bool do_flip, bool want_wps )
{
    bool                have_brk_at_ip;
    bool                act_wps;
    bool                first_time;
    mad_trace_how       how;
    execute_state       es;
    unsigned            conditions;
    unsigned            run_conditions;
    bool                already_stopping;
    bool                force_stop;

    if( !want_wps )
        ++InCall;
    tracing = TraceStart( tracing );
    WriteDbgRegs();
    first_time = true;
    es = ES_NORMAL;
    conditions = 0;
    run_conditions = 0;
    if( !HaveRemoteAsync() ) {
        DUIPlayDead( true );
    }
    how = MTRH_STOP;
    for( ;; ) {
        switch( es ) {
        case ES_FORCE_BREAK:
        case ES_NORMAL:
            if( tracing ) {
                how = TraceHow( false );
            } else {
                _SwitchOn( SW_EXECUTE_LONG );
                how = MTRH_BREAK;
            }
            break;
        case ES_STEP_ONE:
            how = TraceHow( true );
            break;
        }
        if( how == MTRH_STOP )
            break;
        switch( how ) {
        case MTRH_BREAK:
            DbgUpdate( UP_CSIP_JUMPED );
            _SwitchOn( SW_TOUCH_SCREEN_BUFF );
            /* fall through */
        case MTRH_SIMULATE:
        case MTRH_STEP:
        case MTRH_STEPBREAK:
            if( _IsOff( SW_TOUCH_SCREEN_BUFF ) )
                break;
            /* fall through */
        default:
            if( !(ScrnState & USR_SCRN_ACTIVE) && do_flip ) {
                DUIStop();
            }
            _SwitchOff( SW_TOUCH_SCREEN_BUFF );
            break;
        }
        if( first_time ) {
            /* got to be down here so that SW_EXECUTE_LONG is properly set */
            SetThreadStates();
            first_time = false;
        }
        have_brk_at_ip = InsertBPs( (es == ES_FORCE_BREAK) );
        act_wps = UpdateWPs();
        if( how == MTRH_BREAK ) {
            if( have_brk_at_ip ) {
                es = ES_STEP_ONE;
                RemoveBPs();
                continue;       /* back to top */
            }
            if( act_wps && want_wps ) {
                InsertWPs();
            }
        }

        SetMemBefore( tracing );
        switch( how ) {
        case MTRH_SIMULATE:
            if( TraceSimulate() ) {
                conditions = COND_TRACE;
                break;
            }
            /* fall through */
        case MTRH_STEP:
            /* only updates stack/execution */
            conditions = DoRun( true );
            break;
        default:
            /* only updates stack/execution */
            conditions = DoRun( false  );
            break;
        }
        if( _IsOn( SW_EXECUTE_LONG ) ) {
            if( ScrnState & DBG_SCRN_ACTIVE ) {
                _SwitchOn( SW_MIGHT_HAVE_LOST_DISPLAY );
            }
            _SwitchOff( SW_EXECUTE_LONG );
        }
        SetMemAfter( tracing );
        run_conditions &= ~COND_WATCH;
        run_conditions |= conditions;

        RemoveBPs();
        if( conditions & COND_MESSAGE ) {
            if( !RecordMsgText( &conditions ) ) {
                conditions &= ~COND_MESSAGE;
            }
        }
        conditions = CheckBPs( conditions, run_conditions );
        if( _IsOn( SW_BREAK_ON_DEBUG_MESSAGE ) && ( conditions & COND_MESSAGE ) ) {
            conditions |= COND_STOP;
        }
        if( HaveRemoteAsync() && (conditions & COND_THREAD ) ) {
            conditions |= COND_STOP;
        }
        if( how == MTRH_STEPBREAK && (conditions & COND_BREAK) && DbgTmpBrk.status.b.hit ) {
            conditions &= ~COND_BREAK;
            conditions |= COND_TRACE;
        }
        if( conditions & COND_LIBRARIES ) {
            already_stopping = ( conditions & COND_STOPPERS ) != 0;
            conditions &= ~COND_LIBRARIES;
            force_stop = false;
            if( AddLibInfo( already_stopping, &force_stop ) ) {
                if( force_stop || DLLMatch() ) {
                    conditions |= COND_STOP | COND_LIBRARIES;
                }
            }
            ReMapPoints( NULL );
        }
        if( conditions & COND_SECTIONS ) {
            SectTblRead( DbgRegs );
            InvalidateTblCache();
        }
        if( (es == ES_STEP_ONE) && (conditions & COND_TRACE) ) {
            conditions &= ~COND_TRACE;
        }
        if( tracing ) {
            conditions = TraceCheck( conditions );
        }
        if( !(conditions & COND_STOPPERS) && TBreak() ) {
            conditions |= COND_USER;
            break;
        }
        if( conditions & COND_STOPPERS )
            break;
        switch( es ) {
        case ES_STEP_ONE:
            es = ES_FORCE_BREAK;
            break;
        case ES_FORCE_BREAK:
            es = ES_NORMAL;
            break;
        }
        if( (run_conditions & COND_WATCH) && es == ES_NORMAL ) {
            /*
                We got a spurious watch point indication. Make the
                next instruction single step since we might be
                dealing with a control flow opcode on a machine without
                a T-bit (e.g. Alpha).
            */
            es = ES_STEP_ONE;
        }
    }
    TraceStop( tracing );
    DUIPlayDead( false );
    SetProgState( run_conditions );
    _SwitchOff( SW_KNOW_EMULATOR );
    if( !want_wps )
        --InCall;
    return( conditions );
}


static void DisplayMsgText( void )
/********************************/
{
    if( MsgText != NULL ) {
        DUIDlgTxt( MsgText );
        DbgFree( MsgText );
        MsgText = NULL;
    }
}

bool ReportTrap( unsigned conditions, bool stack_cmds )
{
    bool    cmds_pushed;
    char        *p;

    if( conditions & COND_EXCEPTION ) {
        RecordMsgText( &conditions ); // get the 'access violation, etc' message
        p = StrCopy( LIT_ENG( Task_Exception ), TxtBuff );
        if( MsgText != NULL )
            StrCopy( MsgText, p );
        MsgText = DbgRealloc( MsgText, strlen( TxtBuff ) + 1 );
        StrCopy( TxtBuff, MsgText );
        DUIMsgBox( MsgText );
    }
    if( conditions & (COND_EXCEPTION|COND_TERMINATE) ) {
        RingBell();
    }
    DisplayMsgText();
    if( conditions & COND_USER ) {
        DUIInfoBox( LIT_ENG( User_Interupt ) );
        PurgeInpStack();
        RecordAsynchEvent();
    } else if( conditions & COND_TERMINATE ) {
        DUIInfoBox( LIT_ENG( Task_Completed ) );
        _SwitchOff( SW_HAVE_TASK );
    } else if( conditions & COND_LIBRARIES ) {
        Format( TxtBuff, "%s '%s'", LIT_ENG( Break_on_DLL_Load ), GetLastImageName() );
        DUIInfoBox( TxtBuff );
    } else {
        DUIStatusText( LIT_ENG( Empty ) );
    }
    cmds_pushed = DispBPMsg( stack_cmds );
    DbgUpdate( UP_MEM_CHANGE | UP_CSIP_CHANGE | UP_REG_CHANGE |
               UP_CODE_EXECUTED | UP_THREAD_STATE );
    return( cmds_pushed );
}


unsigned Execute( bool tracing, bool do_flip )
{
    unsigned    conditions;
    bool        stack_cmds;
    static unsigned executing = 0;

    if( !CheckStackPos() )
        return( COND_USER );
    if( !AdvMachState( ACTION_EXECUTE ) )
        return( COND_USER );

    if( executing == 0 ) {
        ++executing;
        HookNotify( true, HOOK_EXEC_START );
        --executing;
    }

    /* get rid of useless pending input information */
    for( ;; ) {
        if( CurrToken != T_LINE_SEPARATOR )
            break;
        if( InpStack == NULL )
            break;
        if( InpStack->type & (INP_HOLD|INP_STOP_PURGE) )
            break;
        if( InpStack->rtn( InpStack->handle, INP_RTN_EOL ) )
            continue;
        PopInpStack();
    }
    _SwitchOff( SW_TRAP_CMDS_PUSHED );
    conditions = ExecProg( tracing, do_flip, true );
    SetCodeDot( GetRegIP() );
    stack_cmds = true;
    if( tracing && (conditions & COND_BREAK) )
        stack_cmds = false;
    if( ReportTrap( conditions, stack_cmds ) ) {
        _SwitchOn( SW_TRAP_CMDS_PUSHED );
    }
    if( executing == 0 ) {
        ++executing;
        HookNotify( true, HOOK_EXEC_END );
        --executing;
    }
    if( conditions & COND_TERMINATE ) {
        HookNotify( true, HOOK_PROG_END );
    }
    return( conditions );
}

unsigned Go( bool do_flip )
{
    char        *p;
    unsigned    conditions;

    p = StrCopy( GetCmdName( CMD_GO ), TxtBuff );
    if( UserTmpBrk.status.b.active ) {
        p = Format( p, " %A", UserTmpBrk.loc.addr );
        RecordEvent( TxtBuff );
    } else {
        RecordGo( TxtBuff );
    }
    conditions = Execute( false, do_flip );
    CheckEventRecorded();
    return( conditions );
}
