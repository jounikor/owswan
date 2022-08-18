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
* Description:  Loop optimizations.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "indvars.h"
#include "cgmem.h"
#include "cfloat.h"
#include "stackcg.h"
#include "zoiks.h"
#include "i64.h"
#include "data.h"
#include "fpu.h"
#include "makeins.h"
#include "utils.h"
#include "namelist.h"
#include "propind.h"
#include "blips.h"
#include "edge.h"
#include "redefby.h"
#include "makeblk.h"
#include "loopopts.h"
#include "split.h"
#include "insutil.h"
#include "insdead.h"
#include "blktrim.h"
#include "unroll.h"
#include "optimize.h"
#include "tree.h"
#include "treefold.h"
#include "splitvar.h"
#include "cachecon.h"
#include "feprotos.h"
#include "revcond.h"


/* block flag usage                                                 */
/*                                                                  */
/* BLK_BLOCK_MARKED is used in the sense of will execute            */
/*                                                                  */

/* target specific. Can a pointer get anywhere near the boundary pointer values? */
/* For 8086, yes. 0000 and FFFF are quite possible as pointer values */

#define _POINTER_GETS_NEAR_BOUNDS 1

typedef struct block_list {
    block                   *blk;
    struct block_list       *next;
} block_list;

block                   *Head;
block                   *PreHead;
induction               *IndVarList;
block                   *Loop;

static int              NumIndVars;
static bool             LoopProtected;
static bool             MemChangedInLoop;

static void     InitIndVars( void )
/**********************************
    Initialize for induction variable processing
*/
{
    IndVarList = NULL;                 /* initialize */
}

static level_depth      MaxDepth( void )
/***************************************
    return the depth of the deepest nested loop in the procedure
*/
{
    level_depth     depth;
    block           *blk;

    depth = 0;
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        if( depth < blk->depth ) {
            depth = blk->depth;
        }
    }
    return( depth );
}


static void     ReplaceAllOccurences( name *of, name *with )
/***********************************************************
    Replace all occurences of "of" with "with" in the program.
*/
{
    block       *blk;
    instruction *ins;
    opcnt       i;

    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            for( i = ins->num_operands; i-- > 0; ) {
                RepOp( &ins->operands[i], of, with );
            }
            if( ins->result != NULL ) {
                RepOp( &ins->result, of, with );
            }
        }
    }
}


static bool     InLoop( block *blk )
/***********************************
    return true if blk is in the loop defined by Head.
*/
{
    if( blk == Head )
        return( true );
    for( ; blk != NULL; blk = blk->loop_head ) {
        if( blk->loop_head == Head ) {
            return( true );
        }
    }
    return( false );
}


block    *AddPreBlock( block *postblk )
/*********************************************
    There is no preheader for this loop (loop has no initialization) so
    add a block right in front of the loop header and move any branch
    that goes from outside the loop to the header, go through the
    preheader first.
*/
{
    block_edge  *edge;
    block       *preblk;

    preblk = NewBlock( postblk->label, false );
    /* set up new block to look like it was generated after postblk */
    _SetBlkAttr( preblk, BLK_JUMP );
    preblk->id = NO_BLOCK_ID;
    preblk->gen_id = postblk->gen_id;
    preblk->ins.hd.line_num = postblk->ins.hd.line_num;
    postblk->ins.hd.line_num = 0;
    preblk->loop_head = postblk->loop_head; /**/
    preblk->depth = postblk->depth;
    if( _IsBlkAttr( postblk, BLK_LOOP_HEADER ) ) {
        // we don't always add this block before a loop header
        // for instance, in the floating point scheduling stuff
        // we use this routine to add a safe 'decache' block
        // after a loop  BBB - Oct 14, 1997
        preblk->depth -= 1;
    }
    preblk->next_block = postblk;
    preblk->prev_block = postblk->prev_block;
    postblk->prev_block = preblk;
    if( preblk->prev_block != NULL ) {
        preblk->prev_block->next_block = preblk;
    }
    preblk->input_edges = NULL;
    preblk->inputs = 0;
    /* make preblk go to postblk */
    preblk->targets++;
    edge = &preblk->edge[0];
    edge->destination.u.blk = postblk;
    edge->source = preblk;
    edge->flags = SOURCE_IS_PREHEADER | DEST_IS_BLOCK;
    edge->next_source = postblk->input_edges;
    postblk->input_edges = edge;
    postblk->inputs++;
    postblk->label = AskForNewLabel();
    FixBlockIds();
    return( preblk );
}


static bool     IsPreHeader( block *test ) {
/*******************************************
    return true if block "test" will serve as a preheader for "Loop".  A
    preheader must branch directly to the loop head, and no other block
    that is not in the loop may branch into the loop.
*/

    block_num   i;
    block       *other;

    /* check that test only goes to the loop head */
    if( test->targets != 1 )
        return( false );
    if( test->edge[0].destination.u.blk != Head )
        return( false );
    if( _IsBlkAttr( test, BLK_IN_LOOP ) )
        return( false );
    /* check that no other block outside the loop branches into the loop */
    for( other = HeadBlock; other != NULL; other = other->next_block ) {
        if( other != test && !_IsBlkAttr( other, BLK_IN_LOOP ) ) {
            for( i = other->targets; i-- > 0; ) {
                if( _IsBlkAttr( other->edge[i].destination.u.blk, BLK_IN_LOOP ) ) {
                    return( false );
                }
            }
        }
    }
    test->edge[0].flags |= SOURCE_IS_PREHEADER;
    return( true );
}


static block    *FindPreHeader( void )
/*************************************
    See if there is a basic block that will suffice as a pre-header for
    "Loop".
*/
{
    block       *preheader;
    block_edge  *edge;

    for( edge = Head->input_edges; edge != NULL; edge = edge->next_source ) {
        if( edge->flags & SOURCE_IS_PREHEADER ) {
            return( edge->source );
        }
    }
    /* maybe there is a 'user defined' preheader*/
    for( preheader = HeadBlock; preheader != NULL; preheader = preheader->next_block ) {
        if( IsPreHeader( preheader ) ) {
            return( preheader );
        }
    }
    return( NULL );
}


static void     PreHeader( void )
/********************************
    Make sure that "Loop" has a preheader "PreHead"
*/
{
    block_edge  *edge;
    block_edge  *next;

    PreHead = FindPreHeader();
    if( PreHead == NULL ) {
        PreHead = AddPreBlock( Head );
        for( edge = Head->input_edges; edge != NULL; edge = next ) {
            next = edge->next_source;
            if( edge->source != PreHead && !_IsBlkAttr( edge->source, BLK_IN_LOOP ) ) {
                MoveEdge( edge, PreHead );
            }
        }
    }
}


void     MarkLoop( void )
/*******************************
    Mark the current loop (defined by Head) as IN_LOOP.  Also mark any
    blocks in the loop containing a branch out of the loop as LOOP_EXIT.
*/
{
    block       *other_blk;
    block_num   targets;
    block_edge  *edge;

    Loop = NULL;
    for( other_blk = HeadBlock; other_blk != NULL; other_blk = other_blk->next_block ) {
        if( InLoop( other_blk ) ) {
            _MarkBlkAttr( other_blk, BLK_IN_LOOP );
            other_blk->u.loop = Loop;
            Loop = other_blk;
        }
    }
    for( other_blk = Loop; other_blk != NULL; other_blk = other_blk->u.loop ) {
        edge = &other_blk->edge[0];
        for( targets = other_blk->targets; targets > 0; --targets ) {
            if( !_IsBlkAttr( edge->destination.u.blk, BLK_IN_LOOP ) ) {
                _MarkBlkAttr( other_blk, BLK_LOOP_EXIT );
            }
            ++edge;
        }
    }
    PreHeader();
}


void     UnMarkLoop( void )
/*********************************
    Turn off the loop marking bits for the current loop.
*/
{
    block       *blk;

    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        _MarkBlkAttrNot( blk, BLK_IN_LOOP | BLK_LOOP_EXIT );
        blk->u.loop = NULL;
    }
}

block    *NextInLoop( block *blk )
/***************************************/
{
    return( blk->u.loop );
}


block    *NextInProg( block *blk )
/***************************************/
{
    return( blk->next_block );
}


void     MakeJumpBlock( block *cond_blk, block_edge *exit_edge )
/**********************************************************************
    Turn the loop condition exit block into one that just transfers out
    of the loop.
*/
{
    block_edge  *edge;

    RemoveInputEdge( &cond_blk->edge[0] );
    RemoveInputEdge( &cond_blk->edge[1] );
    _MarkBlkAttrNot( cond_blk, BLK_CONDITIONAL );
    _MarkBlkAttr( cond_blk, BLK_JUMP );
    cond_blk->targets = 1;
    edge = &cond_blk->edge[0];
    edge->flags = exit_edge->flags;
    edge->source = cond_blk;
    edge->destination.u.blk = exit_edge->destination.u.blk;
    edge->next_source = exit_edge->destination.u.blk->input_edges;
    exit_edge->destination.u.blk->input_edges = edge;
    exit_edge->destination.u.blk->inputs++;
}


static bool     KillOneTrippers( void )
/**************************************
    Nuke the loops that are only going to go around one time.
*/
{
    block       *blk;
    block       *curr;
    block       *loop_head;
    instruction *ins;
    instruction *next;
    block_num   i;
    bool        change;

    change = false;
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( i = blk->targets; i-- > 0; ) {
            if( blk->edge[i].flags & ONE_ITER_EXIT ) {
                for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = next ) {
                    next = ins->head.next;
                    if( _OpIsCondition( ins->head.opcode ) && ins->result == NULL ) {
                        FreeIns( ins );
                    }
                }
                if( _IsBlkAttr( blk, BLK_LOOP_HEADER ) ) {
                    _MarkBlkAttrNot( blk, BLK_LOOP_HEADER );
                    loop_head = blk;
                } else {
                    _MarkBlkAttrNot( blk->loop_head, BLK_LOOP_HEADER );
                    loop_head = blk->loop_head;
                }
                for( curr = HeadBlock; curr != NULL; curr = curr->next_block ) {
                    if( curr->loop_head == loop_head ) {
                        curr->loop_head = loop_head->loop_head;
                    }
                }
                MakeJumpBlock( blk, &blk->edge[i] );
                change = true;
                break;
            }
        }
    }
    return( change );
}


void     UnMarkInvariants( void )
/***************************************
    Turn off the bits which indicate that a name is loop invariant
*/
{
    name        *op;

    for( op = Names[N_TEMP]; op != NULL; op = op->n.next_name ) {
        _SetLoopUsage( op, 0 );
    }
    for( op = Names[N_MEMORY]; op != NULL; op = op->n.next_name ) {
        _SetLoopUsage( op, 0 );
    }
}


static void     ZapRegister( hw_reg_set regs )
/*********************************************
    Flip bits required given that N_REGISTER "regs" has been changed
*/
{
    name        *other;

    for( other = Names[N_REGISTER]; other != NULL; other=other->n.next_name ) {
        if( HW_Ovlap( other->r.reg, regs ) ) {
            other->r.reg_index++;
        }
    }
}


static void     ZapTemp( name *op )
/**********************************
    Flip bits required given that N_TEMP "op" has been changed
*/
{
    name        *alias;

    alias = op;
    for( ;; ) {
        if( TempsOverlap( alias, op ) ) {
            if( _ChkLoopUsage( alias, VU_INVARIANT ) ) {
                _SetLoopUsage( alias, VU_VARIANT | VU_VARIED_ONCE );
            } else {
                _SetLoopUsage( alias, VU_VARIANT );
            }
            // if temp is defined once within multiple loops, we want to make
            // sure MULT_DEFINITION gets set (so that we don't assume ONE_DEFINITION
            // applies across the entire function)
            if( alias->t.temp_flags & LOOP_DEFINITION ) {
                alias->t.temp_flags |= MULT_DEFINITION;
            }
            alias->t.temp_flags |= LOOP_DEFINITION;
        }
        alias = alias->t.alias;
        if( alias == op ) {
            break;
        }
    }
}


static void     ZapMemory( name *op )
/************************************
    Flip bits required given that N_MEMORY "op" has been changed
*/
{
    name        *other;

    for( other = Names[N_MEMORY]; other != NULL; other=other->n.next_name ) {
        if( other->v.symbol == op->v.symbol ) {
            if( _ChkLoopUsage( other, VU_INVARIANT ) ) {
                _SetLoopUsage( other, VU_VARIANT | VU_VARIED_ONCE );
            } else {
                _SetLoopUsage( other, VU_VARIANT );
            }
        }
    }
}


