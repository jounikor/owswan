/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2018 The Open Watcom Contributors. All Rights Reserved.
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


#include "_cgstd.h"
#include "coderep.h"
#include "cgaux.h"
#include "zoiks.h"
#include "data.h"
#include "bldins.h"
#include "makeins.h"
#include "types.h"
#include "makeaddr.h"
#include "namelist.h"
#include "makeblk.h"
#include "rgtbl.h"
#include "insutil.h"
#include "typemap.h"
#include "bldcall.h"
#include "parmreg.h"
#include "parm.h"
#include "bgcall.h"


an      BGCall( cn call, bool use_return, bool in_line )
/******************************************************/
{
    instruction         *call_ins;
    instruction         *conv_ins;
    call_state          *state;
    name                *result;
    hw_reg_set          ret_addr;
    instruction         *ins;


    call_ins = call->ins;
    state = call->state;

    if( state->attr & ROUTINE_MODIFIES_NO_MEMORY ) {
        call_ins->flags.call_flags |= CALL_WRITES_NO_MEMORY;
    }
    if( state->attr & ROUTINE_READS_NO_MEMORY ) {
        call_ins->flags.call_flags |= CALL_READS_NO_MEMORY;
    }
    if( !use_return ) {
        call_ins->flags.call_flags |= CALL_IGNORES_RETURN;
    }

    result = BGNewTemp( call->tipe );
    if( call_ins->type_class == XX ) {
        call_ins->result = result;
        ret_addr = ParmReg( CP, 4, 4, call->state );
        ins = MakeUnary( OP_LA, result, AllocRegName( ret_addr ), CP );
        AddIns( ins );
    } else {
        call_ins->result = AllocRegName( state->return_reg );
    }
    AssgnParms( call, in_line );
    AddCallIns( call_ins, call );
    if( use_return ) {
#if 1
        if( call_ins->type_class != XX ){
            conv_ins = MakeConvert( call_ins->result, result, result->n.type_class,
                                    call_ins->result->n.type_class );
            AddIns( conv_ins );
        } else {
            // conv_ins = MakeMove( call_result, result, XX );
        }
#endif
    }
    return( MakeTempAddr( result ) );
}


void    BGProcDecl( cg_sym_handle sym, type_def *tipe )
/*****************************************************/
{
    type_class_def      type_class;
    name                *temp;
    hw_reg_set          reg;

    type_class = AddCallBlock( sym, tipe );
    SaveTargetModel = TargetModel;
    if( tipe != TypeNone ) {
        if( type_class == XX ) {
            reg = HW_D3;
            temp = AllocTemp( WD );
            temp->v.usage |= USE_IN_ANOTHER_BLOCK;
            AddIns( MakeMove( AllocRegName( reg ), temp, WD ) );
            HW_TurnOn( CurrProc->state.parm.used, reg );
            CurrProc->targ.return_points = temp;
        }
    }
}


type_def    *PassParmType( cg_sym_handle func, type_def* tipe, call_class cclass )
/********************************************************************************/
{
    /* unused parameters */ (void)cclass;

    tipe = QParmType( func, NULL, tipe );
    return( tipe );
}

instruction    *PushOneParm( instruction *ins, name *curr,
                                     type_class_def type_class,
                                     type_length offset,
                                     call_state *state )
/********************************************************/
{
    instruction *new;
    name        *dst;
    name        *stack_reg;

    /* unused parameters */ (void)state;

    stack_reg = AllocRegName( StackReg() );
    dst = AllocIndex( stack_reg, NULL, offset + STACK_HEADER_SIZE, type_class );
    new = MakeMove( curr, dst, type_class );
    SuffixIns( ins, new );
    return( new );
}

name    *StReturn( an retval, type_def *tipe, instruction **pins )
/****************************************************************/
{
    name        *index;

    /* unused parameters */ (void)retval; (void)pins;

    index = AllocIndex( CurrProc->targ.return_points, NULL, 0, TypeClass( tipe ) );
    return( index );
}

void    InitTargProc( void )
/**************************/
{
    CurrProc->targ.debug = NULL;
    CurrProc->targ.base_is_fp = false;
}


void    SaveToTargProc( void )
/****************************/
{
    CurrProc->targ.max_stack = MaxStack;
}


void    RestoreFromTargProc( void )
/*************************************/
{
    MaxStack = CurrProc->targ.max_stack;
}

reg_set_index   CallIPossible( instruction *ins )
/***********************************************/
{
    /* unused parameters */ (void)ins;

    return( RL_WORD );
}
