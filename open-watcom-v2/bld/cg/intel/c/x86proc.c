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
* Description:  Intel x86 procedure call generation.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "cgaux.h"
#include "stackcg.h"
#include "zoiks.h"
#include "data.h"
#include "display.h"
#include "rtrtn.h"
#include "utils.h"
#include "objout.h"
#include "dbsyms.h"
#include "flowsave.h"
#include "object.h"
#include "x86enc2.h"
#include "encode.h"
#include "targetin.h"
#include "targetdb.h"
#include "x86proc.h"
#include "opttell.h"
#include "i87data.h"
#include "x86objd.h"
#include "x86obj.h"
#include "optab.h"
#include "rgtbl.h"
#include "x86base.h"
#include "pccode.h"
#include "pcencode.h"
#include "x86enc.h"
#include "x86temps.h"
#include "feprotos.h"


#define WINDOWS_CHEAP  ( ( _IsModel( DLL_RESIDENT_CODE ) &&         \
               ( CurrProc->state.attr & ROUTINE_LOADS_DS ) )        \
            || ( _IsTargetModel( CHEAP_WINDOWS )                    \
               && (CurrProc->prolog_state & (GENERATE_EXPORT | GENERATE_FAT_PROLOG)) == 0 ) )

#define DO_WINDOWS_CRAP ( _IsTargetModel( WINDOWS )                 \
               && ( !WINDOWS_CHEAP || CurrProc->contains_call ) )

#define DO_BP_CHAIN ( ( (_IsTargetModel( NEED_STACK_FRAME ) || _IsModel( DBG_CV ) ) \
               && CurrProc->contains_call )                                         \
             || (CurrProc->prolog_state & GENERATE_FAT_PROLOG) )

#define CHAIN_FRAME ( DO_WINDOWS_CRAP || DO_BP_CHAIN )

#define CHEAP_FRAME ( _IsTargetModel( NEED_STACK_FRAME ) || \
              _IsntTargetModel( WINDOWS ) || WINDOWS_CHEAP )

#define FAR_RET_ON_STACK ( (_RoutineIsLong( CurrProc->state.attr ) ) \
             && (CurrProc->state.attr & ROUTINE_NEVER_RETURNS) == 0 )

#define HW_STACK_CHECK HW_xAX
#define HW_LOAD_DS     HW_xAX

type_length StackDepth;

hw_reg_set   PushRegs[] = {
    HW_D( HW_xAX ),
    HW_D( HW_xBX ),
    HW_D( HW_xCX ),
    HW_D( HW_xDX ),
    HW_D( HW_xSI ),
    HW_D( HW_xDI ),
    HW_D( HW_DS ),
    HW_D( HW_ES ),
    HW_D( HW_FS ),
    HW_D( HW_GS ),
    HW_D( HW_SS ),
    HW_D( HW_xBP ),
    HW_D( HW_EMPTY )
};


static  bool    ScanInstructions( void )
/**********************************/
{
    block       *blk;
    instruction     *ins;
    name        *addr;
    bool        sp_constant;

    CurrProc->contains_call = true;
    if( BlockByBlock )
        return( false );
    CurrProc->contains_call = false;
    sp_constant = true;
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            if( _OpIsCall( ins->head.opcode ) ) {
                if( ins->head.opcode == OP_CALL_INDIRECT ) {
                    CurrProc->contains_call = true;
                } else {
                    addr = ins->operands[CALL_OP_ADDR];
                    if( addr->n.class != N_MEMORY || addr->m.memory_type != CG_LBL ||
                      !AskIfRTLabel( addr->v.symbol ) ) {
                        CurrProc->contains_call = true;
                    }
                }
                if( HW_COvlap( ins->zap->reg, HW_xSP ) ) {
                    CurrProc->state.attr |= ROUTINE_NEEDS_PROLOG;
                    sp_constant = false;
                }
            }
        }
    }
    return( sp_constant );
}


#if _TARGET & _TARG_80386
static  void    ChkFDOp( name *op, level_depth depth )
/****************************************************/
{
    if( op->n.class != N_TEMP )
        return;
    if( (op->v.usage & (USE_IN_ANOTHER_BLOCK | USE_ADDRESS)) == 0 )
        return;
    if( op->t.temp_flags & STACK_PARM )
        return;
    if( op->t.location != NO_LOCATION ) {
        if( op->v.usage & (USE_ADDRESS | HAS_MEMORY) )
            return;
        if( op->t.alias != op )
            return;
        if( depth == 0 )
            return;
        op->t.location = NO_LOCATION;
    }
    op->t.temp_flags |= USED_AS_FD;
    CurrProc->targ.has_fd_temps = true;
}
#endif


