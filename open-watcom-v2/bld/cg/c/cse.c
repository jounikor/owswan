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
* Description:  Common Subexpression Elimination (aka CommonSex).
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "zoiks.h"
#include "tree.h"
#include "cfloat.h"
#include "data.h"
#include "fpu.h"
#include "makeins.h"
#include "foldins.h"
#include "utils.h"
#include "namelist.h"
#include "peepopt.h"
#include "blips.h"
#include "redefby.h"
#include "indvars.h"
#include "loopopts.h"
#include "rgtbl.h"
#include "split.h"
#include "insutil.h"
#include "insdead.h"
#include "typemap.h"
#include "blktrim.h"
#include "treefold.h"
#include "fixindex.h"
#include "generate.h"
#include "flograph.h"
#include "cse.h"
#include "varusage.h"
#include "feprotos.h"


/* block flag usage                                                 */
/*                                                                  */
/* BLK_BLOCK_VISITED is used in the sense of partition root         */
/*                                                                  */

#define INS_DEFINES_OWN_OPERAND INS_MARKED

/* Borrow a few fields and bits to label trees with bits and link stuff */

#define _INSBITS( ins )   _LBitScalar((ins)->head.live.within_block)
#define _BLKBITS( blk )   _LBitScalar((blk)->available_bit)
#define _INSLINK( ins )   (*(instruction **)&(ins)->u2.cse_link)
#define _NAMELINK( op )   (*(instruction **)&(op)->v.conflict)

typedef enum {
        ALL_LIVE,
        OP_DIES,
        RESULT_DIES
} who_dies;

static instruction      *ExprHeads[LAST_CSE_OP + 1];
static bool             LeaveIndVars;

static  void    ReCalcAddrTaken( void )
/**************************************
*/
{
    name        *temp;

    for( temp = Names[N_TEMP]; temp != NULL; temp = temp->n.next_name ) {
        if( temp->v.usage & VAR_VOLATILE )
            continue;
        if( temp->v.symbol != NULL && (FEAttr( temp->v.symbol ) & FE_ADDR_TAKEN) )
            continue;
        if( temp->t.temp_flags & STACK_PARM )   /* See DoParmDecl() */
            continue;
        temp->v.usage &= ~USE_ADDRESS;
    }
    FindReferences();
}


static  bool    LoadAddr( void )
/*******************************
    Propagate load address instructions by replacing them with moves of
    "relocatable constants". For example:
       LA  x => t    is replaced by    MOV address(x) => t
    Then PropagateMoves will move the ADDRESS(x) constants down and the
    MOV adresss(x) instruction might go away.
*/
{
    block       *blk;
    instruction *ins;
    bool        change;

    change = false;
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            change |= LoadAToMove( ins );
        }
    }
    return( change );
}


static  bool    FindDefnBlocks( block *blk, instruction *cond, opcnt i )
/**********************************************************************/
{
    block_edge  *edge;
    block_edge  *other_input;
    block_edge  *next_source;
    block       *input;
    instruction *prev;
    name        *op;
    instruction *new_cond;
    instruction *new_ins;
    block_edge  *jump_edge;
    block       *new_dest_blk;
    bool        change;

    change = false;
    op = cond->operands[i];
    for( edge = blk->input_edges; edge != NULL; edge = next_source ) {
        next_source = edge->next_source;
        input = edge->source;
        if( !_IsBlkAttr( input, BLK_JUMP ) )
            continue;
        for( prev = input->ins.hd.prev; prev->head.opcode != OP_BLOCK; prev = prev->head.prev ) {
            if( _IsntReDefinedBy( prev, op ) )
                continue;
            if( prev->head.opcode != OP_MOV )
                break;
            if( prev->result != op )
                break;
            if( input->depth < blk->depth ) { // don't make 2 entries into loop
                for( other_input = blk->input_edges; other_input != NULL; other_input = other_input->next_source ) {
                    if( other_input->source->depth < blk->depth && other_input->source != input ) {
                        break;
                    }
                }
                if( other_input != NULL ) {
                    break;
                }
            }
            new_cond = NewIns( 2 );
            Copy( cond, new_cond, INS_SIZE );
            new_cond->head.prev = new_cond;
            new_cond->head.next = new_cond;
            new_cond->operands[i] = prev->operands[0];
            new_ins = FoldIns( new_cond );
            if( new_ins == NULL ) {
                new_ins = new_cond;
            }
            if( _OpIsCondition( new_ins->head.opcode ) ) {
                if( _TrueIndex( new_ins ) == _FalseIndex( new_ins ) ) {
                    new_dest_blk = blk->edge[_TrueIndex( new_ins )].destination.u.blk;
                    if( new_dest_blk != blk ) {
                        jump_edge = &input->edge[0];
                        RemoveInputEdge( jump_edge );
                        new_dest_blk->inputs++;
                        jump_edge->next_source = new_dest_blk->input_edges;
                        new_dest_blk->input_edges = jump_edge;
                        jump_edge->destination.u.blk = new_dest_blk;
                        if( input->depth < blk->depth ) {
                            MoveHead( blk, new_dest_blk );
                        }
                        change = true;
                    }
                }
            }
            FreeIns( new_ins );
            break;
        }
    }
    return( change );
}


static  bool    StretchABlock( block *blk )
/******************************************
    see StretchEdges
*/
{
    instruction         *ins;
    name                *op;
    opcnt               i;

    if( blk->ins.hd.prev != blk->ins.hd.next )
        return( false );
    ins = blk->ins.hd.next;
    if( !_OpIsCondition( ins->head.opcode ) )
        return( false );
    op = ins->operands[0];
    i = 1;
    if( op->n.class != N_CONSTANT ) {
        op = ins->operands[1];
        i = 0;
    }
    if( op->n.class != N_CONSTANT )
        return( false );
    if( IsVolatile( ins->operands[i] ) )
        return( false );
    return( FindDefnBlocks( blk, ins, i ) );
}