void     MarkInvariants( void )
/*************************************
    Mark all N_TEMP/N_MEMORY names as INVARIANT with respect to the
    current loop "Loop", then traverse the loop and turn off the
    INVARIANT bit for anything that could be modified by an instruction
    withing the loop.  We borrow the block_usage field for the marking
    procedure since the block_usage field is only used during the live
    information calculation.  We also distinguish names which are varied
    once (VARIED_ONCE) from names which are varied more than once within
    the loop.
*/
{
    name        *op;
    block       *blk;
    instruction *ins;
    bool        free_index;
    bool        have_call;


    for( op = Names[N_TEMP]; op != NULL; op = op->n.next_name ) {
        if( op->v.usage & VAR_VOLATILE ) {
            _SetLoopUsage( op, 0 );
        } else {
            _SetLoopUsage( op, VU_INVARIANT );
        }
    }
    for( op = Names[N_MEMORY]; op != NULL; op = op->n.next_name ) {
        if( op->v.usage & VAR_VOLATILE ) {
            _SetLoopUsage( op, 0 );
        } else {
            _SetLoopUsage( op, VU_INVARIANT );
        }
    }
    for( op = Names[N_REGISTER]; op != NULL; op = op->n.next_name ) {
        op->r.reg_index = 0;
    }
    have_call = false;
    free_index = false;
    MemChangedInLoop = false;
    for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            if( _OpIsCall( ins->head.opcode ) && (ins->flags.call_flags & CALL_WRITES_NO_MEMORY) == 0 ) {
                MemChangedInLoop = true;
                have_call = true;
            }
            ZapRegister( ins->zap->reg );
            if( ins->result != NULL ) {
                op = ins->result;
                switch( op->n.class ) {
                case N_REGISTER:
                    ZapRegister( ins->result->r.reg );
                    break;
                case N_INDEXED:
                    if( op->i.base == NULL ) {
                        if( _IsntModel( FORTRAN_ALIASING ) ) {
                            free_index = true;
                        }
                    } else if( op->i.base->n.class == N_TEMP ) {
                        ZapTemp( op->i.base );
                    } else if( !free_index ) {
                        ZapMemory( op->i.base );
                    }
                    MemChangedInLoop = true;
                    break;
                case N_TEMP:
                    if( op->v.usage & USE_ADDRESS ) {
                        MemChangedInLoop = true;
                    }
                    ZapTemp( op );
                    break;
                case N_MEMORY:
                    MemChangedInLoop = true;
                    if( !free_index ) {
                        ZapMemory( op );
                    }
                    break;
                }
            }
        }
    }
    /*
        Now check to see if the destination of an instruction is live on
        entry to the loop before we consider hoisting it in FindRegInvar...
        BBB - may 13, 1993
    */
    // ZapRegister( Head->ins.hd.next->head.live.regs );
    if( have_call || free_index ) {
        if( _IsntModel( FORTRAN_ALIASING ) ) {
            for( op = Names[N_TEMP]; op != NULL; op = op->n.next_name ) {
                if( op->v.usage & USE_ADDRESS ) {
                    _SetLoopUsage( op, VU_VARIANT );
                }
            }
            for( op = Names[N_MEMORY]; op != NULL; op = op->n.next_name ) {
                if( !NameIsConstant( op ) ) {
                    _SetLoopUsage( op, VU_VARIANT );
                }
            }
        } else { /* could only be have_call */
            for( op = Names[N_MEMORY]; op != NULL; op = op->n.next_name ) {
                if( NameIsConstant( op ) )
                    continue;
                switch( op->m.memory_type ) {
                case CG_FE:
                    if( FEAttr( op->v.symbol ) & FE_VISIBLE ) {
                        _SetLoopUsage( op, VU_VARIANT );
                    }
                    break;
                case CG_BACK:
                case CG_VTB:
                case CG_TBL:
                    break;
                default:
                    _SetLoopUsage( op, VU_VARIANT );
                    break;
                }
            }
        }
    }
}


static bool     InvariantReg( name *op )
/**************************************/
{
    if( op->n.class != N_REGISTER )
        return( false );
    if( op->r.reg_index != 0 )
        return( false );
    return( true );
}


bool     InvariantOp( name *op )
/**************************************
    return true if "op" is invariant with respect to "Loop"
*/
{
    switch( op->n.class ) {
    case N_CONSTANT:
        return( true );
    case N_MEMORY:
    case N_TEMP:
        return( _ChkLoopUsage( op, VU_INVARIANT ) );
    case N_INDEXED:
        if( op->i.index_flags & X_VOLATILE )
            return( false );
        if( !InvariantOp( op->i.index ) )
            return( false );
        if( op->i.base == NULL )
            return( !MemChangedInLoop );
        return( InvariantOp( op->i.base ) );
    default:
        return( false );
    }
}


typedef enum {
    CMP_TRUE,
    CMP_FALSE,
    CMP_UNKNOWN
} value;

static value    OpLTZero( name *op, bool fp )
{
    if( op->n.class != N_CONSTANT )
        return( CMP_UNKNOWN );
    /* relocatable constants et all are always > 0 */
    if( op->c.const_type != CONS_ABSOLUTE )
        return( CMP_FALSE );
    if( fp && CFTest( op->c.value ) < 0 )
        return( CMP_TRUE );
    if( !fp && op->c.lo.int_value < 0 )
        return( CMP_TRUE );
    return( CMP_FALSE );
}

static value    OpEQZero( name *op, bool fp )
{
    if( op->n.class != N_CONSTANT )
        return( CMP_UNKNOWN );
    /* relocatable constants et all are always != 0 */
    if( op->c.const_type != CONS_ABSOLUTE )
        return( CMP_FALSE );
    if( fp && CFTest( op->c.value ) == 0 )
        return( CMP_TRUE );
    if( !fp && op->c.lo.int_value == 0 )
        return( CMP_TRUE );
    return( CMP_FALSE );
}


bool     Hoistable( instruction *ins, block *blk )
/********************************************************
    Is it safe to hoist instruction "ins" out of the loop (or if)?
*/
{
    bool        will_execute;
    bool        dangerous;
    opcnt       i;
    value       v;
    bool        big_const;

    will_execute = false;
    dangerous = false;
    big_const = false;
    if( blk != NULL && LoopProtected && _IsBlkMarked( blk ) ) {
        will_execute = true;
    }
    if( _IsFloating( ins->type_class ) && _IsntTargetModel( I_MATH_INLINE ) ) {
        dangerous = true;
    } else {
        for( i = 0; i < ins->num_operands; ++i ) {
            if( ins->operands[i]->n.class == N_INDEXED ) {
                dangerous = true;
            }
        }
        if( ins->result != NULL ) {
            if( ins->result->n.class == N_INDEXED ) {
                dangerous = true;
            }
        }
    }
    if( _IsFloating( ins->type_class ) || _IsI64( ins->type_class ) ) {
        big_const = true;
    }
    switch( ins->head.opcode ) {
    case OP_DIV:
    case OP_MOD:
    case OP_FMOD:
        v = OpEQZero( ins->operands[1], big_const );
        if( v == CMP_TRUE )
            return( false );
        if( v == CMP_UNKNOWN )
            dangerous = true;
        break;
    // case OP_TAN: dangerous for Pi/2 which is an impossible float
    case OP_SQRT:
        v = OpLTZero( ins->operands[0], big_const );
        if( v == CMP_TRUE )
            return( false );
        if( v == CMP_UNKNOWN )
            dangerous = true;
        break;
    case OP_LOG10:
    case OP_LOG:
        v = OpLTZero( ins->operands[0], big_const );
        if( v == CMP_TRUE )
            return( false );
        v = OpEQZero( ins->operands[0], big_const );
        if( v == CMP_TRUE )
            return( false );
        if( v == CMP_UNKNOWN )
            dangerous = true;
        break;
    case OP_POW:
        // bad for x ** y where x <= 0 and -1 < y < 1 and y != 0
        dangerous = true;
        break;
    case OP_MOV:
    case OP_CONVERT:
    case OP_ROUND:
        if( ins->operands[0]->n.class != N_INDEXED )
            return( false );
        break;
#if _TARGET & _TARG_RISC
        // on RISC architectures, we want to hoist OP_LAs as they will
        // usually turn into expensive lha, la style pairs in the encoder
    case OP_LA:
        break;
#endif
    default:
        if( ins->num_operands != 2 )
            return( false );
        if( ins->result == NULL )
            return( false );
        break;
    }
    if( dangerous && !will_execute )
        return( false );
    return( true );
}



static bool     InvariantExpr( instruction *ins, block *blk )
/************************************************************
    Return true if the operands of instruction "ins" are invariant with
    respect to "Loop", and "ins" could be safely brought out of the
    loop.
*/
{
    opcnt       i;

    if( !Hoistable( ins, blk ) )
        return( false );
    // For OP_LA, operand need not be invariant, only its address.
    if( (ins->head.opcode == OP_LA) && (ins->operands[0]->n.class == N_MEMORY) ) {
        return( true );
    }
    for( i = ins->num_operands; i-- > 0; ) {
        if( !InvariantOp( ins->operands[i] ) ) {
            return( false );
        }
    }
    return( true );
}


static void     UpdateLoopLiveInfo( instruction *ins )
/****************************************************/
{
    hw_reg_set  reg;
    block       *blk;
    opcnt       i;

    HW_Asgn( reg, ins->result->r.reg );
    for( i = 0; i < ins->num_operands; ++i ) {
        if( ins->operands[i]->n.class == N_REGISTER ) {
            HW_TurnOn( reg, ins->operands[i]->r.reg );
        }
    }
    for( ; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
        HW_TurnOn( ins->head.live.regs, reg );
    }
    HW_TurnOn( ins->head.live.regs, reg );
    for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
        ins = (instruction *)&blk->ins;
        do {
            HW_TurnOn( ins->head.live.regs, reg );
            ins = ins->head.next;
        } while( ins->head.opcode != OP_BLOCK );
    }
}

induction    *FindIndVar( name *op )
/******************************************
    Find an existing induction variable entry for "op".
*/
{
    induction   *var;

    for( var = IndVarList; var != NULL; var = var->next ) {
        if( var->name == op ) {
            break;
        }
    }
    return( var );
}


static invariant    *CopyInvariant( invariant *invar )
/*****************************************************
    Return a copy of invariant list "invar"
*/
{
    invariant   *new;

    if( invar == NULL ) {
        new = NULL;
    } else {
        new = CGAlloc( sizeof( invariant ) );
        new->name = invar->name;
        new->times = invar->times;
        new->id = invar->id;
        new->next = SafeRecurseCG( (func_sr)CopyInvariant, invar->next );
    }
    return( new );
}


static void     MulInvariant( invariant *invar, signed_32 by )
/*************************************************************
    Multiply all entries in invariant list "invar" by "by"
*/
{
    for( ; invar != NULL; invar = invar->next ) {
        invar->times *= by;
    }
}


static invariant    *NewInvariant( name *op, int times )
/*******************************************************
    bag a new "invariant"
*/
{
    invariant   *new;

    new = CGAlloc( sizeof( invariant ) );
    new->name = op;
    new->times = times;
    new->next = NULL;
    new->id = 0;
    return( new );
}


static void     FreeInvariant( invariant *invar )
/************************************************
    Free an invariant list.
*/
{
    invariant   *next;

    for( ; invar != NULL; invar = next ) {
        next = invar->next;
        CGFree( invar );
    }
}


static bool     SameInvariant( invariant *i1, invariant *i2 )
/************************************************************
    return true if "i1" and "i2" are idential invariant lists.
*/
{
    for( ;; ) {
        if( i1 == NULL && i2 == NULL )
            return( true );
        if( i1 == NULL )
            return( false );
        if( i2 == NULL )
            return( false );
        if( i1->name != i2->name )
            return( false );
        if( i1->times != i2->times )
            return( false );
        i1 = i1->next;
        i2 = i2->next;
    }
}

static bool     DifferentClasses( type_class_def type_class1, type_class_def type_class2 )
/***********************************************************************
    return true if "type_class1" and "type_class2" are different, ignoring signed v.s.
    unsigned differences.
*/
{
    return( Unsigned[type_class1] != Unsigned[type_class2] );
}


void     CommonInvariant( void )
/**************************************
    find moves from one ONE_DEFINITION temp to another. These are
    loop invariant expressions that got hauled out of the loop
    from different places, then got caught as common subexpressions
    in the loop header. We can replace all occurences of the destination
    with the source.
*/
{
    block               *blk;
    instruction         *ins;
    name                *op;
    name                *res;

    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            op = ins->operands[0];
            res = ins->result;
            if( ( ins->head.opcode == OP_MOV ) &&
                ( op->n.class == N_TEMP ) &&
                (op->t.temp_flags & ONE_DEFINITION) &&
                ( res->n.class == N_TEMP ) &&
                (res->t.temp_flags & ONE_DEFINITION) &&
                ( (res->t.temp_flags & MULT_DEFINITION) == 0 ) &&
                !BlockByBlock ) {
                ReplaceAllOccurences( res, op );
                op->t.temp_flags |= CROSSES_BLOCKS;
            }
        }
    }
    for( op = Names[N_TEMP]; op != NULL; op = op->n.next_name ) {
        op->t.temp_flags &= ~ONE_DEFINITION;
    }
}


