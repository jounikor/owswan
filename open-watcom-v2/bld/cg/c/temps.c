/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2019 The Open Watcom Contributors. All Rights Reserved.
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
#include "data.h"
#include "zoiks.h"
#include "targetin.h"
#include "cgsrtlst.h"
#include "insdead.h"
#include "namelist.h"
#include "optab.h"
#include "temps.h"
#include "varusage.h"
#include "feprotos.h"


typedef struct stack_temp {
        struct stack_temp       *others;
        instruction_id          first;
        instruction_id          last;
} stack_temp;

typedef struct stack_entry {
        struct stack_temp       temp;
        struct stack_entry      *link;
        type_length             location;
        type_length             size;
} stack_entry;

static    stack_entry   *StackMap;

/* borrow some bits for FindOnlyIns stuff */
#define USED_NEVER      EMPTY
#define USED_ONCE       USE_WITHIN_BLOCK
#define USED_TWICE      USE_IN_ANOTHER_BLOCK


static  void    StackEntry( stack_temp *st_temp, name *temp )
/***********************************************************/
{
    stack_entry *new;

    new = CGAlloc( sizeof( stack_entry ) );
    new->link = StackMap;
    new->size = temp->n.size;
    new->location = temp->t.location;
    new->temp.first = st_temp->first;
    new->temp.last = st_temp->last;
    new->temp.others = NULL;
    StackMap = new;
}


void    InitStackMap( void )
/**************************/
{
    StackMap = NULL;
}


static  void    ReInitStackMap( void )
/************************************/
/* we are flushing blocks so make all stack locations from block flushed*/
/* reusable in the next block to be flushed*/
{
    stack_entry *stack;
    stack_temp  *other;
    stack_temp  *next;

    for( stack = StackMap; stack != NULL; stack = stack->link ) {
        for( other = stack->temp.others; other != NULL; other = next ) {
            next = other->others;
            CGFree( other );
        }
        stack->temp.others = NULL;
        stack->temp.first = LAST_INS_ID;
        stack->temp.last = FIRST_INS_ID;
    }
}


static  void    TellTempLocs( void )
/***********************************
    C++ front end wants to know where all the temps are in memory
    for exception handling. They really only care about the relative
    displacements between the temps, so we don't have to worry about
    things like SP relative offsets being different at different spots
    in the code.
    When we do the TEMP_LOC_NAME request, the front end can either respond
    with a TEMP_LOC_YES, for us to tell them the location, or a TEMP_LOC_NO,
    if they don't care about the location for this name, or a TEMP_LOC_QUIT
    which means they don't want to know about any other temporaries.
    Correction: They now want actual offsets on the Alpha and PPC, so we
    just add in CurrProc->locals.size. This doesn't affect them on the
    Intel as the relative values are still the same.
*/
{
    name        *temp;
    int         ans;

    for( temp = Names[N_TEMP]; temp != NULL; temp = temp->n.next_name ) {
        if( temp->v.symbol != NULL && (temp->t.temp_flags & ALIAS) == 0 ) {
            ans = (int)(pointer_uint)FEAuxInfo( temp->v.symbol, TEMP_LOC_NAME );
            if( ans == TEMP_LOC_QUIT )
                break;
            if( temp->t.location == NO_LOCATION )
                continue;
            if( ans == TEMP_LOC_YES ) {
                FEAuxInfo( (pointer)(pointer_uint)TempLocation( temp ), TEMP_LOC_TELL );
            }
        }
    }
}


static  stack_entry     *ReUsableStack(stack_temp *st_temp,name *temp)
/********************************************************************/
{
    type_length size;
    stack_entry *stack;
    stack_temp  *other;

    size = temp->n.size;
    for( stack = StackMap; stack != NULL; stack = stack->link ) {
        if( size <= stack->size ) {
            for( other = &stack->temp; other != NULL; other = other->others ) {
                if( other->first <= st_temp->last && other->last >= st_temp->first ) {
                    break;
                }
            }
            if( other == NULL ) {
                break;
            }
        }
    }
    return( stack );
}


static bool SetLastUse( name *op, name *temp, stack_temp *new, instruction *ins )
/*******************************************************************************/
{
    if( op->n.class == N_INDEXED && op->i.base != NULL ) {
        op = op->i.base;
    }
    if( op->n.class == N_TEMP ) {
        if( DeAlias( op ) == temp ) {
            _INS_NOT_BLOCK ( ins );
            new->last = ins->id;
            return( true );
        }
    }
    return( false );
}