static  bool    StretchEdges( void )
/***********************************

    Try to detect code like:
        found = 1;
        goto somewhere;

    somewhere:;
        if( found ) goto elsewhere;

*/
{
    block       *blk;
    bool        change;

    change = false;
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        if( StretchABlock( blk ) ) {
            change = true;
        }
    }
    return( change );
}


bool    PropRegsOne( void )
/*************************/
/*
 * We can't propagate registers very far, but one instruction is safe.
 * This is specially for the case when we have a(b(x)). We'll generate
 * call    B => reg
 * mov     reg => temp
 * mov     temp => ??? (or push temp)
 * call    A
 */
{
#if 0
    block       *blk;
    instruction *ins;
    instruction *next;
    name        *reg;
    name        **opp;
    bool        change;

    change = false;
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            if( ins->head.opcode != OP_MOV )
                continue;
            reg = ins->operands[0];
            if( FPSideEffect( ins ) )
                continue;
            if( reg->n.class != N_REGISTER )
                continue;
            next = ins->head.next;
            if( next->head.opcode == OP_BLOCK )
                break;
            if( FPStackIns( next ) )
                continue;
            if( next->head.opcode == OP_LA )
                continue;
            if( next->head.opcode == OP_CAREFUL_LA )
                continue;
            if( next->num_operands != 1 ) {
                if( next->num_operands <= OpcodeNumOperands( next ) )
                    continue;
                if( !IsSegReg( reg->r.reg ) )
                    continue;
                opp = &next->operands[next->num_operands - 1];
            } else {
                if( _IsConvert( next ) )
                    continue;
                opp = &next->operands[0];
            }
            if( *opp != ins->result )
                continue;
            *opp = reg;
            change = true;
        }
    }
    return( change );
#else
    return( false );
#endif
}


static  void    TreeBits( block *root )
/**************************************
    Label the partition (tree) with bits such that block A is the
    ancestor of block B IFF _BLKBITS(A) & _BLKBITS(B) == _BLKBITS(A).
    Simply speaking, we add a new bit for every node in the tree.  A
    child node gets its parents bit set plus one new one for itself.
    This allows for simple ancestor, sibling, first common ancestor
    checking.
*/
{
    block       *daddy;
    a_bit_set   next_bit;
    bool        change;
    block       *blk;
    instruction *ins;
    block       **owner;

    next_bit = 1;
    _BLKBITS( root ) = next_bit;
    do {
        change = false;
        for( blk = root->u.partition; blk != root; blk = blk->u.partition ) {
            daddy = blk->input_edges->source;
            if( _BLKBITS( blk ) == 0 && _BLKBITS( daddy ) ) {
                next_bit <<= 1;
                if( next_bit == 0 )
                    break;
                _BLKBITS( blk ) = _BLKBITS( daddy ) | next_bit;
                change = true;
            }
        }
        if( next_bit == 0 ) {
            break;
        }
    } while( change );

    /* rip off any blocks in the partition without bits*/
    /* and propagate the bits through the instructions*/

    owner = &root->u.partition;
    for( ;; ) {
        blk = *owner;
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            _INSBITS( ins ) = _BLKBITS( blk );
        }
        if( _BLKBITS( blk ) == EMPTY ) {
            *owner = blk->u.partition;
        } else {
            owner = &blk->u.partition;
        }
        if( blk == root ) {
            break;
        }
    }
}


static  void    FindPartition( void )
/************************************
    Partition the flow graph into trees, with root indicated by
    BLK_BLOCK_VISITED.  Nodes of the tree (except root) may have only one
    input edge.  These are the partitions in which dataflow is easy to
    deal with since there is no merging of information.
*/
{
    block       *blk;
    block       *oth;
    block       *temp;
    block_edge  *edge;
    block_num   i;

    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        if( _IsBlkAttr( blk, BLK_BIG_LABEL ) || blk->inputs != 1 ) {
            _MarkBlkVisited( blk );
        }
        blk->u.partition = blk;
        _BLKBITS( blk ) = EMPTY;
    }
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        edge = &blk->edge[0];
        for( i = blk->targets; i > 0; --i ) {
            if( edge->flags & DEST_IS_BLOCK ) {
                oth = edge->destination.u.blk;
                if( !_IsBlkVisited( oth ) && oth->inputs == 1 ) {
                    temp = oth->u.partition;
                    oth->u.partition = blk->u.partition;
                    blk->u.partition = temp;
                }
            }
            ++edge;
        }
    }
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        if( _IsBlkVisited( blk ) ) {
            TreeBits( blk );
        }
    }
}


void    SetCSEBits( instruction *ins, instruction *new_ins )
/***********************************************************
    set the ancestor bits of "new_ins" to be the same as "ins".
*/
{
    a_bit_set   bits;

    if( ins->head.opcode == OP_BLOCK ) {
        bits = _BLKBITS( _BLOCK( ins ) );
    } else {
        bits = _INSBITS( ins );
    }
    _INSBITS( new_ins ) = bits;
}


