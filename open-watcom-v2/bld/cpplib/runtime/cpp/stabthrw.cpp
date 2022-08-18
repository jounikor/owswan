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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "cpplib.h"
#include <cstdlib>
#include <cstring>

#ifdef __USE_PD
#   include <setjmpex.h>
#   pragma extref _setjmpex;
#else
#   include <setjmp.h>
#endif

#include "rtexcept.h"
#include "exc_pr.h"
#include "rtmsgs.h"


static
//_WCNORETURN
void fneDispatch(               // DISPATCH "unexpected"
    DISPATCH_EXC *dispatch )    // - dispatch control
{
    THREAD_CTL *ctl;            // - thread-specific data
    ACTIVE_EXC *exc;            // - active exception
    ACTIVE_EXC *srch;           // - previous exception for this FNEXC

    ctl = dispatch->rtc->thr;
    ctl->abort_msg = RTMSG_FNEXC;
    exc = ctl->excepts;
    exc->dispatch = dispatch;
    for( srch = exc; ; ) {
        if( srch == NULL ) {
            // first time through fn_exc
            exc->dispatch = dispatch;
            exc->fnexc_state = exc->state;
            exc->state = EXCSTATE_UNEXPECTED;
            _EXC_PR_FNEXC marker( dispatch->rtc, NULL, dispatch->rw, dispatch->rethrow ? NULL : exc );
            std::unexpected();
//            marker._state = EXCSTATE_TERMINATE;
//            CPPLIB( call_terminate )( RTMSG_RET_UNEXPECT, ctl );
            // never return
        }
        DISPATCH_EXC *cand = srch->dispatch;
        if( NULL != cand && dispatch->rw == cand->rw && dispatch->state_var == cand->state_var ) {
            if( srch->state == EXCSTATE_UNEXPECTED ) {
                // throw/rethrow did not get through fn-exc
                exc->state = EXCSTATE_BAD_EXC;
                _EXC_PR_FNEXC marker( dispatch->rtc, NULL, dispatch->rw, dispatch->rethrow ? NULL : exc );
                throw std::bad_exception();
                // never return
            }
            if( srch->state == EXCSTATE_BAD_EXC ) {
                // throw of bad_exception did not get through fn-exc
                _EXC_PR_DTOR marker( dispatch->rtc, NULL, EXCSTATE_TERMINATE, dispatch->rethrow ? NULL : exc );
                CPPLIB( call_terminate )( RTMSG_FNEXC, ctl );
                // never return
            }
            if( srch != exc ) {
                CPPLIB( corrupted_stack )();
                // never return
            }
        }
        srch = srch->prev;
        if( exc == srch ) {
            CPPLIB( corrupted_stack )();
            // never return
        }
    }
}


#define throwCnvSig( d ) CPPLIB( ts_refed )( (d)->cnv_try->signature )


// never return
static
//_WCNORETURN
void catchDispatch(             // DISPATCH A CATCH BLOCK
    DISPATCH_EXC *dispatch )    // - dispatch control
{
    RW_DTREG* blk;              // - function block for dispatch
    DTOR_CMD* cmd;              // - try command
    jmp_buf *env;               // - addr[ jmp_buf ]
    ACTIVE_EXC *active;         // - active exception
    void *tgt;                  // - target try variable
    void *src;                  // - source address
    RT_TYPE_SIG sig;            // - signature for catch
    unsigned cnv_offset;        // - conversion offset

    blk = dispatch->rw;
    cmd = dispatch->try_cmd;
    active = dispatch->exc;
    active->state = EXCSTATE_DISPATCH;
    env = (jmp_buf*)( (char*)blk + cmd->try_cmd.jmp_buf );
    sig = cmd->try_cmd.sigs[ dispatch->catch_no ];
    if( sig != NULL ) {
        cnv_offset = dispatch->cnv_try->offset;
        src = cnv_offset + (char*)active->data;
        tgt = (char*)blk + cmd->try_cmd.offset;
        switch( sig->hdr.type ) {
        case THROBJ_PTR_CLASS :
            if( dispatch->zero ) {
                *(void**)tgt = NULL;
            } else {
                *(void**)tgt = *(char**)active->data + cnv_offset;
            }
            break;
        case THROBJ_VOID_STAR :
        case THROBJ_PTR_SCALAR :
        case THROBJ_PTR_FUN :
            if( dispatch->zero ) {
                std::memset( tgt, 0, sig->scalar.size );
                break;
            }
        case THROBJ_SCALAR :
            std::memcpy( tgt, src, sig->scalar.size );
            break;
        case THROBJ_CLASS :
            sig = throwCnvSig( dispatch );
            (*sig->clss.copyctor)( tgt, src );
            break;
        case THROBJ_CLASS_VIRT :
            sig = throwCnvSig( dispatch );
            (*sig->clss_v.copyctor)( tgt, CTOR_NULL, src );
            break;
        case THROBJ_REFERENCE :
            sig = CPPLIB( ts_refed )( sig );
            switch( sig->hdr.type ) {
            case THROBJ_PTR_CLASS :
                if( dispatch->zero ) {
                    active->extra_object = NULL;
                } else {
                    active->extra_object = *(char**)active->data + cnv_offset;
                }
                *(void**)tgt = &active->extra_object;
                break;
            case THROBJ_PTR_SCALAR :
            case THROBJ_PTR_FUN :
            case THROBJ_VOID_STAR :
                if( dispatch->zero ) {
                    active->extra_object = NULL;
                    src = &active->extra_object;
                }
            case THROBJ_SCALAR :
            case THROBJ_CLASS :
            case THROBJ_CLASS_VIRT :
                *(void**)tgt = src;
                break;
            }
            break;
        default :
            GOOF_EXC( "unexpected exception type" );
            // never return
        }
    }
    longjmp( *env, dispatch->catch_no + 1 );
    // never return
}