static bool_maybe   DifferentIV( induction *alias, induction *new )
/******************************************************************
*/
{
    if( alias->basic != new->basic )
        return( MB_TRUE );
    if( alias->plus2 != new->plus2 )
        return( MB_TRUE );
    if( alias->times != new->times )
        return( MB_TRUE );
    if( alias->ivtimes != new->ivtimes )
        return( MB_TRUE );
    if( alias->lasttimes != new->lasttimes )
        return( MB_TRUE );
    if( !SameInvariant( alias->invar, new->invar ) )
        return( MB_TRUE );
    if( DifferentClasses( alias->name->n.type_class, new->name->n.type_class ) ) {
        return( MB_TRUE );
    }
    if( alias->plus != new->plus )
        return( MB_MAYBE );
    return( MB_FALSE );
}


static induction    *AddIndVar( instruction *ins,
                                name *op,
                                induction *prev,
                                invariant *invar,
                                name *ivtimes,
                                invar_id lasttimes,
                                signed_32 times,
                                signed_32 plus,
                                signed_32 plus2,
                                signed_32 iv_mult,
                                type_class_def type_class )
/****************************************************************
    Add an induction variable to our list of induction variables for the
    current loop. Two idendical induction variables are put on a
    list of aliases. See INDVARS.H for description of the parameters
    used to fill in the fields.
*/
{
    induction   *new;
    induction   *alias;

    new = FindIndVar( op );
    if( new == NULL ) {
        new = CGAlloc( sizeof( induction ) );
        new->name = op;
        new->state = EMPTY;
        if( prev == NULL ) {
            new->basic = new;
            _SetV( new, IV_BASIC | IV_TOP );
        } else {
            new->basic = prev->basic;
            _ClrV( prev, IV_TOP );
            _SetV( new, IV_TOP );
        }
        new->use_count = 0;
        new->index_use_count = 0;
        new->prev = prev;
        new->times = times;
        new->ivtimes = ivtimes;
        new->lasttimes = lasttimes;
        new->plus = plus;
        new->plus2 = plus2;
        new->header = Head;
        new->invar = CopyInvariant( invar );
        MulInvariant( new->invar, iv_mult );
        new->ins = ins;
        new->type_class = type_class;
        new->alias = new;
        new->next = IndVarList;
        IndVarList = new;
        NumIndVars++;
        for( alias = IndVarList; alias != NULL; alias = alias->next ) {
            if( alias == new )
                continue;
            if( _IsV( alias, IV_ALIAS ) )
                continue;
            if( DifferentIV( alias, new ) != MB_FALSE )
                continue;
            new->alias = alias->alias;
            alias->alias = new;
            _SetV( new, IV_ALIAS );
            break;
        }
    }
    return( new );
}


static induction    *FindOrAddIndVar( name *op, type_class_def type_class )
/******************************************************************************
    Find an existing induction variable entry for "op", or add one if op
    is an alias for a temporary with an existing entry.
*/
{
    induction   *var1;
    induction   *var2;

    var1 = FindIndVar( op );
    if( var1 != NULL )
        return( var1 );
    if( op->n.class != N_TEMP )
        return( NULL );
    if( op->v.offset != 0 )
        return( NULL );     // BBB - May 31, 1994
    var1 = FindIndVar( DeAlias( op ) );
    if( var1 == NULL )
        return( NULL );
    if( _IsV( var1, IV_DEAD ) || _IsntV( var1, IV_BASIC ) )
        return( NULL );
    var2 = AddIndVar( var1->ins, op, var1->prev, var1->invar, var1->ivtimes,
                      var1->lasttimes, var1->times, var1->plus,
                      var1->plus2, 1, type_class );
    if( _IsV( var1, IV_SURVIVED ) )
        _SetV( var2, IV_SURVIVED );
    if( _IsV( var1, IV_INTRODUCED ) )
        _SetV( var2, IV_INTRODUCED );
    return( var2 );
}


static void     FreeVar( induction *var )
/******************************************
    Free one induction variable
*/
{
    FreeInvariant( var->invar );
    CGFree( var );
}


void    FiniIndVars( void )
/**************************
    clean up the induction variable lists.
*/
{
    induction   *next;

    for( ; IndVarList != NULL; IndVarList = next ) {
        next = IndVarList->next;
        FreeVar( IndVarList );
    }
    NumIndVars = 0;
}


static  bool    ListContainsVar( name * list, name *ivname )
/*************************************************************
    Does the invariant list "list" contain the invariant name "ivname"
*/
{
    for( ; list != NULL; list = list->n.next_name ) {
        if( list == ivname ) {
            return( true );
        }
    }
    return( false );
}


static  void    FreeBadVars( void )
/**********************************
    LoopInsDead may have freed some N_TEMP names.  Free the associated
    induction variables.
*/
{
    induction   **owner;
    induction   *var;

    owner = &IndVarList;
    for( ;; ) {
        var = *owner;
        if( var == NULL )
            break;
        if( ListContainsVar( Names[N_TEMP], var->name )
          || ListContainsVar( Names[N_MEMORY], var->name ) ) {
            owner = &var->next;
        } else {
            *owner = var->next;
            FreeVar( var );
        }
    }
}


bool    Inducable( block *blk, instruction *ins )
/***********************************************/
{
    name        *cons;
    name        *op;

    if( blk->depth == 0 )
        return( false );
    if( ins->head.opcode != OP_ADD && ins->head.opcode != OP_SUB ) {
        return( false );
    }
    if( _IsFloating( ins->type_class ) || _IsI64( ins->type_class ) ) {
        return( false );
    }
#ifdef _TARG_IS_SEGMENTED
    if( ins->type_class == PT ) {
        return( false );
    }
#endif
    op = ins->operands[0];
    cons = ins->operands[1];
    if( cons->n.class != N_CONSTANT ) {
        op = ins->operands[1];
        cons = ins->operands[0];
    }
    if( cons->n.class != N_CONSTANT )
        return( false );
    if( cons->c.const_type != CONS_ABSOLUTE )
        return( false );
    if( op != ins->result )
        return( false );
    if( op->n.class != N_TEMP && op->n.class != N_MEMORY )
        return( false );
    return( true );
}


static void     CheckBasic( instruction *ins, union name *name, union name *cons )
/*********************************************************************************
    Check if "ins" qualifies as an instruction creating a basic
    induction variable.  Add an induction variable entry if it is.
*/
{
    if( name->n.class == N_TEMP || name->n.class == N_MEMORY ) {
        if( FindIndVar( name ) == NULL ) {
            if( ins->head.opcode == OP_ADD ) {
                AddIndVar( ins, name, NULL, NULL, NULL, 0, 1,
                           cons->c.lo.int_value, 0, 1, ins->type_class );
            } else { /* OP_SUB*/
                AddIndVar( ins, name, NULL, NULL, NULL, 0, 1,
                          -(cons->c.lo.int_value), 0, 1, ins->type_class );
            }
        }
    }
}

static  bool    BasicNotRedefined( induction *var, instruction *ins )
/********************************************************************
    Check that there is no assignment to the basic induction variable of var
    between the assignment to var and ins (in the same block).
*/
{
    instruction *other;

    for( other = var->ins->head.next; other->head.opcode != OP_BLOCK; other = other->head.next ) {
        if( other == ins )
            return( true );
        if( _IsReDefinedBy( other, var->basic->name ) ) {
            break;
        }
    }
    return( false );
}


static  bool    KillIndVars( instruction *ins )
/**********************************************
    Kill all induction variables that haven't already survived one pass
    of the loop and are modified by the current instruction (defined >1
    times in loop)
*/
{
    induction   *var;
    bool        killed;

    killed = false;
    for( var = IndVarList; var != NULL; var = var->next ) {
        if( var->ins != ins ) {
            if( _IsntV( var, IV_SURVIVED ) ) {
                if( _IsReDefinedBy( ins, var->name ) ) {
                    killed = true;
                    _SetV( var, IV_DEAD );
                }
            }
        }
    }
    return( killed );
}


static  void    CheckNonBasic( instruction *ins, induction *var,
                               name *cons, bool reverse )
/**********************************************************
    See if "ins" creates a non-basic induction variable.  (A linear
    function of another induction variable).  If it does, add an
    approptriate induction variable entry for it the result of the
    instruction.  We try to keep things simple by turning a*( b + c )
    into a*b + a*c when we add induction variables, since they will be
    constants and fold.
*/
{
    signed_32   plus;
    signed_32   plus2;
    signed_32   times;
    signed_32   c;
    signed_32   mul;
    invariant   *invar;
    name        *ivtimes;
    invar_id    lasttimes;

    if( var == NULL )
        return;
    if( _IsV( var, IV_DEAD ) )
        return;
    if( ins->result->n.class != N_TEMP && ins->result->n.class != N_MEMORY )
        return;
    if( reverse && ( ins->head.opcode == OP_DIV || ins->head.opcode == OP_LSHIFT ) )
        return;
    if( _IsV( var, IV_BASIC ) ) {
        plus = 0;
        plus2 = 0;
        times = 1;
    } else {
        plus2 = var->plus2;
        plus = var->plus;
        times = var->times;
        if( !BasicNotRedefined( var, ins ) ) {
            return;
        }
    }
    invar = var->invar;
    ivtimes = var->ivtimes;
    lasttimes = var->lasttimes;
    c = 0;
    if( cons != NULL ) {
        c = cons->c.lo.int_value;
    }
    switch( ins->head.opcode ) {
    case OP_MOV:
    case OP_CONVERT:
        AddIndVar( ins, ins->result, var, invar, ivtimes, lasttimes,
                   times, plus, plus2, 1, ins->type_class );
        break;
    case OP_ADD:
        AddIndVar( ins, ins->result, var, invar, ivtimes, lasttimes,
                   times, plus + c, plus2, 1, ins->type_class );
        break;
    case OP_MUL:
        AddIndVar( ins, ins->result, var, invar, ivtimes, lasttimes,
                   c * times, c * plus, c * plus2, c, ins->type_class );
        break;
    case OP_SUB:
        if( reverse ) {
            AddIndVar( ins, ins->result, var, invar, ivtimes, lasttimes,
                       -times, c-plus, -plus2, -1, ins->type_class );
        } else {
            AddIndVar( ins, ins->result, var, invar, ivtimes, lasttimes,
                       times, plus-c, plus2, 1, ins->type_class );
        }
        break;
    case OP_LSHIFT:
        mul = 1;
        while( c != 0 ) {
            c--;
            mul <<= 1;
        }
        AddIndVar( ins, ins->result, var, invar, ivtimes, lasttimes,
                   mul * times, mul * plus, mul * plus2, mul, ins->type_class );
        break;
    }
}


static  void    CheckInvariant( instruction *ins, induction *var,
                                name *invariant_op, bool reverse )
/*******************************************************************
    Check if "ins" is adding, subtracting or multiplying an existing
    induction variable by a loop invariant (non-constant) variable.  If
    it is, add an appropriate induction variable entry for the result of
    the instruction.
*/
{
    signed_32   plus;
    signed_32   plus2;
    signed_32   times;
    int         iv_mult;
    invariant   *invar;
    name        *ivtimes;
    invar_id    lasttimes;

    if( var == NULL )
        return;
    if( _IsV( var, IV_DEAD ) )
        return;
    if( ins->result->n.class != N_TEMP && ins->result->n.class != N_MEMORY ) {
        return;
    }
    iv_mult = 1;
    ivtimes = var->ivtimes;
    lasttimes = var->lasttimes;
    plus = var->plus;
    plus2 = var->plus2;
    times = var->times;
    if( _IsV( var, IV_BASIC ) ) {
        times = 1;
        plus = 0;
        plus2 = 0;
    } else {
        if( !BasicNotRedefined( var, ins ) ) {
            return;
        }
    }
    if( ins->head.opcode != OP_SUB ) {
        reverse = false;
    }
    switch( ins->head.opcode ) {
    case OP_SUB:
        /* be careful to only allow ONE pointer into an induction expression*/
        if( ins->type_class == CP )
            return;
        if( ins->type_class == PT )
            return;
        if( reverse ) { /* reverse subtract. Negate! */
            times = -times;
            plus = -plus;
            plus2 = -plus2;
        } else {
            iv_mult = -1;
        }
        /* fall through */
    case OP_ADD:
        invar = NewInvariant( invariant_op, iv_mult );
        invar->next = CopyInvariant( var->invar );
        if( invar->next == NULL ) {
            invar->id = 1;
        } else {
            invar->id = invar->next->id + 1;
        }
        if( reverse ) {
            MulInvariant( invar->next, -1 );
        }
        break;
    case OP_MUL:
        if( var->ivtimes != NULL )
            return;
        ivtimes = invariant_op;
        plus2 = plus;
        plus = 0;
        invar = CopyInvariant( var->invar );
        if( invar != NULL ) {
            lasttimes = invar->id;
        } else {
            lasttimes = 0;
        }
        break;
    default:
        return;
    }
    AddIndVar( ins, ins->result, var, invar, ivtimes, lasttimes,
               times, plus, plus2, 1, ins->type_class );
    FreeInvariant( invar );
}