static  instruction *WhichIsAncestor( instruction *ins1, instruction *ins2 )
/***************************************************************************
    Return which of instructions "ins1" or "ins2" is always executed
    first, or a pointer the last instruction in a block which is a
    common ancestor to "ins1" and "ins2".
*/
{
    instruction *first;
    instruction *ins;
    a_bit_set   bits1;
    a_bit_set   bits2;
    block       *blk;

    bits1 = _INSBITS( ins1 );
    bits2 = _INSBITS( ins2 );
    if( bits1 == bits2 ) {  /* same_block*/
        first = NULL;
        if( ins1 != ins2 ) {
            ins = ins1;
            for( ;; ) {
                ins = ins->head.next;
                if( ins == ins2 ) {
                    first = ins1;
                } else if( ins->head.opcode == OP_BLOCK ) {
                    first = ins2;
                }
                if( first != NULL ) {
                    break;
                }
            }
        }
    } else if( (bits1 & bits2) == bits1 ) {
        first = ins1;
    } else if( (bits1 & bits2) == bits2 ) {
        first = ins2;
    } else { /* find a hoist point*/
        bits1 &= bits2;
        blk = InsBlock( ins1 );
        while( _BLKBITS( blk ) != bits1 ) {
            blk = blk->u.partition;
        }
        for( first = blk->ins.hd.prev; first->head.opcode == OP_NOP; first = first->head.prev ) {
            if( first->flags.nop_flags & NOP_ZAP_INFO ) {
                break;
            }
        }
        /* scan back over all the conditional branches at the end of block*/
        for( ; ; first = first->head.prev ) {
            if( ( first->head.opcode != OP_SELECT ) && !_OpIsCondition( first->head.opcode ) ) {
                break;
            }
        }
    }
    return( first );
}


static  void    CleanPartition( void )
/*************************************
    Turn of all the bits we turned on in FindPartition.
*/
{
    block       *blk;

    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        _MarkBlkUnVisited( blk );
        blk->u.partition = NULL;
    }
}


static bool CanCrossBlocks( instruction *ins1, instruction *ins2, name *op )
/***************************************************************************
    If we're generating a partial routine, we cannot cause an
    existing N_TEMP to cross blocks, due to an assumption
    in ForceTempsMemory.
*/
{
    if( !BlockByBlock )
        return( true );
    if( op->n.class != N_TEMP )
        return( true );
    if( _INSBITS( ins1 ) == _INSBITS( ins2 ) )
        return( true );
    if( op->t.temp_flags & CROSSES_BLOCKS )
        return( true );
    return( false );
}


static  void    UseInOther( instruction *ins1, instruction *ins2, name *op )
/***************************************************************************
    Indicate that we have caused operand "op" to be used in both "ins1"
    and "ins2" where it was previously only used in one of those
    instructions.  This may change it's USE_IN_ANOTHER_BLOCK status.
*/
{
    if( _INSBITS( ins1 ) != _INSBITS( ins2 ) ) {
        if( op->n.class == N_MEMORY ) {
            op->v.usage |= USE_IN_ANOTHER_BLOCK;
        } else if( op->n.class == N_TEMP ) {
            op->t.temp_flags |= CROSSES_BLOCKS;
        }
    }
}


static  bool    UnOpsLiveFrom( instruction *first, instruction *last )
/*********************************************************************
    Do the operand and result of "first" live until instruction "last"?
*/
{
    instruction *ins;

    for( ins = last->head.prev; ; ins = ins->head.prev ) {
        while( ins->head.opcode == OP_BLOCK ) {
            ins = _BLOCK( ins )->input_edges->source->ins.hd.prev;
        }
        if( ins == first )
            break;
        if( _IsReDefinedBy( ins, first->operands[0] ) )
            return( false );
        if( _IsReDefinedBy( ins, first->result ) ) {
            return( false );
        }
    }
    return( true );
}


static  who_dies BinOpsLiveFrom( instruction *first,
                                instruction *last,
                                name *op1, name *op2, name *result )
/*******************************************************************
    Determine which of "op1", "op2", and "result" live from instruction
    "first" to instruction "last".
*/
{
    instruction *ins;
    bool        result_dies;

    if( result == NULL ) {
        for( ins = last->head.prev; ; ins = ins->head.prev ) {
            while( ins->head.opcode == OP_BLOCK ) {
                ins = _BLOCK( ins )->input_edges->source->ins.hd.prev;
            }
            if( ins == first )
                break;
            if( _IsReDefinedBy( ins, op1 ) )
                return( OP_DIES );
            if( _IsReDefinedBy( ins, op2 ) ) {
                return( OP_DIES );
            }
        }
    } else {
        result_dies = false;
        for( ins = last->head.prev; ; ins = ins->head.prev ) {
            while( ins->head.opcode == OP_BLOCK ) { /* 89-09-05 */
                ins = _BLOCK( ins )->input_edges->source->ins.hd.prev;
            }
            if( ins == first )
                break;
            if( _IsReDefinedBy( ins, op1 ) )
                return( OP_DIES );
            if( _IsReDefinedBy( ins, op2 ) )
                return( OP_DIES );
            if( _IsReDefinedBy( ins, result ) ) {
                result_dies = true;
            }
        }
        if( result_dies ) {
            return( RESULT_DIES );
        }
    }
    return( ALL_LIVE );
}

static  bool            HoistLooksGood( instruction *target, instruction *orig )
/*******************************************************************************
    Does it look like a good idea to hoist a busy expression from the original
    instruction to the hoist point. Currently, this is not a good idea if we
    are not overly concerned about code size and we would be hoisting the expr
    out of a switch statement.
*/
{
    block               *target_blk;
    block               *blk;

    if( OptForSize > 50 )
        return( true );
    target_blk = InsBlock( target );
    blk = InsBlock( orig );
    if( blk == target_blk )
        return( true );
    while( blk != NULL ) {
        if( blk->inputs != 1 )
            _Zoiks( ZOIKS_103 ); // because we're in a partition
        blk = blk->input_edges->source;
        if( _IsBlkAttr( blk, BLK_SELECT ) )
            return( false );
        if( blk == target_blk ) {
            return( true );
        }
    }
    _Zoiks( ZOIKS_104 );
    return( true );
}