#if _TARGET & _TARG_80386
static  void    ScanForFDOps( void )
/**********************************/
{
    block       *blk;
    instruction *ins;
    opcnt       i;
    level_depth depth;

    CurrProc->contains_call = false;
    if( BlockByBlock )
        return;
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        depth = blk->depth;
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            if( ins->type_class == FD || ins->type_class == FL ) {
                for( i = ins->num_operands; i-- > 0; ) {
                    ChkFDOp( ins->operands[i], depth );
                }
                if( ins->result != NULL ) {
                    ChkFDOp( ins->result, depth );
                }
            }
        }
    }
}
#endif


#if _TARGET & _TARG_80386
static  block *ScanForLabelReturn( block *blk ) {
/***********************************************/

    block       *son;
    block_num   i;

    if( _IsBlkAttr( blk, BLK_RETURN | BLK_CALL_LABEL ) )
        return( NULL );
    blk->edge[0].flags |= DOWN_ONE_CALL;
    if( _IsBlkAttr( blk, BLK_LABEL_RETURN ) )
        return( blk );
    for( i = 0; i < blk->targets; ++i ) {
        son = blk->edge[i].destination.u.blk;
        if( son->edge[0].flags & DOWN_ONE_CALL )
            continue;
        if( SafeRecurseCG( (func_sr)ScanForLabelReturn, son ) == NULL ) {
            return( NULL );
        }
    }
    return( blk );
}
#endif


#if _TARGET & _TARG_80386
static  bool    ScanLabelCalls( void ) {
/*********************************

    Make sure that all blocks that are called are only called
    one level deep. Mark output edges of all such blocks as
    DOWN_ONE_CALL. This is so we can adjust our x[esp] offsets
    accordingly.
*/

    block   *blk;

    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        if( !_IsBlkAttr( blk, BLK_CALL_LABEL ) )
            continue;
        if( ScanForLabelReturn( blk->edge[0].destination.u.blk ) == NULL ) {
            return( false );
        }
    }
    return( true );
}
#endif


static  void    AdjustPushLocals( void ) {
/**********************************/

    instruction *ins;

    for( ins = HeadBlock->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
        if( DoesSomething( ins ) )
            break;
        if( ins->head.opcode == OP_MOV && ins->head.state == OPERANDS_NEED_WORK ) {
            QuickSave( ins->operands[0]->r.reg, OP_PUSH );
            AdjustPushLocal( ins->result );
        }
    }
}

static  bool    NeedBPProlog( void ) {
/******************************/

    if( CurrProc->parms.size != 0 )
        return( true );
    if( CurrProc->locals.size != 0 )
        return( true );
    if( CurrProc->targ.push_local_size != 0 )
        return( true );
    if( CurrProc->lex_level > 0 )
        return( true );
    if( CurrProc->targ.sp_align )
        return( true );
    if( BlockByBlock != 0 )
        return( true );
    if( CurrProc->state.attr & ROUTINE_NEEDS_PROLOG )
        return( true );
    if( FAR_RET_ON_STACK ) {
        if( CHAIN_FRAME ) {
            return( true );
        }
    }
    return( false );
}


static void FindIfExported( void ) {
/****************************/

    cg_sym_handle sym;

    sym = AskForLblSym( CurrProc->label );
    if( sym == NULL )
        return;
    if( SymIsExported( sym ) ) {
        CurrProc->prolog_state |= GENERATE_EXPORT;
    }
}


static  bool    NeedStackCheck( void )
/******************************/
{
    return( FEStackChk( AskForLblSym( CurrProc->label ) ) != 0 );
}


static void DoStackCheck( void ) {
/**************************/

    if( CurrProc->prolog_state & ( GENERATE_THUNK_PROLOG | GENERATE_RDOSDEV_PROLOG ) )
        return;
#if _TARGET & _TARG_80386
    if( CurrProc->prolog_state & GENERATE_GROW_STACK ) {
        if( BlockByBlock || CurrProc->locals.size >= 4 * 1024 ) {
            GenUnkPush( &CurrProc->targ.stack_check );
            DoRTCall( RT_GROW, true );
        }
        return;
    }
#endif
    if( NeedStackCheck() ) {
#if _TARGET & _TARG_8086
        if( HW_COvlap( CurrProc->state.parm.used, HW_STACK_CHECK ) ) {
            QuickSave( HW_STACK_CHECK, OP_PUSH );
        }
        GenUnkMov( HW_STACK_CHECK, &CurrProc->targ.stack_check );
        DoRTCall( RT_CHK, false );
        if( HW_COvlap( CurrProc->state.parm.used, HW_STACK_CHECK ) ) {
            QuickSave( HW_STACK_CHECK, OP_POP );
        }
#else
        GenUnkPush( &CurrProc->targ.stack_check );
        DoRTCall( RT_CHK, true );
#endif
    }
}