static  iv_usage        Uses( name *op, name *indvar )
/*****************************************************
    Find out how "indvar" is used within "op".
*/
{
    iv_usage    uses;

    uses = IVU_UNUSED;
    if( op->n.class == N_INDEXED ) {
        if( SameThing( op->i.index, indvar ) ) {
            uses = IVU_USED_AS_INDEX;
        }
    } else {
        if( SameThing( op, indvar ) ) {
            uses = IVU_USED_AS_OPERAND;
        } else if( InvariantOp( op ) ) {
            uses = IVU_USED_AS_INVARIANT;
        }
    }
    return( uses );
}


static  void    ChkIVUses( induction *var, name *op )
/***************************************************/
{
    iv_usage    usage;

    usage = Uses( op, var->name );
    if( usage == IVU_USED_AS_INDEX || usage == IVU_USED_AS_OPERAND ) {
        var->use_count++;
        if( usage == IVU_USED_AS_OPERAND ) {
            _SetV( var, IV_USED );
        }
    }
    if( usage == IVU_USED_AS_INDEX ) {
        var->index_use_count++;
        _SetV( var, IV_INDEXED );
    }
    if( usage == IVU_USED_AS_INDEX ) {
        op = op->i.index;
        usage = IVU_USED_AS_OPERAND;
    }
    if( usage == IVU_USED_AS_OPERAND ) {
        if( op != var->name ) {
            _SetV( var, IV_NOREPLACE );
        }
    }
}


static  void    MarkUses( instruction *ins )
/******************************************/
{
    induction   *var;
    opcnt       i;

    for( var = IndVarList; var != NULL; var = var->next ) {
        if( _IsV( var, IV_SURVIVED ) )
            continue;
        if( ins == var->ins )
            continue;
        for( i = ins->num_operands; i-- > 0; ) {
            ChkIVUses( var, ins->operands[i] );
        }
        if( ins->result != NULL ) {
            ChkIVUses( var, ins->result );
        }
    }
}


static  void    MarkSurvivors( void )
/************************************
    Mark induction variables that have survived one pass of the loop
*/
{
    block       *blk;
    instruction *ins;
    induction   *var;

    for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            KillIndVars( ins );
            MarkUses( ins );
        }
    }
    for( var = IndVarList; var != NULL; var = var->next ) {
        if( _IsntV( var, IV_DEAD ) ) {
            _SetV( var, IV_SURVIVED );
        }
    }
}


static  void    AdjOneIndex( name **pop, induction *var, induction *new )
/***********************************************************************/
{
    name        *op;

    op = *pop;
    if( op->n.class != N_INDEXED )
        return;
    if( Uses( op, var->name ) != IVU_USED_AS_INDEX )
        return;
    *pop = ScaleIndex( new->name, op->i.base,
                      op->i.constant - ( new->plus - var->plus ),
                      op->n.type_class, op->n.size,
                      op->i.scale, op->i.index_flags );
}


static  void    AdjustIndex( induction *var, induction *new )
/***********************************************************/
{
    block       *blk;
    instruction *ins;
    opcnt       i;

    for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            for( i = 0; i < ins->num_operands; ++i ) {
                AdjOneIndex( &ins->operands[i], var, new );
            }
            if( ins->result != NULL ) {
                AdjOneIndex( &ins->result, var, new );
            }
        }
    }
}


static  void    MergeVars( void )
/*******************************/
{
    induction   *var;
    block       *varblock;
    block       *otherblock;
    induction   *other;

    for( var = IndVarList; var != NULL; var = var->next ) {
        if( _IsntV( var, IV_TOP ) )
            continue;
        if( _IsV( var, IV_DEAD ) )
            continue;
        if( _IsV( var , IV_BASIC | IV_ALIAS ) )
            continue;
        varblock = InsBlock( var->ins );
        for( other = var->next; other != NULL; other = other->next ) {
            if( _IsntV( other, IV_TOP ) )
                continue;
            if( _IsV( other, IV_DEAD ) )
                continue;
            if( _IsV( other , IV_BASIC | IV_ALIAS ) )
                continue;
            if( DifferentIV( var, other ) != MB_MAYBE )
                continue;
            if( _IsV( other, IV_USED ) )
                continue;
            if( _IsntV( other, IV_INDEXED ) )
                continue;
            otherblock = InsBlock( other->ins );
            if( otherblock != varblock )
                continue;
            _INS_NOT_BLOCK( var->ins );
            _INS_NOT_BLOCK( other->ins );
            if( var->ins->id < other->ins->id ) {
                if( !BasicNotRedefined( var, other->ins ) )
                    continue;
                AdjustIndex( other, var );
            } else {
                if( !BasicNotRedefined( other, var->ins ) )
                    continue;
                AdjustIndex( var, other );
                var = var->next;
                break;
            }
        }
    }
}


static  bool    IsAddressMode( induction *var )
/*********************************************/
{
#if 0 // when this doesn't work, it REALLY doesn't work
    if( var->use_count != 1 )
        return( false );
    if( _IsntV( var, IV_INDEXED ) )
        return( false );
    if( var->times != 1 && var->times != 2 &&
        var->times != 4 && var->times != 8 )
        return( false );
    if( var->ivtimes != NULL )
        return( false );
    if( var->lasttimes != 0 )
        return( false );
    if( var->invar != NULL ) {
        if( var->invar->times != 1 )
            return( false );
        if( var->invar->next != NULL )
            return( false );
        if( var->plus != 0 || var->plus2 != 0 ) {
            return( false );
        }
    } else {
        if( var->plus != 0 ) {
            if( var->plus2 != 0 ) {
                return( false );
            }
        }
    }
    return( true );
#else
    /* unused parameters */ (void)var;

    return( false );
#endif
}


static  void    ScanNonBasic( instruction *ins )
/***********************************************
    see FindNonBasics ^
*/
{
    induction   *var;
    name        *op;

    if( KillIndVars( ins ) )
        return;
    if( _IsFloating( ins->type_class ) || _IsI64( ins->type_class ) )
        return;
    if( ins->head.opcode == OP_MOV ) {
        var = FindIndVar( ins->operands[0] );
        CheckNonBasic( ins, var, NULL, false );
        return;
    }
    if( ins->head.opcode != OP_ADD
         && ins->head.opcode != OP_SUB
         && ins->head.opcode != OP_MUL
         && ins->head.opcode != OP_LSHIFT )
        return;
    if( ins->operands[0]->n.class == N_CONSTANT ) {
        op = ins->operands[0];
        var = FindOrAddIndVar( ins->operands[1], ins->type_class );
        if( op->c.const_type == CONS_ABSOLUTE ) {
            CheckNonBasic( ins, var, op, true );
        } else {
            CheckInvariant( ins, var, op, true );
        }
    } else if( ins->operands[1]->n.class == N_CONSTANT ) {
        op = ins->operands[1];
        var = FindOrAddIndVar( ins->operands[0], ins->type_class );
        if( op->c.const_type == CONS_ABSOLUTE ) {
            CheckNonBasic( ins, var, op, false );
        } else {
            CheckInvariant( ins, var, op, false );
        }
    } else if( InvariantOp( ins->operands[0] ) ) {
        var = FindOrAddIndVar( ins->operands[1], ins->type_class );
        CheckInvariant( ins, var, ins->operands[0], true );
    } else if( InvariantOp( ins->operands[1] ) ) {
        var = FindOrAddIndVar( ins->operands[0], ins->type_class );
        CheckInvariant( ins, var, ins->operands[1], false );
    }
}


static  void    ScanForNonBasics( block *blk )
/*********************************************
    see FindNonBasics ^
*/
{
    instruction *ins;

    for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
        ScanNonBasic( ins );
    }
}


static  void    FindNonBasics( void )
/************************************
    Find all non-basic induction variables 'i' in the loop this is a
    variable whose only definition in the loop is i = j + - * / << const
    or  i = j +- loop invariant variable where j is an existing
    induction variable
*/
{
    block       *blk;

    for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
        ScanForNonBasics( blk );
    }
    MarkSurvivors();
}


static  void    ScanBasic( instruction *ins )
/********************************************
    see FindBasics ^
*/
{
    name    *op;

    if( KillIndVars( ins ) )
        return;
    if( ins->head.opcode != OP_ADD && ins->head.opcode != OP_SUB )
        return;
    if( _IsFloating( ins->type_class ) || _IsI64( ins->type_class ) )
        return;
#ifdef _TARG_IS_SEGMENTED
    if( ins->type_class == PT )
        return;
#endif
    op = ins->operands[0];
    if( op->n.class == N_CONSTANT && op->c.const_type == CONS_ABSOLUTE ) {
        if( ins->operands[1] == ins->result ) {
            if( ins->head.opcode == OP_ADD ) {
                CheckBasic( ins, ins->result, op );
            }
        }
    } else {
        op = ins->operands[1];
        if( op->n.class == N_CONSTANT && op->c.const_type == CONS_ABSOLUTE ) {
            if( ins->operands[0] == ins->result ) {
                CheckBasic( ins, ins->result, op );
            }
        }
    }
}


static  void    ScanForBasics( block *blk )
/******************************************
    See FindBasics ^
*/
{
    instruction *ins;

    for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
        ScanBasic( ins );
    }
}


static  void    FindBasics( void )
/*********************************
    Find all the basic induction variables in the current loop a basic
    induction variable is one whose only defn in the loop is i = i +-
    constant;
*/
{
    block       *blk;

    for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
        ScanForBasics( blk );
    }
    MarkSurvivors();
}


static  void    ReplaceOccurences( name *of, name *with )
/**********************************************************
    Replace all occurences of "of" with "with" in "Loop".
*/
{
    block       *blk;
    instruction *ins;
    opcnt       i;

    for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            for( i = ins->num_operands; i-- > 0; ) {
                RepOp( &ins->operands[i], of, with );
            }
            if( ins->result != NULL ) {
                RepOp( &ins->result, of, with );
            }
        }
    }
}


static  name    *FindPointerPart( induction *var )
/***************************************************
    Given induction variable "var", find out where the pointer part of
    the induction variable is.  There is only one since expressions
    involving pointers only allow one pointer with the addition and
    subtraction of integers.
*/
{
    invariant   *invar;
    name        *first;

    first = var->basic->name;
    if( first->n.type_class == CP || first->n.type_class == PT )
        return( first );
    for( invar = var->invar; invar != NULL; invar = invar->next ) {
        first = invar->name;
        if( first->n.type_class == CP || first->n.type_class == PT ) {
            return( first );
        }
    }
    return( NULL );
}


static  instruction     *Multiply( name *op, signed_32 by, name *temp,
                                   type_class_def type_class, instruction *prev )
/*****************************************************************************
    Generate "optimal" code for op*by => temp and suffix instruction
    "prev" with it.  Return a pointer to the last instruction generated.
*/
{
    bool        negative;
    int         log2;
    instruction *ins;

    if( by < 0 ) {
        negative = true;
        by = -by;
    } else {
        negative = false;
    }
    log2 = GetLog2( by );
    if( log2 == 0 ) {
        ins = MakeMove( op, temp, type_class );
        SuffixIns( prev, ins );
    } else if( log2 == -1 ) {
        ins = MakeBinary( OP_MUL, op, AllocS32Const( by ), temp, type_class );
        SuffixIns( prev, ins );
    } else {
        ins = MakeBinary( OP_LSHIFT, op, AllocS32Const( log2 ), temp, type_class );
        SuffixIns( prev, ins );
    }
    if( negative ) {
        prev = MakeUnary( OP_NEGATE, temp, temp, op->n.type_class );
        SuffixIns( ins, prev );
        ins = prev;
    }
    return( ins );
}


static  instruction     *MakeMul( instruction *prev,
                                  name *op, signed_32 by, name *ivtimes )
/**************************************************************************
    Generate "optimal" instruction calcualte "op" * "by" * "ivtimes" and
    place them after "prev".  Return a pointer to the last instruction
    generated.
*/
{
    instruction         *ins;
    name                *temp;
    type_class_def      type_class;

    type_class = op->n.type_class;
    temp = AllocTemp( type_class );
    ins = Multiply( op, by, temp, type_class, prev );
    if( ivtimes != NULL ) {
        prev = MakeBinary( OP_MUL, temp, ivtimes, temp, type_class );
        SuffixIns( ins, prev );
        ins = prev;
    }
    return( ins );
}


void    SuffixPreHeader( instruction *ins )
/********************************************
    Suffix the pre-header of "Loop" with ins.
*/
{
    instruction *last;

    for( last = PreHead->ins.hd.prev; last->head.opcode == OP_NOP; last = last->head.prev ) {
        if( last->flags.nop_flags & NOP_ZAP_INFO ) {
            break;
        }
    }
    SuffixIns( last, ins );
}