static  instruction     *ProcessExpr( instruction *ins1, instruction *ins2, bool signed_matters )
/************************************************************************************************
    Given two instruction "ins1" and "ins2" in the same partition, first
    find out if they have the same operands, and if they do, determine
    "WhichIsAncestor", which gives us the common ancestor of "ins1" and
    "ins2".  If this is one of ins1 or ins2, we have a common
    subexpressions as long as the operands of the ancestor aren't
    redefined between the two (BinOpsLiveFrom).  If we have a hoist
    point, (different common ancestor) we have a very busy expression as
    long as the operands of ins1 and ins2 aren't redefined between the
    hoist point and either of ins1 and ins2 (BinOpsLiveFrom).
*/
{
    instruction         *first;
    instruction         *new_ins;
    instruction         *dead_or_new_ins;
    name                *temp;
    type_class_def      type_class;
    who_dies            killed;
    opcnt               i;

    dead_or_new_ins = NULL;
    i = ( _OpIsBinary( ins1->head.opcode ) ) ? 1 : 0;
    if( ins1->head.opcode == OP_CONVERT ) {
        if( ins1->base_type_class != ins2->base_type_class ) {
            return( false );
        }
    }
    if( signed_matters ) {
        if( ins1->type_class != ins2->type_class ) {
            return( NULL );
        }
    } else {
        if( Unsigned[ins1->type_class] != Unsigned[ins2->type_class] ) {
            return( NULL );
        }
    }
    if( ins1->operands[0] != ins2->operands[0] || ins1->operands[i] != ins2->operands[i] ) {
        if( !_OpCommutes( ins1->head.opcode ) )
            return( NULL );
        if( ins1->operands[0] != ins2->operands[i] || ins1->operands[i] != ins2->operands[0] ) {
            return( NULL );
        }
    }
    first = WhichIsAncestor( ins1, ins2 );
    if( first == ins2 ) {
        ins2 = ins1;
        ins1 = first;
    }
    if( first == ins1 ) { /* or used to be ins2*/
        if( (ins1->ins_flags & INS_DEFINES_OWN_OPERAND) == 0 ) {
            killed = BinOpsLiveFrom( ins1, ins2, ins1->operands[0], ins1->operands[i], ins1->result );
            if( killed != OP_DIES ) {
                type_class = ins1->result->n.type_class;
                if( killed == RESULT_DIES || !CanCrossBlocks( ins1, ins2, ins1->result ) ) {
                    temp = AllocTemp( type_class );
                    new_ins = MakeMove( temp, ins1->result, type_class );
                    ins1->result = temp;
                    SuffixIns( ins1, new_ins );
                    SetCSEBits( ins1, new_ins );
                    FPNotStack( temp );
                }
                new_ins = MakeMove( ins1->result, ins2->result, type_class );
                UseInOther( ins1, ins2, ins1->result );
                dead_or_new_ins = ins2;
                SetCSEBits( ins2, new_ins );
                SuffixIns( ins2, new_ins );
                FPNotStack( ins1->result );
            }
        }
    } else if( first != NULL ) { /* we calculated a hoist point*/
        if( ins1->operands[0]->n.class != N_INDEXED
          && ins1->operands[i]->n.class != N_INDEXED
          && Hoistable( ins1, NULL )
          && BinOpsLiveFrom( first->head.next, ins1, ins1->operands[0], ins1->operands[i], NULL ) == ALL_LIVE
          && BinOpsLiveFrom( first->head.next, ins2, ins2->operands[0], ins2->operands[i], NULL ) == ALL_LIVE
          && HoistLooksGood( first, ins1 )
          && HoistLooksGood( first, ins2 ) ) {
            type_class = ins1->type_class;
            temp = AllocTemp( type_class );
            temp->t.temp_flags |= CROSSES_BLOCKS;
            if( _OpIsBinary( ins1->head.opcode ) ) {
                new_ins = MakeBinary( ins1->head.opcode, ins1->operands[0], ins1->operands[1], temp, type_class );
            } else {
                new_ins = MakeUnary( ins1->head.opcode, ins1->operands[0], temp, type_class );
            }
            new_ins->base_type_class = ins1->base_type_class;
            dead_or_new_ins = new_ins;
            SetCSEBits( first, new_ins );
            SuffixIns( first, new_ins );
            new_ins = MakeMove( temp, ins1->result, type_class );
            SetCSEBits( ins1, new_ins );
            SuffixIns( ins1, new_ins );
            new_ins = MakeMove( temp, ins2->result, type_class );
            SetCSEBits( ins2, new_ins );
            SuffixIns( ins2, new_ins );
        }
    }
    return( dead_or_new_ins );
}


static  bool    OkToInvert( name *div )
/**************************************
   Is it OK to compute the reciprical of 'div'? Either it must
   be exactly representable (a integer power of two) or the user
   must have said that it was OK.
*/
{
    if( _IsModel( FP_UNSTABLE_OPTIMIZATION ) )
        return( true );
    if( (div->n.class == N_TEMP) && (div->t.temp_flags & CONST_TEMP) ) {
        div = div->v.symbol;
    }
    if( div->n.class != N_CONSTANT )
        return( false );
    if( div->c.const_type != CONS_ABSOLUTE )
        return( false );
    if( !CFIs32( div->c.value ) )
        return( false );
    if( GetLog2( div->c.lo.int_value ) == -1 )
        return( false );
    return( true );
}


