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
* Description:  Load/store calculations.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "zoiks.h"
#include "data.h"
#include "redefby.h"
#include "nullprop.h"
#include "conflict.h"
#include "loadstor.h"


/* block flag usage                                                 */
/*                                                                  */
/* BLK_BLOCK_MARKED is used in the sense of real reference          */
/* BLK_BLOCK_VISITED is used in the sense of no load/store          */
/*                                                                  */
#define BLK_CONTAINS_CALL   BLK_LOOP_EXIT /* borrow. Only used during loop opts */
/*                                                                  */


static  bool    SameConf( name *op, instruction *ins, conflict_node *conf )
/*************************************************************************/
{
    if( op->n.class == N_INDEXED ) {
        if( NameConflict( ins, op->i.index ) == conf )
            return( true );
        if( HasTrueBase( op ) ) {
            if( NameConflict( ins, op->i.base ) == conf ) {
                return( true );
            }
        }
    } else {
        if( NameConflict( ins, op ) == conf ) {
            return( true );
        }
    }
    return( false );
}


static  void    CheckRefs( conflict_node *conf, block *blk )
/***********************************************************
    mark block if it contains a reference to conf.
    Also mark as BLK_CONTAINS_CALL if it does
*/
{
    opcnt       i;
    instruction *ins;

    if( _IsBlkAttr( blk, BLK_BIG_LABEL | BLK_RETURN | BLK_BIG_JUMP ) ) {
        _MarkBlkMarked( blk );
        return;
    }
    for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
        for( i = ins->num_operands; i-- > 0; ) {
            if( SameConf( ins->operands[i], ins, conf ) ) {
                _MarkBlkMarked( blk );
                return;
            }
        }
        if( ins->result != NULL ) {
            if( SameConf( ins->result, ins, conf ) ) {
                _MarkBlkMarked( blk );
                return;
            }
        }
        if( _OpIsCall( ins->head.opcode ) &&
           ( (ins->flags.call_flags & CALL_WRITES_NO_MEMORY) == 0 ||
               (ins->flags.call_flags & CALL_READS_NO_MEMORY) == 0 ) ) {
            _MarkBlkAttr( blk, BLK_CONTAINS_CALL );
        }
    }
}


static  void    LoadStoreIfCall( global_bit_set *id )
/****************************************************
    Turn on bits for need_load/need_store for conflict id in all blocks
    which have a call but no real reference to id.  This is sort of
    backwards, since it would cause a load at the start of the block and
    store at the end, but that will force stores in all ancestor blocks,
    and load in all successor blocks (done by PropagateLoadStoreBits).
    TurnOffLoadStoreBits will then turn off the bits we turned on in
    this block, and we achieve an optimial load/store scheme for
    cacheing a static.
*/
{
    block               *blk;
    data_flow_def       *flow;

    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        if( _IsBlkAttr( blk, BLK_CONTAINS_CALL ) && !_IsBlkMarked( blk ) ) {
            flow = blk->dataflow;
            _GBitTurnOn( flow->need_load, *id );
            _GBitTurnOn( flow->need_store, *id );
            _GBitTurnOn( flow->call_exempt, *id );
        }
    }
}


static  void    TurnOffLoadStoreBits( global_bit_set *id )
/*********************************************************
    If a block has need_load and need_store but never really references
    id, we can get rid of the load/store.
*/
{
    block               *blk;
    data_flow_def       *flow;

    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        if( !_IsBlkMarked( blk ) ) {
            flow = blk->dataflow;
            if( _GBitOverlap( flow->need_load, *id ) && _GBitOverlap( flow->need_store, *id ) ) {
                _GBitTurnOff( flow->need_load, *id );
                _GBitTurnOff( flow->need_store, *id );
            }
        }
    }
}


static  void    PropagateLoadStoreBits( block *start, global_bit_set *id )
/*************************************************************************
    Make sure that ancestors of need_load blocks do a store at the end
    and successors of need_store blocks do a load at the beginning.  This
    will be a conservative estimate, fixed up by TurnOffLoadStoreBits
*/
{
    data_flow_def       *source_dat;
    data_flow_def       *blk_dat;
    bool                change;
    block_edge          *edge;
    block               *blk;

    do {
        change = false;
        for( blk = start; blk != NULL; blk = blk->next_block ) {
            blk_dat = blk->dataflow;
            for( edge = blk->input_edges; edge != NULL; edge = edge->next_source ) {
                source_dat = edge->source->dataflow;
                if( _GBitOverlap( source_dat->out, *id ) &&
                    _GBitOverlap( blk_dat->in, *id ) ) {
                    /* NB: there are 3 IFs to minimize the & of iterations*/
                    if( _GBitOverlap( source_dat->need_store, *id ) ) {
                        change |= !_GBitOverlap( blk_dat->need_load, *id );
                        _GBitTurnOn( blk_dat->need_load, *id );
                    }
                    if( _GBitOverlap( blk_dat->need_load, *id ) ) {
                        change |= !_GBitOverlap( source_dat->need_store, *id );
                        _GBitTurnOn( source_dat->need_store, *id );
                    }
                    if( _GBitOverlap( source_dat->need_store, *id ) ) {
                        change |= !_GBitOverlap( blk_dat->need_load, *id );
                        _GBitTurnOn( blk_dat->need_load, *id );
                    }
                }
            }
        }
    } while( change );
}