static  void    EmitNameInCode( void )
/************************************/
{
    cg_sym_handle   sym;
    const char      *name;
    label_handle    lbl;
    uint            len;

    sym = AskForLblSym( CurrProc->label );
    if( sym == NULL )
        return;
    name = FEName( sym );
    len = Length( name );
    lbl = AskForNewLabel();
    TellKeepLabel( lbl );
    CodeLabel( lbl, 0 );
    GenKillLabel( lbl );
    EyeCatchBytes( name, len );
}


static  int ProEpiDataSize( void )
/********************************/
{
    return( _RoundUp( (int)(pointer_uint)FEAuxInfo( NULL, PROEPI_DATA_SIZE ), WORD_SIZE ) );
}


static  void    PrologHook( void )
/********************************/
{
    int      size;

    if( (CurrProc->prolog_state & GENERATE_PROLOG_HOOKS) == 0 )
        return;
    size = ProEpiDataSize();
    if( size != 0 ) {
        GenRegSub( HW_xSP, size );
        CurrProc->targ.base_adjust += size;
    }
#if _TARGET & _TARG_8086
    DoRTCall( RT_PROHOOK, false );
#else
//    GenPushC( CurrProc->parms.size );
    DoRTCall( RT_PROHOOK, false );
#endif
}


static  void    EpilogHook( void )
/********************************/
{
    int      size;

    if( CurrProc->prolog_state & GENERATE_EPILOG_HOOKS ) {
        DoRTCall( RT_EPIHOOK, false );
    }
    size = ProEpiDataSize();
    if( size != 0 ) {
        GenRegAdd( HW_xSP, size );
    }
}


