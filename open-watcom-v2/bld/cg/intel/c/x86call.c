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


#include "_cgstd.h"
#include "coderep.h"
#include "cgmem.h"
#include "cgaux.h"
#include "zoiks.h"
#include "makeins.h"
#include "data.h"
#include "display.h"
#include "rtrtn.h"
#include "objout.h"
#include "types.h"
#include "makeaddr.h"
#include "namelist.h"
#include "makeblk.h"
#include "rgtbl.h"
#include "split.h"
#include "insutil.h"
#include "typemap.h"
#include "bldcall.h"
#include "bldins.h"
#include "x86segs.h"
#include "cgprotos.h"
#include "parm.h"
#include "bgcall.h"


static  void    AddCall( instruction *ins, cn call );

#if _TARGET & _TARG_80386
static  void    Far16Parms( cn call ) {
/*************************************/

    instruction         *ins;
    type_length         parm_size;
    pn                  parm, next;
    instruction         *call_ins;
    name                *eax;
    name                *ecx;
    name                *esi;
    label_handle        lbl;
    type_length         offset;
    name                *parmlist;
    call_state          *state;
    rt_class            rtindex;

    call_ins = call->ins;
    parm_size = 0;
    state = call->state;
    for( parm = call->parms; parm != NULL; parm = parm->next ) {
        parm_size += _RoundUp( parm->name->tipe->length, 2 );
    }
    parmlist = SAllocTemp( XX, parm_size );
    parmlist->v.usage |= NEEDS_MEMORY | USE_IN_ANOTHER_BLOCK | USE_ADDRESS;
    offset = 0;
    for( parm = call->parms; parm != NULL; parm = parm->next ) {
        parm->name->u.i.ins->result = STempOffset( parmlist, offset,
                                                 TypeClass( parm->name->tipe ),
                                                 parm->name->tipe->length );
        offset += _RoundUp( parm->name->tipe->length, 2 );
    }
    for( parm = call->parms; parm != NULL; parm = next ) {
        next = parm->next;
        parm->name->format = NF_ADDR;   /* so instruction doesn't get freed! */
        BGDone( parm->name );
        CGFree( parm );
    }
    eax = AllocRegName( HW_EAX );
    ecx = AllocRegName( HW_ECX );
    esi = AllocRegName( HW_ESI );
    HW_TurnOn( state->parm.used, eax->r.reg );
    HW_TurnOn( state->parm.used, ecx->r.reg );
    HW_TurnOn( state->parm.used, esi->r.reg );
    ins = MakeMove( AllocS32Const( parm_size ), ecx, WD );
    AddIns( ins );
    ins = MakeUnary( OP_LA, parmlist, esi, WD );
    AddIns( ins );
    if( ins->head.opcode == OP_CALL ) {
        ins = MakeUnary( OP_LA, call->name->u.n.name, eax, WD );
    } else {
        ins = MakeMove( GenIns( call->name ), eax, WD );
        call_ins->head.opcode = OP_CALL;
    }
    call_ins->num_operands = 2;
    AddIns( ins );
    if( call_ins->type_class == XX ) {
        if( state->attr & ROUTINE_ALLOCS_RETURN ) {
            rtindex = RT_Far16Cdecl;
        } else {
            rtindex = RT_Far16Pascal;
        }
    } else {
        rtindex = RT_Far16Func;
    }
    lbl = RTLabel( rtindex );
    call->name->u.n.name = AllocMemory( lbl, 0, CG_LBL, WD );
    call_ins->flags.call_flags |= CALL_FAR16 | CALL_POPS_PARMS;
    call_ins->operands[CALL_OP_USED] = AllocRegName( state->parm.used );
    call_ins->operands[CALL_OP_POPS] = AllocS32Const( 0 );
    call_ins->zap = &call_ins->operands[CALL_OP_USED]->r;
}
#endif