static  bool    ProcessDivide( instruction *ins1, instruction *ins2 )
/********************************************************************
    Given two instruction "ins1" and "ins2" in the same partition, first
    find out if they have the same divisor, and if they do, determine
    "WhichIsAncestor", which gives us the common ancestor of "ins1" and
    "ins2".  If this is one of ins1 or ins2, we have a common
    subexpressions as long as the divisor of the ancestor aren't
    redefined between the two (BinOpsLiveFrom).
*/
{
    instruction         *first;
    instruction         *new_ins;
    name                *temp;
    who_dies            killed;
    name                *divisor;

    if( ins1->type_class != ins2->type_class )
        return( false );
    if( !FPDivIsADog( ins1->type_class ) )
        return( false );
    if( ins1->operands[1] != ins2->operands[1] )
        return( false );
    first = WhichIsAncestor( ins1, ins2 );
    if( first == ins2 ) {
        ins2 = ins1;
        ins1 = first;
    }
    if( first == ins1 ) {
        divisor = ins1->operands[1];
        if( !OkToInvert( divisor ) )
            return( false );
        if( _IsntReDefinedBy( ins1, divisor ) ) {
            killed = BinOpsLiveFrom( ins1, ins2, divisor, divisor, NULL );
            if( killed != OP_DIES ) {
                temp = AllocTemp( ins1->type_class );
                new_ins = MakeBinary( OP_DIV, AllocIntConst( 1 ),
                                     divisor, temp, ins1->type_class );
                UseInOther( ins1, ins2, temp );
                SetCSEBits( ins1, new_ins );
                PrefixIns( ins1, new_ins );
                FPNotStack( temp );
                ins1->operands[1] = temp;
                ins2->operands[1] = temp;
                ins1->head.opcode = OP_MUL;
                ins2->head.opcode = OP_MUL;
                return( true );
            }
        }
    }
    return( false );
}


static  void    DeleteFromList( instruction **owner,
                                instruction *ins, instruction *new )
/*******************************************************************
    pull an instruction of a "like" opcode list.
*/
{
    while( *owner != ins ) {
        owner = &_INSLINK( *owner );
    }
    *owner = new;
}

static  bool    DoOneOpcode( opcode_defs opcode )
/************************************************
    Here we check all instructions with opcode "opcode" against all
    others with the same opcode, and if we process a common
    subexpression, unlink the instructions from the list of like
    opcodes.
*/
{
    bool        change;
    instruction *ins1;
    instruction *ins2;
    instruction *next1;
    instruction *next2;
    instruction *which_ins;
    bool        signed_matters;

    change = false;
    switch( opcode ) {
    case OP_ADD:
    case OP_EXT_ADD:
    case OP_SUB:
    case OP_EXT_SUB:
    case OP_AND:
    case OP_OR:
    case OP_XOR:
    case OP_NEGATE:
    case OP_COMPLEMENT:
    case OP_LSHIFT:
        signed_matters = false;
        break;
    default:
        signed_matters = true;
        break;
    }
    for( ins1 = ExprHeads[opcode]; ins1 != NULL; ins1 = next1 ) {
        next1 = _INSLINK( ins1 );
        for( ins2 = next1; ins2 != NULL; ins2 = next2 ) {
            next2 = _INSLINK( ins2 );
            which_ins = ProcessExpr( ins1, ins2, signed_matters );
            if( which_ins != NULL ) {
                change = true;
                if( which_ins == ins1 ) {
                    DeleteFromList( &ExprHeads[opcode], ins1, next1 );
                    FreeIns( ins1 );
                    break;
                } else if( which_ins == ins2 ) {
                    DeleteFromList( &ExprHeads[opcode], ins2, next2 );
                    FreeIns( ins2 );
                    if( ins2 == next1 ) {
                        next1 = ins1;
                        break;
                    }
                } else { /* hoisted code from ins1 & ins2 to which_ins*/
                    DeleteFromList( &ExprHeads[opcode], ins1, next1 );
                    DeleteFromList( &ExprHeads[opcode], ins2, next2 );
                    FreeIns( ins1 );
                    FreeIns( ins2 );
                    _INSLINK( which_ins ) = ExprHeads[opcode];
                    ExprHeads[opcode] = which_ins;
                    next1 = which_ins;
                    break;
                }
            }
        }
    }
    return( change );
}


static  bool    DoDivides( void )
/********************************
*/
{
    bool        change;
    instruction *ins1;
    instruction *ins2;
    instruction *next1;
    instruction *next2;

    change = false;
    for( ins1 = ExprHeads[OP_DIV]; ins1 != NULL; ins1 = next1 ) {
        next1 = _INSLINK( ins1 );
        for( ins2 = next1; ins2 != NULL; ins2 = next2 ) {
            next2 = _INSLINK( ins2 );
            if( ProcessDivide( ins1, ins2 ) ) {
                DeleteFromList( &ExprHeads[OP_DIV], ins1, next1 );
                DeleteFromList( &ExprHeads[OP_DIV], ins2, next2 );
                next1 = ExprHeads[OP_DIV];
                change = true;
                break;
            }
        }
    }
    return( change );
}


static  void    CleanTableEntries( block *root )
/***********************************************
    Clean up the _INSLINK and _PARTITION fields in partition "root"
*/
{
    instruction *ins;
    block       *blk;

    blk = root;
    for( ;; ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            _INSLINK( ins ) = NULL;
        }
        blk = blk->u.partition;
        if( blk == root ) {
            break;
        }
    }
}