static  void    DoLoadDS( void )
{
#if _TARGET & _TARG_8086
    if( _IsntTargetModel( BIG_CODE ) && _RoutineIsInterrupt( CurrProc->state.attr ) ) {
#else
    if( _IsntTargetModel( LOAD_DS_DIRECTLY ) ) {
#endif
        DoRTCall( RT_GETDS, false );
    } else {
        if( HW_COvlap( CurrProc->state.parm.used, HW_LOAD_DS ) ) {
            QuickSave( HW_LOAD_DS, OP_PUSH );
        }
        GenLoadDS();
        if( HW_COvlap( CurrProc->state.parm.used, HW_LOAD_DS ) ) {
            QuickSave( HW_LOAD_DS, OP_POP );
        }
    }
}


static  int LoadDS( void )
/************************/
{
    int     size;

    size = 0;
    if( CurrProc->state.attr & ROUTINE_LOADS_DS ) {
        if( HW_COvlap( CurrProc->state.unalterable, HW_DS ) ) {
            QuickSave( HW_DS, OP_PUSH );
            size = WORD_SIZE;
            DoLoadDS();
        }
    }
    return( size );
}


static  void    UnloadDS( void )
/******************************/
{
    if( CurrProc->state.attr & ROUTINE_LOADS_DS ) {
        if( HW_COvlap( CurrProc->state.unalterable, HW_DS ) ) {
            QuickSave( HW_DS, OP_POP );
        }
    }
}


static  void    MoveParms( void )
/*******************************/
{
    int     i;

    for( i = 0; Parm8087[i] != NULL; ++i ) {
        GFstpM( Parm8087[i] );
    }
}


static  void    AllocStack( void )
/********************************/
{
    type_length     size;

    /* keep stack aligned */
    size = _RoundUp( CurrProc->locals.size, WORD_SIZE );
    CurrProc->locals.size = size;
    if( BlockByBlock ) {
        GenUnkSub( HW_xSP, &CurrProc->targ.prolog_loc );
        if( CurrProc->prolog_state & GENERATE_TOUCH_STACK ) {
            GenTouchStack( true );
        }
    } else if( size <= 2 * WORD_SIZE && OptForSize > 50 ) {
        while( size > 0 ) {
            QuickSave( HW_xAX, OP_PUSH );
            size -= WORD_SIZE;
        }
    } else if( size != 0 ) {
        GenRegSub( HW_xSP, size );
        if( CurrProc->prolog_state & GENERATE_TOUCH_STACK ) {
            GenTouchStack( false );
        }
    }
    if( CurrProc->targ.sp_align ) {
        GenRegAnd( HW_xSP, -( 2 * WORD_SIZE ) );
        CurrProc->prolog_state |= GENERATE_RESET_SP;
    }
}


static  int PushAll( void )
/*************************/
/* Save all registers and establish somewhat sane environment.
 * Used for __interrupt routines only.
 */
{
    if( _CPULevel( CPU_186 ) ) {
        Gpusha();
    } else {
        QuickSave( HW_xAX, OP_PUSH );
        QuickSave( HW_xCX, OP_PUSH );
        QuickSave( HW_xDX, OP_PUSH );
        QuickSave( HW_xBX, OP_PUSH );
        QuickSave( HW_xSP, OP_PUSH );
        QuickSave( HW_xBP, OP_PUSH );
        QuickSave( HW_xSI, OP_PUSH );
        QuickSave( HW_xDI, OP_PUSH );
    }
    QuickSave( HW_DS, OP_PUSH );
    QuickSave( HW_ES, OP_PUSH );
    if( _CPULevel( CPU_386 ) ) {
        QuickSave( HW_FS, OP_PUSH );
        QuickSave( HW_GS, OP_PUSH );
    } else {
        QuickSave( HW_xAX, OP_PUSH );
        QuickSave( HW_xAX, OP_PUSH );
    }
    GenRegMove( HW_xSP, HW_xBP );
    AllocStack();
    Gcld();
    if( HW_COvlap( CurrProc->state.unalterable, HW_DS ) ) {
        DoLoadDS();
        // If ES is also unalterable, copy DS to ES; else things
        // like memcpy() are likely to blow up
        if( HW_COvlap( CurrProc->state.unalterable, HW_ES ) ) {
            QuickSave( HW_DS, OP_PUSH );
            QuickSave( HW_ES, OP_POP );
        }
    }
    /* 8 general purpose registers + 4 segment registers */
    return( 12 * WORD_SIZE );
}


static  void    PopAll( void ) {
/************************/

    if( CurrProc->locals.size != 0 ) {
        GenRegMove( HW_xBP, HW_xSP );
    }
    if( _CPULevel( CPU_386 ) ) {
        QuickSave( HW_GS, OP_POP );
        QuickSave( HW_FS, OP_POP );
    } else {
        QuickSave( HW_xAX, OP_POP );
        QuickSave( HW_xAX, OP_POP );
    }
    QuickSave( HW_ES, OP_POP );
    QuickSave( HW_DS, OP_POP );
    if( _CPULevel( CPU_186 ) ) {
        Gpopa();
    } else {
        QuickSave( HW_xDI, OP_POP );
        QuickSave( HW_xSI, OP_POP );
        QuickSave( HW_xBP, OP_POP );
        QuickSave( HW_xBX, OP_POP );
        QuickSave( HW_xBX, OP_POP );
        QuickSave( HW_xDX, OP_POP );
        QuickSave( HW_xCX, OP_POP );
        QuickSave( HW_xAX, OP_POP );
    }
}


static  void    DoEnter( level_depth level )
/******************************************/
{
    type_length size;

    /* keep stack aligned */
    size = _RoundUp( CurrProc->locals.size, WORD_SIZE );
    CurrProc->locals.size = size;

    CurrProc->parms.base += WORD_SIZE;
    if( BlockByBlock ) {
        GenUnkEnter( &CurrProc->targ.prolog_loc, level );
        if( CurrProc->prolog_state & GENERATE_TOUCH_STACK ) {
            GenTouchStack( true );
        }
    } else {
        GenEnter( size, level );
        if( size != 0 && ( CurrProc->prolog_state & GENERATE_TOUCH_STACK ) ) {
            GenTouchStack( false );
        }
    }
}


static  void    Enter( void ) {
/***********************/

    level_depth lex_level;
    int         i;

    lex_level = CurrProc->lex_level;
#if _TARGET & _TARG_80386
    if( !CurrProc->targ.sp_frame && _CPULevel( CPU_186 ) && CurrProc->locals.size < 65536
      && ( lex_level > 0 || ( CurrProc->locals.size != 0 && OptForSize > 50 ) ) ) {
#else
    if( !CurrProc->targ.sp_frame && _CPULevel( CPU_186 )
      && ( lex_level > 0 || ( CurrProc->locals.size != 0 && OptForSize > 50 ) ) ) {
#endif
        DoEnter( lex_level );
        HW_CTurnOn( CurrProc->state.used, HW_xBP );
        CurrProc->state.attr |= ROUTINE_NEEDS_PROLOG;
    } else {
        if( NeedBPProlog() ) {
            if( !CurrProc->targ.sp_frame || CurrProc->targ.sp_align ) {
                HW_CTurnOn( CurrProc->state.used, HW_xBP );
                CurrProc->parms.base += WORD_SIZE;
                QuickSave( HW_xBP, OP_PUSH );
                i = 0;
                while( --lex_level > 0 ) {
                    i -= 2;
                    GenPushOffset( i );
                }
                GenRegMove( HW_xSP, HW_xBP );
                if( CurrProc->lex_level > 1 ) {
                    GenRegAdd( HW_xBP, ( CurrProc->lex_level - 1 ) * WORD_SIZE );
                }
                if( CurrProc->lex_level > 0 ) {
                    QuickSave( HW_xBP, OP_PUSH );
                }
            }
            CurrProc->state.attr |= ROUTINE_NEEDS_PROLOG;
        }
        AllocStack();
    }
}


static  void    CalcUsedRegs( void ) {
/******************************/

    block   *blk;
    instruction *ins;
    name    *result;
    hw_reg_set  used;

    HW_CAsgn( used, HW_EMPTY );
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            result = ins->result;
            if( result != NULL && result->n.class == N_REGISTER ) {
                HW_TurnOn( used, result->r.reg );
            }
            /* place holder for big label doesn't really zap anything */
            if( ins->head.opcode != OP_NOP ) {
                HW_TurnOn( used, ins->zap->reg );
                if( HW_COvlap( ins->zap->reg, HW_xSP ) ) {
                    CurrProc->prolog_state |= GENERATE_RESET_SP;
                }
            }
        }
    }
    if( !CurrProc->targ.sp_frame || CurrProc->targ.sp_align ) {
        HW_CTurnOff( used, HW_xBP );
    }
    HW_TurnOn( CurrProc->state.used, used );
}