an      BGCall( cn call, bool use_return, bool in_line )
/******************************************************/
{
    instruction         *call_ins;
    call_state          *state;
    name                *ret_ptr = NULL;
    name                *result;
    name                *temp;
    name                *reg_name;
    instruction         *ret_ins = NULL;
    hw_reg_set          return_reg;
    hw_reg_set          zap_reg;

    if( call->name->tipe == TypeProcParm ) {
        SaveDisplay( OP_PUSH );
    }

    state = call->state;
    result = BGNewTemp( call->tipe );
    call_ins = call->ins;

/*   If we have a return value that won't fit in a register*/
/*   pass a pointer to result as the first parm*/

    if( call_ins->type_class == XX ) {
        if( _RoutineIsFar16( state->attr ) ) {
            if( state->attr & ROUTINE_ALLOCS_RETURN ) {
                HW_CAsgn( state->return_reg, HW_EAX );
            } else {
                HW_CAsgn( state->return_reg, HW_EBX );
            }
        }
        if( ( state->attr & ROUTINE_ALLOCS_RETURN ) == 0 ) {
            if( HW_CEqual( state->return_reg, HW_EMPTY ) ) {
                ret_ptr = AllocTemp( WD );
            } else {
                ret_ptr = AllocRegName( state->return_reg );
            }
            ret_ins = MakeUnary( OP_LA, result, ret_ptr, WD );
            HW_TurnOn( state->parm.used, state->return_reg );
            call_ins->flags.call_flags |= CALL_RETURNS_STRUCT;
        }
    }
    if( _IsTargetModel(FLOATING_DS) && (state->attr&ROUTINE_NEEDS_DS_LOADED) ) {
        HW_CTurnOn( state->parm.used, HW_DS );
    }
    if( _RoutineIsFar16( state->attr ) ) {
#if _TARGET & _TARG_80386
        Far16Parms( call );
#endif
    } else {
        if( AssgnParms( call, in_line ) ) {
            if( state->attr & ROUTINE_REMOVES_PARMS ) {
                call_ins->flags.call_flags |= CALL_POPS_PARMS;
            }
        }
    }

    if( state->attr & (ROUTINE_MODIFIES_NO_MEMORY | ROUTINE_NEVER_RETURNS) ) {
        /* a routine that never returns can not write any memory as far
            as this routine is concerned */
        call_ins->flags.call_flags |= CALL_WRITES_NO_MEMORY;
    }
    if( state->attr & ROUTINE_READS_NO_MEMORY ) {
        call_ins->flags.call_flags |= CALL_READS_NO_MEMORY;
    }
    if( state->attr & ROUTINE_NEVER_RETURNS ) {
        call_ins->flags.call_flags |= CALL_ABORTS;
    }
    if( _RoutineIsInterrupt( state->attr ) ) {
        call_ins->flags.call_flags |= CALL_INTERRUPT | CALL_POPS_PARMS;
    }
    if( !use_return ) {
        call_ins->flags.call_flags |= CALL_IGNORES_RETURN;
    }
    if( call_ins->type_class == XX ) {
        reg_name = AllocRegName( state->return_reg );
        if( state->attr & ROUTINE_ALLOCS_RETURN ) {
            call_ins->result = reg_name;
            AddCall( call_ins, call );
            if( use_return ) {
                temp = AllocTemp( WD ); /* assume near pointer*/
                AddIns( MakeMove( reg_name, temp, WD ) );
                temp = SAllocIndex( temp, NULL, 0,
                                    result->n.type_class, call->tipe->length );
                AddIns( MakeMove( temp, result, result->n.type_class ) );
            }
        } else {
            call_ins->result = result;
            AddIns( ret_ins );
            if( HW_CEqual( state->return_reg, HW_EMPTY ) ) {
                AddIns( MakeUnary( OP_PUSH, ret_ptr, NULL, WD ) );
                state->parm.offset += TypeClassSize[WD];
                call_ins->operands[CALL_OP_POPS] =
                        AllocS32Const( call_ins->operands[CALL_OP_POPS]->c.lo.int_value + TypeClassSize[WD] );
                if( state->attr & ROUTINE_REMOVES_PARMS ) {
                    call_ins->flags.call_flags |= CALL_POPS_PARMS;
                }
            }
            AddCall( call_ins, call );
        }
    } else {
        return_reg = state->return_reg;
        zap_reg = call_ins->zap->reg;
        HW_CTurnOn( zap_reg, HW_FLTS );
        HW_OnlyOn( return_reg, zap_reg );
        call_ins->result = AllocRegName( return_reg );
        reg_name = AllocRegName( state->return_reg );
        AddCall( call_ins, call );
        if( use_return ) {
            ret_ins = MakeMove( reg_name, result, result->n.type_class );
            if( HW_COvlap( reg_name->r.reg, HW_FLTS ) ) {
                ret_ins->stk_entry = 1;
                ret_ins->stk_exit = 0;
            }
            AddIns( ret_ins );
        }
    }
    if( state->parm.offset != 0 && ( state->attr & ROUTINE_REMOVES_PARMS ) == 0 ) {
        reg_name = AllocRegName( HW_xSP );
        AddIns( MakeBinary( OP_ADD, reg_name,
                AllocS32Const( state->parm.offset ), reg_name, WD ) );
    }
    return( MakeTempAddr( result ) );
}


void    BGProcDecl( cg_sym_handle sym, type_def *tipe )
/*****************************************************/
{
    hw_reg_set          reg;
    name                *temp;
    type_class_def      type_class;
    segment_id          old_segid;
    label_handle        lbl;

    SaveTargetModel = TargetModel;
    type_class = AddCallBlock( sym, tipe );
    if( tipe != TypeNone ) {
        if( type_class == XX ) {
            if( CurrProc->state.attr & ROUTINE_ALLOCS_RETURN ) {
                old_segid = SetOP( AskBackSeg() );
                lbl = AskForNewLabel();
                DataLabel( lbl );
                DGUBytes( tipe->length );
                CurrProc->targ.return_points = (name *)SAllocMemory( lbl, 0, CG_LBL, TypeClass( tipe ), tipe->length );
                SetOP( old_segid );
            } else {
                reg = CurrProc->state.return_reg;
                if( HW_CEqual( reg, HW_EMPTY ) ) {
                    temp = DoParmDecl( NULL, TypeInteger, HW_EMPTY );
                } else {
                    temp = AllocTemp( WD );
                    temp->v.usage |= USE_IN_ANOTHER_BLOCK;
                    AddIns( MakeMove( AllocRegName( reg ), temp, WD ) );
                    HW_TurnOn( CurrProc->state.parm.used, reg );
                }
                CurrProc->targ.return_points = temp;
            }
        }
    }
    if( CurrProc->state.attr & ROUTINE_FARSS ) {
        TargetModel |= FLOATING_SS;
    }
}