static  bool    DoArithOps( block *root )
/****************************************
    Given a partition whose root is "root", do common subexpressions and
    very busy expressions on arithmetic operators. The approach is fairly
    simple. First, link every instruction in the partition together, if the
    instruction is an arithmetic instruction and doesn't have volatile
    operands or something wierd. Then, check every pair of like instructions,
    for possible common subexpressions. (See DoOneOpcode as well).
*/
{
    block       *blk;
    opcode_defs opcode;
    instruction *ins;
    bool        change;
    opcnt       i;

    for( opcode = FIRST_CSE_OP; opcode <= LAST_CSE_OP; ++opcode ) {
        ExprHeads[opcode] = NULL;
    }
    blk = root;
    for( ;; ) {
        for( ins = blk->ins.hd.next; (opcode = ins->head.opcode) != OP_BLOCK; ins = ins->head.next ) {
            i = ( _OpIsBinary( opcode ) ) ? 1 : 0;
            if( _OpIsCSE( opcode )
              && !( LeaveIndVars && Inducable( blk, ins ) )
              && ins->operands[0]->n.class != N_REGISTER
              && ins->operands[i]->n.class != N_REGISTER
              && ins->result->n.class != N_REGISTER
              && !IsVolatile( ins->result ) ) {
                ins->ins_flags &= ~INS_DEFINES_OWN_OPERAND;
                if( _IsReDefinedBy( ins, ins->operands[0] ) ) {
                    ins->ins_flags |= INS_DEFINES_OWN_OPERAND;
                }
                if( _OpIsBinary( opcode ) ) {
                    if( _IsReDefinedBy( ins, ins->operands[1] ) ) {
                        ins->ins_flags |= INS_DEFINES_OWN_OPERAND;
                    }
                }
                _INSLINK( ins ) = ExprHeads[opcode];
                ExprHeads[opcode] = ins;
            }
        }
        blk = blk->u.partition;
        if( blk == root ) {
            break;
        }
    }
    change = false;
    for( opcode = FIRST_CSE_OP; opcode <= LAST_CSE_OP; ++opcode ) {
        if( DoOneOpcode( opcode ) ) {
            change = true;
        }
    }
    if( DoDivides() )
        change = true;
    CleanTableEntries( root );
    return( change );
}


static  bool    PropagateExprs( void )
/*************************************
    Do common subexpression and very busy expressions, for each
    "partition".
*/
{
    bool        change;
    block       *blk;

    change = false;
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        SXBlip();
        if( _IsBlkVisited( blk ) ) {
            change |= DoArithOps( blk );
        }
    }
    return( change );
}


static bool FixOneStructRet( instruction *call )
/***********************************************
    Fix one structured return copy.
*/
{
    instruction *mova;
    instruction *movr;
    name        *res;
    name        *op;

    res = call->result;
    mova = call->head.prev;
    if( mova->head.opcode != OP_MOV )
        return( false );
    op = mova->operands[0];
    if( op->n.class != N_CONSTANT )
        return( false );
    if( op->c.const_type != CONS_TEMP_ADDR )
        return( false );
    if( op->c.value != res )
        return( false );
    for( movr = call->head.next; movr->head.opcode == OP_NOP; ) {
        movr = movr->head.next;
    }
    if( movr->head.opcode != OP_MOV )
        return( false );
    if( movr->type_class != XX )
        return( false );
    if( movr->operands[0] != res )
        return( false );
    res = movr->result;
    if( res->n.class != N_TEMP )
        return( false );
    call->result = res;
    mova->operands[0] = AllocAddrConst( res, 0, CONS_TEMP_ADDR, mova->type_class );
    FreeIns( movr );
    return( true );
}

static  bool    FixStructRet( block *root )
/******************************************

    search for code that looks like

    MOV     XX TMPADDR(t1) => reg
    CALL    rtn, parms => t1
    MOV     XX t1 => t2

    and if t2 is not USE_ADDRESS, change it to

    MOV     TMPADDR(t2) => reg
    CALL    rtn, parms => t2

    and turn off the USE_ADDRESS bit of t1
*/
{
    block       *blk;
    instruction *ins;
    bool        change;

    change = false;
    blk = root;
    for( ;; ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            if( _OpIsCall( ins->head.opcode ) ) {
                if( ins->result != NULL &&
                   ins->result->n.class == N_TEMP &&
                   (ins->flags.call_flags & CALL_RETURNS_STRUCT) &&
                   FixOneStructRet( ins ) ) {
                    change = true;
                }
               ins->flags.call_flags &= ~CALL_RETURNS_STRUCT;
            }
        }
        blk = blk->u.partition;
        if( blk == root ) {
            break;
        }
    }
    return( change );
}

static  block   *NextBlock( block *blk, void *parm )
/***************************************************/
{
    if( blk->u.partition == (block *)parm )
        return( NULL );
    return( blk->u.partition );
}

static  bool    isMoveIns( instruction *ins )
/******************************************
    Is "ins" a move type instruction?
*/
{
    if( ins->head.opcode == OP_MOV )
        return( true );
    if( _IsConvert( ins ) && ins->operands[0]->n.class == N_CONSTANT
      && ins->operands[0]->c.const_type == CONS_ABSOLUTE )
        return( true );
    return( false );
}


static  bool    CanLinkMove( instruction *ins )
/**********************************************
    Is "ins" suitable for copy propagation?
*/
{
    if( ins->num_operands != 1 )
        return( false );
    /* only propagate constants and temps*/
    if( ins->operands[0]->n.class == N_REGISTER )
        return( false );
    if( ins->operands[0]->n.class == N_TEMP && (ins->operands[0]->t.temp_flags & STACK_PARM) )
        return( false );
    if( ins->result->n.class == N_REGISTER )
        return( false );
    if( ins->operands[0] == ins->result )
        return( false );
    if( IsVolatile( ins->result ) )
        return( false );
    if( _IsReDefinedBy( ins, ins->operands[0] ) )
        return( false );
    if( FPIsConvert( ins ) )
        return( false );
    if( IsTrickyPointerConv( ins ) )
        return( false );
    return( true );
}

static void CreateLink( instruction *ins, name *link )
/****************************************************/
{
    if( link->n.class == N_INDEXED ) {
        link = link->i.index;
    }
    if( link->n.class != N_REGISTER ) {
        _INSLINK( ins ) = _NAMELINK( link );
        _NAMELINK( link ) = ins;
    }
}

static void RemoveLink( instruction *ins, name *link )
/****************************************************/
{
    instruction **last;
    instruction *curr;

    if( link->n.class == N_INDEXED ) {
        link = link->i.index;
    }
    if( link->n.class != N_REGISTER ) {
        last = (instruction **)&_NAMELINK( link );
        for( curr = _NAMELINK( link ); curr != NULL; curr = _INSLINK( curr ) ) {
            if( curr == ins ) {
                *last = _INSLINK( ins );
                _INSLINK( ins ) = NULL;
                break;
            }
            last = &_INSLINK( curr );
        }
    }
}