static  void    IncAndInit( induction *var, name *iv, type_class_def type_class )
/****************************************************************************
    Generate code in the pre-header to initialize newly created
    induction variable "iv", based on the information found in "var".
    There is a fair bit of jiggery pokery going on here, since an
    induction variable entry may contain one entry of type pointer
    (either the basic induction variable or one of the invariant
    expressions) and the rest of type integer.  This means we have to be
    REAL careful with typing when generating code.
*/
{
    instruction *ins;
    invariant   *invar;
    name        *first;
    induction   *basic;
    name        *temp;

    /* generate the addition of the appropriate constant in the loop */

    basic = var->basic;
    if( var->ivtimes != NULL ) {
        temp = AllocTemp( var->ivtimes->n.type_class );
        ins = Multiply( var->ivtimes, var->times * basic->plus,
                        temp, temp->n.type_class, PreHead->ins.hd.prev );
        temp = ins->result;
        temp->t.temp_flags |= CROSSES_BLOCKS;
    } else {
        temp = AllocS32Const( var->times * basic->plus );
    }
    ins = MakeBinary( OP_ADD, iv, temp, iv, type_class );
    PrefixIns( basic->ins, ins );

    /* initialize the new induction variable in the loop preheader. */

    temp = AllocTemp( type_class );
    if( type_class != CP && type_class != PT ) {
        first = NULL;
    } else {
        first = FindPointerPart( var );
    }
    if( first != NULL ) { /* can do this because ptr multiplies aren't allowed*/
        ins = MakeBinary( OP_ADD, first, AllocS32Const( var->plus ), temp, type_class );
        SuffixPreHeader( ins );
    }
    if( first != basic->name ) {
        ins = MakeMul( PreHead->ins.hd.prev, basic->name, var->times, var->ivtimes );
        if( first == NULL ) {
            ins->result = temp;
        } else {
            ins = MakeBinary( OP_ADD, temp, ins->result, temp, type_class );
            SuffixPreHeader( ins );
        }
    }
    if( first == NULL ) {
        ins = MakeBinary( OP_ADD, temp, AllocS32Const( var->plus ), temp, type_class );
        SuffixPreHeader( ins );
    }
    for( invar = var->invar; invar != NULL; invar = invar->next ) {
        if( invar->name != first ) {
            if( invar->times == 1 && ( invar->id > var->lasttimes ) ) {
                ins = MakeBinary( OP_ADD, temp, invar->name, temp, type_class );
                SuffixPreHeader( ins );
            } else {
                ins = MakeMul(PreHead->ins.hd.prev,invar->name, invar->times,
                      (invar->id > var->lasttimes) ? NULL : var->ivtimes );
                ins = MakeBinary( OP_ADD, temp, ins->result, temp, type_class );
                SuffixPreHeader( ins );
            }
        }
    }
    if( var->plus2 != 0 ) {
        ins = Multiply( var->ivtimes, var->plus2,
                        AllocTemp( var->ivtimes->n.type_class ),
                        var->ivtimes->n.type_class, PreHead->ins.hd.prev );
        ins = MakeBinary( OP_ADD, temp, ins->result, iv, type_class );
    } else {
        ins = MakeMove( temp, iv, type_class );
    }
    SuffixPreHeader( ins );
}


static  void *MarkDown( block *blk )
/**********************************/
{
    block_num   i;

    if( !_IsBlkAttr( blk, BLK_IN_LOOP ) )
        return( NULL );
    if( blk == Head )
        return( NULL );
    if( !_IsBlkMarked( blk ) )
        return( NULL );
    _MarkBlkUnMarked( blk );
    for( i = blk->targets; i-- > 0; ) {
        SafeRecurseCG( (func_sr)MarkDown, blk->edge[i].destination.u.blk );
    }
    return( NULL );
}


static  void    LabelDown( instruction *frum,
                           instruction *avoiding, bool go_around )
/********************************************************************
    start labeling instructions INS_VISITED starting at "frum", stopping
    if we hit instruction "avoiding".  If we missed "avoiding", label
    all successor blocks as BLK_BLOCK_VISITED. See paint analogy in PathFrom.
*/
{
    block       *blk;
    block_num   i;
    block_edge  *edge;
    instruction *ins;

    for( ; frum->head.opcode != OP_BLOCK; frum = frum->head.next ) {
        if( frum == avoiding )
            return;
        frum->ins_flags |= INS_VISITED;
    }
    if( frum == avoiding )
        return;
    blk = _BLOCK( frum );
    edge = &blk->edge[0];
    for( i = blk->targets; i > 0; --i ) {
        blk = edge->destination.u.blk;
        if( ( go_around || blk != Head ) && _IsBlkAttr( blk, BLK_IN_LOOP ) ) {
            ins = blk->ins.hd.next;
            if( ins->head.opcode == OP_BLOCK || (ins->ins_flags & INS_VISITED) == 0 ) {
                _MarkBlkVisited( blk );
            }
            ++edge;
        }
    }
}


static  bool    PathFrom( instruction *frum, instruction *to,
                          instruction *avoiding, bool go_around )
/******************************************************************
    Return true if there is a path from 'from' to 'to' avoiding
    'avoiding'.  Conceptually, this is done by dropping some paint on
    instruction "from" and letting it flow until it hits either "to",
    "avoiding", or the end of the program.  If "to" gets painted, then
    we return true.
*/
{
    bool        change;
    block       *blk;
    bool        foundpath;
    instruction *ins;

    LabelDown( frum, avoiding, go_around );
    do {
        change = false;
        for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
            if( _IsBlkVisited( blk ) ) {
                _MarkBlkUnVisited( blk );
                change = true;
                LabelDown( blk->ins.hd.next, avoiding, go_around );
            }
        }
    } while( change );
    if( to->ins_flags & INS_VISITED ) {
        foundpath = true;
    } else {
        foundpath = false;
    }
    for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
        _MarkBlkUnVisited( blk );
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            ins->ins_flags &= ~INS_VISITED;
        }
    }
    return( foundpath );
}


static  bool    NoPathThru( instruction *ins1,
                            instruction *ins2, instruction *ins3 )
/*******************************************************************
    return true if there is no path from ins1 to ins2 going through ins3
*/
{
    if( PathFrom( ins3, ins2, ins1, false ) && PathFrom( ins1, ins3, ins2, false ) ) {
        return( false ); /* we found a path from ins1 to ins2 going thru ins3 */
    }
    if( PathFrom( ins2, ins3, ins1, false ) && PathFrom( ins3, ins1, ins2, false ) ) {
        return( false ); /* we found a path from ins2 to ins1 going thru ins3 */
    }
    return( true );
}


static  void    MarkWillExecBlocks( void )
/****************************************/
{
    block       *blk;
    block_num   i;
    instruction *nop;

    for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
        _MarkBlkMarked( blk );
    }

    /* First, prune some blocks we know won't necessarily execute */

    for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
        if( !_IsBlkAttr( blk, BLK_LOOP_EXIT ) )
            continue;
        for( i = blk->targets; i-- > 0; ) {
            MarkDown( blk->edge[i].destination.u.blk );
        }
    }

    for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
        if( blk->targets <= 1 )
            continue;
        for( i = blk->targets; i-- > 0; ) {
            if( blk->edge[i].destination.u.blk->inputs == 1 ) {
                _MarkBlkUnMarked( blk->edge[i].destination.u.blk );
            }
        }
    }

    if( Head->ins.hd.prev == Head->ins.hd.next ) {
        nop = MakeNop();
        // Must (can hit block) and Ok to (ins killed) not renumber
        PrefixInsRenum( Head->ins.hd.next, nop, false ); // so PathFrom works
    } else {
        nop = NULL;
    }

    for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
        if( !_IsBlkMarked( blk ) )
            continue;
        if( blk->ins.hd.next == (instruction *)&blk->ins ) {
            _MarkBlkUnMarked( blk );
            continue;
        }
        if( PathFrom( Head->ins.hd.prev, Head->ins.hd.next, blk->ins.hd.next, true ) ) {
            _MarkBlkUnMarked( blk );
        }
    }
    if( nop != NULL ) {
        FreeIns( nop );
    }
}


static  void    UnMarkWillExecBlocks( void )
/******************************************/
{
    block       *blk;

    for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
        _MarkBlkUnMarked( blk );
    }
}

static  bool    InstructionWillExec( instruction *ins )
/********************************************************
    true if the given instruction will be executed every time through the loop.
*/
{
    instruction *top;

    top = Head->ins.hd.next;
    if( top == ins )
        return( true );
    if( PathFrom( top->head.next, top, ins, true ) ) {
        return( false );
    }
    return( true );
}


static  void    NewTarget( block_edge *edge, block *blk )
/*********************************************************/
{
    edge->destination.u.blk = blk;
    edge->next_source = blk->input_edges;
    blk->input_edges = edge;
    blk->inputs++;
}


void    MoveDownLoop( block *cond )
/**********************************
    Muck about so that "cond" will come out after the blocks which jump
    to it when we sort the blocks into original order (gen_id) prior to
    actual code dumping.
*/
{
    block_num   cond_id;
    block_num   after_id;
    block       *blk;
    block_edge  *edge;
    block       *after;
    block_num   i;

    after = cond->input_edges->source;
    for( edge = cond->input_edges; edge != NULL; edge = edge->next_source ) {
        if( edge->source->gen_id > after->gen_id ) {
            after = edge->source;
        }
    }
    if( cond->gen_id > after->gen_id )
        return;
    cond_id = cond->gen_id;
    after_id = after->gen_id;
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        if( blk->gen_id >= cond_id && blk->gen_id <= after_id ) {
            blk->gen_id--;
        }
    }
    cond->gen_id = after_id;
    cond->edge[0].flags &= ~BLOCK_LABEL_DIES;
    for( edge = cond->input_edges; edge != NULL; edge = edge->next_source ) {
        edge->flags &= ~DEST_LABEL_DIES;
    }
    for( i = cond->targets; i-- > 0; ) {
        cond->edge[i].flags &= ~DEST_LABEL_DIES;
        cond->edge[i].destination.u.blk->edge[0].flags &= ~BLOCK_LABEL_DIES;
    }
}


static  void            AdjustOp( instruction *blk_end, name **pop,
                                  name *var, signed_32 adjust )
/***************************************************************/
{
    name        *op;

    op = *pop;
    if( op->n.class == N_TEMP && blk_end == NULL ) {
        op->t.temp_flags |= CROSSES_BLOCKS;
    }
    if( op->n.class != N_INDEXED )
        return;
    if( op->i.index != var )
        return;
    *pop = ScaleIndex( op->i.index, op->i.base,
                       op->i.constant + adjust,
                       op->n.type_class, op->n.size,
                       op->i.scale, op->i.index_flags );
}


instruction     *DupIns( instruction *blk_end, instruction *ins,
                                 name *var, signed_32 adjust )
/**************************************************************/
{
    instruction *new;
    opcnt       num_operands;
    opcnt       i;

    new = NewIns( ins->num_operands );
    num_operands = ins->num_operands;
    Copy( ins, new, offsetof( instruction, operands ) + num_operands * sizeof( name * ) );
    for( i = 0; i < num_operands; ++i ) {
        AdjustOp( blk_end, &new->operands[i], var, adjust );
    }
    if( new->result != NULL ) {
        AdjustOp( blk_end, &new->result, var, adjust );
    }
    if( blk_end == NULL ) {
        SuffixPreHeader( new );
    } else {
        SuffixIns( blk_end, new );
        blk_end = new;
    }
    new->head.line_num = ins->head.line_num;
    return( blk_end );
}


instruction     *DupInstrs( instruction *blk_end,
                            instruction *first, instruction *last,
                            induction *var, signed_32 adjust )
/****************************************************************/
{
    instruction *ins;
    name        *ind;

    ind = NULL;
    if( var != NULL ) {
        ind = var->name;
    }
    for( ins = first; ins != last->head.next; ins = ins->head.next ) {
        blk_end = DupIns( blk_end, ins, ind, adjust );
    }
    return( blk_end );
}


#if 0
static  bool    BlueMoonUnRoll( block *cond_blk, induction *var,
                        instruction *cond, instruction *indins,
                        block_edge *exit_edge, bool know_bounds,
                        signed_32 initial, signed_32 final )