static  void    ScanForLastUse( block *blk, stack_temp *new, name *temp )
/***********************************************************************/
{
    instruction *ins;
    opcnt       i;

    for( ins = blk->ins.hd.prev; ins->head.opcode != OP_BLOCK; ins = ins->head.prev ) {
        if( ins->result != NULL && ins->result->n.class == N_TEMP
          && DeAlias( ins->result ) == temp ) {
            if( SideEffect( ins ) ) {
                new->last = new->first + 1;
                return;
            } else {
                DoNothing( ins );
            }
        } else {
            for( i = ins->num_operands; i-- > 0; ) {
                if( SetLastUse( ins->operands[i], temp, new, ins ) ) {
                    return;
                }
            }
            if( ins->result != NULL ) {
                if( SetLastUse( ins->result, temp, new, ins ) ) {
                    return;
                }
            }
        }
    }
    new->last = new->first;
}


static  void    ScanForFirstDefn( block *blk, stack_temp *new, name *temp )
/*************************************************************************/
{
    instruction *ins;
    opcnt       i;

    for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
        if( ins->result != NULL ) {
            if( ins->result->n.class == N_TEMP ) {
                if( DeAlias( ins->result ) == temp ) {
                    new->first = ins->id;
                    return;
                }
             }
        }
        for( i = ins->num_operands; i-- > 0; ) {
            if( ins->operands[i]->n.class == N_TEMP ) {
                if( DeAlias( ins->operands[i] ) == temp ) {
                    new->first = ins->id;
                    return;
                }
            }
        }
    }
    new->first = FIRST_INS_ID;
}


static  void    CalcRange( stack_temp *new, name *temp )
/******************************************************/
{
    block       *blk;

    for( blk = HeadBlock; blk->id != temp->t.u.block_id; ) {
        blk = blk->next_block;
    }
    ScanForFirstDefn( blk, new, temp );
    ScanForLastUse( blk, new, temp );
}


static  bool    In( name *op, name *name )
/****************************************/
{
    if( op->n.class == N_INDEXED ) {
        if( In( op->i.index, name ) )
            return( true );
        if( op->i.base != NULL && (op->i.index_flags & X_FAKE_BASE) == 0 ) {
            return( In( op->i.base, name ) );
        }
    } else if( op->n.class == N_TEMP ) {
        if( DeAlias( op ) == name ) {
            return( true );
        }
    }
    return( false );
}


static  bool    UsedByLA( instruction *ins, name *temp )
/******************************************************/
{
    if( ins->head.opcode == OP_LA || ins->head.opcode == OP_CAREFUL_LA ) {
        if( ins->operands[0] == temp ) {
            return( true );
        }
    }
    return( false );
}


static  instruction     *FindOnlyIns( name *name, bool *any_references )
/**********************************************************************/
{
    block       *blk;
    instruction *ins;
    instruction *onlyins;
    opcnt       i;

    if( name->v.block_usage == USED_NEVER ) {
        *any_references = false;
        return( NULL );
    }
    if( name->v.block_usage == USED_TWICE ) {
        *any_references = true;
        return( NULL );
    }
    *any_references = true;
    onlyins = NULL;
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            for( i = ins->num_operands; i-- > 0; ) {
                if( In( ins->operands[i], name ) ) {
                    if( onlyins != NULL )
                        return( NULL );
                    onlyins = ins;
                }
            }
            if( ins->result != NULL ) {
                if( In( ins->result, name ) ) {
                    if( onlyins != NULL )
                        return( NULL );
                    onlyins = ins;
                }
            }
        }
    }
    if( onlyins == NULL ) {
        *any_references = false;
    }
    return( onlyins );
}


void    PropLocal( name *temp )
/*****************************/
{
    name        *scan;

    for( scan = temp->t.alias; scan != temp; scan = scan->t.alias ) {
        scan->t.location = temp->t.location;
        scan->v.usage |= HAS_MEMORY;
    }
}


static  void    NewLocation( name *temp, type_length size )
/*********************************************************/
{
    SetTempLocation( temp, size );
    temp->v.usage |= HAS_MEMORY;
    PropLocal( temp );
}