static  int Push( hw_reg_set to_push )
/************************************/
{
    int         size;
    int         i;

    size = 0;
    if( _IsntModel( NO_OPTIMIZATION ) && CurrProc->targ.sp_frame && !CurrProc->targ.sp_align ) {
        FlowSave( &to_push );
    }
    for( i = 0; i < sizeof( PushRegs ) / sizeof( PushRegs[0] ) - 1; i++ ) {
        if( HW_CEqual( to_push, HW_EMPTY ) )
            break;
        if( HW_Ovlap( PushRegs[i], to_push ) ) {
            QuickSave( PushRegs[i], OP_PUSH );
            size += WORD_SIZE;
            HW_TurnOff( to_push, PushRegs[i] );
        }
    }
    return( size );
}

static  void        Pop( hw_reg_set to_pop )
/******************************************/
{
    int         i;

    if( _IsntModel( NO_OPTIMIZATION ) && CurrProc->targ.sp_frame && !CurrProc->targ.sp_align ) {
        FlowRestore( &to_pop );
    }
    for( i = sizeof( PushRegs ) / sizeof( PushRegs[0] ) - 1; i-- > 0 ; ) {
        if( HW_CEqual( to_pop, HW_EMPTY ) )
            break;
        if( HW_Ovlap( PushRegs[i], to_pop ) ) {
            QuickSave( PushRegs[i], OP_POP );
            HW_TurnOff( to_pop, PushRegs[i] );
        }
    }
}

static void    GenLeaSP( int offset )
/************************************
    LEA         [e]sp,[[e]bp+offset]
*/
{
    _Code;
    LayOpword( M_LEA );
    OpndSize( HW_xSP );
    LayReg( HW_xSP );
    Inst[RMR] |= Displacement( offset, HW_xBP );
    Inst[RMR] |= DoIndex( HW_xBP );
    _Emit;
}

static  void    DoEpilog( void )
/******************************/
{
    hw_reg_set  to_pop;
    bool    is_long;
    type_length size;

    if( _RoutineIsInterrupt( CurrProc->state.attr ) ) {
        PopAll();
    } else {
        to_pop = SaveRegs();
        HW_CTurnOff( to_pop, HW_FLTS );
        if( CHAIN_FRAME ) {
            if( (CurrProc->state.attr & ROUTINE_NEEDS_PROLOG)
             || CurrProc->locals.size+CurrProc->targ.push_local_size != 0 ) {
                if( CurrProc->targ.base_adjust == 0 ) {
                    GenRegMove( HW_xBP, HW_xSP );
                } else {
                    GenLeaSP( -CurrProc->targ.base_adjust );
                }
            }
            HW_CTurnOff( to_pop, HW_xBP );
        } else {
            if( CurrProc->state.attr & ROUTINE_NEEDS_PROLOG ) {
                size = CurrProc->locals.size + CurrProc->targ.push_local_size;
                if( (CurrProc->prolog_state & GENERATE_RESET_SP) || size != 0 ) {
                    /* sp is not pointing at saved registers already */
                    if( CurrProc->targ.sp_frame ) {
                        if( CurrProc->targ.sp_align ) {
                            GenRegMove( HW_xBP, HW_xSP );
                            QuickSave( HW_xBP, OP_POP );
                        } else if( size != 0 ) {
                            GenRegAdd( HW_xSP, size );
                        }
                    } else if( CurrProc->targ.base_adjust != 0 ) {
                        GenLeaSP( -CurrProc->targ.base_adjust );
                    } else if( _CPULevel( CPU_186 ) && (!_CPULevel( CPU_486 ) || OptForSize > 50) ) {
                        GenLeave();
                        HW_CTurnOff( to_pop, HW_xBP );
                    } else {
                        GenRegMove( HW_xBP, HW_xSP );
                    }
                }
            }
        }
        Pop( to_pop );
        UnloadDS();
        if( CHAIN_FRAME ) {
            if( NeedBPProlog() ) {
                EpilogHook();
                if( FAR_RET_ON_STACK ) {
                    if( CHEAP_FRAME ) {
                        GenCypWindowsEpilog();
                    } else {
                        GenWindowsEpilog();
                    }
                } else {
                    QuickSave( HW_xBP, OP_POP );
                }
            }
        }
        if( CurrProc->prolog_state & GENERATE_RDOSDEV_PROLOG ) {
            GenRdosdevEpilog();
        }
    }

    is_long = _RoutineIsLong( CurrProc->state.attr ) ||
        _RoutineIsFar16( CurrProc->state.attr );
#if _TARGET & _TARG_80386
    if( CurrProc->prolog_state & GENERATE_THUNK_PROLOG ) {
        QuickSave( HW_xSP, OP_POP );
    }
    if( _IsTargetModel( NEW_P5_PROFILING | P5_PROFILING ) ) {
        GenP5ProfilingEpilog( CurrProc->label );
    }
#endif

    if( _RoutineIsInterrupt( CurrProc->state.attr ) ) {
        GenReturn( 0, false, true );
    } else if( CurrProc->state.attr & ROUTINE_REMOVES_PARMS ) {
        GenReturn( CurrProc->parms.size, is_long, false );
    } else {
        GenReturn( 0, is_long, false );
    }
}

