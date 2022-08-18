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
#include "data.h"
#include "savings.h"
#include "rgtbl.h"
#include "conflict.h"
#include "regsave.h"


static  save_def        MaxConstSave;

#define COST( s, t ) ( ( (s)*size + (t)*time ) / TOTAL_WEIGHT )
#define _NEXTCON( c ) (*(conflict_node**)&((c)->tree))

void    InitWeights( uint size )
/*****************************************
    Set up a structure describing the savings/costs involved
    with operations like loads, stores, saving a memory reference, etc.
    "size" is the importance of code size (vs. speed) expressed
    as a percentage between 0 and 100.
*/
{
    uint        time;

    AdjTimeSize( &time, &size );
    SetLoopCost( time );

/*   Indexing    - save load of index register*/
/*   Loads       - cost move memory to register instruction*/
/*   Stores      - cost move register to memory instruction*/
/*   Uses        - replace memory reference with register reference*/
/*   Definitions - save move register to memory*/
/*   Prologs    - push a register*/
/*   Epilogs    - pop a register*/


    if( _CPULevel( CPU_486 ) /* or 586 */ ) {
        SetCost( Save.load_cost, COST( 3,1 ) );
        SetCost( Save.store_cost, COST( 3,1 ) );
        SetCost( Save.use_save, COST( 1,1 ) );
        SetCost( Save.def_save, COST( 1,1 ) );
        SetCost( Save.push_cost, COST( 1,1 ) );
        SetCost( Save.pop_cost, COST( 1,1 ) );
    } else if( _CPULevel( CPU_386 ) ) {
        SetCost( Save.load_cost, COST( 3,4 ) );
        SetCost( Save.store_cost, COST( 3,2 ) );
        SetCost( Save.use_save, COST( 1,(4-2) ) );
        SetCost( Save.def_save, COST( 1,2 ) );
        SetCost( Save.push_cost, COST( 1,2 ) );
        SetCost( Save.pop_cost, COST( 1,4 ) );
    } else { // how about some 286 and 186 timings!
        SetCost( Save.load_cost, COST( 3,17 ) );
        SetCost( Save.store_cost, COST( 3,18 ) );
        SetCost( Save.use_save, COST( 1,(18-3) ) );
        SetCost( Save.def_save, COST( 1,18 ) );
        SetCost( Save.push_cost, COST( 1,11 ) );
        SetCost( Save.pop_cost, COST( 1,8 ) );
    }
    Save.index_save     = Save.load_cost[WD];
}


bool    WorthProlog( conflict_node *conf, hw_reg_set reg )
/*******************************************************************
    decide if the savings associated with giving conflict "conf"
    is worth the cost incurred by generating a prolog to
    save and restore register "reg"
*/
{
    save_def            cost;
    save_def            savings;
    hw_reg_set          must_save;
    type_class_def      type_class;
    name                *op;

    type_class = conf->name->n.type_class;
    must_save = MustSaveRegs();
    if( BlockByBlock || HW_Ovlap( reg, GivenRegisters ) ||
       !HW_Ovlap( reg, must_save ) ) {
        cost = 0;
    } else {
        cost = Save.pop_cost[type_class] + Save.push_cost[type_class];
    }
    op = conf->name;
    savings = conf->savings;
    if( _ConstTemp( op ) ) {
        /* adjust for the initial load */
        cost += Weight( Save.load_cost[type_class] + Save.def_save[type_class], conf->start_block );
        /* Adjust by a fudge factor */
        savings /= LOOP_FACTOR;
    } else {
        savings -= MaxConstSave;
    }
    return( savings >= cost );
}


void        ConstSavings( void )
/**************************************

    Ensure constants are cached last by making sure that all
    "real" conflict nodes have higher savings than all conflict
    nodes for temps holding a constant value.  Make sure as well,
    that all "outer" loop constant temporary conflicts inherit the
    savings of any inner conflict that they define.
*/
{
    conflict_node       *conf;
    conflict_node       *other;
    block               *blk;
    instruction         *ins;

    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            if( ins->head.opcode != OP_MOV )
                continue;
            if( !_ConstTemp(ins->operands[0]) )
                continue;
            if( !_ConstTemp(ins->result) )
                continue;
            conf = NameConflict( ins, ins->operands[0] );
            if( conf == NULL )
                continue;
            other = NameConflict( ins, ins->result );
            if( other == NULL )
                continue;
            if( _Isnt( conf, CST_SAVINGS_JUST_CALCULATED ) )
                continue;
            if( _Isnt( other, CST_SAVINGS_CALCULATED ) )
                continue;
            _NEXTCON( conf ) = other;
        }
    }
    for( conf = ConfList; conf != NULL; conf = conf->next_conflict ) {
        if( _Is( conf, CST_SAVINGS_JUST_CALCULATED ) && _ConstTemp( conf->name ) ) {
            for( other=_NEXTCON(conf); other != NULL; other=_NEXTCON(other) ) {
                conf->savings += other->savings;
                if( _Is( other, CST_CONF_VISITED ) ) {
                    break;
                }
            }
            _SetTrue( conf, CST_CONF_VISITED ); /* already summed down the list */
        }
    }
    for( conf = ConfList; conf != NULL; conf = conf->next_conflict ) {
        _NEXTCON( conf ) = NULL;
        _SetFalse( conf, CST_CONF_VISITED );
    }
    MaxConstSave = 0;
    for( conf = ConfList; conf != NULL; conf = conf->next_conflict ) {
        if( !_ConstTemp( conf->name ) )
            continue;
        _SetFalse( conf, CST_SAVINGS_JUST_CALCULATED );
        if( _Isnt( conf, CST_SAVINGS_CALCULATED ) )
            continue;
        if( conf->savings < MaxConstSave )
            continue;
        MaxConstSave = conf->savings;
    }
    for( conf = ConfList; conf != NULL; conf = conf->next_conflict ) {
        if( _ConstTemp( conf->name ) )
            continue;
        if( _Isnt( conf, CST_SAVINGS_JUST_CALCULATED ) )
            continue;
        _SetFalse( conf, CST_SAVINGS_JUST_CALCULATED );
        if( conf->savings != 0 ) {
            conf->savings += MaxConstSave;
        }
    }
}