// never return
static
//_WCNORETURN
void processThrow(              // PROCESS A THROW
    void *object,               // - address of object
    THROW_RO *throw_ro,         // - thrown R/O block
    bool is_zero )              // - true ==> thrown object is zero constant
{
    _RTCTL rt_ctl;              // - R/T control
    DISPATCH_EXC dispatch;      // - dispatch control
    FsExcRec excrec;            // - system exception record
    volatile bool unwound;
    void __based(__segname("_STACK")) *force_this_routine_to_have_an_EBP_frame = NULL;

//  CPPLIB( DbgRtDumpAutoDtor )();

#if 0
//
// Check for throws under bad circumstances
//
    ACTIVE_EXC *exc = rt_ctl.thr->excepts;
    if( exc == NULL ) {
        if( rt_ctl.thr->flags.terminated ) {
            CPPLIB( fatal_runtime_error )( RTMSG_THROW_TERMIN, 1 );
            // never return
        }
    } else {
        switch( exc->state ) {
        case EXCSTATE_DISPATCH :
        case EXCSTATE_UNEXPECTED :
        case EXCSTATE_BAD_EXC :
            break;
        case EXCSTATE_UNWIND :
        {   CPPLIB( free_exc )( &rt_ctl, exc );
            _EXC_PR marker( &rt_ctl, NULL, EXCSTATE_TERMINATE );
            CPPLIB( call_terminate )( RTMSG_THROW_DTOR, rt_ctl.thr );
            // never return
        }
        case EXCSTATE_TERMINATE :
            CPPLIB( free_exc )( &rt_ctl, exc );
            CPPLIB( fatal_runtime_error )( RTMSG_THROW_TERMIN, 1 );
            // never return
        case EXCSTATE_CTOR :
            CPPLIB( free_exc )( &rt_ctl, exc );
            CPPLIB( fatal_runtime_error )( RTMSG_THROW_CTOR, 1 );
            // never return
        case EXCSTATE_DTOR :
            CPPLIB( free_exc )( &rt_ctl, exc );
            CPPLIB( fatal_runtime_error )( RTMSG_EXC_DTOR, 1 );
            // never return
        default:
            GOOF_EXC( "getActiveExc: unexpected exception state" );
            // never return
        }
    }
#endif
//
// Setup for dispatch
//
    rt_ctl.thr->abort_msg = NULL;
    CPPLIB( exc_setup )( &dispatch, throw_ro, is_zero, &rt_ctl, object, &excrec );
    unwound = false;
//
// raising exception locates the catch/fn-exception to be dispatched
// also fills in excrec.addr
//
    if( 0 == dispatch.fs_last ) {
        // no search when no R/W blocks
        dispatch.type = DISPATCHABLE_NO_CATCH;
    } else {
        FS_RAISE_EXCEPTION( &excrec );
    }
//
// comes here twice, unless an error situation:
//  unwound == 0 : after catch located, before unwinding
//  unwound == 1 : after unwinding
//
    switch( dispatch.type ) {
    case DISPATCHABLE_STOP :
        if( NULL == dispatch.srch_ctl ) {
            GOOF_EXC( "DISPATCHABLE_STOP: no srch_ctl" );
            // never return
        }
        switch( dispatch.srch_ctl->_state ) {
        case EXCSTATE_UNWIND :
        {
            _EXC_PR_FREE marker( &rt_ctl, NULL, EXCSTATE_TERMINATE, dispatch.rethrow ? NULL : rt_ctl.thr->excepts );
            CPPLIB( call_terminate )( RTMSG_THROW_DTOR, rt_ctl.thr );
            // never return
        }
        case EXCSTATE_TERMINATE :
            CPPLIB( fatal_runtime_error )( RTMSG_THROW_TERMIN, 1 );
            // never return
        case EXCSTATE_CTOR :
            CPPLIB( fatal_runtime_error )( RTMSG_THROW_CTOR, 1 );
            // never return
        case EXCSTATE_DTOR :
            CPPLIB( fatal_runtime_error )( RTMSG_EXC_DTOR, 1 );
            // never return
        default:
            GOOF_EXC( "DISPATCHABLE_STOP: unexpected exception state" );
            // never return
        }
      case DISPATCHABLE_FNEXC :
      case DISPATCHABLE_CATCH :
        if( !unwound ) {
            ACTIVE_EXC *active; // - saved exception
            unwound = true;
            if( dispatch.rethrow ) {
// do we still need fnexc_state ?
                dispatch.exc->fnexc_state = dispatch.exc->state;
                if( dispatch.popped ) {
                    active = dispatch.exc;
                    active->cat_try = dispatch.try_cmd;
                    active->rw = dispatch.rw;
                }
            } else {
                active = CPPLIB( alloc_exc )( object, dispatch.ro, &rt_ctl );
                dispatch.exc = active;
                active->cat_try = dispatch.try_cmd;
                active->rw = dispatch.rw;
            }
            if( dispatch.type == DISPATCHABLE_FNEXC ) {
                if( dispatch.rethrow ) {
                    dispatch.exc->state = dispatch.exc->fnexc_state;
                }
                break;
            }
            dispatch.exc->state = EXCSTATE_UNWIND;
#ifdef RW_REGISTRATION
            FS_UNWIND_GLOBAL( dispatch.rw, excrec.addr, &excrec );
#endif
#ifdef PD_REGISTRATION
            CPPLIB( PdUnwind )( &excrec );
#endif
        }
        // NT returns here instead of after RAISE_EXCEPTION if
        // there are no blocks to pop
        // nb. need to execute same code as after RAISE_EXCEPTION
        CPPLIB( destruct_internal )( dispatch.state_var, dispatch.rw );
        break;
    }
//
// dispatch the exception
//
    switch( dispatch.type ) {
    case DISPATCHABLE_FNEXC :
        fneDispatch( &dispatch );
        // never return
    case DISPATCHABLE_CATCH :
        catchDispatch( &dispatch );
        // never return
    case DISPATCHABLE_NO_CATCH :
        if( dispatch.rethrow ) {
            CPPLIB( fatal_runtime_error )( RTMSG_RETHROW, 1 );
            // never return
        } else {
            _EXC_PR marker( &rt_ctl, NULL, EXCSTATE_TERMINATE );
            CPPLIB( call_terminate )( RTMSG_NO_HANDLER, rt_ctl.thr );
            // never return
        }
#if 0 // not now
    case DISPATCHABLE_SYS_EXC :
    {
        char buffer[ sizeof( RTMSG_SYS_EXC ) ];
        std::memcpy( buffer, RTMSG_SYS_EXC, sizeof( buffer ) );
        ltoa( dispatch.system_exc, buffer + sizeof( buffer) - 9, 16 );
        CPPLIB( fatal_runtime_error )( buffer, 1 );
        // never return
    }
#endif
    default :
        GOOF_EXC( "throw: invalid DISPATCHABLE" );
        // never return
    }
}