bool    CanZapBP( void )
/**********************/
{
    return( !CHAIN_FRAME );
}


void    AddCacheRegs( void )
/**************************/
{
#if _TARGET & _TARG_80386
    if( CurrProc->targ.never_sp_frame )
        return;
    if( _IsntModel( MEMORY_LOW_FAILS ) )
        return;
    if( OptForSize > 50 )
        return;
    if( _IsTargetModel( FLOATING_DS | FLOATING_SS ) )
        return;
    if( !ScanInstructions() )
        return;
    if( !CanZapBP() )
        return;
    if( !ScanLabelCalls() )
        return;
    if( CurrProc->state.attr & ROUTINE_WANTS_DEBUGGING )
        return;
    if( CurrProc->lex_level > 0 )
        return;
    if( _FPULevel( FPU_586 ) ) {
        ScanForFDOps();
    }
    if( CurrProc->targ.has_fd_temps ) {
        CurrProc->targ.sp_frame = true;
        CurrProc->targ.sp_align = true;
    } else if( !DO_BP_CHAIN && _IsntTargetModel( WINDOWS ) &&
            !_RoutineIsInterrupt( CurrProc->state.attr ) ) {
        /*
         * We cannot make EBP available under Windows because the SS
         * selector might not cover the data segment and so we cannot use
         * it as an index. Puke - BBB Feb 18, 1994
         */
        CurrProc->targ.sp_frame = true;
        HW_CTurnOff( CurrProc->state.unalterable, HW_xBP );
    }
#endif
}


void DoRTCall( rt_class rtindex, bool pop )
/*****************************************/
{
    DoCall( RTLabel( rtindex ), true, _IsTargetModel( BIG_CODE ), pop );
}

