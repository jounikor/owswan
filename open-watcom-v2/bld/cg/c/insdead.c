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
* Description:  Identify and eliminate dead (useless) instructions.
*
****************************************************************************/


#include "_cgstd.h"
#include "coderep.h"
#include "data.h"
#include "fpu.h"
#include "makeins.h"
#include "redefby.h"
#include "insdead.h"
#include "namelist.h"
#include "optab.h"
#include "breakrtn.h"


static  void    InitVisitedTemps( void )
/***************************************
    Mark all N_TEMP and N_MEMORY names as Not visited. If NO_OPTIMIZATION
    is on, mark them all as visited.
*/
{
    name        *op;
    name        *alias;

    for( op = Names[N_TEMP]; op != NULL; op = op->n.next_name ) {
        op->t.temp_flags &= ~VISITED;
    }
    for( op = Names[N_TEMP]; op != NULL; op = op->n.next_name ) {
        if( op->v.usage & (USE_ADDRESS | VAR_VOLATILE) ) {
            alias = op;
            do {
                alias->t.temp_flags |= VISITED;
                alias = alias->t.alias;
            } while( alias != op );
        }
    }
    if( _IsModel( NO_OPTIMIZATION ) ) {
        for( op = Names[N_TEMP]; op != NULL; op = op->n.next_name ) {
            if( _FrontEndTmp( op ) ) {
                op->t.temp_flags |= VISITED;
            }
        }
    }
    if( BlockByBlock || _IsModel ( NO_OPTIMIZATION ) ) {
        for( op = Names[N_TEMP]; op != NULL; op = op->n.next_name ) {
            if( op->v.usage & USE_IN_ANOTHER_BLOCK ) {
                op->t.temp_flags |= VISITED;
            }
        }
    }
}


static  bool            FreeUselessIns( block *tail, bool just_the_loop,
                                        bool in_regalloc )
/***********************************************************************
    Free any instructions which have not been marked
    with the INS_VISITED bit. See below for the setting
    of this bit.
*/
{
    block       *blk;
    instruction *ins;
    instruction *prev;
    bool        change;

    change = false;
    for( blk = tail; blk != NULL; blk = blk->prev_block ) {
        if( just_the_loop && !_IsBlkAttr( blk, BLK_IN_LOOP ) ) {
            for( ins = blk->ins.hd.prev; ins->head.opcode != OP_BLOCK; ins = ins->head.prev ) {
                ins->ins_flags &= ~INS_VISITED;
            }
        } else {
            for( ins = blk->ins.hd.prev; ins->head.opcode != OP_BLOCK; ins = prev ) {
                prev = ins->head.prev;
                if( ( ins->ins_flags & INS_VISITED ) == 0 ) {
                    change = true;
                    if( in_regalloc ) {
                        DoNothing( ins );
                    } else {
                        FreeIns( ins );
                    }
                } else {
                    ins->ins_flags &= ~INS_VISITED;
                }
            }
        }
    }
    return( change );
}


static  void            FreeUnVisitedTemps( void )
/*************************************************
    Free any temps which have not been marked VISITED. They are
    useless. See below for setting of this bit.
*/
{
    name        *op;
    name        **owner;

    owner = &Names[N_TEMP];
    for( ;; ) {
        op = *owner;
        if( op == NULL )
            break;
        if( op->t.temp_flags & VISITED ) {
            op->t.temp_flags &= ~VISITED;
            owner = &op->n.next_name;
        } else {
            *owner = op->n.next_name;
            FreeAName( op );
        }
    }
}


bool    VolatileIns( instruction *ins )
/********************************************
    Does the instruction access/define a volatile variable?
    This is a utility routine for any module to use.
*/
{
    opcnt   i;

    for( i = ins->num_operands; i-- > 0; ) {
        if( IsVolatile( ins->operands[i] ) ) {
            return( true );
        }
    }
    if( ins->result != NULL ) {
        if( IsVolatile( ins->result ) ) {
            return( true );
        }
    }
    return( false );
}


bool    SideEffect( instruction *ins )
/***************************************************
    Is an instruction a side effect instruction, such as one
    that changes the 8087 stack or a SUB with a following SBB.
    This is a utility routine for any module to use.
*/
{
    if( ins->head.opcode == OP_PUSH )
        return( true );
    if( ins->head.opcode == OP_POP )
        return( true );
    if( (ins->ins_flags & INS_CC_USED)
      && ins->head.opcode != OP_MOV )
        return( true );
    if( FPSideEffect( ins ) )
        return( true );
    return( VolatileIns( ins ) );
}


static  bool    MarkUseful( name *op )
/*************************************
    Mark an operand as useful. If the operand is an indexed name,
    its base and index become useful as well.
*/
{
    bool        change;
    name        *alias;

    change = false;
    if( op->n.class == N_TEMP ) {
        if( ( op->t.temp_flags & VISITED ) == 0 ) {
            change = true;
            alias = op;
            do {
                alias->t.temp_flags |= VISITED;
                alias = alias->t.alias;
            } while( alias != op );
        }
    } else if( op->n.class == N_INDEXED ) {
        change |= MarkUseful( op->i.index );
        if( op->i.base != NULL ) {
            change |= MarkUseful( op->i.base );
        }
    } else if( op->n.class == N_CONSTANT ) {
        if( op->c.const_type == CONS_TEMP_ADDR ) {
            change |= MarkUseful( op->c.value );
        }
    }
    return( change );
}