static  void    AllocNewLocal( name *temp )
/*****************************************/
{
    type_length size;
    stack_entry *stack;
    stack_temp  *new_st_temp;
    instruction *ins;
    stack_temp  st_temp;
    bool        any_references;

#if 0
    // this is disabled because users can generate code which
    // may be incorrect (most likely) but which we don't want
    // to trigger this assert.
    /* such as:
    enum MonthEnum { InvalidMonth = 0, January, February, March };
    int IntoDays( void ) {
        MonthEnum i = January;
        for ( ; i <= 11; ((short&) i)++ ) ;
        return 0;
    } */
    // run through the aliases making sure that none
    // are larger than the master  BBB - May 26, 1997
    {
        name    *t;

        for( t = temp->t.alias; t != temp; t = t->t.alias ) {
            if( t->t.temp_flags & CG_INTRODUCED )
                continue;
            assert( t->n.size <= temp->n.size );
        }
    }
#endif
    size = _RoundUp( temp->n.size, REG_SIZE ); /* align size*/
    if( (temp->v.usage & (USE_IN_ANOTHER_BLOCK | USE_ADDRESS)) == 0
     && temp->t.u.block_id != NO_BLOCK_ID ) {
        CalcRange( &st_temp, temp );
        if( st_temp.first != st_temp.last ) { /*% actually needed*/
            stack = ReUsableStack( &st_temp, temp );
            if( stack != NULL ) {
                temp->t.location = stack->location;
                new_st_temp = CGAlloc( sizeof( stack_temp ) );
                new_st_temp->first = st_temp.first;
                new_st_temp->last = st_temp.last;
                new_st_temp->others = stack->temp.others;
                stack->temp.others = new_st_temp;
            } else {
                SetTempLocation( temp, size );
                StackEntry( &st_temp, temp );
            }
            temp->v.usage |= HAS_MEMORY;
            PropLocal( temp );
        } else {
            temp->v.usage &= ~NEEDS_MEMORY;
        }
    } else {
        if( BlockByBlock || (temp->v.usage & USE_ADDRESS) ) {
            NewLocation( temp, size );
            return;
        }
        ins = FindOnlyIns( temp, &any_references );
        if( ins == NULL || UsedByLA( ins, temp ) || SideEffect( ins ) ) {
            if( any_references ) {
                NewLocation( temp, size );
            } else {
                temp->v.usage &= ~NEEDS_MEMORY;
            }
            return;
        }
        DoNothing( ins );
        temp->v.usage &= ~NEEDS_MEMORY;
    }
}


static  void    MarkUsage( name *op )
/***********************************/
{
    if( op->n.class == N_INDEXED ) {
        if( op->i.index != NULL ) {
            MarkUsage( op->i.index );
        }
        if( op->i.base != NULL && (op->i.index_flags & X_FAKE_BASE) == 0 ) {
            MarkUsage( op->i.base );
        }
    } else if( op->n.class == N_TEMP ) {
        op = DeAlias( op );
        if( op->v.block_usage == USED_NEVER ) {
            op->v.block_usage = USED_ONCE;
        } else {
            op->v.block_usage = USED_TWICE;
        }
    }
}


static  void    CalcNumberOfUses( void )
/**************************************/
{
    name        *temp;
    block       *blk;
    instruction *ins;
    opcnt       i;

    for( temp = Names[N_TEMP]; temp != NULL; temp = temp->n.next_name ) {
        if( temp->t.temp_flags & STACK_PARM ) {
            temp->v.block_usage = USED_TWICE;
        } else {
            temp->v.block_usage = USED_NEVER;
        }
    }
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            for( i = ins->num_operands; i-- > 0; ) {
                MarkUsage( ins->operands[i] );
            }
            if( ins->result != NULL ) {
                MarkUsage( ins->result );
            }
        }
    }
}


void    AssignOtherLocals( void )
/*******************************/
{
    name        *temp;
    name        **owner;
    name        *rest;

    rest = NULL;
    if( LastTemp != NULL ) {
        rest = LastTemp->n.next_name;
        LastTemp->n.next_name = NULL;
    }
    Names[N_TEMP] = SortList( Names[N_TEMP], offsetof( name, n.next_name ), TempAllocBefore );
    if( LastTemp != NULL ) {
        for( owner = &Names[N_TEMP]; *owner != NULL; ) {
            owner = &(*owner)->n.next_name;
        }
        *owner = rest;
    }
    if( !BlockByBlock ) {
        CalcNumberOfUses();
    }
    for( temp = Names[N_TEMP]; temp != LastTemp; temp = temp->n.next_name ) {
        if( (temp->v.usage & NEEDS_MEMORY) == 0 )
            continue;
        if( temp->v.usage & HAS_MEMORY )
            continue;
        if( temp->t.temp_flags & ALIAS )
            continue;
        AllocNewLocal( temp );
    }
}