void    GenProlog( void )
/***********************/
{
    segment_id  old_segid;
    hw_reg_set  to_push;
    unsigned    ret_size;
    pointer     label;
    pointer     origlabel; // Original label for generated __far16 thunks
    fe_attr     attr;

    ScanInstructions();       /* Do These 2 calls before using DO_WINDOWS_CRAP! */
    FindIfExported();
    old_segid = SetOP( AskCodeSeg() );

    if( CurrProc->prolog_state & GENERATE_FUNCTION_NAME ) {
        EmitNameInCode();
    }

    if( _IsModel( DBG_NUMBERS ) ) {
        CodeLineNumber( HeadBlock->ins.hd.line_num, false );
    }

    if( _IsModel( DBG_LOCALS ) ){  // d1+ or d2
        EmitRtnBeg();
    }
    if( CurrProc->state.attr & ROUTINE_WANTS_DEBUGGING ) {
        CurrProc->state.attr |= ROUTINE_NEEDS_PROLOG;
    }

    CurrProc->parms.base = 0;
    CurrProc->parms.size = CurrProc->state.parm.offset;

    origlabel = label = CurrProc->label;

#if _TARGET & _TARG_80386
    if( _RoutineIsFar16( CurrProc->state.attr ) ) {
        label = GenFar16Thunk( CurrProc->label, CurrProc->parms.size,
                    CurrProc->state.attr & ROUTINE_REMOVES_PARMS );
        // CurrProc->label = label; - ugly mess if following are combined
    }
#endif

    CodeLabel( label, DepthAlign( PROC_ALIGN ) );

    attr = FEAttr( AskForLblSym( origlabel ) );

    if( CurrProc->prolog_state & GENERATE_RDOSDEV_PROLOG ) {
        GenRdosdevProlog();
    }

#if _TARGET & _TARG_80386
    if( (attr & FE_NAKED) == 0 ) {
        if( _IsTargetModel( NEW_P5_PROFILING | P5_PROFILING ) ) {
            GenP5ProfilingProlog( label );
        }
        if( CurrProc->prolog_state & GENERATE_THUNK_PROLOG ) {
            QuickSave( HW_xSP, OP_PUSH );
            GenPushC( CurrProc->parms.size );
            GenUnkPush( &CurrProc->targ.stack_check );
            if( NeedStackCheck() ) {
                DoRTCall( RT_THUNK_STK, true );
            } else {
                DoRTCall( RT_THUNK, true );
            }
            CurrProc->parms.base += WORD_SIZE;
        }
    }
#endif

    if( _RoutineIsInterrupt( CurrProc->state.attr ) ||
        (CurrProc->state.attr & ROUTINE_NEVER_RETURNS) ) {
        ret_size = 0;
    } else if( _RoutineIsLong( CurrProc->state.attr ) ) {
        ret_size = 2 * WORD_SIZE;
    } else if( _RoutineIsFar16( CurrProc->state.attr ) ) {
        ret_size = 2 * WORD_SIZE;
    } else {
        ret_size = WORD_SIZE;
    }

    CurrProc->parms.base += ret_size;
    CalcUsedRegs();

    to_push = SaveRegs();
    HW_CTurnOff( to_push, HW_FLTS );
    if( !CurrProc->targ.sp_frame || CurrProc->targ.sp_align ) {
        HW_CTurnOff( to_push, HW_xBP );
    }

    if( attr & FE_NAKED ) {
        // don't do anything - empty prologue
    } else if( _RoutineIsInterrupt( CurrProc->state.attr ) ) {
        ret_size = -PushAll();
        CurrProc->targ.base_adjust = 0;
        MoveParms();
    } else {
        if( CHAIN_FRAME ) {
            CurrProc->targ.base_adjust = 0;
            if( NeedBPProlog() ) {
                HW_CTurnOn( CurrProc->state.used, HW_xBP );
                CurrProc->parms.base += WORD_SIZE;
                if( FAR_RET_ON_STACK ) {
                    if( CHEAP_FRAME ) {
                        GenCypWindowsProlog();
                    } else {
#if _TARGET & _TARG_8086
                        // Windows prologs zap AX, so warn idiot user if we
                        // generate one for a routine in which AX is live
                        // upon entry to routine, or unalterable.
                        if( HW_COvlap( CurrProc->state.unalterable, HW_xAX ) ||
                            HW_COvlap( CurrProc->state.parm.used, HW_xAX ) ) {
                            FEMessage( MSG_ERROR, "exported routine with AX live on entry" );
                        }
#endif
                        GenWindowsProlog();
                        CurrProc->targ.base_adjust += 2; /* the extra push DS */
                    }
                } else {
                    QuickSave( HW_xBP, OP_PUSH );
                    GenRegMove( HW_xSP, HW_xBP );
                }
                PrologHook();
            }
            DoStackCheck();
            CurrProc->targ.base_adjust += LoadDS();
            CurrProc->targ.base_adjust += Push( to_push );
            CurrProc->parms.base += CurrProc->targ.base_adjust;
            AllocStack();
            AdjustPushLocals();
        } else {
            DoStackCheck();
            CurrProc->parms.base += LoadDS();
            if( (CurrProc->state.attr & ROUTINE_NEVER_RETURNS) == 0 ) {
                CurrProc->parms.base += Push( to_push );
            }
            Enter();
            AdjustPushLocals();
            if( _IsModel( NO_OPTIMIZATION ) || CurrProc->targ.sp_frame ) {
                CurrProc->targ.base_adjust = 0;
            } else {
                CurrProc->targ.base_adjust = AdjustBase();
                if( CurrProc->targ.base_adjust != 0 ) {
                    GenRegSub( HW_xBP, -CurrProc->targ.base_adjust );
                }
            }
        }
        RelocParms();
        MoveParms();
    }
    CurrProc->prolog_state |= GENERATED_PROLOG;

    if( _IsModel( DBG_LOCALS ) ){  // d1+ or d2
        DbgRetOffset( CurrProc->parms.base - CurrProc->targ.base_adjust - ret_size );
        EmitProEnd();
    }
    SetOP( old_segid );

    if( CurrProc->prolog_state & GENERATE_EXPORT ) {
        OutDLLExport( ( CurrProc->parms.size + WORD_SIZE - 1 ) / WORD_SIZE, AskForLblSym( CurrProc->label ) );
    }
}


