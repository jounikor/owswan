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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "cgaux.h"
#include "cgmem.h"
#include "data.h"
#include "utils.h"
#include "rgtbl.h"
#include "typemap.h"
#include "makeblk.h"
#include "bldcall.h"
#include "feprotos.h"


hw_reg_set SavedRegs( void )
/**************************/
{
    hw_reg_set          saved;

    HW_CAsgn( saved, HW_EMPTY );
    HW_CTurnOn( saved, HW_R9 );
    HW_CTurnOn( saved, HW_R10 );
    HW_CTurnOn( saved, HW_R11 );
    HW_CTurnOn( saved, HW_R12 );
    HW_CTurnOn( saved, HW_R13 );
    HW_CTurnOn( saved, HW_R14 );
    HW_CTurnOn( saved, HW_R15 );
    HW_CTurnOn( saved, HW_R26 );
    HW_CTurnOn( saved, HW_F2 );
    HW_CTurnOn( saved, HW_F3 );
    HW_CTurnOn( saved, HW_F4 );
    HW_CTurnOn( saved, HW_F5 );
    HW_CTurnOn( saved, HW_F6 );
    HW_CTurnOn( saved, HW_F7 );
    HW_CTurnOn( saved, HW_F8 );
    HW_CTurnOn( saved, HW_F9 );
    return( saved );
}

void    UpdateReturn( call_state *state, type_def *tipe, type_class_def type_class, aux_handle aux )
/**************************************************************************************************/
{
    /* unused parameters */ (void)tipe; (void)aux;

    state->return_reg = ReturnReg( type_class );
}

type_class_def  CallState( aux_handle aux, type_def *tipe, call_state *state )
/****************************************************************************/
{
    type_class_def      type_class;
    byte                i;
    hw_reg_set          parms[20];
    hw_reg_set          *parm_src;
    hw_reg_set          *parm_dst;
    call_class          cclass;

    state->unalterable = FixedRegs();
    if( FEAttr( AskForLblSym( CurrProc->label ) ) & FE_VARARGS ) {
        HW_TurnOn( state->unalterable, VarargsHomePtr() );
    }
    HW_CAsgn( state->modify, HW_FULL );
    HW_TurnOff( state->modify, SavedRegs() );
    HW_CTurnOff( state->modify, HW_UNUSED );
    state->used = state->modify;     /* anything not saved is used*/
    state->attr = 0;
    cclass = *(call_class *)FEAuxInfo( aux, CALL_CLASS );
    if( cclass & SETJMP_KLUGE ) {
        state->attr |= ROUTINE_IS_SETJMP;
    }
    if( cclass & SUICIDAL ) {
        state->attr |= ROUTINE_NEVER_RETURNS;
    }
    if( cclass & NO_MEMORY_CHANGED ) {
        state->attr |= ROUTINE_MODIFIES_NO_MEMORY;
    }
    if( cclass & NO_MEMORY_READ ) {
        state->attr |= ROUTINE_READS_NO_MEMORY;
    }
    i = 0;
    parm_dst = &parms[0];
    for( parm_src = ParmRegs(); !HW_CEqual( *parm_src, HW_EMPTY ); ++parm_src ) {
        *parm_dst = *parm_src;
        if( HW_Ovlap( *parm_dst, state->unalterable ) ) {
            FEMessage( MSG_BAD_SAVE, aux );
        }
        HW_CTurnOff( *parm_dst, HW_UNUSED );
        parm_dst++;
        i++;
    }
    *parm_dst = *parm_src;
    i++;
    state->parm.table = CGAlloc( i * sizeof( hw_reg_set ) );
    Copy( parms, state->parm.table, i * sizeof( hw_reg_set ) );
    HW_CAsgn( state->parm.used, HW_EMPTY );
    state->parm.curr_entry = state->parm.table;
    state->parm.offset  = 0;
    type_class = ReturnTypeClass( tipe, state->attr );
    UpdateReturn( state, tipe, type_class, aux );
    return( type_class );
}

#if 0
hw_reg_set      RAReg( void )
/***************************/
{
    return( HW_R26 );
}
#endif

hw_reg_set      CallZap( call_state *state )
/******************************************/
{
    hw_reg_set  zap;
    hw_reg_set  tmp;

    zap = state->modify;
    if( (state->attr & ROUTINE_MODIFY_EXACT) == 0 ) {
        HW_TurnOn( zap, state->parm.used );
        HW_TurnOn( zap, state->return_reg );
        HW_TurnOn( zap, ReturnAddrReg() );
        tmp = ReturnReg( WD );
        HW_TurnOn( zap, tmp );
    }
    return( zap );
}

hw_reg_set      MustSaveRegs( void )
/**********************************/
{
    hw_reg_set  save;
    hw_reg_set  tmp;

    HW_CAsgn( save, HW_FULL );
    HW_TurnOff( save, CurrProc->state.modify );
    HW_CTurnOff( save, HW_UNUSED );
    if( CurrProc->state.attr & ROUTINE_MODIFY_EXACT ) {
        HW_TurnOff( save, CurrProc->state.return_reg );
    } else {
        tmp = CurrProc->state.parm.used;
        HW_TurnOn( tmp, CurrProc->state.return_reg );
        HW_TurnOff( save, tmp );
    }
    tmp = StackReg();
    HW_TurnOff( save, tmp );
    if( HW_CEqual( CurrProc->state.return_reg, HW_EMPTY ) ) {
        tmp = ReturnReg( WD );
        HW_TurnOff( save, tmp );
    }
    tmp = CurrProc->state.unalterable;
    HW_TurnOff( tmp, DisplayReg() );
    HW_TurnOff( tmp, StackReg() );
    HW_TurnOff( save, tmp );
    return( save );
}

hw_reg_set      SaveRegs( void )
/******************************/
{
    hw_reg_set   save;

    save = MustSaveRegs();
    HW_OnlyOn( save, CurrProc->state.used );
    return( save );
}

bool            IsStackReg( name *n )
/***********************************/
{
    if( n == NULL )
        return( false );
    if( n->n.class != N_REGISTER )
        return( false );
    if( !HW_CEqual( n->r.reg, HW_R30 ) && !HW_CEqual( n->r.reg, HW_D30 ) )
        return( false );
    return( true );
}

hw_reg_set      HighOffsetReg( hw_reg_set regs )
/**********************************************/
{
    /* unused parameters */ (void)regs;

    return( HW_EMPTY );
}

hw_reg_set      LowOffsetReg( hw_reg_set regs )
/*********************************************/
{
    /* unused parameters */ (void)regs;

    return( HW_EMPTY );
}