static  bool    MarkOpsUseful( instruction *ins )
/************************************************
    We have decided that an instruction is useful, therefore we must
    mark all of its operands as useful
*/
{
    opcnt       i;
    bool        change;

    change = false;
    ins->ins_flags |= INS_VISITED;
    for( i = ins->num_operands; i-- > 0; ) {
        change |= MarkUseful( ins->operands[i] );
    }
    return( change );
}


static  bool    CheckUseful( instruction *ins )
/**********************************************
    Mark an instruction INS_VISITED if it is useful. A useful
    instruction is one that causes a branch, defines memory, a register
    or a VISITED operand. We return true whenver an instruction gets
    marked INS_VISITED that wasn't before.
*/
{
    name        *res;
    opcode_defs opcode;
    bool        change;

    opcode = ins->head.opcode;
    change = false;
    res = ins->result;
    if( !_OpIsJump( opcode )
     && opcode != OP_PUSH
     && opcode != OP_POP
     && opcode != OP_NOP
     && opcode != OP_CALL_INDIRECT
     && opcode != OP_CALL ) {
        if( SideEffect( ins ) ) {
            if( res != NULL ) {
                change |= MarkUseful( res );
            }
            change |= MarkOpsUseful( ins );
            return( change );
        }
        if( res != NULL ) {
            if( res->n.class == N_MEMORY || res->n.class == N_REGISTER ) {
                change |= MarkOpsUseful( ins );
                return( change );
            }
            if( res->n.class == N_INDEXED ) {
                change |= MarkUseful( res );
                change |= MarkOpsUseful( ins );
                return( change );
            }
            if( res->n.class == N_TEMP ) {
                if( res->t.temp_flags & VISITED ) {
                    change |= MarkOpsUseful( ins );
                    return( change );
                }
            }
        }
    } else {
        if( res != NULL ) {
            change |= MarkUseful( res );
        }
        change |= MarkOpsUseful( ins );
    }
    return( change );
}


static  void            FindUsefulIns( block * tail, bool just_the_loop,
                                        bool in_regalloc )
/***********************************************************************
    This goes around calling CheckUseful until no more instructions
    are found to be useful. Initially, instructions which affect
    branches, or define memory/registers are useful.
    The operands of a useful instruction are marked VISITED.
    On subsequent passes, instructions which define VISITED names
    are also marked useful and their operands are marked VISITED.
    Iterating, we find out every useful instruction in the routine,
    and anything not marked useful gets killed.

*/
{
    bool        change;
    block       *blk;
    instruction *ins;

    if( just_the_loop ) {
        for( blk = tail; blk != NULL; blk = blk->prev_block ) {
            if( !_IsBlkAttr( blk, BLK_IN_LOOP ) ) {
                for( ins = blk->ins.hd.prev; ins->head.opcode != OP_BLOCK; ins = ins->head.prev ) {
                    MarkOpsUseful( ins );
                    if( ins->result != NULL ) {
                        MarkUseful( ins->result );
                    }
                }
            }
        }
    }
    do {
        change = false;
        for( blk = tail; blk != NULL; blk = blk->prev_block ) {
            if( !just_the_loop || _IsBlkAttr( blk, BLK_IN_LOOP ) ) {
                for( ins = blk->ins.hd.prev; ins->head.opcode != OP_BLOCK; ins = ins->head.prev ) {
                    if( !in_regalloc || DoesSomething( ins ) ) {
                        change |= CheckUseful( ins );
                    }
                }
            }
        }
    } while( change );
}


static  bool    RemoveUselessStuff( bool just_the_loop, bool in_regalloc )
/************************************************************************
    This routine removes useless instructions from the routine. Note
    that if BreakExists(), there are some extra blocks hanging off
    BlockList that we must consider in our analysis. This is for FORTRAN
    when we are generating partial routines. HeadBlock points to what
    we really want to generate, BlockList points the the rest. We discover
    which instruction are useless by marking all useful instructions
    in the program
*/
{
    block       *tail;
    bool        change;

    InitVisitedTemps();
    FindUsefulIns( BlockList, just_the_loop, in_regalloc );
    tail = TailBlocks();
    if( tail != NULL ) {
        FindUsefulIns( tail, just_the_loop, in_regalloc );
    }
    change = FreeUselessIns( BlockList, just_the_loop, in_regalloc );
    if( tail != NULL ) {
        change |= FreeUselessIns( tail, just_the_loop, in_regalloc );
    }
    if( !in_regalloc )
        FreeUnVisitedTemps();
    return( change );
}


static bool DoInsDead( bool just_the_loop, bool in_regalloc )
/***********************************************************/
{
    if( BlockByBlock && !BreakExists() )
        return( false );
    return( RemoveUselessStuff( just_the_loop, in_regalloc ) );
}


bool    InsDead( void )
/**********************
    Remove any dead or useless instructions in the program we can find.
*/
{
    return( DoInsDead( false, false ) );
}

bool    RegInsDead( void )
/*************************
    Remove any dead or useless instructions in the program we can find.
*/
{
    return( DoInsDead( false, true ) );
}

bool    LoopInsDead( void )
/*************************/
{
    return( DoInsDead( true, false ) );
}