static void PropAParm( name *temp )
/*********************************/
{
    name        *base;

    if( temp->n.class == N_INDEXED ) {
        temp = temp->i.base;
        if( temp == NULL ) {
            return;
        }
    }
    if( temp->n.class != N_TEMP )
        return;
    if( temp->v.usage & HAS_MEMORY )
        return;
    if( temp->t.location == NO_LOCATION )
        return;
    base = DeAlias( temp );
    if( base != temp )
        PropAParm( base );
    temp->t.location += temp->v.offset - base->v.offset;
    temp->v.usage |= HAS_MEMORY;
    temp->t.temp_flags |= STACK_PARM;
}


void    ParmPropagate( void )
/***************************/
{
    instruction *ins;
    block       *blk;
    opcnt       i;

    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            for( i = ins->num_operands; i-- > 0; ) {
                PropAParm( ins->operands[i] );
            }
            if( ins->result != NULL ) {
                PropAParm( ins->result );
            }
        }
    }
}


void    AllocALocal( name *name )
/*******************************/
{
    name = DeAlias( name );
    if( (name->v.usage & HAS_MEMORY) == 0 ) {
        name->v.block_usage = USED_ONCE;
        AllocNewLocal( name );
    }
}


static void AssgnATemp( name *temp, block_num curr_id )
/*****************************************************/
{
    if( temp->n.class == N_INDEXED && temp->i.base != NULL ) {
        temp = temp->i.base;
    }
    if( temp->n.class != N_TEMP )
        return;
    temp = DeAlias( temp );
    if( temp->v.usage & HAS_MEMORY ) {
        PropLocal( temp );
        return;
    }
    if( (temp->v.usage & NEEDS_MEMORY) == 0 )
        return;
    if( ( curr_id == NO_BLOCK_ID )
      || ( temp->t.u.block_id == curr_id )
      || ( temp->t.u.block_id == NO_BLOCK_ID ) ) {
        AllocALocal( temp );
    }
}


void    FiniStackMap( void )
/**************************/
{
    stack_entry *next1;
    stack_temp  *other;
    stack_temp  *next2;
    name        *temp;

    for( temp = Names[N_TEMP]; temp != NULL; temp = temp->n.next_name ) {
        if( temp->t.temp_flags & ALIAS )
            continue;
        if( (temp->v.usage & USE_ADDRESS) == 0 )
            continue;
        if( temp->v.usage & HAS_MEMORY )
            continue;
        AllocNewLocal( temp );
    }

    for( ; StackMap != NULL; StackMap = next1 ) {
        next1 = StackMap->link;
        for( other = StackMap->temp.others; other != NULL; other = next2 ) {
            next2 = other->others;
            CGFree( other );
        }
        CGFree( StackMap );
    }
    TellTempLocs();
}


void    AssgnMoreTemps( block_num curr_id )
/*****************************************/
/* run the block list. It's faster if we're using /od */
{
    instruction *ins;
    block       *blk;
    opcnt       i;

    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            for( i = ins->num_operands; i-- > 0; ) {
                AssgnATemp( ins->operands[i], curr_id );
            }
            if( ins->result != NULL ) {
                AssgnATemp( ins->result, curr_id );
            }
        }
    }
    if( curr_id == NO_BLOCK_ID ) {
        FiniStackMap();
        InitStackMap();
    } else {
        ReInitStackMap();
    }
}


void            CountTempRefs( void )
/***********************************/
{
    block               *blk;
    instruction         *ins;
    opcnt               i;
    name                *temp;

    for( temp = Names[N_TEMP]; temp != NULL; temp = temp->n.next_name ) {
        temp->t.u.ref_count = 0;
    }
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( ins = blk->ins.hd.next; ins->head.opcode != OP_BLOCK; ins = ins->head.next ) {
            for( i = ins->num_operands; i-- > 0; ) {
                if( ins->operands[i]->n.class == N_TEMP ) {
                    ins->operands[i]->t.u.ref_count++;
                }
            }
            if( ins->result != NULL ) {
                if( ins->result->n.class == N_TEMP ) {
                    ins->result->t.u.ref_count++;
                }
            }
        }
    }
}


void    AssignTemps( void )
/*************************/
/*   Parameters on stack have already been assigned locations*/
{
    TransferTempFlags();        /* make sure whole structure goes in mem*/
    ParmPropagate();            /* assign temps which may use parm memory*/
    PushLocals();               /* assign locals which may be pushed */
    AssignOtherLocals();        /* assign memory to all other locals*/
}