void    InitStackDepth( block *blk )
/**********************************/
{
    if( blk->edge[0].flags & DOWN_ONE_CALL ) {
        StackDepth = WORD_SIZE;
    } else {
        StackDepth = 0;
    }
    StackDepth += blk->stack_depth;
}


void        AdjustStackDepth( instruction *ins )
/**********************************************/
{
    name        *op;
    type_length adjust;

    if( !DoesSomething( ins ) )
        return;
    switch( ins->head.opcode ) {
    case OP_ADD:
    case OP_SUB:
        if( ins->operands[0] != ins->result )
            return;
        if( ins->result->n.class != N_REGISTER )
            return;
        if( !HW_CEqual( ins->result->r.reg, HW_xSP ) )
            return;
        op = ins->operands[1];
        if( op->n.class != N_CONSTANT ) {
            _Zoiks( ZOIKS_077 );
            return;
        }
        adjust = op->c.lo.int_value;
        if( ins->head.opcode == OP_SUB ) {
            StackDepth += adjust;
        } else {
            StackDepth -= adjust;
        }
        break;
    case OP_PUSH:
        StackDepth += WORD_SIZE;
        break;
    case OP_POP:
        StackDepth -= WORD_SIZE;
        break;
    case OP_CALL:
    case OP_CALL_INDIRECT:
        if( ins->flags.call_flags & CALL_POPS_PARMS ) {
            op = ins->operands[CALL_OP_POPS];
            if( op->n.class == N_CONSTANT ) {
                StackDepth -= op->c.lo.int_value;
            }
        }
    default:
        break;
    }
}

void     AdjustStackDepthDirect( int adjust )
/*******************************************/
{
    StackDepth += adjust;
}


bool    BaseIsSP( name *op )
/**************************/
{
    if( !CurrProc->targ.sp_frame )
        return( false );
    if( CurrProc->targ.sp_align && ( op->t.temp_flags & STACK_PARM ) ) {
        return( false );
    }
    return( true );
}


type_length NewBase( name *op )
/*****************************/
{
    if( !BaseIsSP( op ) ) {
        return( op->t.location - CurrProc->targ.base_adjust );
    }
    return( op->t.location + CurrProc->locals.size
        + CurrProc->targ.push_local_size + StackDepth );
}

type_length PushSize( type_length len )
/*************************************/
{
    if( len < WORD_SIZE )
        return( WORD_SIZE );
    return( _RoundUp( len, WORD_SIZE ) );
}


void    GenEpilog( void )
/***********************/
{
    type_length stack;
    fe_attr attr;

    attr = FEAttr( AskForLblSym( CurrProc->label ) );

    stack = - CurrProc->locals.size - CurrProc->locals.base
         - CurrProc->targ.push_local_size;
    PatchBigLabels( stack );

    if( _IsModel( DBG_LOCALS ) ){  // d1+ or d2
        EmitEpiBeg();
    }

    if( (attr & FE_NAKED) == 0 ) {
        if( (CurrProc->state.attr & ROUTINE_NEVER_RETURNS) == 0 ) {
            DoEpilog();
        }

        if( BlockByBlock ) {
            AbsPatch( CurrProc->targ.prolog_loc, CurrProc->locals.size );
        }

        if( CurrProc->targ.stack_check != NULL ) {
            AbsPatch( CurrProc->targ.stack_check,
                  CurrProc->locals.size +
                  CurrProc->parms.base  +
                  WORD_SIZE * CurrProc->lex_level +
                  CurrProc->targ.push_local_size +
                  MaxStack );
        }
    }


    CurrProc->prolog_state |= GENERATED_EPILOG;
    if( _IsModel( DBG_LOCALS ) ) {  // d1+ or d2
        EmitRtnEnd();
    }
}


int AskDisplaySize( level_depth level )
/*************************************/
{
    return( level * WORD_SIZE );
}

int ParmsAtPrologue( void )
/*************************/
{
    unsigned    ret_size;
    unsigned    parms_off_sp;

    parms_off_sp= 0;

#if _TARGET & _TARG_80386
    if( CurrProc->prolog_state & GENERATE_THUNK_PROLOG ) {
        parms_off_sp += WORD_SIZE;
    }
#endif

    if( _RoutineIsInterrupt( CurrProc->state.attr ) ||
       ( CurrProc->state.attr & ROUTINE_NEVER_RETURNS ) ) {
        ret_size = 0;
    } else if( _RoutineIsLong( CurrProc->state.attr ) ) {
        ret_size = 2 * WORD_SIZE;
    } else if( _RoutineIsFar16( CurrProc->state.attr ) ) {
        ret_size = 2 * WORD_SIZE;
    } else {
        ret_size = WORD_SIZE;
    }

    parms_off_sp += ret_size;
    return( parms_off_sp );
}
