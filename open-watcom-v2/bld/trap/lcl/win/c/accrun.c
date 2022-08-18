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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <ctype.h>
#include <dos.h>
#include "cpuglob.h"
#include "stdwin.h"
#include "wdebug.h"
#include "trpimp.h"
#include "trpld.h"
#include "trpsys.h"
#include "dbgrmsg.h"


/*
 * SingleStepMode - allow single stepping
 */
void SingleStepMode( void )
{

    IntResult.EFlags |= TRACE_BIT;
    TraceOn = TRUE;

} /* SingleStepMode */


/*
 * runProg:
 *
 * - we turn on the T-bit for single_step mode, and set the debug registers
 *   for watch points (if we can).
 * - we then switch to the debuggee, and wait for a fault to occur.
 * - after the fault, we record information about it and return
 *
 */
static trap_elen runProg( bool single_step )
{
    private_msg         pmsg;
    BOOL                watch386;
    BOOL                dowatch;
    BOOL                ton;
    appl_action         appl_act;
    prog_go_ret         *ret;

    ret = GetOutPtr( 0 );

    if( DebugeeTask == NULL ) {
        ret->conditions = COND_TERMINATE;
        return( sizeof( *ret ) );
    }

    IntResult.EFlags &= ~TRACE_BIT;
    dowatch = FALSE;
    watch386 = FALSE;

    SetInputLock( false );

    if( single_step ) {
        SingleStepMode();
    } else if( WPCount != 0 ) {
        dowatch = TRUE;
        watch386 = SetDebugRegs();
    }

    ret->conditions = 0;
    appl_act = RESTART_APP;
    while( DebugeeTask != NULL ) {
        if( dowatch && !watch386 ) {
            SingleStepMode();
        }
        ton = TraceOn;
        pmsg = DebuggerWaitForMessage( RUNNING_DEBUGEE, TaskAtFault, appl_act );
        TraceOn = FALSE;

        if( pmsg == FAULT_HIT ) {
            switch( IntResult.InterruptNumber ) {
            case INT_1:
                ret->conditions = COND_TRACE;
                if( watch386 ) {
                    if( GetDR6() & 0xf ) {
                        ret->conditions = COND_WATCH;
                        if( DebugDebugeeOnly ) {
                            if( !CheckWatchPoints() ) {
                                appl_act = CHAIN;
                                continue;
                            }
                        }
                    }
                    break;
                }
                if( !ton && DebugDebugeeOnly ) {
                    appl_act = CHAIN;
                    continue;
                }
                if( dowatch ) {
                    if( CheckWatchPoints() ) {
                        ret->conditions = COND_WATCH;
                    } else {
                        appl_act = RESTART_APP;
                        continue;
                    }
                }
                break;
            case INT_3:
                if( DebugDebugeeOnly ) {
                    if( !IsOurBreakpoint( IntResult.CS, IntResult.EIP ) ) {
                        IntResult.EIP++;
                        appl_act = CHAIN;
                        continue;
                    }
                }
                ret->conditions = COND_BREAK;
                break;
            default:
                if( DebugDebugeeOnly ) {
                    if( TaskAtFault != DebugeeTask ) {
                        appl_act = CHAIN;
                        continue;
                    }
                }
                ret->conditions = COND_EXCEPTION;
                break;
            }
            break;
        } else if( pmsg == TASK_ENDED ) {
            ret->conditions = COND_TERMINATE;
            DebugeeTask = NULL;
            IntResult.CS = TerminateCSIP >> 16;
            IntResult.EIP = (DWORD) (WORD) TerminateCSIP;
            break;
        } else if( pmsg == GET_CHAR ) {
            IntResult.EAX = 'b';
            continue;
        } else if( pmsg == OUT_STR ) {
            ret->conditions = COND_MESSAGE;
            break;
        } else if( pmsg == ASYNCH_STOP ) {
            ret->conditions = COND_USER;
            break;
        } else if( pmsg == DLL_LOAD || pmsg == DLL_LOAD32 ) {
            break;
        }
    }
    ClearDebugRegs();
    ret->program_counter.offset = IntResult.EIP;
    ret->stack_pointer.offset = IntResult.ESP;
    ret->program_counter.segment = IntResult.CS;
    ret->stack_pointer.segment = IntResult.SS;
    if( ModuleTop > CurrentModule && pmsg != TASK_ENDED && pmsg != DLL_LOAD32 ) {
        ret->conditions |= COND_LIBRARIES;
        Out((OUT_MAP,"ModuleTop=%d, CurrentModule=%d", ModuleTop, CurrentModule ));
    }
    if( HasSegAliases() ) {
        ret->conditions |= COND_ALIASING;
    }
    Out(( OUT_RUN,"Back from runProg %4.4x", ret->conditions ));
    return( sizeof( *ret ) );
} /* runProg */

trap_retval TRAP_CORE( Prog_go )( void )
{
    Out(( OUT_RUN, "ReqProg_go" ));
    return( runProg( FALSE ) );
}

trap_retval TRAP_CORE( Prog_step )( void )
{
    Out(( OUT_RUN, "ReqProg_step" ));
    return( runProg( TRUE ) );
}