name    *StReturn( an retval, type_def *tipe, instruction **pins )
/****************************************************************/
{
    name        *retp;
    name        *ptr;
    name        *off;
    name        *seg;
    hw_reg_set  reg;

    if( CurrProc->state.attr & ROUTINE_ALLOCS_RETURN ) {
        retp = CurrProc->targ.return_points;
        AddIns( MakeUnary( OP_LA, retp, AllocRegName( CurrProc->state.return_reg ), WD ) );
        *pins = NULL;
    } else {
        if( _IsTargetModel( FLOATING_SS ) || _IsTargetModel( FLOATING_DS ) ) {
            ptr = AllocTemp( CP );
            off = OffsetPart( ptr );
            seg = SegmentPart( ptr );
            AddIns( MakeMove( AllocRegName( HW_SS ), seg, U2 ) );
        } else {
            ptr = AllocTemp( WD );
            off = ptr;
        }
        AddIns( MakeMove( CurrProc->targ.return_points, off, WD ) );
        retp = SAllocIndex( ptr, NULL, 0, TypeClass( retval->tipe ), tipe->length );
        reg = ReturnReg( WD, false );
        *pins = MakeMove( CurrProc->targ.return_points, AllocRegName( reg ), WD );
        CurrProc->state.return_reg = reg;
    }
    return( retp );
}


static  void    AddCall( instruction *ins, cn call ) {
/****************************************************/

    name        *proc_name;

    if( _IsTargetModel(FLOATING_DS) && (call->state->attr&ROUTINE_NEEDS_DS_LOADED) ) {
        AddIns( MakeMove( NearSegment(), AllocRegName( HW_DS ), U2 ) );
    }
    if( call->name->tipe == TypeProcParm ) {
        proc_name = AllocTemp( ClassPointer );
/* what to do?        proc_name->usage |= USE_REGISTER;*/
        AddIns( MakeMove( ins->operands[CALL_OP_ADDR], proc_name, ClassPointer ));
        ins->operands[CALL_OP_ADDR] = proc_name;
        SetDisplay( GenIns( call->name ) );
        AddIns( ins );
        SaveDisplay( OP_POP );
    } else {
        AddCallIns( ins, call );
    }
}


reg_set_index   CallIPossible( instruction *ins )
/***********************************************/
{
     if( ins->operands[CALL_OP_ADDR]->n.type_class == CP )
        return( RL_ );
     if( ins->operands[CALL_OP_ADDR]->n.type_class == PT )
        return( RL_ );
#if _TARGET & _TARG_8086
     return( RL_WORD );
#else
     return( RL_DOUBLE );
#endif
}


void    InitTargProc( void )
/**************************/
{
    CurrProc->targ.stack_check = NULL;
    CurrProc->targ.push_local_size = 0;
    CurrProc->targ.debug = NULL;
    CurrProc->targ.return_points = NULL;
    CurrProc->targ.sp_frame = false;
    CurrProc->targ.sp_align = false;
    CurrProc->targ.has_fd_temps = false;
    CurrProc->targ.never_sp_frame = false;
    CurrProc->targ.tls_index = NULL;
}


void    SaveToTargProc( void )
/****************************/
{
    CurrProc->targ.max_stack = MaxStack;
}


void    RestoreFromTargProc( void )
/*********************************/
{
    MaxStack = CurrProc->targ.max_stack;
}


void    PushInSameBlock( instruction *ins )
/*****************************************/
{
#if ( _TARGET & _TARG_8086 )
    /* unused parameters */ (void)ins;
#else
    if( InsBlock( ins ) != CurrBlock ) {
        CurrProc->targ.never_sp_frame = true;
    }
#endif
}


instruction *   PushOneParm( instruction *ins, name *curr,
                                type_class_def type_class,
                                type_length offset,
                                call_state *state )
/********************************************************/
{
    instruction *new;
//    int         size;

    /* unused parameters */ (void)state; (void)offset;

    new = MakeUnary( OP_PUSH, curr, NULL, type_class );
    SuffixIns( ins, new );
#if 0
    if( curr->n.class == N_CONSTANT ) {
        size = TypeClassSize[type_class];
    } else {
        size = curr->n.size;
    }
#endif
    return( new );
}


void    PreCall( cn call )
/************************/
{
    /* unused parameters */ (void)call;
}


void    PostCall( cn call )
/*************************/
{
    /* unused parameters */ (void)call;
}

type_def    *PassParmType( cg_sym_handle func, type_def* tipe, call_class cclass )
/********************************************************************************/
{
    if( cclass & FAR16_CALL )
        return( tipe );
    return( QParmType( func, NULL, tipe ) );
}