/****************************************************************************
*/
{
    instruction         *first;
    instruction         *last;
    instruction         *ins;
    instruction         *blk_end;
    signed_32           iterations;
    int                 i;
    signed_32           adjust;
    int                 unroll_count;
    int                 remainder;
    int                 num_instrs;

    if( Loop->u.loop != NULL )
        return( false );
    if( var->use_count - var->index_use_count != 1 )
        return( false );
    if( var->ins->head.next != cond )
        return( false );
    if( OptForSize > 0 )
        return( false );

    if( know_bounds ) {
        iterations = ( final - initial ) / var->plus;
    } else {
        iterations = cond_blk->iterations;
    }
    if( iterations == 0 )
        return( false );

#define MAX_UNROLL_EXPANSION    30

    num_instrs = CountIns( cond_blk );
    /* Figure out the largest amount of unrolling that won't exceed the
       maximum code expansion factor */
    unroll_count = MAX_UNROLL_EXPANSION / num_instrs;
    if( unroll_count > iterations )
        unroll_count = iterations;
    for( ;; ) {
        if( unroll_count <= 1 )
            return( false );
        remainder = iterations % unroll_count;
        if( ( unroll_count + remainder ) * num_instrs <= MAX_UNROLL_EXPANSION )
            break;
        --unroll_count;
    }
    cond->head.next->head.prev = cond->head.prev;
    cond->head.prev->head.next = cond->head.next;
    cond->head.next = cond;
    cond->head.prev = cond;
    blk_end = indins->head.prev;
    indins->head.next->head.prev = indins->head.prev;
    indins->head.prev->head.next = indins->head.next;
    indins->head.next = indins;
    indins->head.prev = indins;
    first = cond_blk->ins.hd.next;
    last = blk_end;

    adjust = 0;
    for( i = 0; i < remainder; ++i ) {
        DupInstrs( NULL, first, last, var, adjust );
        adjust += var->plus;
    }
    if( know_bounds ) {
        ins = MakeMove( AllocS32Const( initial + remainder * var->plus ),
                        var->name, var->name->n.type_class );
    } else {
        ins = MakeBinary( OP_ADD, var->name, AllocS32Const( remainder * var->plus ),
                          var->name, var->name->n.type_class );
    }
    SuffixPreHeader( ins );
    adjust = var->plus;
    for( i = 1; i < unroll_count; ++i ) {
        blk_end = DupInstrs( blk_end, first, last, var, adjust );
        adjust += var->plus;
    }
    if( (unroll_count + remainder) != iterations ) {
        ins = MakeBinary( OP_ADD, var->name,
                          AllocS32Const( unroll_count * var->plus ), var->name,
                          var->ins->type_class );
        SuffixIns( blk_end, ins );
        blk_end = DupIns( ins, cond, var->name, 0 );
    } else {
        _MarkBlkAttrNot( cond_blk, BLK_LOOP_HEADER );
        MakeJumpBlock( cond_blk, exit_edge );
        if( know_bounds ) {
            ins = MakeMove( AllocS32Const( final ), var->name,
                            var->name->n.type_class );
        } else {
            ins = MakeBinary( OP_ADD, var->name, AllocS32Const( iterations * var->plus ),
                              var->name, var->name->n.type_class );
        }
        SuffixIns( blk_end, ins );
    }
    FreeIns( cond );
    FreeIns( indins );
    return( true );
}
#endif


static  induction       *FindReplacement( induction *var )
/***********************************************************
    Find a replacement induction variable for var - the one with the
    'simplest' times and plus fields is the the prefered replacement
*/
{
    induction           *replacement;
    induction           *other;
    uint                log2rep;
    uint                log2oth;
    type_class_def      var_type_class;
    type_class_def      oth_type_class;

    if( var->ivtimes != NULL )
        return( NULL );
    replacement = NULL;
    var_type_class = Unsigned[var->name->n.type_class];
    if( Unsigned[var->ins->type_class] != var_type_class )
        return( NULL );
    log2rep = 0;
    for( other = IndVarList; other != NULL; other = other->next ) {
        if( _IsV( other, IV_DEAD ) )
            continue;
        if( _IsntV( other, ( IV_INTRODUCED | EMPTY ) ) )
            continue;
        if( other == var )
            continue;
        if( other->ivtimes != NULL )
            continue;
        if( other->basic != var )
            continue;
        oth_type_class = Unsigned[other->name->n.type_class];
        if( oth_type_class == var_type_class || ( oth_type_class == WD && var_type_class == CP )
#ifndef _TARG_IS_SEGMENTED
          || ( oth_type_class == WD && var_type_class == PT ) || ( oth_type_class == PT && var_type_class == WD )
#endif
        ) {
            log2oth = GetLog2( other->times );
            if( ( replacement == NULL )
             || ( replacement->invar != NULL && other->invar == NULL )
             || ( log2oth < log2rep )
             || ( log2oth == log2rep && other->plus < replacement->plus ) ) {
                replacement = other;
                log2rep = GetLog2( replacement->times );
            }
        }
    }
    return( replacement );
}


bool    AnalyseLoop( induction *var, bool *ponecond,
                             instruction **pcond, block **pcond_blk )
/********************************************************************/
{
    bool        can_replace;
    block       *blk;
    block       *first_blk;
    instruction *ins;
    iv_usage    usage;
    opcnt       i;

    *ponecond = false;
    *pcond = NULL;
    *pcond_blk = NULL;
    can_replace = true;
    first_blk = PreHead->edge[0].destination.u.blk;
    for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            if( var == NULL || ins != var->ins ) {
                if( !_OpIsCondition( ins->head.opcode ) || ins->result != NULL ) {
                    if( var != NULL ) {
                        for( i = ins->num_operands; i-- > 0; ) {
                            usage = Uses( ins->operands[i], var->name );
                            if( usage == IVU_USED_AS_INDEX || usage == IVU_USED_AS_OPERAND ) {
                                can_replace = false;
                            }
                        }
                        if( ins->result != NULL ) {
                            usage = Uses( ins->result, var->name );
                            if( usage == IVU_USED_AS_INDEX || usage == IVU_USED_AS_OPERAND ) {
                                can_replace = false;
                            }
                        }
                    }
                } else {
                    if( _IsBlkAttr( blk, BLK_LOOP_EXIT ) ) {
                        if( *pcond == NULL ) {
                            *pcond = ins;
                            *ponecond = true;
                            *pcond_blk = blk;
                        } else {
                            *ponecond = false;
                            if( first_blk == blk ) { // preferred by TwistLoop
                                *pcond = ins;
                                *pcond_blk = blk;
                            }
                        }
                        if( var != NULL ) {
                            for( i = ins->num_operands; i-- > 0; ) {
                                usage = Uses( ins->operands[i], var->name );
                                if( usage == IVU_USED_AS_INDEX ) {
                                    can_replace = false;
                                }
                            }
                        }
                    } else if( var != NULL ) {
                        for( i = ins->num_operands; i-- > 0; ) {
                            usage = Uses( ins->operands[i], var->name );
                            if( usage == IVU_USED_AS_INDEX || usage == IVU_USED_AS_OPERAND ) {
                                can_replace = false;
                            }
                        }
                    }
                }
            }
        }
    }
    return( can_replace );
}


static  signed_32       Sgn( signed_32 x )
/****************************************/
{
    if( x < 0 )
        return( -1 );
    return( 1 );
}


static  signed_32       Abs( signed_32 x )
/****************************************/
{
    if( x < 0 )
        return( -x );
    return( x );
}


static  name    *InitialValue( name *op )
/****************************************
    Scan the loop pre-header to find an assignment of a constant to
    "op", to find out what the initial value of "op" is going into the
    loop.
*/
{
    instruction *other;
    block       *blk;

    other = PreHead->ins.hd.prev;
    for( ;; ) {
        if( other->head.opcode == OP_BLOCK ) {
            blk = _BLOCK( other );
            if( _IsBlkAttr( blk, BLK_IN_LOOP ) )
                return( NULL );
            if( blk->inputs != 1 )
                return( NULL );
            other = blk->input_edges->source->ins.hd.prev;
            continue;  /* re-check in case of empty blocks */
        }
        if( _IsReDefinedBy( other, op ) )
            break;
        other = other->head.prev;
    }
    if( other->head.opcode != OP_MOV )
        return( NULL );
    if( other->result != op )
        return( NULL );
    op = other->operands[0];
    if( op->n.class != N_CONSTANT )
        return( NULL );
    return( op );
}


bool    CalcFinalValue( induction *var, block *blk, instruction *ins,
                                signed_32 *final, signed_32 *initial )
/*********************************************************************
    See if we can figure out what the final value of induction variable
    "var" will be, based on the facts that instruction "ins" (in block
    "blk") is the only exit from "Loop".
*/
{
    name                *temp;
    name                *op;
    signed_32           init;
    signed_32           test;
    signed_32           incr;
    signed_32           remd;
    signed_32           dist;

    if( _IsV( var, IV_DEAD ) )
        return( false );
    if( ins == NULL )
        return( false );
    if( ins->head.opcode == OP_BIT_TEST_TRUE )
        return( false );
    if( ins->head.opcode == OP_BIT_TEST_FALSE )
        return( false );
    if( ins->operands[0]->n.class == N_CONSTANT ) {
        temp = ins->operands[0];
        ins->operands[0] = ins->operands[1];
        ins->operands[1] = temp;
        RevCond( ins );
    }
    if( _IsBlkAttr( blk->edge[_TrueIndex( ins )].destination.u.blk, BLK_IN_LOOP ) ) {
        _SetBlockIndex( ins, _FalseIndex( ins ), _TrueIndex( ins ) );
        FlipCond( ins );
    }
    if( ins->head.opcode == OP_CMP_NOT_EQUAL )
        return( false );
    op = ins->operands[1];
    if( op->n.class != N_CONSTANT )
        return( false );
    if( op->c.const_type != CONS_ABSOLUTE )
        return( false );
    if( ins->operands[0] != var->name )
        return( false );
    op = InitialValue( var->name );
    if( op == NULL )
        return( false );
    if( op->c.const_type != CONS_ABSOLUTE )
        return( false );
    init = op->c.lo.int_value;
    *initial = init;
    test = ins->operands[1]->c.lo.int_value;
    incr = var->plus;
    dist = test - init;
    if( Sgn( dist ) != Sgn( incr ) )
        return( false );
    remd = ( Abs( dist ) % Abs( incr ) ) * Sgn( incr );
    switch( ins->head.opcode ) {
    case OP_CMP_EQUAL:
        if( remd != 0 )
            return( false );
        *final = test;
        break;
    case OP_CMP_GREATER:
        if( incr <= 0 )
            return( false );
        if( test <= init )
            return( false );     // wraps or exits immediately BBB - July, 1996
        *final = test + ( incr - remd );
        break;
    case OP_CMP_GREATER_EQUAL:
        if( incr <= 0 )
            return( false );
        if( test <= init )
            return( false );     // wraps or exits immediately BBB - July, 1996
        if( remd == 0 ) {
            *final = test;
        } else {
            *final = test + ( incr - remd );
        }
        break;
    case OP_CMP_LESS:
        if( incr >= 0 )
            return( false );
        if( test >= init )
            return( false );     // wraps or exits immediately BBB - July, 1996
        *final = test + ( incr - remd );
        break;
    case OP_CMP_LESS_EQUAL:
        if( incr >= 0 )
            return( false );
        if( test >= init )
            return( false );     // wraps or exits immediately BBB - July, 1996
        if( remd == 0 ) {
            *final = test;
        } else {
            *final = test + ( incr - remd );
        }
        break;
    }
    if( InstructionWillExec( var->ins ) ) {
        Head->iterations = ( *final - *initial ) / incr;
        _MarkBlkAttr( Head, BLK_ITERATIONS_KNOWN );
    }
    return( true );
}


static  bool    FinalValue( instruction *ins, block *blk, induction *var )
/***************************************************************************
    Figure out the final value of the induction variable if possible.  If
    we can, put assignments of the final value at each loop exit so that
    the value of the induction instruction is not used outside the loop
    Also, change the exit condition to be exit when equal since we can
    always replace induction variables with other ones in equality
    comparisons
*/
{
    instruction         *other;
    name                *op;
    signed_32           final;
    signed_32           initial;
    type_class_def      type_class;
    block               *dest;

    if( !CalcFinalValue( var, blk, ins, &final, &initial ) ) {
        return( false );
    }
    dest = blk->edge[_TrueIndex( ins )].destination.u.blk;
    if( _IsBlkAttr( dest, BLK_UNKNOWN_DESTINATION ) )
        return( false );
    if( _IsBlkAttr( dest, BLK_LOOP_HEADER ) )
        return( false );
    // type_class = var->name->n.type_class;
    type_class = ins->type_class;
    assert( TypeClassSize[type_class] == TypeClassSize[var->name->n.type_class] );
    op = AllocS32Const( final );
    other = MakeCondition( OP_CMP_EQUAL, var->name, op,
                           _TrueIndex( ins ), _FalseIndex( ins ), type_class );
    ReplIns( ins, other );
    if( dest->inputs == 1 ) {
        // if dest has 1 input then we can do this assignment since we
        // know we must have come from the loop when arriving there
        // (otherwise we might be mucking with vars used in a later block)
        ins = MakeMove( op, var->name, type_class );
        SuffixIns( (instruction *)&dest->ins, ins );
    }
    return( true );
}


static  bool    PointerOk( name *op )
/***********************************/
{
#ifdef _POINTER_GETS_NEAR_BOUNDS
    /* unused parameters */ (void)op;

    return( false );
#else
    if( op->n.type_class == PT )
        return( true );
    if( op->n.type_class == CP )
        return( true );
    if( op->n.type_class != U2 )
        return( false );
    if( op->n.class != N_TEMP )
        return( false );
    if( (op->t.temp_flags & INDEXED) == 0 )
        return( false );
    return( true );
#endif
}