static  bool    LinkableMove( instruction *ins )
/***********************************************
    Determine if ins is ok to link into the list of
    moves associated with it's result.
*/
{
    if( !isMoveIns( ins ) )
        return( false );
    if( !CanLinkMove( ins ) )
        return( false );
    if( ins->operands[0]->n.class == N_MEMORY )
        return( false );
    if( ins->operands[0]->n.class == N_INDEXED )
        return( false );
    if( ins->operands[0]->n.class == N_CONSTANT ) {
        if( ins->operands[0]->c.const_type == CONS_ABSOLUTE ) {
            ins->operands[0] = AllocConst( CnvCFToType( ins->operands[0]->c.value, TypeOfTypeClass( ins->type_class ) ) );
        }
    }
    return( true );
}

static  void    LinkMoves( block *root )
/***************************************
    Link together all move instructions in partion defined by "root" using
    a field in ins->result as the head of the list.
*/
{
    block       *blk;
    instruction *ins;

    blk = root;
    for( ;; ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            if( LinkableMove( ins ) ) {
                CreateLink( ins, ins->result );
            }
        }
        blk = blk->u.partition;
        if( blk == root ) {
            break;
        }
    }
}


static  void    LinkMemMoves( block *root )
/***************************************** *
    Link together all move instructions in partion defined by "root" using
    a field in ins->operands[0] as the head of the list.
*/
{
    block       *blk;
    instruction *ins;

    blk = root;
    for(;;) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            if( !isMoveIns( ins ) )
                continue;
            if( !CanLinkMove( ins ) )
                continue;
            if( ins->operands[0]->n.class != N_MEMORY && ins->operands[0]->n.class != N_INDEXED )
                continue;
            CreateLink( ins, ins->operands[0] );
        }
        blk = blk->u.partition;
        if( blk == root ) {
            break;
        }
    }
}


static  void    NullNameLink( name *op )
/***************************************
    Set the "definition" list of "op" back to NULL (clean up routine).
*/
{
    switch( op->n.class ) {
    case N_MEMORY:
    case N_TEMP:
        _NAMELINK( op ) = NULL;
        break;
    case N_INDEXED:
        NullNameLink( op->i.index );
        if( op->i.base != NULL ) {
            NullNameLink( op->i.base );
        }
        break;
    }
}


static  void    CleanMoves( block *root )
/****************************************
    Clean up all the linking we did to instructions / names in partition
    "root".
*/
{
    instruction *ins;
    block       *blk;
    opcnt       i;

    blk = root;
    for( ;; ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            for( i = ins->num_operands; i-- > 0; ) {
                NullNameLink( ins->operands[i] );
            }
            if( ins->result != NULL ) {
                NullNameLink( ins->result );
            }
        }
        blk = blk->u.partition;
        if( blk == root ) {
            break;
        }
    }
    CleanTableEntries( root );
}


static  bool    PropOpnd( instruction *ins, name **op,
                          instruction *definition, bool backward,
                          bool is_opnd )
/****************************************************************
    See if we can propagate the operand from a move into the spot for
    operands "*op" in instruction "ins" given definition (move) list
    "definition".  (See DoPropagateMoves).
*/
{
    bool                change;
    name                *opnd;
    name                *defop;
    name                *defres;
    type_length         disp;
    name                *base;

    opnd = *op;
    change = false;
    for( ; definition != NULL; definition = _INSLINK( definition ) ) {
        if( WhichIsAncestor( definition, ins ) == definition && UnOpsLiveFrom( definition, ins ) ) {
            defop = definition->operands[0];
            defres = definition->result;
            if( backward ) {
                if( defop == opnd && defres->n.class == N_TEMP && is_opnd ) {
                    if( ( _IsFloating( definition->type_class ) == _IsFloating( _OpClass( ins ) ) )
                     && CanCrossBlocks( definition, ins, defres )
                     && !FPStackOp( defres ) ) {
                        UseInOther( definition, ins, defres );
                        *op = defres;
                        change = true;
                    }
                }
            } else {
                if( defres == opnd ) {
                    if( defres->n.class != N_INDEXED && is_opnd ) {
                        if( ( _IsFloating( definition->type_class )
                           == _IsFloating( _OpClass( ins ) ) )
                         && CanCrossBlocks( definition, ins, defop )
                         && !FPStackOp( defop ) ) {
                            UseInOther( definition, ins, defop );
                            *op = defop;
                            change = true;
                        }
                    }
                } else if( opnd->n.class == N_INDEXED && definition->result->n.class == N_TEMP ) {
                    if( defop->n.class == N_TEMP
                          && defop->n.type_class==opnd->i.index->n.type_class
                          && CanCrossBlocks( definition, ins, defop ) ) {
                        UseInOther( definition, ins, defop );
                        *op = ScaleIndex( defop, opnd->i.base,
                                        opnd->i.constant,
                                        opnd->n.type_class, opnd->n.size,
                                        opnd->i.scale, opnd->i.index_flags );
                        change = true;
                    } else if( defop->n.class == N_CONSTANT
                            && ins->head.opcode != OP_SELECT ) {
                        disp = 0;
                        base = NULL;
                        switch( defop->c.const_type ) {
                        case CONS_ABSOLUTE:
                            if( opnd->i.base != NULL && (opnd->i.index_flags & X_FAKE_BASE) == 0 ) {
                                disp = opnd->i.constant + defop->c.lo.int_value;
                                base = opnd->i.base;
                            }
                            break;
                        case CONS_ADDRESS:
                        case CONS_OFFSET:
                        case CONS_TEMP_ADDR:
                            if( opnd->i.base == NULL || (opnd->i.index_flags & X_FAKE_BASE) ) {
                                disp = opnd->i.constant;
                                base = defop->c.value;
                            }
                            break;
                        case CONS_HIGH_ADDR:
                            break;
                        case CONS_SEGMENT:
                            Zoiks( ZOIKS_086 );
                            break;
                        default:
                            Zoiks( ZOIKS_105 );
                            break;
                        }
                        if( base != NULL ) {
                            switch( base->n.class ) {
                            case N_TEMP:
                                *op = STempOffset( base,
                                                disp,
                                                opnd->n.type_class,
                                                opnd->n.size );
                                break;
                            case N_MEMORY:
                                *op = (name *)SAllocMemory( base->v.symbol,
                                            base->v.offset + disp,
                                            base->m.memory_type,
                                            opnd->n.type_class,
                                            opnd->n.size );
                                break;
                            default:
                                Zoiks( ZOIKS_087 );
                                break;
                            }
                            change = true;
                        }
                    }
                }
            }
        }
    }
    return( change );
}