extern "C"
_WPRTLINK
void CPPLIB( catch_done )(      // COMPLETION OF CATCH
#ifdef RW_REGISTRATION
    void
#else
    RW_DTREG *rw                // - current R/W block
#endif
    )
{
    _RTCTL rt_ctl;              // - R/T control
#ifdef RW_REGISTRATION
    RW_DTREG* rw = RwTop( rt_ctl.thr );
#endif
    rt_ctl.setRwRo( rw );
    RO_STATE* state = CPPLIB( stab_entry )( rw->base.ro, rw->fun.base.state_var );
    DTOR_CMD* cmd = TryFromCatch( state->u.cmd_addr );
    ACTIVE_EXC* exc = CPPLIB( find_active )( &rt_ctl, rw, cmd );
    CPPLIB( dtor_free_exc )( exc, &rt_ctl );
    STAB_TRAVERSE traverse;
    CPPLIB( stab_trav_init )( &traverse, &rt_ctl );
    CPPLIB( stab_trav_next )( &traverse );
    rw->fun.base.state_var = traverse.state_var;
}


// never return
extern "C"
_WPRTLINK
//_WCNORETURN
void CPPLIB( throw )(           // THROW AN EXCEPTION OBJECT (NOT CONST ZERO)
    void *object,               // - address of object
    THROW_RO *throw_ro )        // - throw R/O block
{
    processThrow( object, throw_ro, false );
    // never return
}


// never return
extern "C"
_WPRTLINK
//_WCNORETURN
void CPPLIB( throw_zero )(      // THROW AN EXCEPTION OBJECT (CONST ZERO)
    void *object,               // - address of object
    THROW_RO *throw_ro )        // - throw R/O block
{
    processThrow( object, throw_ro, true );
    // never return
}


// never return
extern "C"
_WPRTLINK
//_WCNORETURN
void CPPLIB( rethrow )(        // RE-THROW AN EXCEPTION
    void )
{
    processThrow( NULL, NULL, false );
    // never return
}