static  bool    DangerousTypeChange( induction *var, induction *other )
/************************************************************************
    Is it dangerous to replace "var" with "other"?  This is true if
    "other" might wrap around its limits.
*/
{
    invariant   *invar;

    if( Unsigned[var->type_class] == Unsigned[other->type_class] )
        return( false );
    if( PointerOk( other->name ) )
        return( false );
    for( invar = other->invar; invar != NULL; invar = invar->next ) {
        if( PointerOk( invar->name ) ) {
            return( false );
        }
    }
    return( true );
}

static  bool    ConstOverflowsType( signed_64 *val, type_class_def type_class )
/*************************************************************************
    Return true if the given (signed) constant is too big or too small for
    the given type_class.
*/
{
    type_length         len;
    signed_64           min;
    signed_64           max;
    signed_64           one;

    if( type_class == XX ) {
        _Zoiks( ZOIKS_139 );
        return( false );
    }
    if( type_class == I8 || type_class == U8 )
        return( false );
    len = TypeClassSize[type_class] * 8;
    I32ToI64( 1, &one );
    if( _IsSigned( type_class ) ) {
        // signed type of length 'len' bits
        U64ShiftL( &one, len - 1, &max );
        U64Neg( &max, &min );
        U64Sub( &max, &one, &max );
    } else {
        // unsigned type of length 'len' bits
        I32ToI64( 0, &min );
        U64ShiftL( &one, len, &max );
        U64Sub( &max, &one, &max );
    }
    if( I64Cmp( val, &min ) < 0 )
        return( true );
    if( I64Cmp( val, &max ) > 0 )
        return( true );
    return( false );
}

static  bool    DoReplacement( instruction *ins, induction *rep,
                               opcnt ind, opcnt non_ind, type_class_def type_class )
/******************************************************************************
    Replace operands[ind] with "rep" in instruction "ins".
    operands[non_ind] is guaranteed not to be another induction variable.
    This may generate a bunch of code to calculate the correct linear
    function of the non-induction operand that we need to compare
    against "rep", but this code will all be loop invariant, so it'll
    get hauled out into the pre-header.
*/
{
    name        *non_ind_op;
    instruction *prev_ins;
    instruction *new_ins;
    invariant   *invar;

    non_ind_op = ins->operands[non_ind];
    prev_ins = PreHead->ins.hd.prev;
    if( non_ind_op->n.class == N_CONSTANT
     && non_ind_op->c.const_type == CONS_ABSOLUTE ) {
        signed_64       big_cons;
        signed_64       temp;

        // if we are going to overflow our type, bail!
        I32ToI64( non_ind_op->c.lo.int_value, &big_cons );
        I32ToI64( rep->times, &temp );
        U64Mul( &big_cons, &temp, &big_cons );
        I32ToI64( rep->plus, &temp );
        U64Add( &big_cons, &temp, &big_cons );
        // make sure we always allow negative values - hack for BMark
        if( (big_cons.u._32[I64HI32] & 0x80000000) == 0 ) {
            if( ConstOverflowsType( &big_cons, type_class ) ) {
                return( false );
            }
        }
        ins->type_class = type_class;
        ins->operands[ind] = rep->name;
        ins->operands[non_ind] = AllocS64Const( big_cons.u._32[I64LO32], big_cons.u._32[I64HI32] );
    } else {
        ins->type_class = type_class;
        ins->operands[ind] = rep->name;
        new_ins = MakeMul( prev_ins, non_ind_op, rep->times, NULL );
        prev_ins = new_ins;
        new_ins = MakeBinary( OP_ADD, new_ins->result, AllocS32Const( rep->plus ), new_ins->result, type_class );
        ins->operands[non_ind] = new_ins->result;
        SuffixIns( prev_ins, new_ins );
        prev_ins = new_ins;
    }
    if( rep->times < 0 ) {
        RevCond( ins );
    }
    for( invar = rep->invar; invar != NULL; invar = invar->next ) {
        new_ins = MakeMul( prev_ins, invar->name, invar->times, NULL );
        prev_ins = new_ins;
        if( DifferentClasses( new_ins->result->n.type_class, type_class ) ) {
            new_ins = MakeConvert( new_ins->result, AllocTemp( type_class ),
                                   type_class, ins->operands[non_ind]->n.type_class );
            SuffixIns( prev_ins, new_ins );
            prev_ins = new_ins;
        }
        new_ins = MakeBinary( OP_ADD, new_ins->result, ins->operands[non_ind],
                                AllocTemp( type_class ), type_class );
        ins->operands[non_ind] = new_ins->result;
        SuffixIns( prev_ins, new_ins );
        prev_ins = new_ins;
    }
    return( true );
}

static  bool    RepIndVar( instruction *ins, induction *rep,
                           opcnt i_ind, opcnt i_non_ind, type_class_def type_class )
/******************************************************************************
    One operand of "ins" needs to be replaced with "rep"
*/
{
    if( InvariantOp( ins->operands[i_non_ind] ) ) {
        if( !DoReplacement( ins, rep, i_ind, i_non_ind, type_class ) ) {
            return( false );
        }
    }
    return( true );
}


static  void    RepBoth( instruction *ins, induction *rep, type_class_def type_class )
/****************************************************************
    Both operands of "ins" need to be replaced with "rep".
*/
{
    ins->operands[0] = rep->name;
    ins->operands[1] = rep->name;
    if( rep->times < 0 ) {
        RevCond( ins );
    }
    ins->type_class = type_class;
}


static  bool    ReplUses( induction *var, induction *rep,
                          instruction *ins, type_class_def type_class )
/*******************************************************************
    Replace all uses of induction variable "var" with "rep" in
    instruction "ins".
*/
{
    iv_usage    op1use;
    iv_usage    op2use;

    if( ins->head.opcode == OP_CMP_EQUAL && !DangerousTypeChange( var, rep ) ) {
        op1use = Uses( ins->operands[0], var->name ); /* UNUSED | USED_AS_OP*/
        op2use = Uses( ins->operands[1], var->name ); /* UNUSED | USED_AS_OP*/
        if( op1use == IVU_USED_AS_OPERAND ) {
            if( op2use == IVU_USED_AS_OPERAND ) {
                RepBoth( ins, rep, type_class );
            } else if( op2use == IVU_USED_AS_INVARIANT ) {
                if( !RepIndVar( ins, rep, 0, 1, type_class ) ) {
                    return( false );
                }
            }
        } else if( op2use == IVU_USED_AS_OPERAND ) {
            if( op1use == IVU_USED_AS_INVARIANT ) {
                if( !RepIndVar( ins, rep, 1, 0, type_class ) ) {
                    return( false );
                }
            }
        }
        return( true );
    }
    return( false );
}


static  void    Replace( induction *var, induction *replacement )
/******************************************************************
    Replace all occurences of "var" with "replacement" in conditional
    loop exit instructions, provided that these are the only places that
    "var" is used.  Also, if we can calculate what the final value of
    "var" will be based on the loop exit condition, etc, put an
    assignment of the final value following the loop.  This might cause
    all updates of the variable within the loop to become obsolete.
*/
{
    block               *blk;
    instruction         *ins;
    instruction         *cond;
    block               *cond_blk;
    bool                onecond;
    type_class_def      type_class;
    bool                free_ins;

    if( !AnalyseLoop( var, &onecond, &cond, &cond_blk ) )
        return;
    type_class = replacement->type_class;
    free_ins = false;
    if( onecond ) {
        if( FinalValue( cond, cond_blk, var ) ) {
            free_ins = true;
        }
    }
    for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            if( _OpIsCompare( ins->head.opcode ) ) {
                if( !ReplUses( var, replacement, ins, type_class ) ) {
                    return;
                }
            }
        }
    }
    if( free_ins ) {
        FreeIns( var->ins );
    }
    _SetV( var, IV_DEAD );
}


static  bool    DoLoopInvariant( bool(*rtn)(void) )
/*************************************************
    Do loop invariant code motion from the outside in.  First haul any
    invariant expressions out of the innermost loops, then the next
    level, etc, so that an invariant expression gets pulled out of all
    loops with a minimum number of passes.
*/
{
    level_depth         max_depth;
    level_depth         depth;
    block               *blk;
    bool                change;

    change = false;
    max_depth = MaxDepth();
    for( depth = 1; depth <= max_depth; ++depth ) { /* do loop invariant code motion from the outside in */
        for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
            if( _IsBlkAttr( blk, BLK_LOOP_HEADER ) && blk->depth == depth ) {
                LPBlip();
                Head = blk;
                MarkLoop();
                LoopProtected = false;
                MarkInvariants();
                while( rtn() ) {
                    change = true;
                }
                UnMarkInvariants();
                UnMarkLoop();
            }
        }
    }
    return( change );
}


static  bool    FindRegInvariants( void )
/****************************************
    Find any invariant, hoistable expressions in "Loop", and
    pre-calculate the expression in the loop pre-header, putting the
    result into a tempory and replaceing the calculation within the loop
    with a reference to that temporary.  Also, if there are any
    invariant operands whose pre-loop values can be calculated,
    propagate the constant values into the loop.  The copy propagator in
    CSE does not handle this so we do it here.
*/
{
    block       *blk;
    instruction *ins;
    instruction *next;
    opcnt       i;
    bool        change;

    change = false;
    for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = next ) {
            next = ins->head.next;
            if( ins->result == NULL )
                continue;
            if( ins->result->n.class != N_REGISTER )
                continue;
            if( HW_Ovlap( ins->result->r.reg, Head->ins.hd.next->head.live.regs ) )
                continue;
            if( ins->result->r.reg_index != 1 )
                continue;
            if( SideEffect( ins ) )
                continue;
            if( _OpIsCall( ins->head.opcode ) )
                continue;
            if( _OpIsCondition( ins->head.opcode ) )
                continue;
            for( i = 0; i < ins->num_operands; ++i ) {
                if( !HW_CEqual( ins->zap->reg, HW_EMPTY ) )
                    break;
                if( InvariantReg( ins->operands[i] ) )
                    continue;
                if( !InvariantOp( ins->operands[i] ) ) {
                    break;
                }
            }
            if( i != ins->num_operands )
                continue;
            next->head.prev = ins->head.prev;
            ins->head.prev->head.next = next;
            SuffixPreHeader( ins );
            UpdateLoopLiveInfo( ins );
            change = true;
        }
    }
    return( change );
}


static  bool    FindInvariants( void )
/*************************************
    Find any invariant, hoistable expressions in "Loop", and
    pre-calculate the expression in the loop pre-header, putting the
    result into a tempory and replaceing the calculation within the loop
    with a reference to that temporary.  Also, if there are any
    invariant operands whose pre-loop values can be calculated,
    propagate the constant values into the loop.  The copy propagator in
    CSE does not handle this so we do it here.
*/
{
    block       *blk;
    instruction *ins;
    instruction *next;
    name        *op;
    name        *temp;
    opcnt       i;
    bool        change;

    change = false;
    for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = next ) {
            next = ins->head.next;
            if( InvariantExpr( ins, blk ) ) {
                next = ins->head.next;
                next->head.prev = ins->head.prev;
                ins->head.prev->head.next = next;
                op = ins->result;
                if( ( op->n.class == N_TEMP || op->n.class == N_MEMORY )
                  && ( _ChkLoopUsage( op, VU_VARIED_ONCE )  )
                  && (op->v.usage & USE_IN_ANOTHER_BLOCK) == 0
                  && !BlockByBlock ) {
                    SuffixPreHeader( ins );
                    if( op->n.class == N_TEMP ) {
                        op->t.temp_flags |= ( CROSSES_BLOCKS | ONE_DEFINITION );
                    }
                    _SetLoopUsage( op, VU_INVARIANT );
                    FPNotStack( op );
                } else {
                    temp = AllocTemp( op->n.type_class );
                    temp->n.size = op->n.size;
                    temp->t.temp_flags |= ( CROSSES_BLOCKS | ONE_DEFINITION );
                    ins->result = temp;
                    SuffixPreHeader( ins );
                    PrefixIns( next, MakeMove( temp, op, op->n.type_class ) );
                }
                change = true;
            } else if( ins->head.opcode != OP_CALL_INDIRECT ) {
                /* propagate some constants into the loop*/
                for( i = 0; i < ins->num_operands; ++i ) {
                    op = ins->operands[i];
                    if( !InvariantOp( op ) )
                        continue;
                    op = InitialValue( op );
                    if( op == NULL )
                        continue;
                    // don't propagate a LEA into the loop!
                    if( op->c.const_type == CONS_TEMP_ADDR )
                        continue;
                    if( op->c.const_type == CONS_HIGH_ADDR )
                        continue;
                    ins->operands[i] = op;
                    change = true;
                }
            }
        }
    }
    return( change );
}


bool    LoopInvariant( void )
/***************************/
{
    return( DoLoopInvariant( &FindInvariants ) );
}


bool    LoopRegInvariant( void )
/******************************/
{
    return( DoLoopInvariant( &FindRegInvariants ) );
}