static  bool    PropMoves( block *root, bool backward )
/******************************************************
    For each operand of each instruction in partition "root", see if
    there is a move preceeding it whose operand could be used instead.
    (See DoPropagateMoves)
*/
{
    opcnt       i;
    block       *blk;
    bool        change;
    instruction *ins;
    name        *idx;
    opcode_defs opcode;

    change = false;
    blk = root;
    for( ;; ) {
        for( ins = blk->ins.hd.next; (opcode = ins->head.opcode) != OP_BLOCK; ins = ins->head.next ) {
            if( opcode != OP_LA
              && opcode != OP_CAREFUL_LA
              && opcode != OP_EXT_ADD
              && opcode != OP_EXT_SUB
              && opcode != OP_CALL_INDIRECT
              && opcode != OP_LOAD_UNALIGNED
              && opcode != OP_STORE_UNALIGNED ) {
                for( i = OpcodeNumOperands( ins ); i-- > 0; ) {
                    switch( ins->operands[i]->n.class ) {
                    case N_INDEXED:
                        idx = ins->operands[i]->i.index;
                        switch( idx->n.class ) {
                        case N_REGISTER:
                        case N_CONSTANT:
                            break;
                        default:
                            change |= PropOpnd( ins, &ins->operands[i], _NAMELINK( idx ), backward, true );
                            break;
                        }
                        break;
                    case N_CONSTANT:
                    case N_REGISTER:
                        break;
                    default:
                        change |= PropOpnd( ins, &ins->operands[i], _NAMELINK( ins->operands[i] ), backward, true );
                        break;
                    }
                }
                if( ins->result != NULL ) {
                    if( ins->result->n.class == N_INDEXED ) {
                        idx = ins->result->i.index;
                        if( idx->n.class != N_REGISTER ) {
                            RemoveLink( ins, ins->result );
                            change |= PropOpnd( ins, &ins->result, _NAMELINK( idx ), backward, false );
                            if( LinkableMove( ins ) ) {
                                CreateLink( ins, ins->result );
                            }
                        }
                    }
                }
            }
        }
        blk = blk->u.partition;
        if( blk == root ) {
            break;
        }
    }
    return( change );
}


static  bool    DoPropagateMoves( void )
/***************************************
    First link all moves in a partion together with a special field in
    the symbol table of ins->result being used to store the head of the
    list, and then try to propagate moves in each partition.  Once moves
    are linked this way, we can do copy propagation easily, since the
    operand of any instruction has a list of moves which define that
    operand hanging from it.  We then check for a definition that is an
    ancestor of the operation and if the operand and result of the move
    live (UnOpsLiveFrom) between the move and the use, we just replace
    the operand of the instruction with the operand of the move.
*/
{
    bool        change;
    bool        block_changed;
    block       *blk;

    change = false;
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        SXBlip();
        if( _IsBlkVisited( blk ) ) {
            do {
                block_changed = false;
                if( FixStructRet( blk ) ) {
                    block_changed = true;
                    change = true;
                }
                if( PeepOpt( blk, NextBlock, blk, false ) ) {
                    block_changed = true;
                    change = true;
                }
                if( ConstFold( blk ) ) {
                    block_changed = true;
                    change = true;
                }
                LinkMoves( blk );
                if( PropMoves( blk, false ) ) {
                    block_changed = true;
                    change = true;
                }
                CleanMoves( blk );
                LinkMemMoves( blk );
                if( PropMoves( blk, true ) ) {
                    block_changed = true;
                    change = true;
                }
                CleanMoves( blk );
            } while( block_changed );
        }
    }
    return( change );
}

bool    PropagateMoves( void )
/*****************************
    Do copy propagation.
*/
{
    bool        change;

    change = false;
    FindPartition();
    while( DoPropagateMoves() ) {
        change = true;
    }
    CleanPartition();
    return( change );
}


bool    CommonSex( bool leave_indvars_alone )
/****************************************************
    Do COMMON SubEXpression and related optimizations.
*/
{
    bool        loops_killed;
    bool        change;
    bool        anychange;

    loops_killed = false;
    LeaveIndVars = leave_indvars_alone;
    FindPartition();
    anychange = LoadAddr();
    if( PropRegsOne() )
        anychange = true;
    for( ;; ) {
        ReCalcAddrTaken();
        change = false;
        if( DoPropagateMoves() )
            change = true;
        if( StretchEdges() ) {
            change = true;
            loops_killed = true;
            CleanPartition();
            FindPartition();
        }
        if( DeadBlocks() ) {
            change = true;
            loops_killed = true;
            CleanPartition();
            FindPartition();
        }
        if( PropagateExprs() )
            change = true;
        if( change )
            InsDead();
        if( change ) {
            anychange = true;
        } else {
            break;
        }
    }
    CleanPartition();
    if( loops_killed ) {
        BlockTrim();
        MakeFlowGraph();
    }
    return( anychange );
}