static  void    CalculateLoadStore( conflict_node *conf )
/********************************************************
    If we are going to cache a global variable in a register, we have a
    few problems, which are resolved in this routine.  If a subroutine
    is called, we must store the register into memory just before the
    subroutine call, and load the new value afterward.  We must also
    store before leaving the range of the conflict node, and load it if
    the conflict range is entered from without.  This routine uses an
    iterative algorithm to determine which blocks will need to have
    loads/stores at the beginning/end because of this. All blocks outside
    the range of the conflict are marked as "need load" and "need store".
    All blocks which don't reference the conflict variable, yet have a
    call are marked as "need load" and "need store". An iterative algorithm
    is used to ensure consistency in the state of the variable/register.
    "need load" requires "need store" in all ancestor blocks. "need store"
    requires "need load" in all successor blocks. Once this is done,
    we optimize by "painting" regions of the blocks which don't reference
    the variable at all. Internal "need load"/"need store" attributes
    are turned off in these regions.
*/
{
    global_bit_set      id;
    block               *blk;
    data_flow_def       *flow;

    _MarkBlkAllAttrNot( BLK_CONTAINS_CALL | BLK_BLOCK_MARKED | BLK_BLOCK_VISITED );
    blk = HeadBlock;
    if( blk != NULL ) {
        _MarkBlkAttr( blk, BLK_BIG_LABEL );
    }
    _GBitAssign( id, conf->id.out_of_block );
    /* turn on bits before the conflict range */
    for( ; blk != NULL; blk = blk->next_block ) {
        if( blk == conf->start_block )
            break;
        _GBitTurnOn( blk->dataflow->need_load, id );
        _GBitTurnOn( blk->dataflow->need_store, id );
        _MarkBlkMarked( blk );
    }
    /* turn on bits in the conflict range */
    for( ; blk != NULL; blk = blk->next_block ) {
        flow = blk->dataflow;
        CheckRefs( conf, blk );
        if( _GBitOverlap( flow->in, id ) && _IsBlkAttr( blk, BLK_BIG_LABEL ) ) {
            _GBitTurnOn( flow->need_load, id );
        } else {
            _GBitTurnOff( flow->need_load, id );
        }
        if( _GBitOverlap( flow->out, id ) && _IsBlkAttr( blk, BLK_RETURN | BLK_BIG_JUMP ) ) {
            _GBitTurnOn( flow->need_store, id );
        } else {
            _GBitTurnOff( flow->need_store, id );
        }
        if( blk->ins.hd.prev != (instruction *)&blk->ins ) {
            _INS_NOT_BLOCK( blk->ins.hd.prev );
            _INS_NOT_BLOCK( conf->ins_range.last );
            if( blk->ins.hd.prev->id >= conf->ins_range.last->id) {
                break;
            }
        }
    }
    /* turn on bits after the conflict range */
    while( blk != NULL ) {
        flow = blk->dataflow;
        blk = blk->next_block;
        if( blk == NULL )
            break;
        _MarkBlkMarked( blk );
        _GBitTurnOn( flow->need_load, id );
        _GBitTurnOn( flow->need_store, id );
    }
    LoadStoreIfCall( &id );
    PropagateLoadStoreBits( conf->start_block, &id );
    TurnOffLoadStoreBits( &id );
    if( NameIsConstant( conf->name ) ) {
        id = conf->id.out_of_block;
        for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
            flow = blk->dataflow;
            _GBitTurnOff( flow->need_store, id );
        }
    }
    _MarkBlkAllAttrNot( BLK_CONTAINS_CALL | BLK_BLOCK_MARKED | BLK_BLOCK_VISITED );
}


void    CalcLoadStore( conflict_node *conf )
/*******************************************
    see below
*/
{
    if( !BlockByBlock ) {
        CalculateLoadStore( conf );
    }
}