void    LoopEnregister( void )
/****************************/
{
    level_depth         depth;
    block               *blk;

    for( depth = MaxDepth(); depth >= 1; --depth ) { /* do loop enregistering from the inside out */
        for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
            if( _IsBlkAttr( blk, BLK_LOOP_HEADER ) && blk->depth == depth ) {
                Head = blk;
                MarkLoop();
                ConstToTemp( PreHead, Loop, NextInLoop );
                UnMarkLoop();
            }
        }
    }
}


static  void    ElimIndVars( void )
/**********************************
    Eliminate induction variables whose only use in the loop is in
    comparisons and the single induction instruction by replacing it
    with another induction variable of it's family in the compare
    instructions.
*/
{
    induction   *next;
    induction   *var;
    induction   *replacement;

    for( var = IndVarList; var != NULL; var = next ) {
        next = var->next;
        if( _IsntV( var, IV_BASIC | IV_INTRODUCED ) ) {
            _SetV( var, IV_DEAD );
        }
    }
    for( var = IndVarList; var != NULL; var = next ) {
        next = var->next;
        if( _IsntV( var, IV_NOREPLACE ) &&
            _IsntV( var, IV_DEAD ) && _IsV( var, IV_BASIC ) ) {
            replacement = FindReplacement( var );
            if( replacement != NULL ) {
                Replace( var, replacement );
            }
        }
    }
}


static  bool    ReduceVar( induction *var )
/******************************************
    Do strength reduction on induction variable "var", if it is current
    to this loop, and the top, or end result, of an induction
    expression.  Once strength reduction is done on the "top" of an
    induction expression, the intermediate induction variables drop away
    since they will no longer be used.  Strength reduction involves
    replacing all occurences of the induction variable in a loop with a
    new induction variable (IV_INTRODUCED) which is initialized at the
    top of the loop and has some appropriate value added to it where the
    basic induction variable gets updated.  For example
        for( i = 0; i < 10; ++i ) x[i] = 0;
    becomes
        for( i = 0, p = &i[0]; p < &i[10]; i++, p++ ) *p = 0;
    If "i" is not used after the loop, all references to it in the loop
    will be removed.

*/
{
    induction           *alias;
    name                *new_temp;
    type_class_def      type_class;
    induction           *new;
    instruction         *ins;

    if( _IsntV( var, IV_TOP ) )
        return( false );
    if( _IsV( var, IV_DEAD ) )
        return( false );
    while( var->ins->num_operands == 1 ) {
        var = FindIndVar( var->ins->operands[0] );
    }
    if( _IsV( var, IV_BASIC | IV_ALIAS ) )
        return( false );
    if( IsAddressMode( var ) )
        return( false );
    type_class = var->type_class;
    new_temp = AllocTemp( type_class );
    new_temp->t.temp_flags |= CROSSES_BLOCKS;
    ins = MakeMove( new_temp, var->name, type_class );
    ReplIns( var->ins, ins );
    var->ins = ins;
    IncAndInit( var, new_temp, type_class );
    for( alias = var->alias; alias != var; alias = alias->alias ) {
        if( NoPathThru( var->ins, alias->ins, var->basic->ins )
          && (alias->name->v.usage & USE_IN_ANOTHER_BLOCK) == 0
          && (var->name->v.usage & USE_IN_ANOTHER_BLOCK) == 0 ) {
            ReplaceOccurences( alias->name, var->name );
            ins = MakeMove( new_temp, alias->name, type_class );
            PrefixIns( alias->ins, ins );
            ins = MakeMove( new_temp, var->name, type_class );
        } else {
            ins = MakeMove( new_temp, alias->name, type_class );
        }
        ReplIns( alias->ins, ins );
        alias->ins = ins;
// why? ins = MakeMove( new_temp, alias->name, type_class );
//      SuffixPreHeader( ins );
    }
    new = AddIndVar( var->basic->ins->head.prev, new_temp,
                     var->basic, var->invar, var->ivtimes,
                     var->lasttimes, var->times,
                     var->plus, var->plus2, 1, ins->type_class );
    _SetV( new, IV_INTRODUCED );
    return( true );
}


static  bool    ReduceInStrength( void )
/***************************************
    For each induction variable, see if we can reduce it in strength.
*/
{
    induction   *var;
    bool        change;

    change = false;
    for( var = IndVarList; var != NULL; var = var->next ) {
        change |= ReduceVar( var );
    }
    return( change );
}


#if 0
static  void    DupNoncondInstrs( block *cond_blk, instruction *cond, block *prehead )
/*************************************************************************************
    Duplicate all instructions in cond_blk, suffixing them to the given pre-header,
    except for the conditional instruction pointed to by cond.
*/
{
    instruction         *ins;

    for( ins = cond_blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
        if( ins == cond )
            continue;
        DupIns( prehead->ins.hd.prev, ins, NULL, 0 );
    }
}
#endif

static  bool    TwistLoop( block_list *header_list, bool unroll )
/****************************************************************
    Try to fall into loops without testing the condition the first
    time if we know how many times the loop will go around.  We
    need a loop which goes around more than once, whose preheader
    jumps straight to the condition which in turn jumps into or
    out of the loop.  We may be recursed several levels deep in
    DoInduction here.  header_list is the list of all loops we've
    recursed through.  If we replace a block which is on this
    list, we have to update the pointer in "header_list".

*/
{
    instruction         *cond;
    instruction         *dupcond;
    block               *cond_blk;
    block_edge          *exit_edge;
    block_edge          *loop_edge;
    bool                onecond;
    signed_32           final;
    signed_32           initial;
    induction           *var;
    block_edge          *edge;
    bool                know_bounds;
    bool                do_the_twist;
    block_list          *list;
    block               *old_prehead;
    block               *old_head;
    block               *new_head;
    block               *blk;

    AnalyseLoop( IndVarList, &onecond, &cond, &cond_blk );
    if( cond_blk == NULL )
        return( false );
    know_bounds = false;
    if( onecond ) {
        var = FindIndVar( cond->operands[0] );
        if( var == NULL )
            var = FindIndVar( cond->operands[1] );
        if( var != NULL ) {
            if( CalcFinalValue( var, cond_blk, cond, &final, &initial ) ) {
                if( final != initial ) {
                    know_bounds = true;
                }
            }
        }
    }
    loop_edge = &cond_blk->edge[0];
    exit_edge = &cond_blk->edge[1];
    if( !_IsBlkAttr( loop_edge->destination.u.blk, BLK_IN_LOOP ) ) {
        loop_edge = &cond_blk->edge[1];
        exit_edge = &cond_blk->edge[0];
    }
    if( unroll && _IsBlkAttr( Head, BLK_ITERATIONS_KNOWN ) && Head->iterations == 1 ) {
        if( cond_blk != Head || Loop->u.loop == NULL ) {
            exit_edge->flags |= ONE_ITER_EXIT;
        }
    }
    if( unroll ) {
        old_head = Head;
        if( UnRoll() ) {
            if( Head != old_head ) {
                // we were beheaded!
                for( list = header_list; list != NULL; list = list->next ) {
                    if( list->blk == old_head ) {
                        list->blk = Head;
                    }
                }

            }
            for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
                if( _IsBlkAttr( blk, BLK_IN_LOOP ) ) {
                    PropIndexes( blk );
                }
            }
            return( true );
        }
    }
    do_the_twist = true;
    edge = &PreHead->edge[0];
    if( edge->destination.u.blk != cond_blk ) {
        do_the_twist = false;
    }
#if 1
    if( cond != cond_blk->ins.hd.next || cond != cond_blk->ins.hd.prev ) {
        do_the_twist = false;
    }
#endif
    if( do_the_twist ) {
        new_head = NULL;
        if( know_bounds ) {
            RemoveInputEdge( edge );
            NewTarget( edge, loop_edge->destination.u.blk );
            MoveDownLoop( cond_blk );
            new_head = loop_edge->destination.u.blk;
            // DupNoncondInstrs( cond_blk, cond, PreHead );
        } else if( OptForSize < 50 && PreHead->gen_id < cond_blk->gen_id ) {
            RemoveInputEdge( edge );
            old_prehead = PreHead;
            PreHead = ReGenBlock( PreHead, NULL );
            for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
                if( blk->loop_head == old_prehead ) {
                    blk->loop_head = PreHead;
                }
            }
            for( list = header_list; list != NULL; list = list->next ) {
                if( list->blk == old_prehead ) {
                    list->blk = PreHead;
                }
            }
            // DupNoncondInstrs( cond_blk, cond, PreHead );
            _MarkBlkAttrNot( PreHead, BLK_JUMP );
            _MarkBlkAttr( PreHead, BLK_CONDITIONAL );
            dupcond = MakeCondition( cond->head.opcode, cond->operands[0],
                                     cond->operands[1], _TrueIndex( cond ),
                                     _FalseIndex( cond ), cond->type_class );
            SuffixPreHeader( dupcond );
            NewTarget( &PreHead->edge[0], cond_blk->edge[0].destination.u.blk );
            NewTarget( &PreHead->edge[1], cond_blk->edge[1].destination.u.blk );
            PreHead->edge[0].flags = DEST_IS_BLOCK;
            PreHead->edge[1].flags = DEST_IS_BLOCK;
            MoveDownLoop( cond_blk );
            new_head = loop_edge->destination.u.blk;
        }
        if( new_head != NULL && new_head != Head ) {
            for( blk = Loop; blk != NULL; blk = blk->u.loop ) {
                if( blk->loop_head == Head ) {
                    blk->loop_head = new_head;
                }
            }
            new_head->loop_head = Head->loop_head;
            Head->loop_head = new_head;
            _MarkBlkAttrNot( Head, BLK_LOOP_HEADER );
            _MarkBlkAttr( new_head, BLK_LOOP_HEADER );
            Head = new_head;
        }
    }
    if( do_the_twist || know_bounds ) {
        UnMarkLoop();
        MarkLoop();
        MarkWillExecBlocks();
        LoopProtected = true;
        MarkInvariants();
        while( FindInvariants() )
            ;
        UnMarkInvariants();
        UnMarkWillExecBlocks();
        return( true );
    }
    return( false );
}


static  bool    DoInduction( block_list *header, bool reduce, bool unroll )
/**************************************************************************
    Process induction variables in the loop whose loop header is
    "header".  We do any loops contained inside this loop before
    we do this loop, since by the strict definition of an
    induction variable, an induction variable in a loop is also an
    induction variable any loop containing that loop.  For
    practical purposes though, once an induction variable is
    processed for its "natural" loop (the deepest one) we don't
    consider it for any outer loops.  "header" points to a linked
    list of pointers to blocks.  These are, the current loop
    header, its loop header, etc ...
*/
{
    block_list  curr_block;
    int         old;
    bool        change;
    induction   **owner;
    induction   *list;

    change = false;
    list = NULL;
    owner = &list;

    /* do all loops inside this loop separately*/

    for( curr_block.blk = HeadBlock; curr_block.blk != NULL; curr_block.blk = curr_block.blk->next_block ) {
        if( _IsBlkAttr( curr_block.blk, BLK_LOOP_HEADER ) && curr_block.blk->loop_head == header->blk ) {
            curr_block.next = header;
            change |= DoInduction( &curr_block, reduce, unroll );
            while( *owner != NULL ) {   // hook inner indvars onto "list"
                owner = &(*owner)->next;
            }
            *owner = IndVarList;
            IndVarList = NULL;
        }
    }

    /* now do this loop, BUT, don't consider any induction*/
    /* variables from the inner loops*/

    IndVarList = list;
    if( header->blk != NULL ) {
        LPBlip();
        while( list != NULL ) {
            _SetV( list, IV_DEAD );
            list = list->next;
        }
        Head = header->blk;
        NumIndVars = 0;
        MarkLoop();
        FindBasics();
        if( !reduce || NumIndVars == 0 ) {
            if( !BlockByBlock && TwistLoop( header, unroll ) ) {
                change = true;
            }
        } else {
            LoopProtected = false;
            MarkInvariants();
            for(;;) {
                old = NumIndVars;
                FindNonBasics();
                if( old == NumIndVars ) {
                    break;
                }
            }
            MergeVars();
            if( ReduceInStrength() ) {
                change = true;
                _MarkBlkAttr( PreHead, BLK_IN_LOOP );
                LoopInsDead();
                _MarkBlkAttrNot( PreHead, BLK_IN_LOOP );
            }
            FreeBadVars();
            ElimIndVars();
            if( !BlockByBlock && TwistLoop( header, unroll ) ) {
                change = true;
            }
            UnMarkInvariants();
        }
        UnMarkLoop();
    }
    return( change );
}


static  bool    Induction( bool reduce, bool unroll )
/****************************************************
    Do induction variable processing
*/
{
    block_list  list;
    bool        change;

    list.blk = NULL;
    list.next = NULL;
    InitIndVars();
    change = DoInduction( &list, reduce, unroll );
    FiniIndVars();
    if( unroll ) {
        change |= KillOneTrippers();
    }
    return( change );
}


bool    IndVars( void )
/*********************/
{
    return( Induction( true, false ) );
}


bool    TransLoops( bool unroll )
/*******************************/
{
    return( Induction( false, unroll ) );
}
